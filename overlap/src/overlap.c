
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
	int a;
	int b;
	int result;
	int result2;
	int useCount;
	int useCount2;
	int maxUse;
	int maxUse2;
}appData;

/*
  userThread Specification:
 *Pass appData structure to a thread and use variable a and b 100 times whenever a is set to 5.
 *Use a local "uses" variable to count 100 of our uses.
 *Increment application's useCount in case someone else want to keep track of how many times
  our data has been used
 *Add a small usleep to make sure the scheduler gives the other threads a chance to run.
*/
void *userThread(void *data){
	int uses = 0;
	appData *threadData = (appData*)data;
	while(uses < threadData->maxUse){
		if(threadData->a == 5){
			// increase the value of result by the value of a+b
			threadData->result += (threadData->a)+(threadData->b);

			// track number of uses
			threadData->useCount ++;
			uses++;
		}
		usleep(1);
	}
	return 0;
}

/*
   changerThread Specification:
 * function that will change our data.
 * will continue changing the value of a and b until they have been used elsewhere (userThread) 100 times.
 * toggle the value of a between 5 and 50.
 * fake a CPU-intensive calculation for b with usleep(1000) call.
 * value of a will change => 1mx later b will be changed.
 * there will be a 1ms gap where a and b should not be used elsewhere in the application.
*/
void * changerThread(void *data){
	appData *threadData = (appData*)data;
	while(((threadData->useCount) + (threadData->useCount2)) <
			((threadData->maxUse) + (threadData->maxUse2))){
		// value of a will be either 5 or 50
		if(threadData->a == 5){
			threadData->a = 50;
		}else{
			threadData->a = 5;
		}
		threadData->b = (threadData->a)+usleep(1000);
		usleep(1);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	printf("Start Overlap program\n");
	pthread_t cThread, uThread;
	appData thread = {5, 5, 0, 0, 0, 0, 100, 0};
	void *retval;

	pthread_create(&uThread, NULL, userThread, &thread);
	pthread_create(&cThread, NULL, changerThread, &thread);

	pthread_join(cThread, &retval);
	pthread_join(uThread, &retval);

	printf("Result should be %d, but is %d\n", thread.maxUse*(5+5), thread.result);
	printf("Main terminating...\n");
	return EXIT_SUCCESS;
}
