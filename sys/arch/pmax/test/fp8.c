#include <stdio.h>
#include <ctype.h>
#include <math.h>

int lineno;

union {
	unsigned w[2];
	double	d;
} uda, udb, udc;

/*
 * Read test vectors from file and test the fp emulation by
 * comparing it with the hardware.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	register int c;
	char buf[10];
	unsigned arg[4];

	for (lineno = 1; ; lineno++) {
		c = scanf("%x %x", &arg[1], &arg[0]);
		if (c < 0)
			break;
		if (c != 2) {
			fprintf(stderr, "line %d: expected 2 args\n",
				lineno);
			goto skip;
		}

		uda.w[0] = arg[0];
		uda.w[1] = arg[1];
		udc.d = modf(uda.d, &udb.d);

		printf("line %d: modf(%x,%x) = %x,%x int %x,%x\n",
			lineno, uda.w[1], uda.w[0],
			udc.w[1], udc.w[0], udb.w[1], udb.w[0]);
	skip:
		while ((c = getchar()) != EOF && c != '\n')
			;
	}
}

trapsignal(p, sig, code)
	int p, sig, code;
{
	printf("line %d: signal(%d, %x)\n", lineno, sig, code);
}
