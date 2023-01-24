#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <iostream>
#include <queue>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <chrono>
#include <random>
#include <time.h>
union semun
{
    int val; /* val for SETVAL */
    struct semid_ds *buf; /* Buffer for IPC_STAT and IPC_SET */
    unsigned short *array; /* Buffer for GETALL and SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO and SEM_INFO*/
};

using namespace std;
typedef struct
{
    char name[11];
    double price;
} commodity;

typedef struct
{
    int bufferSize;
    int in;
    int out;
} shmsg;

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        perror("You have to enter 5 arguments!");
        exit(1);
    }
    time_t now;
    char buff[100] ;
    struct tm tm_now ;
    time_t s;
    struct timespec ts;
    long x;
    char *cName = (char *)malloc(10);
    strcpy(cName, argv[1]);
    double mean;
    double stdev;
    int interval;
    int entries;
    sscanf(argv[2],"%lf",&mean);
    sscanf(argv[3],"%lf",&stdev);
    sscanf(argv[4], "%d", &interval);
    sscanf(argv[5], "%d", &entries);

    // Grab the Semaphore Set
    int semid;
    key_t key;
    key = ftok("mySemaphore",64);
    if ((semid = semget(key, 3, 0666 | IPC_CREAT)) == -1)
    {
        perror("semget");
        exit(1);
    }

    // Grab the First Shared Memory. It contains the buffer size, and indices (in and out) for consumer and producers.
    int shmid = shmget(12345,sizeof(shmsg),0666|IPC_CREAT);
    if (shmid == -1)
    {
        perror("Shared memory");
        return 1;
    }
    // Attach the shared memory to the current producer address space.
    shmsg *seg;
    seg = (shmsg*)shmat(shmid,0,0);
    if (seg == (void *) -1)
    {
        perror("Shared memory attach");
        return 1;
    }
    seg->bufferSize=entries;

    // Grab the Second Shared Memory. It contains the array of commodities.
    shmid = shmget(54321,sizeof(commodity) * seg->bufferSize,0666|IPC_CREAT);
    if (shmid == -1)
    {
        perror("Shared memory");
        return 1;
    }

    // Attach the shared memory to the current producer address space.
    commodity *commoditiesArray;
    commoditiesArray = (commodity*)shmat(shmid,0,0);
    if (commoditiesArray == (void *) -1)
    {
        perror("Shared memory attach");
        return 1;
    }
    // Infinite Loop
    while(1)
    {
        //Produce
        now = time(NULL) ;
        clock_gettime(CLOCK_REALTIME,&ts);
        x = ts.tv_nsec;
        localtime_r(&now, &tm_now) ;
        s = tm_now.tm_sec;
        strftime(buff, sizeof(buff), "[%m/%d/%Y %H:%M:%S.", &tm_now) ;
        commodity temp;
        strcpy(temp.name, cName);
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator (seed);
        std::normal_distribution<double> distribution (mean,stdev);
        temp.price = distribution(generator);
        fprintf(stderr, "%s%ld] %s: generating a new value %.3lf\n",buff, x, temp.name, temp.price);

        //SemWait(e), The producer is blocked if the buffer is full.
        struct sembuf sb = {2, -1, 0}; /* set to allocate resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        //SemWait(s). The producer is blocked if another producer or the consumer are in the critical section.
        now = time(NULL) ;
        clock_gettime(CLOCK_REALTIME,&ts);
        x = ts.tv_nsec;
        localtime_r(&now, &tm_now) ;
        s = tm_now.tm_sec;
        strftime(buff, sizeof(buff), "[%m/%d/%Y %H:%M:%S.", &tm_now) ;
        fprintf(stderr, "%s%ld] %s: trying to get mutex on shared buffer\n",buff, x, temp.name);
        sb = {1, -1, 0}; /* set to allocate resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        //Put in shared memory
        now = time(NULL) ;
        clock_gettime(CLOCK_REALTIME,&ts);
        x = ts.tv_nsec;
        localtime_r(&now, &tm_now) ;
        s = tm_now.tm_sec;
        strftime(buff, sizeof(buff), "[%m/%d/%Y %H:%M:%S.", &tm_now) ;
        fprintf(stderr, "%s%ld] %s: placing %.3f on shared buffer\n",buff, x, temp.name, temp.price);
        commoditiesArray[seg->in] = temp;
        seg->in = (seg->in + 1)%(seg->bufferSize);
        //SemSignal(s)

        sb = {1, 1, 0}; /* free resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        //SemSignal(n)
        sb = {0, 1, 0}; /* free resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        clock_gettime(CLOCK_REALTIME,&ts);
        x = ts.tv_nsec;
        now = time(NULL) ;
        localtime_r(&now, &tm_now) ;
        s = tm_now.tm_sec;
        strftime(buff, sizeof(buff), "[%m/%d/%Y %H:%M:%S.", &tm_now) ;
        fprintf(stderr, "%s%ld] %s: sleeping for %d ms\n\n",buff, x, temp.name, interval);
        usleep(interval * 1000);
    }
    //detach from shared memory
    shmdt(seg);
    return 0;
}
