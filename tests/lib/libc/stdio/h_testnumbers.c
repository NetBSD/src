#include <stdio.h>
#include <stdlib.h>
#include <err.h>

int
main(void)
{
	char line[1024];
	size_t i = 0;
	while (fgets(line, sizeof(line), stdin) != NULL) {
		if ((size_t)atoi(line) != i)
			errx(EXIT_FAILURE, "bad line \"%s\", expected %zu\n",
			    line, i);
		i++;
	}
	return EXIT_SUCCESS;
}
