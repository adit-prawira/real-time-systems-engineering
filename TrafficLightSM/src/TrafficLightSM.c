#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_RUNTIME 30
#define MAX_BUFF_SIZE 2

enum states {
	state0, state1, state2, state3, state4, state5, state6
};

typedef struct{
	volatile char byte1;
	enum states currentState;
	int counter;
	pthread_rwlock_t dataSource;
}appData;

// Single step trafficlight state machine, where it will take pointer of the
// current state and the current counter
void singlestep_trafficlight_statemachine(enum states * currentState, int *counter){
	switch (*currentState){
		case state0:
			*currentState = state1;
			break;
		case state1:
			printf("EWR-NSR(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state2;
			break;
		case state2:
			printf("EWG-NSR(%d) -> Wait for 2 seconds\n", *currentState);
			sleep(2);
			*currentState = state3;
			break;
		case state3:
			printf("EWY-NSR(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state4;
			break;
		case state4:
			printf("EWR-NSR(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state5;
			break;
		case state5:
			printf("EWR-NSG(%d) -> Wait for 2 seconds\n", *currentState);
			sleep(2);
			*currentState = state6;
			break;
		case state6:
			printf("EWR-NSY(%d) -> Wait for 1 second\n", *currentState);
			sleep(1);
			*currentState = state1;
			break;
	}
	*counter += 1;
}

// Consumer thread that will print messages and wait for a keyboard input
// in the case when one road turns green.
void * consumer(void *data){
	puts("Consumer thread started...");
	appData *td = (appData*) data;
	char current = 0, new = 0;
	while(td->counter < MAX_RUNTIME){
		// if the current state is at 2 or 5 wait for key board input to be entered
		if(td->currentState == state2 || td->currentState == state5){
			pthread_rwlock_rdlock(&td->dataSource);
				new = td->byte1;
			pthread_rwlock_unlock(&td->dataSource);
			if(new!=current){
				current = new;
			}
		}else{
			// Otherwise send the current state and counter to the state machine
			// to print the current state of the traffic light and move on to the next
			// state.
			singlestep_trafficlight_statemachine(&td->currentState, &td->counter);
		}
	}
	printf("Consumer thread terminated...\n");
	return 0;
}

// Producer thread that will read keyboard input every time state2 and state5 are met
// where in those case one of the road will turn green.
void *producer(void * data){
	puts("Producer thread started...");
	appData *td = (appData*) data;

	// only take 2 characters which is either 'e' or 'n' with additional character of '\0'
	char buf[MAX_BUFF_SIZE] = {};
	while(td->counter < MAX_RUNTIME){

		// only perform keyboard reading when the state is at 2 and 5
		if(td->currentState == state2 || td->currentState == state5){
			// Specified the character of what the machine requested
			if(td->currentState == state2){
				printf("Press 'n' to stop cars from North-South:\n");
			}
			if(td->currentState == state5){
				printf("Press 'e' to stop cars from East-West:\n");
			}

			gets(buf); // reading keyboard input
			pthread_rwlock_wrlock(&td->dataSource); // Write lock
				td->byte1 = buf[0];

				// send counter and the current state to the state machine
				singlestep_trafficlight_statemachine(&td->currentState, &td->counter);
			pthread_rwlock_unlock(&td->dataSource); // unlock
		}
	}
	printf("Producer thread terminated...\n");
	return 0;
}

int main(int argc, char *argv[]) {
	pthread_t cThread, pThread;
	appData data = {0, state0, 0, PTHREAD_RWLOCK_INITIALIZER};

	// start producer and consumer thread
	pthread_create(&pThread, NULL, producer, &data);
	pthread_create(&cThread, NULL, consumer, &data);

	pthread_join(pThread, NULL);
	pthread_join(cThread, NULL);

	puts("Main Terminated...");
	return EXIT_SUCCESS;
}
