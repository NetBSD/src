#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int
main(void)
{

	printf("Test of pthread_exit() in main thread only.\n");

	pthread_exit(NULL);

	printf("You shouldn't see this.");
	exit(1);

}
