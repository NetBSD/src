#include <stdio.h>

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

	while (scanf("%lf", &ud.d) == 1) {
		uf.f = ud.d;
		printf("%g = (%x,%x) (%x)\n", ud.d, ud.w[1], ud.w[0], uf.w);
	}
	exit(0);
}
