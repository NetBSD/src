#include <stdio.h>

int
main(int argc, char *argv[])
{
	char b[10];
	(void)gets(b);
	(void)printf("%s\n", b);
	return 0;
}
