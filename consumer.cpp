#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <queue>
#include <map>
#include<string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

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


union semun
{
    int val; /* val for SETVAL */
    struct semid_ds *buf; /* Buffer for IPC_STAT and IPC_SET */
    unsigned short *array; /* Buffer for GETALL and SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO and SEM_INFO*/
};

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        perror("You have to enter the Bounded Buffer Size!");
        exit(0);
    }
    char *names[11];
    for (int i = 0; i < 11; i++)
        names[i] = (char *) malloc(11);
    strcpy(names[0], "ALUMINIUM");
    strcpy(names[1], "COPPER");
    strcpy(names[2], "COTTON");
    strcpy(names[3], "CRUDEOIL");
    strcpy(names[4], "GOLD");
    strcpy(names[5], "LEAD");
    strcpy(names[6], "MENTHAOIL");
    strcpy(names[7], "NATURALGAS");
    strcpy(names[8], "NICKEL");
    strcpy(names[9], "SILVER");
    strcpy(names[10], "ZINC");

    int bufferSize;
    sscanf(argv[1],"%d",&bufferSize);

    // Initializing 3 semaphores
    key_t key;
    int semid;
    union semun arg;
    queue<double> q[11];

    // Initialization of the first shared memory
    int shmid = shmget(12345,sizeof(shmsg),0666|IPC_CREAT);
    if (shmid == -1)
    {
        perror("Shared memory");
        return 1;
    }
    shmsg *seg;
    seg = (shmsg*)shmat(shmid,0,0);
    if (seg == (void *) -1)
    {
        perror("Shared memory attach");
        return 1;
    }
    seg->bufferSize = bufferSize;
    seg->in = 0;
    seg->out = 0;

    // Shared Memory 2: array of commodities
    shmid = shmget(54321,sizeof(commodity) * bufferSize,0666|IPC_CREAT);
    if (shmid == -1)
    {
        perror("Shared memory");
        return 1;
    }
    commodity *commoditiesArray;
    commoditiesArray = (commodity*)shmat(shmid,0,0);
    if (commoditiesArray == (void *) -1)
    {
        perror("Shared memory attach");
        return 1;
    }


    /* create a semaphore set with 3 semaphores:
        0-Semaphore n = 0 (number of commodities in buffer)
        1-Semaphore s = 1 (to access the buffer)
        2-Semaphore e = maximum size of buffer */
    key = ftok("mySemaphore",64);
    if ((semid = semget(key, 3, 0666 | IPC_CREAT)) == -1)
    {
        perror("semget");
        exit(0);
    }
    /* initialize semaphore #0 to 0: */
    arg.val = 0;
    if (semctl(semid, 0, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(1);
    }
    /* initialize semaphore #1 to 1: */
    arg.val = 1;
    if (semctl(semid, 1, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(0);
    }
    /* initialize semaphore #2 to buffer size: */
    arg.val = bufferSize;
    if (semctl(semid, 2, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(0);
    }


    printf("\e[1;1H\e[2J");
    printf("+-------------------------------------+\n");
    printf("| Currency      |  Price   | AvgPrice |\n");
    printf("+-------------------------------------+\n");
    for (int i = 0; i < 11; i++)
    {
        printf("| %s", names[i]);
        int len = strlen(names[i]);
        int spaces = 14-len;
        for (int m = 0; m < spaces; m++)
        {
            printf(" ");
        }
        printf("|");
        double init=0;
        printf("\033[1;34m%8.2lf  \033[0m", init);
        printf("|");
        printf("\033[1;34m%8.2lf  \033[0m", init);
        printf("|\n");
    }
    printf("+-------------------------------------+\n");


    while(1)
    {
        int index;
        double price;

        //SemWait(n) The consumer is blocked if the buffer is empty.
        struct sembuf sb = {0, -1, 0}; /* set to allocate resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        //SemWait(s) The consumer is blocked if a producer is in the critical section.
        sb = {1, -1, 0}; /* set to allocate resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        //Take
        for (int i = 0; i < 11; i++)
        {
            if ((strcasecmp(names[i], commoditiesArray[seg->out].name) == 0))
            {
                index = i;
                break;
            }
        }
        price = commoditiesArray[seg->out].price;
        seg->out = ((seg->out) + 1)%(seg->bufferSize);
        q[index].push(price);
        double sum=0;
        double oldSum = 0;
        double oldPrice = 0;
        double old_avg = 0;
        for(int j=0; j<q[index].size(); j++)
        {
            if (j != q[index].size()-1)
                oldSum += q[index].front();
            if (j ==  q[index].size()-2)
                oldPrice = q[index].front();
            double w = q[index].front();
            q[index].pop();
            sum += w;
            q[index].push(w);
        }
        double avg = sum/((1.0)*q[index].size());
        if (q[index].size()!= 1)
            old_avg = oldSum/((1.0)*(q[index].size()-1));
        if(q[index].size() == 5)
        {
            q[index].pop();
        }
        //SemSignal(s)
        sb = {1, 1, 0}; /* free resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        //SemSignal(e)
        sb = {2, 1, 0}; /* free resource */
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        //consume (print)
        printf("\033[%d;18H", index+4);

        if (oldPrice == price)
            printf("\033[1;34m%8.2lf  \033[0m", price);
        else if (oldPrice < price)
            printf("\033[1;32m%8.2lf↑ \033[0m", price);
        else
            printf("\033[1;31m%8.2lf↓ \033[0m", price);

        printf("\033[%d;29H", index+4);
        if (old_avg == avg)
            printf("\033[1;34m%8.2lf  \033[0m", avg);
        else if (old_avg < avg)
            printf("\033[1;32m%8.2lf↑ \033[0m", avg);
        else
            printf("\033[1;31m%8.2lf↓ \033[0m", avg);

        printf("\033[15;0H");
        printf("\n");
    }
    //detach from shared memory
    shmdt(seg);

    // destroy the shared memory
    shmctl(shmid,IPC_RMID,NULL);

    return 0;
}
