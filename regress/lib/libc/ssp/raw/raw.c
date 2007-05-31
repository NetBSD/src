#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void raw(char *, size_t);

static void
raw(char *b, size_t len) {
	b[len] = '\0';
}

int
main(int argc, char *argv[])
{
	char b[10];
	size_t len = atoi(argv[1]);

	(void)strncpy(b, "0000000000", sizeof(b));
	raw(b, len);
	(void)printf("%s\n", b);
	return 0;
}
