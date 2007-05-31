#include <stdio.h>

int
main(int argc, char *argv[])
{
	char b[10];
	(void)sprintf(b, "%s", argv[1]);
	(void)printf("%s\n", b);
	return 0;
}
