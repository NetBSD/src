/* $NetBSD: printf.c,v 1.1 2006/02/28 19:30:45 kleink Exp $ */

/*
 * Written by Klaus Klein <kleink@NetBSD.org>, February 28, 2006.
 * Public domain.
 */

#include <assert.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
	char s[2];

	/* PR lib/32951: %.0f formats (0.0,0.5] to "0." */
	assert(snprintf(s, sizeof(s), "%.0f", 0.1) == 1);
	assert(s[0] == '0');
	/* assert(s[1] == '\0'); */

	return 0;
}
