#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

struct resource {
	volatile char data1, data2;
	volatile char data1Ready, data2Ready;
}data;

void *producer(void *notUsed){
	int ID = pthread_self();
	printf("STATUS (THREAD ID: %d): Producer Thread Starting...\n", ID);

	char buf[3] = {};
	while(1){
		printf("Type 2 characters (a-z):\n");
		strcpy(buf, " " );
		gets(buf);
		if(pthread_sleepon_lock() == EOK){
			data.data1 = buf[0];
			data.data2 = buf[1];
			data.data1Ready = 1;
			data.data2Ready = 1;
			pthread_sleepon_signal(&data.data1Ready);
			pthread_sleepon_signal(&data.data2Ready);
			pthread_sleepon_unlock();
		}

	}
	return 0;
}

void *consumerOne(void *notUsed){
	int ID = pthread_self();
	printf("STATUS (THREAD ID: %d): Consumer One Thread Starting...\n", ID);
	while(1){
		if(pthread_sleepon_lock() == EOK){
			while(!data.data1Ready){
				printf("Consumer One start sleeping...\n");
				pthread_sleepon_wait(&data.data1Ready);
				printf("Consumer Two start waking up...\n");
			}
			printf("RECEIVE: Consumer One receive '%c' from Producer\n", data.data1);
			data.data1Ready = 0;
			pthread_sleepon_unlock();
		}else{
			printf("ERROR: sleep error in Consumer One\n");
		}
	}
	return 0;
}
void *consumerTwo(void *notUsed){
	int ID = pthread_self();
	printf("STATUS (THREAD ID: %d): Consumer Two Thread Starting...\n", ID);
	while(1){
		if(pthread_sleepon_lock() == EOK){
			while(!data.data2Ready){
				printf("Consumer Two start sleeping...\n");
				pthread_sleepon_wait(&data.data2Ready);
				printf("Consumer Two start waking up...\n");
			}
			printf("RECEIVE: Consumer Two receive '%c' from Producer\n", data.data2);
			data.data2Ready = 0;
			pthread_sleepon_unlock();
		}else{
			printf("ERROR: sleep error in Consumer Two\n");
		}
	}
	return 0;
}

int main(int argc, char *argv[]){
	pthread_t producerThread, consumerOneThread, consumerTwoThread;
	pthread_create(&consumerOneThread, NULL, consumerOne, NULL);
	pthread_create(&consumerTwoThread, NULL, consumerTwo, NULL);
	pthread_create(&producerThread, NULL, producer, NULL);

	pthread_join(consumerOneThread, NULL);
	pthread_join(consumerTwoThread, NULL);
	printf("Main Thread Terminating...\n");
	return EXIT_SUCCESS;
}
