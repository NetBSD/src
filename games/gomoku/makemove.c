/*	$NetBSD: makemove.c,v 1.42 2022/05/29 17:01:42 rillig Exp $	*/

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
/*	@(#)makemove.c	8.2 (Berkeley) 5/3/95	*/
__RCSID("$NetBSD: makemove.c,v 1.42 2022/05/29 17:01:42 rillig Exp $");

#include "gomoku.h"

const int     dd[4] = {		/* direction deltas */
	1,			/* right */
	-(BSZ + 1) + 1,		/* down + right */
	-(BSZ + 1),		/* down */
	-(BSZ + 1) - 1		/* down + left */
};

static const int weight[5] = { 0, 1, 7, 22, 100 };

static void update_overlap(spot_index);

static bool
is_tie(void)
{

	for (int y = 1; y <= BSZ; y++)
		for (int x = 1; x <= BSZ; x++)
			if (board[PT(x, y)].s_wval != 0)
				return false;
	return true;
}

static void
sortframes_remove(struct combostr *cbp)
{

	if (cbp->c_next == NULL)
		return;

	if (sortframes[BLACK] == cbp)
		sortframes[BLACK] = cbp->c_next;
	if (sortframes[WHITE] == cbp)
		sortframes[WHITE] = cbp->c_next;
	cbp->c_next->c_prev = cbp->c_prev;
	cbp->c_prev->c_next = cbp->c_next;
}

static int
old_weight_value(const struct spotstr *sp, direction r)
{
	union comboval cb;
	int val = 0;
	if ((cb = sp->s_fval[BLACK][r]).s <= 0x500)
		val += weight[5 - cb.cv_force - cb.cv_win];
	if ((cb = sp->s_fval[WHITE][r]).s <= 0x500)
		val += weight[5 - cb.cv_force - cb.cv_win];
	return val;
}

/*
 * Return values:
 *	MOVEOK	everything is OK.
 *	RESIGN	Player resigned.
 *	ILLEGAL	Illegal move.
 *	WIN	The winning move was just played.
 *	TIE	The game is a tie.
 */
int
makemove(player_color us, spot_index mv)
{

	/* check for end of game */
	if (mv == RESIGN)
		return RESIGN;

	/* check for illegal move */
	struct spotstr *sp = &board[mv];
	if (sp->s_occ != EMPTY)
		return ILLEGAL;

	/* make move */
	sp->s_occ = us;
	game.moves[game.nmoves++] = mv;

	/* compute new frame values */
	sp->s_wval = 0;
	for (direction r = 4; r-- > 0; ) {
	    int d = dd[r];
	    struct spotstr *fsp = &board[mv];

	    for (int f = 5; --f >= 0; fsp -= d) {	/* for each frame */
		if (fsp->s_occ == BORDER)
		    goto nextr;
		if ((fsp->s_flags & BFLAG << r) != 0)
		    continue;

		struct combostr *cbp = &frames[fsp->s_frame[r]];
		sortframes_remove(cbp);

		int val = old_weight_value(fsp, r);

		/* compute new combo value for this frame */
		bool space = fsp->s_occ == EMPTY;
		int n = 0;
		sp = fsp;
		for (int off = 5; off-- > 0; sp += d) {	/* for each spot */
		    if (sp->s_occ == us)
			n++;
		    else if (sp->s_occ == EMPTY)
			sp->s_wval -= val;
		    else {
			/* this frame is now blocked, adjust values */
			fsp->s_flags |= BFLAG << r;
			fsp->s_fval[BLACK][r].s = 0x600;
			fsp->s_fval[WHITE][r].s = 0x600;
			while (off-- > 0) {
			    sp += d;
			    if (sp->s_occ == EMPTY)
				sp->s_wval -= val;
			}
			goto nextf;
		    }
		}

		/* check for game over */
		if (n == 5) {
		    game.win_spot = (spot_index)(fsp - board);
		    game.win_dir = r;
		    return WIN;
		}

		/* compute new value & combo number for this frame & color */
		player_color them = us != BLACK ? BLACK : WHITE;
		fsp->s_fval[them][r].s = 0x600;
		union comboval *cp = &fsp->s_fval[us][r];
		/* both ends open? */
		if (space && sp->s_occ == EMPTY) {
		    cp->cv_force = 4 - n;
		    cp->cv_win = 1;
		} else {
		    cp->cv_force = 5 - n;
		    cp->cv_win = 0;
		}
		val = weight[n];
		sp = fsp;
		for (int off = 5; off-- > 0; sp += d)	/* for each spot */
		    if (sp->s_occ == EMPTY)
			sp->s_wval += val;

		/* add this frame to the sorted list of frames by combo value */
		struct combostr *cbp1 = sortframes[us];
		if (cbp1 == NULL)
		    sortframes[us] = cbp->c_next = cbp->c_prev = cbp;
		else {
		    union comboval *cp1 =
			&board[cbp1->c_vertex].s_fval[us][cbp1->c_dir];
		    if (cp->s <= cp1->s) {
			/* insert at the head of the list */
			sortframes[us] = cbp;
		    } else {
			do {
			    cbp1 = cbp1->c_next;
			    cp1 = &board[cbp1->c_vertex].s_fval[us][cbp1->c_dir];
			    if (cp->s <= cp1->s)
				break;
			} while (cbp1 != sortframes[us]);
		    }
		    cbp->c_next = cbp1;
		    cbp->c_prev = cbp1->c_prev;
		    cbp1->c_prev->c_next = cbp;
		    cbp1->c_prev = cbp;
		}

	    nextf:
		;
	    }

	    /* both ends open? */
	    if (fsp->s_occ == EMPTY) {
		union comboval *cp = &fsp->s_fval[BLACK][r];
		if (cp->cv_win != 0) {
		    cp->cv_force++;
		    cp->cv_win = 0;
		}
		cp = &fsp->s_fval[WHITE][r];
		if (cp->cv_win != 0) {
		    cp->cv_force++;
		    cp->cv_win = 0;
		}
	    }

	nextr:
	    ;
	}

	update_overlap(mv);

	if (is_tie())
		return TIE;

	return MOVEOK;
}

