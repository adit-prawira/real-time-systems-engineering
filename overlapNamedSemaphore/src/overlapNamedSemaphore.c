#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
typedef struct {
	int a;
	int b;
	int result;
	int result2;
	int useCount;
	int useCount2;
	int maxUse;
	int maxUse2;
	sem_t semaphore;
}appData;
/*
  userThread Specification (protected with semaphore):
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
		sem_wait(&threadData->semaphore);
		if(threadData->a == 5){
			threadData->result += (threadData->a) + (threadData->b);
			threadData->useCount ++;
			uses++;
		}
		sem_post(&threadData->semaphore);
		usleep(1);
	}
	return 0;
}

/*
   changerThread Specification (protected with semaphore):
 * function that will change our data.
 * will continue changing the value of a and b until they have been used elsewhere (userThread) 100 times.
 * toggle the value of a between 5 and 50.
 * fake a CPU-intensive calculation for b with usleep(1000) call.
 * value of a will change => 1mx later b will be changed.
 * there will be a 1ms gap where a and b should not be used elsewhere in the application.
*/
void * changerThread(void *data){
	appData *threadData = (appData*)data;
	while((threadData->useCount + threadData->useCount2) <
			(threadData->maxUse + threadData->maxUse2)){
		sem_wait(&threadData->semaphore);
		(threadData->a == 5)?(threadData->a = 50):(threadData->a = 5);
		threadData->b = threadData->a + usleep(1000);
		sem_post(&threadData->semaphore);
		usleep(1);
	}
	return 0;
}

int main(void) {
	puts("Starting Overlap Program (Migration from Mutex to Named Semaphore)\n");
	pthread_t uThread, cThread;
	void *returnValue;
	char hostname[100];
	//const char * location = "/MyNamedSemaphore";
	const char * location = "/net/node2/MyNamedSemaphore";
	appData thread = {5, 5, 0, 0, 0, 0, 100, 0, sem_open(location, O_CREAT, S_IRUSR | S_IWUSR, 1)};
	sem_init(&thread.semaphore, NULL, 1);
	memset(hostname, '\0', 100);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));
	printf("Running on Machine: %s\n", hostname);

	pthread_create(&uThread, NULL, userThread, &thread);
	pthread_create(&cThread, NULL, changerThread, &thread);

	pthread_join(uThread, &returnValue);
	pthread_join(cThread, &returnValue);

	sleep(10); // put sleep before unlink and close semaphore to see the file exist in /dev/sem
	sem_unlink(location);
	sem_close(&thread.semaphore);
	sem_destroy(&thread.semaphore);

	printf("Result should be %d, and it is %d by named semaphore\n", thread.maxUse*(5+5), thread.result);
	printf("Main terminating...\n");
	return EXIT_SUCCESS;
}
