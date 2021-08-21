#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Note that mutexes make sure that certain code or data is only accessed by one thread
// at a time
typedef struct{
	int a;
	int b;
	int result;
	int result2;
	int useCount;
	int useCount2;
	int maxUse;
	int maxUse2;
	pthread_mutex_t mutex;
}appData;

int main(void) {
	puts("Hello World!!!"); /* prints Hello World!!! */
	return EXIT_SUCCESS;
}
