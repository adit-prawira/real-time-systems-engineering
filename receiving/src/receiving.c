#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>

#define  MESSAGESIZE 1000

int main(int argc, char *argv[]){
	mqd_t	qd;
	char	buf[MESSAGESIZE]= {};
	struct	mq_attr	attr;
//	const char * MqueueLocation = "/test_queue";
	const char * MqueueLocation = "/net/receive/test_queue";

	qd = mq_open(MqueueLocation, O_RDONLY);
    if (qd != -1)
	{
		mq_getattr(qd, &attr);
		printf ("max. %ld msgs, %ld bytes; waiting: %ld\n",
				attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
		while (mq_receive(qd, buf, attr.mq_msgsize, NULL) > 0)
		{
			printf("dequeue: '%s'\n", buf);
			if (!strcmp(buf, "done"))
				break;
		}
		mq_close(qd);
	}
	else
	{
	  printf("Error %i: %s\n", errno, strerror(errno));
	}
	printf("mqueue receive process exited\n");
	return EXIT_SUCCESS;
}
