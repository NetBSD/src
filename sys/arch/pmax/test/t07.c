int foo;
int foo1;

main(argc, argv)
	char **argv;
{
	int i, j;

	for (i = 0; i < 10; i++) {
		printf("main: bla %d pid %d\n", j = bla(), getpid());
		if (j == 1)
			exit(0);
	}
}

bla()
{
	int i;

	foo = 0;
	switch(fork()) {
	case 0: /* child */
		foo = 30;
		for (i = 0; i < 10; i++) {
			printf("child %d, foo %d\n", i, foo);
			bar(foo);
			foo++;
		}
		return(1);

	case -1:
		perror("fork");
		return(2);

	default:
		for (i = 0; i < 10; i++) {
			printf("parent %d foo %d\n", i, foo);
			bar(foo);
			foo++;
		}
		return(0);
	}
}

bar(a)
{
	printf("bar(%d) pid %d\n", a, getpid());
}
