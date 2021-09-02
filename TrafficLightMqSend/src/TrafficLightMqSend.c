#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
enum states {
	state0, state1, state2, state3, state4, state5, state6
};
#define MESSAGESIZE 2
#define Q_FLAGS O_RDWR | O_CREAT | O_EXCL
#define Q_MODE S_IRUSR | S_IWUSR

void singlestep_trafficlight_statemachine_send(enum states *currentState,
		char buf[MESSAGESIZE], mqd_t qd) {
	switch (*currentState) {
		case state0:
			*currentState = state1;
			break;
		case state1:
			sprintf(buf, "%d", *currentState);
			printf("Sending state -> %d\n", *currentState);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(1);
			*currentState = state2;
			break;
		case state2:
			printf("Press 'n' to stop cars from North-South:\n");
			gets(buf);
			sprintf(buf, "%d%c", *currentState, buf[0]);
			printf("Sending state -> %d\n", *currentState);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(2);
			*currentState = state3;
			break;
		case state3:
			sprintf(buf, "%d", *currentState);
			printf("Sending state -> %d\n", *currentState);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(1);
			*currentState = state4;
			break;
		case state4:
			sprintf(buf, "%d", *currentState);
			printf("Sending state -> %d\n", *currentState);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(1);
			*currentState = state5;
			break;
		case state5:
			printf("Press 'e' to stop cars from East-West:\n");
			gets(buf);
			sprintf(buf, "%d%c", *currentState, buf[0]);
			printf("Sending state -> %d\n", *currentState);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(2);
			*currentState = state6;
			break;
		case state6:
			sprintf(buf, "%d", *currentState);
			printf("Sending state -> %d\n", *currentState);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(1);
			*currentState = state1;
			break;
	}
}

int main(int argc, char *argv[]) {
	mqd_t qd;
	enum states currentState = state0; // initialise the state
	int runTimes = 30, counter = 0; // set total iterations of 30 and start counter with 0
	char buf[MESSAGESIZE] = {};
	struct mq_attr attribute;

	// mq attributes configuration
	attribute.mq_maxmsg = 100;
	attribute.mq_msgsize = MESSAGESIZE;
	attribute.mq_flags = 0;
	attribute.mq_curmsgs = 0;
	attribute.mq_sendwait = 0;
	attribute.mq_recvwait = 0;

	struct mq_attr * customAttribute = &attribute;
	const char * mqueueLocation = "/net/receive/traffic_light_queue"; // set location to put traffic_light_queue
	qd = mq_open(mqueueLocation, Q_FLAGS, Q_MODE, customAttribute);

	// guard to see whether or not the file exist
	if(qd != -1){
		// Processed the single step traffic light state machine inside the for loop
		for(counter = 0; counter < runTimes; counter++){
			singlestep_trafficlight_statemachine_send(&currentState, buf, qd);
		}

		// After all data has been send, terminate threads within 5 seconds
		// close, and unlink mq to the traffic_light_queue file.
		printf("Sending done message\n");
		printf("All states have been sent\n");
		printf("Wait for 5 seconds to close mqueue\n");
		sleep(5);
		mq_send(qd, "d", MESSAGESIZE, 0);
		mq_close(qd);
		mq_unlink(mqueueLocation);
	}
	printf("Sending Process Terminated...\n");
	return EXIT_SUCCESS;
}
