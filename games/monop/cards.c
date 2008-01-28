/*	$NetBSD: cards.c,v 1.15 2008/01/28 06:16:13 dholland Exp $	*/

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
#if 0
static char sccsid[] = "@(#)cards.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: cards.c,v 1.15 2008/01/28 06:16:13 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/endian.h>
#include "monop.ext"
#include "pathnames.h"

/*
 *	These routine deal with the card decks
 */

#define	GOJF	'F'	/* char for get-out-of-jail-free cards	*/

#ifndef DEV
static const char	*cardfile	= _PATH_CARDS;
#else
static const char	*cardfile	= "cards.pck";
#endif

static FILE	*deckf;

static void set_up(DECK *);
static void printmes(void);

/*
 *	This routine initializes the decks from the data file,
 * which it opens.
 */
void
init_decks()
{
	int32_t nc;

	if ((deckf=fopen(cardfile, "r")) == NULL) {
file_err:
		perror(cardfile);
		exit(1);
	}

	/* read number of community chest cards... */
	if (fread(&nc, sizeof(nc), 1, deckf) != 1)
		goto file_err;
	CC_D.num_cards = be32toh(nc);
	/* ... and number of community chest cards. */
	if (fread(&nc, sizeof(nc), 1, deckf) != 1)
		goto file_err;
	CH_D.num_cards = be32toh(nc);
	set_up(&CC_D);
	set_up(&CH_D);
}

/*
 *	This routine sets up the offset pointers for the given deck.
 */
static void
set_up(dp)
	DECK *dp;
{
	int r1, r2;
	int i;

	dp->offsets = (off_t *) calloc(dp->num_cards, sizeof (off_t));
	if (dp->offsets == NULL)
		errx(1, "out of memory");
	if (fread(dp->offsets, sizeof(off_t), dp->num_cards, deckf) !=
	    (unsigned) dp->num_cards) {
		perror(cardfile);
		exit(1);
	}
	/* convert offsets from big-endian byte order */
	for (i = 0; i < dp->num_cards; i++)
		BE64TOH(dp->offsets[i]);
	dp->last_card = 0;
	dp->gojf_used = FALSE;
	for (i = 0; i < dp->num_cards; i++) {
		off_t	temp;

		r1 = roll(1, dp->num_cards) - 1;
		r2 = roll(1, dp->num_cards) - 1;
		temp = dp->offsets[r2];
		dp->offsets[r2] = dp->offsets[r1];
		dp->offsets[r1] = temp;
	}
}

/*
 *	This routine draws a card from the given deck
 */
void
get_card(dp)
	DECK *dp;
{
	char type_maj, type_min;
	int num;
	int i, per_h, per_H, num_h, num_H;
	OWN *op;

	do {
		fseek(deckf, dp->offsets[dp->last_card], SEEK_SET);
		dp->last_card = ++(dp->last_card) % dp->num_cards;
		type_maj = getc(deckf);
	} while (dp->gojf_used && type_maj == GOJF);
	type_min = getc(deckf);
	num = ntohl(getw(deckf));
	printmes();
	switch (type_maj) {
	  case '+':		/* get money		*/
		if (type_min == 'A') {
			for (i = 0; i < num_play; i++)
				if (i != player)
					play[i].money -= num;
			num = num * (num_play - 1);
		}
		cur_p->money += num;
		break;
	  case '-':		/* lose money		*/
		if (type_min == 'A') {
			for (i = 0; i < num_play; i++)
				if (i != player)
					play[i].money += num;
			num = num * (num_play - 1);
		}
		cur_p->money -= num;
		break;
	  case 'M':		/* move somewhere	*/
		switch (type_min) {
		  case 'F':		/* move forward	*/
			num -= cur_p->loc;
			if (num < 0)
				num += 40;
			break;
		  case 'J':		/* move to jail	*/
			goto_jail();
			return;
		  case 'R':		/* move to railroad	*/
			spec = TRUE;
			num = (int)((cur_p->loc + 5)/10)*10 + 5 - cur_p->loc;
			break;
		  case 'U':		/* move to utility	*/
			spec = TRUE;
			if (cur_p->loc >= 12 && cur_p->loc < 28)
				num = 28 - cur_p->loc;
			else {
				num = 12 - cur_p->loc;
				if (num < 0)
					num += 40;
			}
			break;
		  case 'B':
			num = -num;
			break;
		}
		move(num);
		break;
	  case 'T':			/* tax			*/
		if (dp == &CC_D) {
			per_h = 40;
			per_H = 115;
		}
		else {
			per_h = 25;
			per_H = 100;
		}
		num_h = num_H = 0;
		for (op = cur_p->own_list; op; op = op->next)
			if (op->sqr->type == PRPTY) {
				if (op->sqr->desc->houses == 5)
					++num_H;
				else
					num_h += op->sqr->desc->houses;
			}
		num = per_h * num_h + per_H * num_H;
		printf(
		    "You had %d Houses and %d Hotels, so that cost you $%d\n",
		    num_h, num_H, num);
		if (num == 0)
			lucky("");
		else
			cur_p->money -= num;
		break;
	  case GOJF:		/* get-out-of-jail-free card	*/
		cur_p->num_gojf++;
		dp->gojf_used = TRUE;
		break;
	}
	spec = FALSE;
}

/*
 *	This routine prints out the message on the card
 */
static void
printmes()
{
	char c;

	printline();
	fflush(stdout);
	while ((c = getc(deckf)) != '\0')
		putchar(c);
	printline();
	fflush(stdout);
}
