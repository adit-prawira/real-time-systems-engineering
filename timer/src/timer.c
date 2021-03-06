#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <errno.h>

#define MY_PULSE_CODE _PULSE_CODE_MINAVAIL

char *progName = "timer.c";
enum states{
	state0, state1, state2
};
typedef union{
	struct _pulse pulse;
}my_message_t;
int main(void) {
	puts("Simple State Machine started");
	enum states currentState = state0;
	struct sigevent event;
	struct itimerspec itimeA, itimeB;
	timer_t timer_id;
	int chid;
	int rcvid;
	my_message_t msg;

	chid = ChannelCreate(0); // Create communication channel
	event.sigev_notify = SIGEV_PULSE;

	// create connection back to ourselves for the timer to send pulse on
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);
	if(event.sigev_coid == -1){
		printf(stderr, "%s: couldn't ConnectAttach to self!\n", progName);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	struct sched_param threadParam;
	pthread_getschedparam(pthread_self(), NULL, &threadParam);
	event.sigev_priority = threadParam.sched_curpriority;
	event.sigev_code = MY_PULSE_CODE;

	// create timer, binding it to the event
	if(timer_create(CLOCK_REALTIME, &event, &timer_id) == -1){
		printf(stderr, "%s: couldn't create a timer, errno %d\n", progName, errno);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	if(timer_create(CLOCK_REALTIME, &event, &timer_id) == -1){
		printf(stderr, "%s: couldn't create a timer, errno %d\n", progName, errno);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	// timer setup (set 1.5s initial delay)
	itimeA.it_value.tv_sec = 1;
	itimeA.it_value.tv_nsec = 500000000; // 5*10^8 => 0.5 second

	// timer setup (set 1.5s reload interval)
	itimeA.it_interval.tv_sec = 1;
	itimeA.it_interval.tv_nsec = 500000000; // 5*10^8 => 0.5 second


	itimeB.it_value.tv_sec = 5;
	itimeB.it_value.tv_nsec = 0;

	itimeB.it_interval.tv_sec = 5;
	itimeB.it_interval.tv_nsec = 0;

	// Start the timer
	timer_settime(timer_id, 0, &itimeA, NULL);

	// The pulse will be received in 1.5 seconds, and then every 1.5 seconds after
	int runTime = 10, counter = 0;
	for(counter = 0; counter < runTime; counter ++){
		// waiting for message or pulse
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);

		// determine where the message is coming from
		if(rcvid == 0){
			if(msg.pulse.code == MY_PULSE_CODE){
				printf("In state = %d\n", currentState);
				switch(currentState){
					case 0:
						currentState = state1;
						timer_settime(timer_id, 0, &itimeA, NULL);
						break;
					case 1:
						currentState = state2;
						break;
					case 2:
						timer_settime(timer_id, 0, &itimeB, NULL);
						currentState = state0;
						break;
				}
				fflush(stdout);
			}
		}
	}
	printf("\nSwitch statement got called %d times\n", counter);
	printf("Main Terminated...\n");
	return EXIT_SUCCESS;
}
