/*	$NetBSD: div.c,v 1.1 2001/01/08 04:39:31 simonb Exp $	*/

#include <stdio.h>
#include <stdlib.h>

#define	NUM	1999236
#define	DENOM	1000000
#define	QUOT	1
#define	REM	999236

int
main(void)
{
	div_t d;
	ldiv_t ld;
	lldiv_t lld;
	int error = 0;

	d = div(NUM, DENOM);
	ld = ldiv(NUM, DENOM);
	lld = lldiv(NUM, DENOM);

	if (d.quot != QUOT || d.rem != REM) {
		fprintf(stderr, "div returned (%d, %d), expected (%d, %d)\n",
		    d.quot, d.rem, QUOT, REM);
		error++;
	}
	if (ld.quot != QUOT || ld.rem != REM) {
		fprintf(stderr, "ldiv returned (%ld, %ld), expected (%d, %d)\n",
		    ld.quot, ld.rem, QUOT, REM);
		error++;
	}
	if (lld.quot != QUOT || lld.rem != REM) {
		fprintf(stderr, "lldiv returned (%lld, %lld), expected (%d, %d)\n",
		    lld.quot, lld.rem, QUOT, REM);
		error++;
	}
	if (error > 0)
		fprintf(stderr, "div: got %d errors\n", error);
	exit(error);
}
