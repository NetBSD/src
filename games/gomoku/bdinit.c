/*	$NetBSD: bdinit.c,v 1.35 2022/05/29 16:30:44 rillig Exp $	*/

/*
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
/*	from: @(#)bdinit.c	8.2 (Berkeley) 5/3/95	*/
__RCSID("$NetBSD: bdinit.c,v 1.35 2022/05/29 16:30:44 rillig Exp $");

#include <string.h>
#include "gomoku.h"

static void init_overlap(void);

static void
init_spot_flags_and_fval(struct spotstr *sp, int col, int row)
{

	sp->s_flags = 0;
	if (row < 5) {
		/* directions 1, 2, 3 are blocked */
		sp->s_flags |= (BFLAG << 1) | (BFLAG << 2) |
		    (BFLAG << 3);
		sp->s_fval[BLACK][1].s = 0x600;
		sp->s_fval[BLACK][2].s = 0x600;
		sp->s_fval[BLACK][3].s = 0x600;
		sp->s_fval[WHITE][1].s = 0x600;
		sp->s_fval[WHITE][2].s = 0x600;
		sp->s_fval[WHITE][3].s = 0x600;
	} else if (row == 5) {
		/* five spaces, blocked on one side */
		sp->s_fval[BLACK][1].s = 0x500;
		sp->s_fval[BLACK][2].s = 0x500;
		sp->s_fval[BLACK][3].s = 0x500;
		sp->s_fval[WHITE][1].s = 0x500;
		sp->s_fval[WHITE][2].s = 0x500;
		sp->s_fval[WHITE][3].s = 0x500;
	} else {
		/* six spaces, not blocked */
		sp->s_fval[BLACK][1].s = 0x401;
		sp->s_fval[BLACK][2].s = 0x401;
		sp->s_fval[BLACK][3].s = 0x401;
		sp->s_fval[WHITE][1].s = 0x401;
		sp->s_fval[WHITE][2].s = 0x401;
		sp->s_fval[WHITE][3].s = 0x401;
	}
	if (col > (BSZ - 4)) {
		/* directions 0, 1 are blocked */
		sp->s_flags |= BFLAG | (BFLAG << 1);
		sp->s_fval[BLACK][0].s = 0x600;
		sp->s_fval[BLACK][1].s = 0x600;
		sp->s_fval[WHITE][0].s = 0x600;
		sp->s_fval[WHITE][1].s = 0x600;
	} else if (col == (BSZ - 4)) {
		sp->s_fval[BLACK][0].s = 0x500;
		sp->s_fval[WHITE][0].s = 0x500;
		/* if direction 1 is not blocked */
		if ((sp->s_flags & (BFLAG << 1)) == 0) {
			sp->s_fval[BLACK][1].s = 0x500;
			sp->s_fval[WHITE][1].s = 0x500;
		}
	} else {
		sp->s_fval[BLACK][0].s = 0x401;
		sp->s_fval[WHITE][0].s = 0x401;
		if (col < 5) {
			/* direction 3 is blocked */
			sp->s_flags |= (BFLAG << 3);
			sp->s_fval[BLACK][3].s = 0x600;
			sp->s_fval[WHITE][3].s = 0x600;
		} else if (col == 5 &&
		    (sp->s_flags & (BFLAG << 3)) == 0) {
			sp->s_fval[BLACK][3].s = 0x500;
			sp->s_fval[WHITE][3].s = 0x500;
		}
	}
}

/* Allocate one of the pre-allocated frames for each non-blocked frame. */
static void
init_spot_frame(struct spotstr *sp, frame_index *fip)
{

	for (direction r = 4; r-- > 0; ) {
		if ((sp->s_flags & (BFLAG << r)) != 0)
			continue;

		frame_index fi = (*fip)++;
		sp->s_frame[r] = fi;

		struct combostr *cbp = &frames[fi];
		cbp->c_combo.s = sp->s_fval[BLACK][r].s;
		cbp->c_vertex = (spot_index)(sp - board);
		cbp->c_nframes = 1;
		cbp->c_dir = r;
	}
}

