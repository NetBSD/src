#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char b[10];
	size_t len = atoi(argv[1]);
	(void)snprintf(b, len, "%s", "0123456789");
	(void)printf("%s\n", b);
	return 0;
}
