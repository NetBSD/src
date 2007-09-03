#include <sys/param.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char b[MAXPATHLEN];
	size_t len = atoi(argv[1]);
	(void)read(0, b, len);
	(void)printf("%s\n", b);
	return 0;
}
