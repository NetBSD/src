main(argc, argv)
	char **argv;
{
	int fd = open("/dev/console", 1);

	if (fd >= 0)
		write(fd, "Hello\n", 6);
	return(0);
}
