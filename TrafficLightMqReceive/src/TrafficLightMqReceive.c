#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#define MESSAGESIZE 2

// Single staep traffic light state machien that will print message based on
// the data that has been received
void singlestep_trafficlight_statemachine_receive(char buf[MESSAGESIZE]) {
	switch ((int) (buf[0] - '0')) {
	case 1:
		printf("EWR-NSR(%s) -> Wait for 1 second\n", buf);
		printf("Waiting to receive signal...\n");
		break;
	case 2:
		printf("Received '%c' -> stopping car from North-South Road\n", buf[1]);
		printf("EWG-NSR(%c) -> Wait for 2 seconds\n", buf[0]);
		break;
	case 3:
		printf("EWY-NSR(%s) -> Wait for 1 second\n", buf);
		break;
	case 4:
		printf("EWR-NSR(%s) -> Wait for 1 second\n", buf);
		printf("Waiting to receive signal...\n");
		break;
	case 5:
		printf("Received '%c' -> stopping car from East-West Road\n", buf[1]);
		printf("EWR-NSG(%c) -> Wait for 2 seconds\n", buf[0]);
		break;
	case 6:
		printf("EWR-NSY(%s) -> Wait for 1 second\n", buf);
		break;
	}
}

int main(int argc, char *argv[]) {
	mqd_t qd;
	char buf[MESSAGESIZE] = {};
	struct mq_attr attribute;
	const char * mqueueLocation = "/net/receive/traffic_light_queue"; // open traffic_light_queue
	qd = mq_open(mqueueLocation, O_RDONLY);
	if(qd != -1){
		mq_getattr(qd, &attribute);
		printf("max. %u msgs, %u bytes; waiting: %u\n", attribute.mq_maxmsg,
				attribute.mq_msgsize, attribute.mq_curmsgs);

		// will look on the data with size of 2
		while(mq_receive(qd, buf, attribute.mq_msgsize, NULL) > 0){
			singlestep_trafficlight_statemachine_receive(buf);

			// character of d will be sent from "send" node to indicates that all data
			// has been sent => exit loop and close queue.
			if(!strcmp(buf, "d")){
				break;
			}
		}
		mq_close(qd);
	}
	printf("Receive process terminated...\n");
	return EXIT_SUCCESS;
}
