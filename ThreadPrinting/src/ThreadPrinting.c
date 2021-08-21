#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define NTHD_START "=======Numerical Thread started=======\n"
#define CTHD_START "=======Character Thread started=======\n"
#define NTHD_END "=======Numerical Thread terminated=======\n"
#define CTHD_END "=======Character Thread terminated=======\n"

// type definition for the message at the start and end
typedef struct {
	bool useSeparator;
	char start[100];
	char end[100];
}message;

// type definition that will store maximum character per line
typedef struct {
	int threadNumber;
	int delay;
	int upperBound;
	char type;
	message msg; // reference data from message type definition above
} appData;

// expect O(n) time complexity
void *printThread(void * data){
	int count = 0;
	int characterCount = 0;
	appData *thread = (appData*) data;

	// option to print separator
	if(thread->msg.useSeparator){
		printf("%s", thread->msg.start);
	}

	// this will print one character at a time (delay seconds at a time)
	while(characterCount < 100){
		if(thread->type=='i'){
			printf("%d", count);
		}else{
			printf("%c", (97 + count) ); // ASCII Value
		}
		count++;
		if(count == thread->upperBound){
			count = 0;
			// uncomment the following for the particular case in the question
//			if(characterCount + thread->upperBound - 1 == 99 && thread->type=='c'){
//				usleep(1);
//			}

			// add character count by the maximum number of character
			// per line, which is 10 in this case
			characterCount += thread->upperBound;
			printf("\n");
		}
		fflush(stdout);

		// uncomment the code below to see it printing each character one second at a time
		sleep(thread -> delay);
	}
	if(thread->msg.useSeparator){
		printf("%s", thread->msg.end);
	}
	return 0;
}


int main(int argc, char *argv[]) {
	void *returnValue;
	pthread_t thread1, thread2;
	pthread_attr_t threadAttributes2;
	struct sched_param threadParams2;

	// insert data to appData struct type definition
	appData threadData1 = {1, 1, 10, 'c', {false, CTHD_START, CTHD_END}};
	appData threadData2 = {2, 1, 10, 'i', {false, NTHD_START, NTHD_END}};

	// create the first thread with default setup
	pthread_create(&thread1, NULL, printThread, &threadData1);
	pthread_join(thread1, &returnValue);

	// creating the second thread with customized setups
	pthread_attr_init(&threadAttributes2);
	threadParams2.sched_priority = 1;
	pthread_attr_setschedparam(&threadAttributes2, &threadParams2);
	pthread_create(&thread2, NULL, printThread, &threadData2);
	//usleep(1);
	pthread_join(thread2, &returnValue);

	return EXIT_SUCCESS;
}
