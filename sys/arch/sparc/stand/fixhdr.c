#include <stdio.h>
#include <fcntl.h>

main(argc, argv)
int	argc;
char	*argv[];
{
	int	fd;
	struct sun_magic {
		unsigned int	a_dynamic:1;
		unsigned int	a_toolversion:7;
		unsigned char	a_machtype;
#define SUN_MID_SPARC	3
		unsigned short	a_magic;
	} sm;

	if (argc != 2) {
		fprintf(stderr, "Usage: fixhdr <bootfile>\n");
		exit(1);
	}

	if ((fd = open(argv[1], O_RDWR, 0)) < 0) {
		perror("open");
		exit(1);
	}

	if (read(fd, &sm, sizeof(sm)) != sizeof(sm)) {
		perror("read");
		exit(1);
	}

	sm.a_dynamic = 0;
	sm.a_toolversion = 1;
	sm.a_machtype = SUN_MID_SPARC;

	lseek(fd, 0, 0);
	if (write(fd, &sm, sizeof(sm)) != sizeof(sm)) {
		perror("write");
		exit(1);
	}
	(void)close(fd);
	return 0;
}
