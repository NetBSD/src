main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	int fd = open("/dev/console", 1);

	for (i = 0; i<argc; i++) {
		write(fd, argv[i], strlen(argv[i]));
		write(fd, "\n", 1);
	}
	return(0);
}
