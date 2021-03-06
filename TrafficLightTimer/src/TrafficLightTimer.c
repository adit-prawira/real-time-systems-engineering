#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <errno.h>
#define PULSE_CODE _PULSE_CODE_MINAVAIL

char *progName = "TrafficLightTimer.c";
enum states{
	state0, state1, state2, state3, state4, state5, state6
};

typedef union{
	struct _pulse pulse;
}message_t;

// Single step traffic light state machine with no block methods
// but setting new value to the timer instead
void singlestep_trafficlight_statemachine(enum states *currentState,
		timer_t timerId, const struct itimerspec* iTimeG,
		const struct itimerspec* iTimeRY) {
	switch (*currentState) {
		case 0:
			*currentState = state1;
			break;
		case 1:
			printf("EWR-NSR(%d) -> Wait for 1 second\n", *currentState);
			*currentState = state2;
			break;
		case 2:
			// set timer to 2 seconds
			timer_settime(timerId, 0, &*iTimeG, NULL);
			printf("EWG-NSR(%d) -> Wait for 2 seconds\n", *currentState);
			*currentState = state3;
			break;
		case 3:
			// set timer back to 1 second
			timer_settime(timerId, 0, &*iTimeRY, NULL);
			printf("EWY-NSR(%d) -> Wait for 1 second\n", *currentState);
			*currentState = state4;
			break;
		case 4:
			printf("EWR-NSR(%d) -> Wait for 1 second\n", *currentState);
			*currentState = state5;
			break;
		case 5:
			// set timer to 2 seconds
			timer_settime(timerId, 0, &*iTimeG, NULL);
			printf("EWR-NSG(%d) -> Wait for 2 seconds\n", *currentState);
			*currentState = state6;
			break;
		case 6:
			// set timer back to 1 second
			timer_settime(timerId, 0, &*iTimeRY, NULL);
			printf("EWR-NSY(%d) -> Wait for 1 second\n", *currentState);
			*currentState = state1;
			break;
	}
}

int main(void) {
	puts("Traffic Light with timer started...\n");
	int rcvid;
	int chid;
	message_t msg;
	timer_t timerId;
	struct sigevent event;
	int runTimes = 30, counter = 0;
	enum states currentState = state0;
	struct itimerspec iTimeRY, iTimeG;

	// create communication channel
	chid = ChannelCreate(0);
	event.sigev_notify = SIGEV_PULSE;

	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);
	if(event.sigev_coid == -1){
		printf(stderr,  "%s: couldn't ConnectAttach to self\n", progName);
		perror(NULL); // reset error
		exit(EXIT_FAILURE);
	}

	struct sched_param threadParam;
	pthread_getschedparam(pthread_self(), NULL, &threadParam);
	event.sigev_priority = threadParam.sched_curpriority;
	event.sigev_code = PULSE_CODE;

	// create timer and binding if to the event
	if(timer_create(CLOCK_REALTIME, &event, &timerId) == -1){
		printf(stderr, "%s: couldn't create a timer, errno -> %d\n", progName, errno);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	// setup timer for RED, and YELLOW LIGHT
	iTimeRY.it_value.tv_sec = 1;
	iTimeRY.it_value.tv_nsec = 0;
	iTimeRY.it_interval.tv_sec = 1;
	iTimeRY.it_interval.tv_nsec = 0;

	// setup timer for GREEN LIGHT
	iTimeG.it_value.tv_sec = 2;
	iTimeG.it_value.tv_nsec = 0;
	iTimeG.it_interval.tv_sec = 2;
	iTimeG.it_interval.tv_nsec = 0;

	timer_settime(timerId, 0, &iTimeRY, NULL);
	for(counter = 0; counter < runTimes; counter++){
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		if(rcvid == 0){
			if(msg.pulse.code == PULSE_CODE){
				singlestep_trafficlight_statemachine(&currentState, timerId, &iTimeG, &iTimeRY);
				fflush(stdout);
			}
		}
	}
	printf("\nTotal iteration if state machine switch statement calls: %d\n", counter);
	printf("Main terminated...\n");
	return EXIT_SUCCESS;
}
