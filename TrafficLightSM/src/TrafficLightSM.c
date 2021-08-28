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

void singlestep_trafficlight_statemachine(enum states * currentState, int *counter){
	switch (*currentState){
		case state0:
			*currentState = state1;
			*counter += 1;
			break;
		case state1:
			printf("EWR-NSR(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state2;
			*counter += 1;
			break;
		case state2:
			printf("EWG-NSR(%d) -> Wait for 2 seconds\n", *currentState);
			sleep(2);
			*currentState = state3;
			*counter += 1;
			break;
		case state3:
			printf("EWY-NSR(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state4;
			*counter += 1;
			break;
		case state4:
			printf("EWR-NSR(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state5;
			*counter += 1;
			break;
		case state5:
			printf("EWR-NSG(%d) -> Wait for 2 seconds\n", *currentState);
			sleep(2);
			*currentState = state6;
			*counter += 1;
			break;
		case state6:
			printf("EWR-NSY(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state1;
			*counter += 1;
			break;
	}
}

void * consumer(void *data){
	puts("Consumer thread started...");
	appData *td = (appData*) data;
	char current = 0, new = 0;
	while(td->counter < MAX_RUNTIME){
		if(td->currentState == state2 || td->currentState == state5){
			pthread_rwlock_rdlock(&td->dataSource);
				new = td->byte1;
			pthread_rwlock_unlock(&td->dataSource);
			if(new!=current){
				current = new;
			}
		}else{
			singlestep_trafficlight_statemachine(&td->currentState, &td->counter);
		}
	}
	printf("Consumer thread terminated...\n");
	return 0;
}

void *producer(void * data){
	puts("Producer thread started...");
	appData *td = (appData*) data;
	char buf[2] = {};
	while(td->counter < MAX_RUNTIME){
		if(td->currentState == state2 || td->currentState == state5){
			if(td->currentState == state2){
				printf("Press 'n' to stop cars from North-South:\n");
			}
			if(td->currentState == state5){
				printf("Press 'e' to stop cars from East-West:\n");
			}
			gets(buf);// reading keyboard input
			pthread_rwlock_wrlock(&td->dataSource);
				td->byte1 = buf[0];
				singlestep_trafficlight_statemachine(&td->currentState, &td->counter);
			pthread_rwlock_unlock(&td->dataSource);
		}
	}
	printf("Producer thread terminated...\n");
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
