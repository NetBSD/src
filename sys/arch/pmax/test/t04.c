#include <stdio.h>

main(argc, argv)
	char **argv;
{
	int fin = open("/dev/console", 0);
	int fout = open("/dev/console", 1);
	int ferr = open("/dev/console", 1);
	int pid;

	switch(pid = fork()) {
	case 0: /* child */
		printf("child\n");
		return(1);
	case -1:
		perror("fork");
		return(2);

	default:
		printf("parent %d\n", pid);
	}
	return(0);
}
