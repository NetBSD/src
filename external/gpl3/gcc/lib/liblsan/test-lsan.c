#include <stdlib.h>
#include <stdio.h>

int
main(void)
{
	int *a = malloc(7);

	for (int i = 0; i < 10; i++)
		a[i] = i;
	printf("%d\n", a[6]);
	return 0;
}
