#include <stdio.h>
#include <stdlib.h>

union {
	unsigned w;
	float	f;
} uf;

union {
	unsigned w[2];
	double	d;
} ud;

/*
 * Read test vectors from file and test the fp emulation by
 * comparing it with the hardware.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	char buf[1024];

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		ud.d = atof(buf);
		uf.f = ud.d;
		printf("'%s' (%x,%x) (%x)\n", buf, ud.w[1], ud.w[0], uf.w);
	}
	exit(0);
}
