#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#define  MESSAGESIZE 1000
#define Q_FLAGS O_RDWR | O_CREAT | O_EXCL
#define Q_Mode S_IRUSR | S_IWUSR


int main(int argc, char *argv[])
{
    mqd_t	qd;
    int		i;
    char	buf[MESSAGESIZE] = {};
    struct  mq_attr  attr;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = MESSAGESIZE;
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
    attr.mq_sendwait = 0;
	attr.mq_recvwait = 0;
    struct mq_attr * my_attr = &attr;

	//const char * MqueueLocation = "/test_queue";
	const char * MqueueLocation = "/net/receive/test_queue";

    qd = mq_open(MqueueLocation, Q_FLAGS, Q_Mode, my_attr);
	mq_getattr( qd, my_attr );
	printf("my_attr->mq_maxmsg: '%ld'\n", my_attr->mq_maxmsg);
	printf("my_attr->mq_msgsize: '%ld'\n", my_attr->mq_msgsize);
	printf("my_attr->mq_flags: '%ld'\n", my_attr->mq_flags);
	printf("my_attr->mq_curmsgs: '%ld'\n", my_attr->mq_curmsgs);
	printf("my_attr->mq_sendwait: '%ld'\n", my_attr->mq_sendwait);
	printf("my_attr->mq_recvwait: '%ld'\n", my_attr->mq_recvwait);

    if (qd != -1)
    {
		for (i=1; i <= 5; ++i)
        {
			sprintf(buf, "message %d", i);
			printf("queue: '%s'\n", buf);
			mq_send(qd, buf, MESSAGESIZE, 0);
			sleep(2);
		}
		mq_send(qd, "done", 5, 0);
		printf("\nAll Messages sent to the queue\n");
		printf("\nWait here for 10 seconds before closing mqueue\n");
		sleep(10);
		mq_close(qd);
		mq_unlink(MqueueLocation);
    }
    else
    {
    	printf("\nThere was an ERROR opening the message queue!");
    	printf("\nHave you started the 'mqueue' process on the VM target?\n");
    	printf("\nHave you started the 'qnet' process on the VM target?\n");
    }

	printf("\nmqueue send process Exited\n");
	return EXIT_SUCCESS;
}
