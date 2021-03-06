#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
/*
  Specification:
  1. Uses mutex and counting semaphores
  2. Buffer size of 5
  3. Data size of 20
  4. Speed of producer thread is 1 millisecond
  5. Speed of consumer thread is 2 millisecond
*/
#define BUF_SIZE 5

int bufferInPointer = 1; // buffer in
int bufferOutPointer = 0;    // buffer out;
int count = 0;
int i = 0;
sem_t empty, full;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int buffer[BUF_SIZE] = {};
unsigned int data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

void *producer(void *speed)
{
  int i = 0, tmp = 0;
  int *s = (int *)speed;
  while(1){
    if(i > 19) // if i is greater than 19, it will indicates that there should be no more data to be sent
      break;

    tmp = data[i++]; // generate some data from the data array into tmp variable
    sem_wait(&empty);// Reduce number of empty semaphores
    pthread_mutex_lock(&mutex);
    buffer[bufferInPointer] = tmp; // write data to buffer
    bufferInPointer = (bufferInPointer + 1) % BUF_SIZE; // User circular buffer algorithm to update value of buffer in
    count++; // update value of count
    printf("PRODUCER STATUS: PRODUCER inserted value of %d\n", tmp);
    printf("COUNT: it has count %d semaphores\n", count);
    pthread_mutex_unlock(&mutex);
    sem_post(&full); // Add number of full sempahores
    usleep(*s);
  }

  return 0;
}
void *consumer(void *speed){
  int i = 0, tmp = 0;
  int *s = (int *)speed;
  while(1){
    if(i>19)
      break;
    i++;
    sem_wait(&full); // reduce number of full semaphores
    pthread_mutex_lock(&mutex);
    tmp = buffer[bufferOutPointer];
    bufferOutPointer = (bufferOutPointer + 1) % BUF_SIZE;
    count--;
    printf("CONSUMER STATUS: Comsumer receive and removing data with value of %d\n", tmp);
    printf("COUNT: it has count %d of removed semaphores\n", count);
    pthread_mutex_unlock(&mutex);
    sem_post(&empty); // add number of empty semaphores
    usleep(*s);
  }
  return 0;
}
int main(int argc, char *argv[]){
  printf("Circular buffer program running...\n");
  sem_init(&empty, NULL, BUF_SIZE);
  sem_init(&full, NULL, 0);

  pthread_t producerThread, consumerThread;
  int producerSpeed= 1000000, consumerSpeed = 2000000;

  pthread_create(&producerThread, NULL, producer, &producerSpeed);
  pthread_create(&consumerThread, NULL, consumer, &consumerSpeed);

  pthread_join(producerThread, NULL);
  pthread_join(consumerThread, NULL);
  printf("Main Thread is Terminating...\n");
  return EXIT_SUCCESS;
}
