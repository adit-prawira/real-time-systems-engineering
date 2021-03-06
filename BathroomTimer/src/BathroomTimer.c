#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

pthread_mutex_t someoneIsBathing;

struct timespec deadline;

void *shower(void * notUsed){
	int returnedCode = pthread_mutex_timedlock(&someoneIsBathing, &deadline);
	if(returnedCode == EOK){
		printf("STATUS IN: Thread with ID %d is in the shower\n", pthread_self());
		sleep(2);
		printf("STATUS OUT: Thread with ID %d is leaving the shower\n", pthread_self());
		pthread_mutex_unlock(&someoneIsBathing);
	}else{
		printf("ERROR: Thread with ID %d Failed to Lock: %s\n", pthread_self(), strerror(returnedCode));
	}
	return 0;
}
int main(void) {
	printf("Timed Bathroom Program Running...\n");

	pthread_t thread1, thread2;
	pthread_mutex_init(&someoneIsBathing, NULL);
	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec +=1;
	pthread_create(&thread1, NULL, shower, NULL);
	pthread_create(&thread2, NULL, shower, NULL);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	printf("main Thread is terminating...\n");
	return EXIT_SUCCESS;
}
