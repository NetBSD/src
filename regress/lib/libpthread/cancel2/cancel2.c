#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int
main(void)
{
	char str1[] = "You should see this.\n";
	char str2[] = "You shoukd not see this.\n";

	printf("Cancellation test 2: Self-cancellation and disabling.\n");


	pthread_cancel(pthread_self());

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	write(STDOUT_FILENO, str1, sizeof(str1));

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	write(STDOUT_FILENO, str2, sizeof(str2));

	exit(1);
}
