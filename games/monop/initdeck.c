/*	$NetBSD: initdeck.c,v 1.13 2000/07/31 11:39:00 simonb Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifdef __NetBSD__
#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)initdeck.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: initdeck.c,v 1.13 2000/07/31 11:39:00 simonb Exp $");
#endif
#endif /* not lint */
#endif /* __NetBSD__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "deck.h"

#ifndef u_int32_t
#define	u_int32_t	unsigned int
#endif

u_int32_t
h2nl(u_int32_t h)
{
	unsigned char c[4];
	u_int32_t rv;

	c[0] = (h >> 24) & 0xff;
	c[1] = (h >> 16) & 0xff;
	c[2] = (h >>  8) & 0xff;
	c[3] = (h >>  0) & 0xff;
	memcpy(&rv, c, sizeof rv);

	return (rv);
}

/*
 *	This program initializes the card files for monopoly.
 * It reads in a data file with Com. Chest cards, followed by
 * the Chance card.  The two are seperated by a line of "%-".
 * All other cards are seperated by lines of "%%".  In the front
 * of the file is the data for the decks in the same order.
 * This includes the seek pointer for the start of each card.
 * All cards start with their execution code, followed by the
 * string to print, terminated with a null byte.
 */

#define	TRUE	1
#define	FALSE	0

#define	bool	char

const char	*infile		= "cards.inp",	/* input file		*/
		*outfile	= "cards.pck";	/* "packed" file	*/

DECK	deck[2];

FILE	*inf, *outf;

/* initdeck.c */
int		main(int, char *[]);
static void	getargs(int, char *[]);
static void	fwrite_be_offt(off_t, FILE *);
static void	count(void);
static void	putem(void);

int
main(ac, av)
	int ac;
	char *av[];
{
	int i, nc;

	/* sanity test */
	if (sizeof(int) != 4) {
		fprintf(stderr, "sizeof(int) != 4\n");
		exit(1);
	}

	getargs(ac, av);
	if ((inf = fopen(infile, "r")) == NULL) {
		perror(infile);
		exit(1);
	}
	count();
	/*
	 * allocate space for pointers.
	 */
	CC_D.offsets = (off_t *)calloc(CC_D.num_cards + 1, /* sizeof (off_t) */ 8);
	CH_D.offsets = (off_t *)calloc(CH_D.num_cards + 1, /* sizeof (off_t) */ 8);
	if (CC_D.offsets == NULL || CH_D.offsets == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	fseek(inf, 0L, SEEK_SET);
	if ((outf = fopen(outfile, "w")) == NULL) {
		perror(outfile);
		exit(1);
	}

	/*
	 * these fields will be overwritten after the offsets are calculated,
	 * so byte-order doesn't matter yet.
	 */
	fwrite(&nc, sizeof(nc), 1, outf);
	fwrite(&nc, sizeof(nc), 1, outf);
	fwrite(CC_D.offsets, /* sizeof (off_t) */ 8, CC_D.num_cards, outf);
	fwrite(CH_D.offsets, /* sizeof (off_t) */ 8, CH_D.num_cards, outf);

	/*
	 * write out the cards themselves (calculating the offsets).
	 */
	putem();

	fclose(inf);
	fseek(outf, 0, SEEK_SET);

	/* number of community chest cards first... */
	nc = h2nl(CC_D.num_cards);
	fwrite(&nc, sizeof(nc), 1, outf);
	/* ... then number of chance cards. */
	nc = h2nl(CH_D.num_cards);
	fwrite(&nc, sizeof(nc), 1, outf);

	/* dump offsets in big-endian byte order */
	for (i = 0; i < CC_D.num_cards; i++)
		fwrite_be_offt(CC_D.offsets[i], outf);
	for (i = 0; i < CH_D.num_cards; i++)
		fwrite_be_offt(CH_D.offsets[i], outf);

	fflush(outf);
	if (ferror(outf)) {
		perror(outfile);
		exit(1);
	}
	fclose(outf);
	printf("There were %d com. chest and %d chance cards\n",
	    CC_D.num_cards, CH_D.num_cards);
	exit(0);
}

static void
getargs(ac, av)
	int ac;
	char *av[];
{
	if (ac > 1)
		infile = av[1];
	if (ac > 2)
		outfile = av[2];
}

/*
 * count the cards
 */
static void
count() 
{
	bool newline;
	DECK *in_deck;
	int c;

	newline = TRUE;
	in_deck = &CC_D;
	while ((c=getc(inf)) != EOF)
		if (newline && c == '%') {
			newline = FALSE;
			in_deck->num_cards++;
			if (getc(inf) == '-')
				in_deck = &CH_D;
		}
		else
			newline = (c == '\n');
	in_deck->num_cards++;
}

/*
 *	put strings in the file
 */
static void
putem() 
{
	bool newline;
	DECK *in_deck;
	int c;
	int num;

	in_deck = &CC_D;
	CC_D.num_cards = 1;
	CH_D.num_cards = 0;
	CC_D.offsets[0] = ftell(outf);
	putc(getc(inf), outf);
	putc(getc(inf), outf);
	for (num = 0; (c=getc(inf)) != '\n'; )
		num = num * 10 + (c - '0');
	putw(h2nl(num), outf);
	newline = FALSE;
	while ((c=getc(inf)) != EOF)
		if (newline && c == '%') {
			putc('\0', outf);
			newline = FALSE;
			if (getc(inf) == '-')
				in_deck = &CH_D;
			while (getc(inf) != '\n')
				continue;
			in_deck->offsets[in_deck->num_cards++] = ftell(outf);
			if ((c=getc(inf)) == EOF)
				break;
			putc(c, outf);
			putc(c = getc(inf), outf);
			for (num = 0; (c=getc(inf)) != EOF && c != '\n'; )
				num = num * 10 + (c - '0');
			putw(h2nl(num), outf);
		}
		else {
			putc(c, outf);
			newline = (c == '\n');
		}
	putc('\0', outf);
}

/*
 * fwrite_be_offt:
 *	Write out the off paramater as a 64 bit big endian number
 */

static void
fwrite_be_offt(off, f)
	off_t	 off;
	FILE	*f;
{
	int		i;
	unsigned char	c[8];

	for (i = 7; i >= 0; i--) {
		c[i] = off & 0xff;
		off >>= 8;
	}
	fwrite(c, sizeof(c), 1, f);
}
