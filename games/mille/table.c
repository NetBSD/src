/*	$NetBSD: table.c,v 1.9 2009/05/25 23:24:54 dholland Exp $	*/

/*
 * Copyright (c) 1982, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1982, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)table.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: table.c,v 1.9 2009/05/25 23:24:54 dholland Exp $");
#endif
#endif /* not lint */

# define	DEBUG

/*
 * @(#)table.c	1.1 (Berkeley) 4/1/82
 */

# include	"mille.h"

int	main(int, char **);

int
main(int argc, char *argv[])
{
	int	i, j, count;

	printf("   %16s -> %5s %5s %4s %s\n", "Card", "cards", "count",
	    "need", "opposite");
	for (i = 0; i < NUM_CARDS - 1; i++) {
		for (j = 0, count = 0; j < DECK_SZ; j++)
			if (Deck[j] == i)
				count++;
		printf("%2d %16s -> %5d %5d %4d %s\n", i, C_name[i],
		    Numcards[i], count, Numneed[i], C_name[opposite(i)]);
	}
}
