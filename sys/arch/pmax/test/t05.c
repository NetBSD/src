main(argc, argv)
	char **argv;
{
	int fout = open("/dev/console", 1);

	switch(fork()) {
	case 0: /* child */
		write(fout, "child\n", 6);
		return(1);

	case -1:
		write(fout, "error\n", 6);
		return(2);

	default:
		write(fout, "parent\n", 7);
		return(0);
	}
}
