#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

 typedef struct
 {
	 int a; // thread number
	 int b; // counting upper bound value
	 int d; // sleeping time
 } app_data;

 /* This is the code for the thread.  It is not called directly
  * but passed as a parameter to pthread_create in the main thread.
  * Parameters are passed through the single void parameter.
 */
void *thread_ex (void *data)
{
  int count = 1;
  /* Type casting is used here to cast the void parameter
   * to a pointer to a structure of type app_data.
   */
  app_data *td = (app_data*) data;

  while (count <= td->b)
  /* Thread executes for b times */
  {
	  /* printf prints to the output (screen by default).
	   * The text to be printed is within quotes and may contain
	   * place-holders for printing the value of variables.
	   * %d is a place holder for an integer.
	   * Each place-holder is matched, in order, with the variables
	   * listed.  Here a is used as the thread number.
	   */
	  printf ("Thread %d, Iteration %d\n", td->a, count);
	  count++;

	  sleep(td->d);
	  /* Thread sleeps for d seconds */
  }

  printf ("Thread %d terminated\n", td->a);
  return 0;
}

int main(void) {
	pthread_t th1, th2;
	app_data threadData1 = {1, 10, 2}; // count until 10 with 2 seconds sleeping time
	app_data threadData2 = {2, 5, 3}; // count until 5 with 3 seconds sleeping time
	void *retval;
	pthread_create(&th1, NULL, thread_ex, &threadData1);
	pthread_create(&th2, NULL, thread_ex, &threadData2);

	pthread_join(th1, &retval);

	pthread_join(th2, &retval);

	printf("Main process terminated");
	return EXIT_SUCCESS;
}
