#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

enum states {
	state0,
	state1,
	state2,
	state3,
	state4,
	state5,
	state6,
};


void singlestep_trafficlight_statemachine(enum states * currentState){
	switch (*currentState){
		case state0:
			*currentState = state1;
			break;
		case state1:
			printf("EWR-NSR(%d)\n", *currentState);
			sleep(1);
			*currentState = state2;
			break;
		case state2:
			printf("EWG-NSR(%d)\n", *currentState);
			sleep(2);
			*currentState = state3;
			break;
		case state3:
			printf("EWY-NSR(%d)\n", *currentState);
			sleep(1);
			*currentState = state4;
			break;
		case state4:
			printf("EWR-NSR(%d)\n", *currentState);
			sleep(1);
			*currentState = state5;
			break;
		case state5:
			printf("EWR-NSG(%d)\n", *currentState);
			sleep(2);
			*currentState = state6;
			break;
		case state6:
			printf("EWR-NSY(%d)\n", *currentState);
			sleep(1);
			*currentState = state1;
			break;
	}
}


int main(int argc, char *argv[]) {
	printf("Fixed Sequence Traffic Lights State Machine\n");
	int runTimes = 30, counter = 0;
	enum states currentState = state0;
	while(counter < runTimes){
		singlestep_trafficlight_statemachine(&currentState);
		counter++;
	}
	return EXIT_SUCCESS;
}
