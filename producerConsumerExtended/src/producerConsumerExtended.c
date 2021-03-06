#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int dataIsReady = 0;
typedef struct {
	char c;
	int bit;
	pthread_mutex_t mutex;
	pthread_cond_t condvar;
}appData;

void *consumer(void *data){
	puts("Consumer thread is running...\n");
	appData *threadConsumer= (appData*) data;
	while(1){
		pthread_mutex_lock(&threadConsumer->mutex);
		while(!dataIsReady){
			pthread_cond_wait(&threadConsumer->condvar, &threadConsumer->mutex);
		}
		// Processing data
		printf("Consumer: Data received from Producer[%c, %i]\n",
				threadConsumer->c, threadConsumer->bit);
		dataIsReady = 0;
		pthread_cond_signal(&threadConsumer->condvar);
		pthread_mutex_unlock(&threadConsumer->mutex);
	}
	return 0;
}

void *producer(void *data){
	puts("Producer thread is running...\n");
	appData *threadProducer = (appData*)data;
	threadProducer->c = '@';
	threadProducer->bit = 0;
	dataIsReady = 1;
	while(1){
		pthread_mutex_lock(&threadProducer->mutex);
		threadProducer->c++;
		threadProducer->bit++;
		sleep(1);

		printf("Producer: Data(%c, %i) from stimulated hardware is ready\n",
				threadProducer->c, threadProducer->bit);
		while(dataIsReady){
			pthread_cond_wait(&threadProducer->condvar, &threadProducer->mutex);
		}
		dataIsReady=1;
		pthread_cond_signal(&threadProducer->condvar);
		pthread_mutex_unlock(&threadProducer->mutex);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	char hostname[100];
	pthread_t pThread, cThread;
	appData data = {0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

	puts("Starting Extended Producer/Consumer program...\n");
	memset(hostname, '\0', NULL);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));
	printf("Program is running in machine -> %s\n", hostname);

	pthread_create(&pThread, NULL, producer, &data);
	pthread_create(&cThread, NULL, consumer, &data);

	sleep(20);
	puts("\nMain Thread is terminating...\n");
	return EXIT_SUCCESS;
}
