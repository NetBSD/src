#include <stdio.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	char b[10];
	(void)strcpy(b, argv[1]);

	(void)printf("%s\n", b);
	return 0;
}
