#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <process.h>
#include <unistd.h>

void *firstChildThread(void * data){
	printf("=====First child thread started=====\nNow sleeping...\n");
	sleep(20);
	printf("First child thread ID	=> %d\n", gettid());
	printf("~~~~~First child thread finished~~~~~\n%%%% Waiting to enter a number %%%%\n");
	return 0;
}

void *secondChildThread(void *data){
	int n;
	scanf("%d", &n);
	printf("~~~~~Second child thread finished~~~~~\n");
	printf("Second child thread ID	=> %d\n", gettid());
	return 0;
}

int main(int argc, char *argv[]) {
	pthread_t  th1, th2;
	void *retval;

	printf("=====Main thread started=====\n");
	printf("Process ID				=> %d\n", getpid());
	printf("Main thread ID			=> %d\n", gettid());

	pthread_create (&th1, NULL, firstChildThread, NULL);
	pthread_create(&th2, NULL, secondChildThread, NULL);

	pthread_join (th1, &retval);
	pthread_join(th2, &retval);

	printf("~~~~~Main thread finished~~~~~\n");
	return EXIT_SUCCESS;
}
