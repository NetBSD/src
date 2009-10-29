/*	$NetBSD: warshall.c,v 1.1.1.1 2009/10/29 00:46:53 christos Exp $	*/

/* Id: warshall.c,v 1.6 2008/11/24 21:30:35 tom Exp */

#include "defs.h"

static void
transitive_closure(unsigned *R, int n)
{
    int rowsize;
    unsigned i;
    unsigned *rowj;
    unsigned *rp;
    unsigned *rend;
    unsigned *ccol;
    unsigned *relend;
    unsigned *cword;
    unsigned *rowi;

    rowsize = WORDSIZE(n);
    relend = R + n * rowsize;

    cword = R;
    i = 0;
    rowi = R;
    while (rowi < relend)
    {
	ccol = cword;
	rowj = R;

	while (rowj < relend)
	{
	    if (*ccol & (1 << i))
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
    unsigned *relend;

    transitive_closure(R, n);

    rowsize = WORDSIZE(n);
    relend = R + n * rowsize;

    i = 0;
    rp = R;
    while (rp < relend)
    {
	*rp |= (1 << i);
	if (++i >= BITS_PER_WORD)
	{
	    i = 0;
	    rp++;
	}

	rp += rowsize;
    }
}
