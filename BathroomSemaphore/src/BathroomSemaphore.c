#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

#define N_USERS 5

sem_t someoneIsBathing;
void *shower(void *notUsed)
{
  int ID = pthread_self();
  sem_wait(&someoneIsBathing);
  printf("STATUS:  Thread with ID %d is using the shower\n", ID);
  sleep(2);
  printf("STATUS: Thread with ID %d us existing the shower\n", ID);
  sem_post(&someoneIsBathing);
  return 0;
}

int main(int argc, char *argv[]){
  printf("Bathroom Semaphore Program is Runnning...\n");
  sem_init(&someoneIsBathing, NULL, 1);

  pthread_t *userThreadIds;
  userThreadIds = malloc(sizeof(pthread_t) * N_USERS);
  for (int i = 0; i < N_USERS; i++){
    pthread_create(&userThreadIds[i], NULL, shower, NULL);
  }
  for (int j = 0; j < N_USERS; j++){
    pthread_join(userThreadIds[j], NULL);
  }
  printf("Main Thread is Terminating...\n");
  return EXIT_SUCCESS;
}
