#include <err.h>
#include <stdio.h>

int 
main(argc, argv)
	int     argc;
	char  **argv;
{
	FILE   *fp;
	char   *fingerpath = "portal/tcp/localhost/finger";
	char   *name = "";
#define FING_BUFSIZE	16384
	char    buff[FING_BUFSIZE];
	int     n;

	if (argc > 2)
		errx(1, "Error:  usage:  %s [name]", argv[0]);
	fp = fopen(fingerpath, "r+");
	if (!fp)
		err(1, "open of %s", fingerpath);
	if (argv[1])
		name = argv[1];
	fprintf(fp, "%s\n", name);
	n = fread(buff, (size_t) 1, FING_BUFSIZE, fp);
	fwrite(buff, (size_t) 1, n, stdout);
	return 0;
}