static void
update_overlap_same_direction(spot_index s1, spot_index s2,
			      frame_index a, int d, int off_minus_f,
			      direction r)
{
	/*
	 * count the number of empty spots to see if there is
	 * still an overlap.
	 */
	int n = 0;
	spot_index s = s1;
	spot_index es = 0;
	for (int b = off_minus_f; b < 5; b++, s += d) {
		if (board[s].s_occ == EMPTY) {
			es = s;	/* save the intersection point */
			n++;
		}
	}

	frame_index b = board[s2].s_frame[r];
	if (n == 0) {
		if (board[s].s_occ == EMPTY) {
			overlap[a * FAREA + b] &= 0xA;
			overlap[b * FAREA + a] &= 0xC;
			intersect[a * FAREA + b] = s;
			intersect[b * FAREA + a] = s;
		} else {
			overlap[a * FAREA + b] = 0;
			overlap[b * FAREA + a] = 0;
		}
	} else if (n == 1) {
		if (board[s].s_occ == EMPTY) {
			overlap[a * FAREA + b] &= 0xAF;
			overlap[b * FAREA + a] &= 0xCF;
		} else {
			overlap[a * FAREA + b] &= 0xF;
			overlap[b * FAREA + a] &= 0xF;
		}
		intersect[a * FAREA + b] = es;
		intersect[b * FAREA + a] = es;
	}
	/* else no change, still multiple overlap */
}

/*
 * The last move was at 'os', which is part of frame 'a'. There are 6 frames
 * with direction 'rb' that cross frame 'a' in 'os'. Since the spot 'os'
 * cannot be used as a double threat anymore, mark each of these crossing
 * frames as non-overlapping with frame 'a'.
 */
static void
update_overlap_different_direction(spot_index os, frame_index a, direction rb)
{

	int db = dd[rb];
	for (int off = 0; off < 6; off++) {
		const struct spotstr *sp = &board[os - db * off];
		if (sp->s_occ == BORDER)
			break;
		if ((sp->s_flags & BFLAG << rb) != 0)
			continue;

		frame_index b = sp->s_frame[rb];
		overlap[a * FAREA + b] = 0;
		overlap[b * FAREA + a] = 0;
	}
}

/*
 * fix up the overlap array according to the changed 'os'.
 */
static void
update_overlap(spot_index os)
{

	for (direction r = 4; r-- > 0; ) {
	    int d = dd[r];
	    spot_index s1 = os;

	    /* for each frame 'a' that contains the spot 'os' */
	    for (int f = 0; f < 6; f++, s1 -= d) {
		if (board[s1].s_occ == BORDER)
		    break;
		if ((board[s1].s_flags & BFLAG << r) != 0)
		    continue;

		/*
		 * Update all other frames that intersect the current one
		 * to indicate whether they still overlap or not.
		 * Since F1 overlap F2 == F2 overlap F1, we only need to
		 * do the rows 0 <= r1 <= r. The r1 == r case is special
		 * since the two frames can overlap in more than one spot.
		 */
		frame_index a = board[s1].s_frame[r];

		spot_index s2 = s1 - d;
		for (int off = f + 1; off < 6; off++, s2 -= d) {
		    if (board[s2].s_occ == BORDER)
			break;
		    if ((board[s2].s_flags & BFLAG << r) != 0)
			continue;

		    update_overlap_same_direction(s1, s2, a, d, off - f, r);
		}

		/* the other directions can only intersect at spot 'os' */
		for (direction rb = 0; rb < r; rb++)
			update_overlap_different_direction(os, a, rb);
	    }
	}
}
