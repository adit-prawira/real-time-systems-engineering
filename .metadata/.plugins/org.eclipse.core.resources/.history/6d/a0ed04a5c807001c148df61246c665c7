#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define MAX_RUNTIME 30

enum states {
	state0, state1, state2, state3, state4, state5, state6
};

typedef struct{
	volatile char byte1;
	enum states currentState;
	int counter;
	pthread_rwlock_t dataSource;
}appData;

void singlestep_trafficlight_statemachine(enum states * currentState){

}

void * consumer(void *data){
	puts("Consumer thread started...");
	appData *td = (appData*) data;
	char current = 0, new = 0;
	while(td->counter < MAX_RUNTIME){
		pthread_rwlock_rdlock(&td->dataSource);
			new = td->byte1;
		pthread_rwlock_unlock(&td->dataSource);
		printf("Received State: %d\n", td->currentState);
		sleep(2);
		switch (td->currentState){
			case state1:
				printf("EWR-NSR(%d)\n", td->currentState);
				sleep(1);
				break;
			case state2:
				if(new!=current){
					printf("waiting. for input..\n");
					current = new;
					printf("EWG-NSR(%d)\n", td->currentState);
					sleep(2);
				}
				break;
			case state3:
				printf("EWY-NSR(%d)\n", td->currentState);
				sleep(1);
				break;
			case state4:
				printf("EWR-NSR(%d)\n", td->currentState);
				sleep(1);
				break;
			case state5:
				if(new!=current){
					printf("waiting. for input..\n");
					current = new;
					printf("EWR-NSG(%d)\n", td->currentState);
					sleep(2);
				}
				break;
			case state6:
				printf("EWR-NSY(%d)\n", td->currentState);
				sleep(1);
				break;
		}
	}
	return 0;
}

void *producer(void * data){
	puts("Producer thread started...");
	appData *td = (appData*) data;
	char buf[2] = {};
	printf("Enter e/n:\n");
	while(td->counter < MAX_RUNTIME){
		switch (td->currentState){
			case state0:
				td->currentState = state1;
				break;
			case state1:
				td->currentState = state2;
				break;
			case state2:
				gets(buf);// reading keyboard input
				pthread_rwlock_wrlock(&td->dataSource);
					td->byte1 = buf[0];
					td->currentState = state3;
				pthread_rwlock_unlock(&td->dataSource);
				break;
			case state3:
				td->currentState = state4;
				break;
			case state4:
				td->currentState = state5;
				break;
			case state5:
				gets(buf);// reading keyboard input
				pthread_rwlock_wrlock(&td->dataSource);
					td->byte1 = buf[0];
					td->currentState = state6;
				pthread_rwlock_unlock(&td->dataSource);\
				break;
			case state6:
				td->currentState = state1;
				break;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	pthread_t cThread, pThread;
	appData data = {0, state0, 0, PTHREAD_RWLOCK_INITIALIZER};
	pthread_create(&pThread, NULL, producer, &data);
	pthread_create(&cThread, NULL, consumer, &data);
	pthread_join(pThread, NULL);
	pthread_join(cThread, NULL);

	puts("Main Terminated...");
	return EXIT_SUCCESS;
}
