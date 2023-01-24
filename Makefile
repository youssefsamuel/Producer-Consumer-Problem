all: run_producer run_consumer


run_producer:
	g++ -o producer producer.cpp  
run_consumer:
	g++ -o  consumer consumer.cpp  

clean:
	rm producer consumer
	ipcrm -a
 	