void
init_board(void)
{

	game.nmoves = 0;
	game.win_spot = 0;
	game.user_x = 1 + (BSZ - 1) / 2;
	game.user_y = 1 + (BSZ - 1) / 2;

	struct spotstr *sp = board;
	for (int i = 0; i < 1 + BSZ + 1; i++, sp++) {
		sp->s_occ = BORDER;	/* bottom border and corners */
		sp->s_flags = BFLAGALL;
	}

	/* fill the playing area of the board with EMPTY spots */
	frame_index fi = 0;
	memset(frames, 0, sizeof(frames));
	for (int row = 1; row <= BSZ; row++, sp++) {
		for (int col = 1; col <= BSZ; col++, sp++) {
			sp->s_occ = EMPTY;
			sp->s_wval = 0;
			init_spot_flags_and_fval(sp, col, row);
			init_spot_frame(sp, &fi);
		}
		sp->s_occ = BORDER;	/* combined left and right border */
		sp->s_flags = BFLAGALL;
	}

	for (int i = 0; i < BSZ + 1; i++, sp++) {
		sp->s_occ = BORDER;	/* top border and top-right corner */
		sp->s_flags = BFLAGALL;
	}

	sortframes[BLACK] = NULL;
	sortframes[WHITE] = NULL;

	init_overlap();
}

/*
 * Variable names for frames A and B:
 *
 * fi	index of the frame in the global 'frames'
 * d	direction delta, difference between adjacent spot indexes
 * off	index of the spot in the frame, 0 to 5
 */

/*
 * Each entry in the overlap array is a bit mask with eight bits corresponding
 * to whether frame B overlaps frame A (as indexed by overlap[A * FAREA + B]).
 *
 * The eight bits correspond to whether A and B are open-ended (length 6) or
 * closed (length 5).
 *
 *	0	A closed and B closed
 *	1	A closed and B open
 *	2	A open and B closed
 *	3	A open and B open
 *	4	A closed and B closed and overlaps in more than one spot
 *	5	A closed and B open and overlaps in more than one spot
 *	6	A open and B closed and overlaps in more than one spot
 *	7	A open and B open and overlaps in more than one spot
 *
 * As pieces are played during the game, frames that no longer share an empty
 * spot will be removed from the overlap array by setting the entry to 0.
 */
static u_char
adjust_overlap(u_char ov, int ra, int offa, int rb, int offb, int mask)
{
	ov |= (offb == 5) ? mask & 0xA : mask;
	if (rb != ra)
		return ov;

	/* compute the multiple spot overlap values */
	switch (offa) {
	case 0:
		if (offb == 4)
			ov |= 0xA0;
		else if (offb != 5)
			ov |= 0xF0;
		break;
	case 1:
		if (offb == 5)
			ov |= 0xA0;
		else
			ov |= 0xF0;
		break;
	case 4:
		if (offb == 0)
			ov |= 0xC0;
		else
			ov |= 0xF0;
		break;
	case 5:
		if (offb == 1)
			ov |= 0xC0;
		else if (offb != 0)
			ov |= 0xF0;
		break;
	default:
		ov |= 0xF0;
	}

	return ov;
}

/*
 * Given a single spot 's' of frame A, update the overlap information for
 * each frame B that overlaps frame A in that spot.
 */
static void
init_overlap_frame(int fia, int ra, int offa, spot_index s, int mask)
{

	for (int rb = 4; --rb >= 0;) {
		int db = dd[rb];

		for (int offb = 0; offb < 6; offb++) {
			/* spb0 is the spot where frame B starts. */
			const struct spotstr *spb0 = &board[s - offb * db];
			if (spb0->s_occ == BORDER)
				break;
			if ((spb0->s_flags & BFLAG << rb) != 0)
				continue;

			frame_index fib = spb0->s_frame[rb];
			intersect[fia * FAREA + fib] = s;
			u_char *op = &overlap[fia * FAREA + fib];
			*op = adjust_overlap(*op, ra, offa, rb, offb, mask);
		}
	}
}

static void
init_overlap(void)
{

	memset(overlap, 0, sizeof(overlap));
	memset(intersect, 0, sizeof(intersect));

	for (int fia = FAREA; fia-- > 0;) {
		const struct combostr *fa = &frames[fia];
		spot_index s = fa->c_vertex;
		u_char ra = fa->c_dir;
		int da = dd[ra];

		/*
		 * len = 5 if closed, 6 if open. At this early stage, Black
		 * and White have the same value for cv_win.
		 */
		int len = 5 + board[s].s_fval[BLACK][ra].cv_win;

		for (int offa = 0; offa < len; offa++) {
			/* spot[5] in frame A only overlaps if it is open */
			int mask = (offa == 5) ? 0xC : 0xF;

			init_overlap_frame(fia, ra, offa, s + offa * da, mask);
		}
	}
}
