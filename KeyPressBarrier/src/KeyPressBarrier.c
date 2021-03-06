#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

char key = 0;
pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// A thread that will wait for user to input key press from their keyboard
void * getKey(void *notUsed){
	char buf[2] = {};
	while(1){
		gets(buf);
		pthread_mutex_lock(&mutex);
			key = buf[0];
		pthread_mutex_unlock(&mutex);
		sched_yield(); // yield after completing task
	}
	return 0;
}
void *printKey(void *data){
	char ch = *((char*) data);
	int loop = 1, ID =pthread_self();
	printf("STATUS: Thread with ID %d waiting for character '%c'\n", ID, ch);
	while(loop){
		pthread_mutex_lock(&mutex);
			if(key == ch){
				printf("Thread (ID: %d) received '%c'\n", ID, ch);
				loop = 0;
			}
		pthread_mutex_unlock(&mutex);
		sched_yield();
	}
	printf("---> Waiting at Barrier\n");
	pthread_barrier_wait(&barrier);
	return 0;
}

int main(int argc, char *argv[]) {
	printf("Barrier Key Press Program Running...\n");
	printf("Press keys: A or B or C\n");

	pthread_t threadGetKey, *threadKeys;
	pthread_create(&threadGetKey, NULL, getKey, NULL);

	char keys[] = {'A', 'B', 'C'};
	int i =0, n = sizeof(keys)/sizeof(keys[0]);
	threadKeys = malloc(sizeof(pthread_t)*n);

	pthread_barrier_init(&barrier, NULL, n+1);
	for(i = 0; i< n; i++){
		pthread_create(&threadKeys[i], NULL, printKey, &keys[i]);
	}
	pthread_barrier_wait(&barrier);
	printf("Main Thread Terminating...\n");
	return EXIT_SUCCESS;
}
