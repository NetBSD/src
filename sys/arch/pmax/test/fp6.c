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
		c = scanf("%x %x %d", &arg[1], &arg[0], &arg[2]);
		if (c < 0)
			break;
		if (c != 3) {
			fprintf(stderr, "line %d: expected 3 args\n",
				lineno);
			goto skip;
		}

		uda.w[0] = arg[0];
		uda.w[1] = arg[1];
		udc.d = scalb(uda.d, arg[2]);

		printf("line %d: scalb(%x,%x,%d) = %x,%x\n",
			lineno, uda.w[1], uda.w[0], arg[2],
			udc.w[1], udc.w[0]);
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
