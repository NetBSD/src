char hello[] = "hello\n";

main(argc, argv)
	char **argv;
{
	int fd = open("/dev/console", 1);

	hello[0] = 'H';
	if (fd >= 0)
		write(fd, hello, 6);
	return(0);
}
