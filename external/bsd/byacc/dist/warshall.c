/*	$NetBSD: warshall.c,v 1.12 2026/05/03 15:29:20 christos Exp $	*/

/* Id: warshall.c,v 1.10 2025/10/08 00:22:08 tom Exp  */

#include "defs.h"

#include <sys/cdefs.h>
__RCSID("$NetBSD: warshall.c,v 1.12 2026/05/03 15:29:20 christos Exp $");

static void
transitive_closure(unsigned *R, int n)
{
    int rowsize;
    unsigned i;
    unsigned *rowj;
    const unsigned *rp;
    const unsigned *rend;
    const unsigned *relend;
    unsigned *cword;
    unsigned *rowi;

    rowsize = WORDSIZE(n);
    relend = R + n * rowsize;

    cword = R;
    i = 0;
    rowi = R;
    while (rowi < relend)
    {
	unsigned *ccol = cword;

	rowj = R;

	while (rowj < relend)
	{
	    if (*ccol & (1U << i))
	    {
		rp = rowi;
		rend = rowj + rowsize;
		while (rowj < rend)
		    *rowj++ |= *rp++;
	    }
	    else
	    {
		rowj += rowsize;
	    }

	    ccol += rowsize;
	}

	if (++i >= BITS_PER_WORD)
	{
	    i = 0;
	    cword++;
	}

	rowi += rowsize;
    }
}

void
reflexive_transitive_closure(unsigned *R, int n)
{
    int rowsize;
    unsigned i;
    unsigned *rp;
    const unsigned *relend;

    transitive_closure(R, n);

    rowsize = WORDSIZE(n);
    relend = R + n * rowsize;

    i = 0;
    rp = R;
    while (rp < relend)
    {
	*rp |= (1U << i);
	if (++i >= BITS_PER_WORD)
	{
	    i = 0;
	    rp++;
	}

	rp += rowsize;
    }
}
