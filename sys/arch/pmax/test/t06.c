int foo;

main(argc, argv)
	char **argv;
{
	int fout = open("/dev/console", 1);
	int i;

	foo = 0;
	switch(fork()) {
	case 0: /* child */
		foo = 30;
		for (i = 0; i < 10; i++) {
			printf("child %d, foo %d\n", i, foo);
			foo++;
		}
		return(1);

	case -1:
		perror("fork");
		return(2);

	default:
		for (i = 0; i < 10; i++) {
			printf("parent %d foo %d\n", i, foo);
			foo++;
		}
		return(0);
	}
}
