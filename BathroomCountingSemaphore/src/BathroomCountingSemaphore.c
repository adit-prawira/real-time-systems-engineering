#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

// the example is to create 30 threads:
// but only 5 thread at a time can be in the critical section
// Then the semaphore only has a maximum value of 5
#define N_THREADS 30
#define MAX_SEM_VALUE 5

sem_t peopleAreBathing;
void *shower(void *notUsed)
{
  int ID = pthread_self();
  int count = 0;
  sem_wait(&peopleAreBathing);
  sem_getvalue(&peopleAreBathing, &count);
  printf("ENTERING: Thread with ID %d is using the shower, there are %d using the shower\n", ID, MAX_SEM_VALUE - count);
  sleep(2);
  sem_post(&peopleAreBathing);
  sem_getvalue(&peopleAreBathing, &count);
  printf("LEAVING: Thread with ID %d finished showering, there are %d left in the shower\n", ID, MAX_SEM_VALUE - count);
  return 0;
}

int main(int argc, char *argv[]) {
	sem_init(&peopleAreBathing, NULL, MAX_SEM_VALUE);
	pthread_t *threadIds;
	threadIds = malloc(sizeof(pthread_t)*N_THREADS);
	for(int i = 0; i< N_THREADS; i++){
    pthread_create(&threadIds[i], NULL, shower, NULL);
  }
  for (int j = 0; j < N_THREADS; j++){
    pthread_join(threadIds[j], NULL);
  }

    return EXIT_SUCCESS;
}
