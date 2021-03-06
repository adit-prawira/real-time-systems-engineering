#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define N_USERS 5
#define BUF_SIZE 10

int isOpen = 0; // initialize the status of the shower to be closed
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condVar = PTHREAD_COND_INITIALIZER;

// lock mutex when accessing the data
void *shower(void *notUsed){
	int ID = pthread_self();
	pthread_mutex_lock(&mutex);
	while(isOpen==0){ // while shower is not isOpened
		// Block thread according to conditional variable
		// and unlock the associated mutex
		pthread_cond_wait(&condVar, &mutex);
	}
	printf("ENTERING: Thread with ID %d is using the shower\n", ID);
	sleep(2);
	printf("LEAVING: Thread with ID %d is leaving the shower\n", ID);

	pthread_cond_signal(&condVar); // give signal to run the next thread, and to get key
	pthread_mutex_unlock(&mutex);
	return 0;
}

// Get Key Threads
void *getKey(void *notUsed){
	char key[BUF_SIZE] = {};
	while(1){
		gets(key);
		if(tolower(key[0])=='o'){
			printf("----> Shower is opened\n");
			isOpen = 1;
		}else{
			printf("<---- Shower is closed\n");
			isOpen = 0;
		}
		pthread_cond_signal(&condVar);
	}
	sched_yield();
	return 0;
}

int main(int argc, char *argv[]) {
	printf("Bathroom CondVar Program Running...\n");
	printf("Instructions:\n- Press 'o' to close the door\n- Press any key to close it\n");

	pthread_t keyThread;
	pthread_t *userThreadIds;
	userThreadIds = malloc(sizeof(pthread_t)*N_USERS);
	pthread_create(&keyThread, NULL, getKey, NULL);
	for(int i = 0; i < N_USERS; i++){
		pthread_create(&userThreadIds[i], NULL, shower, NULL);
	}
	for(int j = 0; j < N_USERS; j++){
		pthread_join(userThreadIds[j], NULL);
	}

	printf("Main Thread Terminating...\n");
	return EXIT_SUCCESS;
}
