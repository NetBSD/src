#include <sys/param.h>
#include <sys/exec.h>

/*
 * Print simple check sum for text and data section.
 */
void
main(argc, argv)
	int argc;
	char **argv;
{
	register int fd, i, n;
	char *fname;
	struct coff_exec ex;
	int tsum, dsum, word;

	if (argc < 2) {
		printf("usage: sum <file>\n");
		goto err;
	}
	if ((fd = open(fname = argv[1], 0)) < 0)
		goto err;

	/* read the COFF header */
	i = read(fd, (char *)&ex, sizeof(ex));
	if (i != sizeof(ex) || COFF_BADMAG(ex)) {
		printf("No COFF header\n");
		goto cerr;
	}
	if (lseek(fd, COFF_TEXTOFF(ex), 0)) {
		perror("lseek");
		goto cerr;
	}
	tsum = 0;
	for (n = ex.aouthdr.tsize; n > 0; n -= sizeof(int)) {
		if (read(fd, &word, sizeof(word)) != sizeof(word)) {
			perror("read text");
			goto cerr;
		}
		tsum += word;
	}
	dsum = 0;
	for (n = ex.aouthdr.dsize; n > 0; n -= sizeof(int)) {
		if (read(fd, &word, sizeof(word)) != sizeof(word)) {
			perror("read data");
			goto cerr;
		}
		dsum += word;
	}
	printf("tsum %d dsum %d\n", tsum, dsum);

cerr:
	close(fd);
err:
	exit(0);
}
