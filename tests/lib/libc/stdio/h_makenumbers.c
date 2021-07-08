#include <stdio.h>
#include <stdlib.h>
#include <err.h>

int
main(int argc, char *argv[])
{
	size_t i = 0;
	size_t maxi;

	if (argc != 2)
		errx(EXIT_FAILURE, "missing argument");

	maxi = atoi(argv[1]);

	while (i < maxi)
		printf("%zu\n", i++);
	return EXIT_SUCCESS;
}
