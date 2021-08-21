#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* This is the code for the thread.  It is not called directly
 * but passed as a parameter to pthread_create in the main thread.
 * The single void parameter may be used to pass parameters.
*/
 void *thread_ex (void *data)
 {
    printf("Thread started\nNow sleeping...\n");
    sleep(20);
    printf("Finished sleeping\nThread finished\n");
    return 0;
 }

 int main(int argc, char *argv[]) {
 	printf("Welcome to the QNX Momentics thread-ex1\n");
    pthread_t  th1;
    void *retval;

    // Create and start the thread
    pthread_create (&th1, NULL, thread_ex, NULL);

    /* Suspend the main program (itself a thread)
     * until the thread has terminated.
    */
    pthread_join (th1, &retval);

    printf("Main terminated\n");
    return EXIT_SUCCESS;
}


