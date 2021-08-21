#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Note that mutexes make sure that certain code or data is only accessed by one thread
// at a time.
// 1. use pthread_mutex_lock() in front of the code we want to protect and
// 	  pthread_mutex_unlock after that code.
typedef struct{
	int a;
	int b;
	int result;
	int result2;
	int useCount;
	int useCount2;
	int maxUse;
	int maxUse2;
	pthread_mutex_t mutex;
}appData;

/*
  userThread Specification (protected):
 *Pass appData structure to a thread and use variable a and b 100 times whenever a is set to 5.
 *Use a local "uses" variable to count 100 of our uses.
 *Increment application's useCount in case someone else want to keep track of how many times
  our data has been used
 *Add a small usleep to make sure the scheduler gives the other threads a chance to run.
*/
void *userThread(void *data){
	int uses = 0;
	appData *threadData = (appData*)data;

	while(uses < (threadData->maxUse)){
		pthread_mutex_lock(&threadData->mutex);
		if(threadData->a == 5){
			threadData->result += (threadData->a) + (threadData->b);
			threadData->useCount++;
			uses++;
		}
		pthread_mutex_unlock(&threadData->mutex);
		usleep(1);
	}
	return 0;
}
/*
   changerThread Specification (protected):
 * function that will change our data.
 * will continue changing the value of a and b until they have been used elsewhere (userThread) 100 times.
 * toggle the value of a between 5 and 50.
 * fake a CPU-intensive calculation for b with usleep(1000) call.
 * value of a will change => 1mx later b will be changed.
 * there will be a 1ms gap where a and b should not be used elsewhere in the application.
*/
void *changerThread(void *data){
	appData *threadData = (appData*) data;
	while((threadData->useCount + threadData->useCount2) <
			(threadData->maxUse + threadData->maxUse2)){
		pthread_mutex_lock(&threadData -> mutex);
		if(threadData->a == 5){
			threadData->a = 50;
		}else{
			threadData->a = 5;
		}
		threadData->b = threadData->a + usleep(1000);
		pthread_mutex_unlock(&threadData->mutex);
		usleep(1);
	}
	return 0;
}
int main(void) {
	puts("Start overlap program with protected data (Mutex)\n");
	pthread_t cThread, uThread;
	appData thread = {5, 5, 0, 0, 0, 0, 100, 0};
	void *retval;
	pthread_mutex_init(&thread.mutex, NULL);
	pthread_create(&uThread, NULL, userThread, &thread);
	pthread_create(&cThread, NULL, changerThread, &thread);

	pthread_join(uThread, &retval);
	pthread_join(cThread, &retval);

	pthread_mutex_destroy(&thread.mutex);

	printf("Result should be %d,is %d\n", thread.maxUse*(5+5), thread.result);
	printf("Main terminating...\n");
	return EXIT_SUCCESS;
}
