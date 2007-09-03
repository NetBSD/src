#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char b[10];
	size_t len =  atoi(argv[1]);
	(void)memset(b, 0, len);
	return 0;
}
