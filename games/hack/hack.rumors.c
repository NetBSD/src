/*	$NetBSD: hack.rumors.c,v 1.4 1997/10/19 16:58:55 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.rumors.c,v 1.4 1997/10/19 16:58:55 christos Exp $");
#endif				/* not lint */

#include "hack.h"	/* for RUMORFILE and BSD (strchr) */
#include "extern.h"
#define	CHARSZ	8		/* number of bits in a char */
int             n_rumors = 0;
int             n_used_rumors = -1;
char           *usedbits;

void
init_rumors(rumf)
	FILE           *rumf;
{
	int             i;
	n_used_rumors = 0;
	while (skipline(rumf))
		n_rumors++;
	rewind(rumf);
	i = n_rumors / CHARSZ;
	usedbits = (char *) alloc((unsigned) (i + 1));
	for (; i >= 0; i--)
		usedbits[i] = 0;
}

int
skipline(rumf)
	FILE           *rumf;
{
	char            line[COLNO];
	while (1) {
		if (!fgets(line, sizeof(line), rumf))
			return (0);
		if (strchr(line, '\n'))
			return (1);
	}
}

void
outline(rumf)
	FILE           *rumf;
{
	char            line[COLNO];
	char           *ep;
	if (!fgets(line, sizeof(line), rumf))
		return;
	if ((ep = strchr(line, '\n')) != 0)
		*ep = 0;
	pline("This cookie has a scrap of paper inside! It reads: ");
	pline(line);
}

void
outrumor()
{
	int             rn, i;
	FILE           *rumf;
	if (n_rumors <= n_used_rumors ||
	    (rumf = fopen(RUMORFILE, "r")) == (FILE *) 0)
		return;
	if (n_used_rumors < 0)
		init_rumors(rumf);
	if (!n_rumors)
		goto none;
	rn = rn2(n_rumors - n_used_rumors);
	i = 0;
	while (rn || used(i)) {
		(void) skipline(rumf);
		if (!used(i))
			rn--;
		i++;
	}
	usedbits[i / CHARSZ] |= (1 << (i % CHARSZ));
	n_used_rumors++;
	outline(rumf);
none:
	(void) fclose(rumf);
}

int
used(i)
	int             i;
{
	return (usedbits[i / CHARSZ] & (1 << (i % CHARSZ)));
}
