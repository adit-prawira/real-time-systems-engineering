#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
int dataIsReady = 0;

// utilize default attributes to initialize mutex and conditional variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condvar = PTHREAD_COND_INITIALIZER;

// Consumer thread
void *consumer(void *notUsed){
	puts("Consumer thread started...\n");
	while(1){
		pthread_mutex_lock(&mutex);
		// test conditions and wait until it is satisfied
		while(!dataIsReady){
			pthread_cond_wait(&condvar, &mutex);
		}

		// process data;
		puts("Consumer: received data from Producer\n");

		// change the condition that data is not ready because it has been consumed
		dataIsReady = 0;
		pthread_cond_signal(&condvar);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

// Producer thread
void *producer(void *notUser){
	puts("Producer thread started...\n");
	while(1){
		// retrieve data from the hardware
		// and stimulate it by blocking it by 1 second
		sleep(1);
		puts("Producer: Data from stimulated hardware is ready\n");
		pthread_mutex_lock(&mutex);
		// test conditions and wait until it is satisfied
		while(dataIsReady){
			// the function will blocks the calling thread on condition variable (condvar)
			// and unlocks the associated mutex
			pthread_cond_wait(&condvar, &mutex);
		}
		// change the condition as the data is ready to be sent to consumer
		dataIsReady = 1;
		pthread_cond_signal(&condvar);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}


int main(int argc, char *argv[]) {
	char hostname[100];
	pthread_t pThread, cThread;
	puts("Starting Producer/Consumer program...\n");
	memset(hostname, '\0', NULL);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));
	printf("Program is running in machine -> %s\n", hostname);

	pthread_create(&pThread, NULL, producer, NULL);
	pthread_create(&cThread, NULL, consumer, NULL);

	// allowing threads to run in 20 seconds
	sleep(20);
	puts("\nMain Thread is terminating...\n");

	return EXIT_SUCCESS;
}
