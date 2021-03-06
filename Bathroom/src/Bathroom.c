#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define N_ITERATIONS 4
pthread_mutex_t someoneIsBathing;
void *shower(void *notUsed){
	pthread_mutex_lock(&someoneIsBathing);
		printf("STATUS IN: Thread with ID %d is in the shower\n", pthread_self());
		sleep(2);
		printf("STATUS OUT: Thread with ID %d is leaving the shower\n", pthread_self());
	pthread_mutex_unlock(&someoneIsBathing);
	return 0;
}

int main(int argc, char *argv[]) {
	puts("Bathroom Program Running...\n");
	pthread_mutex_init(&someoneIsBathing, NULL);
	pthread_t *threadIds; // create thread ID pointer
	threadIds = malloc(sizeof (pthread_t) *N_ITERATIONS); // creating an array of threadIds with size of 4
	int i;
	for(i = 0; i< N_ITERATIONS; i++){
		pthread_create(&threadIds[i], NULL, shower, NULL);
	}
	for(i = 0; i < N_ITERATIONS; i++){
		pthread_join(threadIds[i], NULL);
	}
	printf("Main thread is terminating...\n");
	return EXIT_SUCCESS;
}
