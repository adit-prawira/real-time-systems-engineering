#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sync.h>
#include <sys/neutrino.h>
#include <pthread.h>

pthread_barrier_t barrier;

void * thread1(void *notUsed){
	time_t now;
	time(&now);
	printf("Thread 1 starting at: %s", ctime(&now));
	fflush(stdout);

	// sleep for 3 seconds
	sleep(3);
	time(&now);
	printf("Thread 1 reached barrier at: %s", ctime(&now));

	pthread_barrier_wait(&barrier);

	// after this point, all threads are expected to be completed
	time(&now);
	printf("Barrier in thread 1 is completed at: %s", ctime(&now));
	return 0;
}

void * thread2(void * notUsed){
	time_t now;
	time(&now);
	printf("Thread 2 starting at: %s", ctime(&now));

	// sleep for 6 seconds
	sleep(6);
	time(&now);
	printf("Thread 2  reached barrier at: %s", ctime(&now));

	pthread_barrier_wait(&barrier);

	// after this point all thread will be completed
	time(&now);
	printf("Barrier in thread 2 is completed at: %s", ctime(&now));
	return 0;
}

void *thread3(void * notUsed){
	time_t now;
		time(&now);
		printf("Thread 3 starting at: %s", ctime(&now));

		// sleep for 10 seconds
		sleep(10);
		time(&now);
		printf("Thread 3  reached barrier at: %s", ctime(&now));

		pthread_barrier_wait(&barrier);

		// after this point all thread will be completed
		time(&now);
		printf("Barrier in thread 3 is completed at: %s", ctime(&now));
		return 0;
}


int main(void) {
	char hostname[100];
	time_t now;

	puts("Starting Barrier program\n");
	memset(hostname, '\0', NULL);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));
	printf("Barrier running in machine -> %s\n", hostname);

	pthread_barrier_init(&barrier, NULL, 4);

	// start thread1, thread2, and thread3
	pthread_create(NULL, NULL, thread1, NULL);
	pthread_create(NULL, NULL, thread2, NULL);
	pthread_create(NULL, NULL, thread3, NULL);

	usleep(1);

	time(&now);
	printf("\nMain waiting at barrier at: %s\n", ctime(&now));

	pthread_barrier_wait(&barrier);

	time(&now);
	printf("Barrier in Main is completed at: %s", ctime(&now));
	fflush(stdout);
	puts("\nMain Terminating...\n");
	return EXIT_SUCCESS;
}
