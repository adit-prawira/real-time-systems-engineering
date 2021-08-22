#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#define Buffer_size 10
#define Number_of_packets 15

int dataIsReady = 0;

typedef struct {
	char buffer[Buffer_size];
	int readfinnished;
	pthread_mutex_t mutex;
	pthread_cond_t condvar;
}appData;

void *consumer(void *data){
	sleep(1); // add 1 second delay
	printf("Consumer thread is running (thread priority:10)...\n");
	appData *threadConsumer= (appData*) data;
	int index = 0;
	char dataValue;
	while(threadConsumer->readfinnished==0){
		pthread_mutex_lock(&threadConsumer->mutex);
		while(!dataIsReady){
			pthread_cond_wait(&threadConsumer->condvar, &threadConsumer->mutex);
		}
		// Processing data
		dataValue = threadConsumer->buffer[index];
		printf("consumer: got data from producer : %c\n",
						dataValue);
		// set readfinnished to 1 as it has reaches the value of 0 and
		// hence break out all the system
		if(dataValue+1 > '@'+Number_of_packets){
			threadConsumer->readfinnished = 1;
		}
		index = (++index) % Buffer_size;
		sleep(1);

		dataIsReady = 0;
		pthread_cond_signal(&threadConsumer->condvar);
		pthread_mutex_unlock(&threadConsumer->mutex);
	}
	printf("\nreadfinnished = %d\n->Consumer thread finished...\n", threadConsumer->readfinnished);
	return 0;
}

void *producer(void *data){
	printf("Producer thread is running (thread priority:10)...\n");
	appData *threadProducer = (appData*)data;
	int index = 0;
	char dataValue = 'A';
	dataIsReady = 1;
	while(threadProducer->readfinnished==0){
		pthread_mutex_lock(&threadProducer->mutex);
		if(dataValue <= '@'+Number_of_packets){
			threadProducer->buffer[index++] = dataValue++;
			printf("producer: Data(%c) from stimulated h/w is ready\n",
					threadProducer->buffer[index-1]);
			if(threadProducer->buffer[index-1] < ('@'+Number_of_packets)){
				threadProducer->buffer[index] = dataValue;
				printf("producer: Data(%c) from stimulated h/w is ready\n",
								threadProducer->buffer[index]);
			}
		}
		sleep(1);
		dataValue++;
		index = (++index) % Buffer_size;

		while(dataIsReady){
			pthread_cond_wait(&threadProducer->condvar, &threadProducer->mutex);
			if(dataValue-2 == '@'+Number_of_packets){
				// as the dataValue reaching O then all the producer need to do it waiting until the
				// the consumer has set a condition that will terminate the entire threads
				printf("->Producer has sent all items - now waiting for consumer to finish...\n");
			}
		}

		dataIsReady=1;
		pthread_cond_signal(&threadProducer->condvar);
		pthread_mutex_unlock(&threadProducer->mutex);

	}
	printf("->Producer thread finished...\n");
	return 0;
}

int main(int argc, char *argv[]) {

	pthread_t pThread, cThread;
	pthread_attr_t  ptAttr, ctAttr;
	struct sched_param  ptParam, ctParam;
	void *retval;
	appData data = {{}, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

	printf("Starting consumer/producer (main thread priority:10)...\n");
	// set up attribute and parameter for producer thread
	pthread_attr_init (&ptAttr);
	pthread_attr_setschedpolicy (&ptAttr, SCHED_RR);
	ptParam.sched_priority = 5;
	pthread_attr_setschedparam (&ptAttr, &ptParam);
	pthread_create(&pThread, &ptAttr, producer, &data);

	// set up attribute and parameter for consumer thread
	pthread_attr_init (&ctAttr);
	pthread_attr_setschedpolicy (&ctAttr, SCHED_RR);
	ctParam.sched_priority = 2;
	pthread_attr_setschedparam (&ctAttr, &ctParam);
	pthread_create(&cThread, &ctAttr, consumer, &data);

	pthread_join(pThread, &retval); // must stay alive

	puts("\nMain Thread is terminating...\n");
	return EXIT_SUCCESS;
}
