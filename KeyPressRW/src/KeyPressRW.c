#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

pthread_rwlock_t dataSource;

struct resource{
	volatile char byte1;
	volatile char byte2;
}data;

void *producer(void *notUsed){
	int ID = pthread_self();
	printf("STATUS (THREAD ID: %d): Producer Thread Starting...\n", ID);
	char buf[3] = {};
	printf("Press 2 keys and press enter:\n");
	while(1){
		gets(buf);
		pthread_rwlock_wrlock(&dataSource);
			data.byte1 = buf[0];
			data.byte2 = buf[1];
		pthread_rwlock_unlock(&dataSource);
	}
	printf("Terminating (THREAD ID: %d): Producer Thread Terminating...\n", ID);
	return 0;
}

void *consumerOne(void *notUsed){
	int ID = pthread_self();
	char currentChar = 0, newChar = 0;

	printf("STATUS (THREAD ID: %d): Consumer One Thread Starting...\n", ID);
	while(1){
		pthread_rwlock_rdlock(&dataSource);
			newChar = data.byte1; // reading keys from data source
		pthread_rwlock_unlock(&dataSource);
		if(newChar != currentChar){
			printf("RECEIVED (THREAD ID: %d): Consumer One Received '%c' from Producer\n", ID, newChar);
			currentChar = newChar;
		}
		sleep(1);
	}
	printf("Terminating (THREAD ID: %d): Consumer One Thread Terminating...\n", ID);
	return 0;
}

void *consumerTwo(void *notUsed){
	int ID = pthread_self();
	char currentChar = 0, newChar = 0;
	printf("STATUS (THREAD ID: %d): Consumer Two Thread Starting...\n", ID);
	while(1){
		pthread_rwlock_rdlock(&dataSource);
			newChar = data.byte2;
		pthread_rwlock_unlock(&dataSource);
		if(newChar != currentChar){
			printf("RECEIVED (THREAD ID: %d): Consumer Two Received '%c' from Producer\n", ID, newChar);
			currentChar = newChar;
		}
		sleep(1);
	}
	printf("Terminating (THREAD ID: %d): Consumer Two Thread Terminating...\n", ID);
	return 0;
}

int main(int argc, char *argv[]){
	pthread_rwlock_init(&dataSource, NULL);

	pthread_t producerThread, consumerOneThread, consumerTwoThread;
	pthread_create(&consumerOneThread, NULL, consumerOne, NULL);
	pthread_create(&consumerTwoThread, NULL, consumerTwo, NULL);
	pthread_create(&producerThread, NULL, producer, NULL);

	pthread_join(consumerOneThread, NULL);
	pthread_join(consumerTwoThread, NULL);

	printf("Main Thread Terminating...\n");
	return EXIT_SUCCESS;
}
