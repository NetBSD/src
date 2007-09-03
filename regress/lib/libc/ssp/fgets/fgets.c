#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char b[10];
	int len = atoi(argv[1]);
	(void)fgets(b, len, stdin);
	(void)printf("%s\n", b);
	return 0;
}
