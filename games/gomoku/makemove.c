/*	$NetBSD: makemove.c,v 1.22 2022/05/27 20:35:58 rillig Exp $	*/

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
__RCSID("$NetBSD: makemove.c,v 1.22 2022/05/27 20:35:58 rillig Exp $");

#include "gomoku.h"

		/* direction deltas */
const int     dd[4] = {
	MRIGHT, MRIGHT+MDOWN, MDOWN, MDOWN+MLEFT
};

static const int weight[5] = { 0, 1, 7, 22, 100 };

static void update_overlap(struct spotstr *);

/*
 * Return values:
 *	MOVEOK	everything is OK.
 *	RESIGN	Player resigned.
 *	ILLEGAL	Illegal move.
 *	WIN	The winning move was just played.
 *	TIE	The game is a tie.
 */
int
makemove(int us, int mv)
{
	struct spotstr *sp, *fsp;
	union comboval *cp;
	struct spotstr *osp;
	struct combostr *cbp, *cbp1;
	union comboval *cp1;
	int d, n;
	int val, bmask;
	bool space;

	/* check for end of game */
	if (mv == RESIGN)
		return RESIGN;

	/* check for illegal move */
	sp = &board[mv];
	if (sp->s_occ != EMPTY)
		return ILLEGAL;

	/* make move */
	sp->s_occ = us;
	movelog[nmoves++] = mv;

	/*
	 * FIXME: The last spot on the board can still complete a frame.
	 *
	 * Example game: A1 B1 C1 D1 E1 F1 G1 H1 J1 K1 L1 M1 N1 O1 P1 Q1 R1
	 * S1 T1 A2 B2 C2 D2 E2 F2 G2 H2 J2 K2 L2 M2 N2 O2 P2 Q2 R2 S2 T2 A3
	 * B3 C3 D3 E3 F3 G3 H3 J3 K3 L3 M3 N3 O3 P3 Q3 R3 S3 T3 A4 B4 C4 D4
	 * E4 F4 G4 H4 J4 K4 L4 M4 N4 O4 P4 Q4 R4 S4 T4 B5 C5 D5 E5 F5 G5 H5
	 * J5 K5 L5 M5 N5 O5 P5 Q5 R5 S5 T5 B6 C6 D6 E6 F6 G6 H6 J6 K6 L6 M6
	 * N6 O6 P6 Q6 R6 S6 T6 B7 C7 D7 E7 F7 G7 H7 J7 K7 L7 M7 N7 O7 P7 Q7
	 * R7 S7 T7 A8 B8 C8 D8 E8 F8 G8 H8 J8 K8 L8 M8 N8 O8 P8 Q8 R8 S8 T8
	 * A9 B9 C9 D9 E9 F9 G9 H9 J9 K9 L9 M9 N9 O9 P9 Q9 R9 S9 T9 A10 B10
	 * C10 D10 E10 F10 G10 H10 J10 K10 L10 M10 N10 O10 P10 Q10 R10 S10
	 * T10 B11 C11 D11 E11 F11 G11 H11 J11 K11 L11 M11 N11 O11 P11 Q11
	 * R11 S11 T11 B12 C12 D12 E12 F12 G12 H12 J12 K12 L12 M12 N12 O12
	 * P12 Q12 R12 S12 T12 B13 C13 D13 E13 F13 G13 H13 J13 K13 L13 M13
	 * N13 O13 P13 Q13 R13 S13 T13 A14 B14 C14 D14 E14 F14 G14 H14 J14
	 * K14 L14 M14 N14 O14 P14 Q14 R14 S14 T14 A15 B15 C15 D15 E15 F15
	 * G15 H15 J15 K15 L15 M15 N15 O15 P15 Q15 R15 S15 T15 A16 B16 C16
	 * D16 E16 F16 G16 H16 J16 K16 L16 M16 N16 O16 P16 Q16 R16 S16 T16
	 * B17 C17 D17 E17 F17 G17 H17 J17 K17 L17 M17 N17 O17 P17 Q17 R17
	 * S17 T17 B18 C18 D18 E18 F18 G18 H18 J18 K18 L18 M18 N18 O18 P18
	 * Q18 R18 S18 T18 A11 A5 A12 A6 A13 A7 A18 A17 A19 B19 C19 D19 E19
	 * F19 G19 H19 J19 K19 L19 T19 M19 S19 N19 R19 O19 Q19
	 *
	 * Now P19 is the last remaining spot, and when Black plays there,
	 * the frame L19-P19 is complete.
	 */
	if (nmoves == BSZ * BSZ)
		return TIE;

	/* compute new frame values */
	sp->s_wval = 0;
	osp = sp;
	for (int r = 4; --r >= 0; ) {		/* for each direction */
	    d = dd[r];
	    fsp = osp;
	    bmask = BFLAG << r;
	    for (int f = 5; --f >= 0; fsp -= d) {	/* for each frame */
		if (fsp->s_occ == BORDER)
		    goto nextr;
		if ((fsp->s_flags & bmask) != 0)
		    continue;

		/* remove this frame from the sorted list of frames */
		cbp = fsp->s_frame[r];
		if (cbp->c_next != NULL) {
			if (sortframes[BLACK] == cbp)
			    sortframes[BLACK] = cbp->c_next;
			if (sortframes[WHITE] == cbp)
			    sortframes[WHITE] = cbp->c_next;
			cbp->c_next->c_prev = cbp->c_prev;
			cbp->c_prev->c_next = cbp->c_next;
		}

		/* compute old weight value for this frame */
		cp = &fsp->s_fval[BLACK][r];
		if (cp->s <= 0x500)
		    val = weight[5 - cp->cv_force - cp->cv_win];
		else
		    val = 0;
		cp = &fsp->s_fval[WHITE][r];
		if (cp->s <= 0x500)
		    val += weight[5 - cp->cv_force - cp->cv_win];

		/* compute new combo value for this frame */
		sp = fsp;
		space = sp->s_occ == EMPTY;
		n = 0;
		for (int i = 5; --i >= 0; sp += d) {	/* for each spot */
		    if (sp->s_occ == us)
			n++;
		    else if (sp->s_occ == EMPTY)
			sp->s_wval -= val;
		    else {
			/* this frame is now blocked, adjust values */
			fsp->s_flags |= bmask;
			fsp->s_fval[BLACK][r].s = 0x600;
			fsp->s_fval[WHITE][r].s = 0x600;
			while (--i >= 0) {
			    sp += d;
			    if (sp->s_occ == EMPTY)
				sp->s_wval -= val;
			}
			goto nextf;
		    }
		}

		/* check for game over */
		if (n == 5)
		    return(WIN);

		/* compute new value & combo number for this frame & color */
		fsp->s_fval[us != BLACK ? BLACK : WHITE][r].s = 0x600;
		cp = &fsp->s_fval[us][r];
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
		for (int i = 5; --i >= 0; sp += d)	/* for each spot */
		    if (sp->s_occ == EMPTY)
			sp->s_wval += val;

		/* add this frame to the sorted list of frames by combo value */
		cbp1 = sortframes[us];
		if (cbp1 == NULL)
		    sortframes[us] = cbp->c_next = cbp->c_prev = cbp;
		else {
		    cp1 = &board[cbp1->c_vertex].s_fval[us][cbp1->c_dir];
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
		cp = &fsp->s_fval[BLACK][r];
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

	update_overlap(osp);

	return MOVEOK;
}

/*
 * fix up the overlap array due to updating spot osp.
 */
static void
update_overlap(struct spotstr *osp)
{
	struct spotstr *sp, *sp1, *sp2;
	int d, d1, n;
	int a, b, bmask, bmask1;
	struct spotstr *esp;
	u_char *str;

	esp = NULL;
	for (int r = 4; --r >= 0; ) {		/* for each direction */
	    d = dd[r];
	    sp1 = osp;
	    bmask = BFLAG << r;
	    for (int f = 0; f < 6; f++, sp1 -= d) {	/* for each frame */
		if (sp1->s_occ == BORDER)
		    break;
		if ((sp1->s_flags & bmask) != 0)
		    continue;
		/*
		 * Update all other frames that intersect the current one
		 * to indicate whether they still overlap or not.
		 * Since F1 overlap F2 == F2 overlap F1, we only need to
		 * do the rows 0 <= r1 <= r. The r1 == r case is special
		 * since the two frames can overlap at more than one point.
		 */
		str = &overlap[(a = (int)(sp1->s_frame[r] - frames)) * FAREA];
		sp2 = sp1 - d;
		for (int i = f + 1; i < 6; i++, sp2 -= d) {
		    if (sp2->s_occ == BORDER)
			break;
		    if ((sp2->s_flags & bmask) != 0)
			continue;
		    /*
		     * count the number of empty spots to see if there is
		     * still an overlap.
		     */
		    n = 0;
		    sp = sp1;
		    for (b = i - f; b < 5; b++, sp += d) {
			if (sp->s_occ == EMPTY) {
			    esp = sp;	/* save the intersection point */
			    n++;
			}
		    }
		    b = (int)(sp2->s_frame[r] - frames);
		    if (n == 0) {
			if (sp->s_occ == EMPTY) {
			    str[b] &= 0xA;
			    overlap[b * FAREA + a] &= 0xC;
			    intersect[a * FAREA + b] = n = (int)(sp - board);
			    intersect[b * FAREA + a] = n;
			} else {
			    str[b] = 0;
			    overlap[b * FAREA + a] = 0;
			}
		    } else if (n == 1) {
			if (sp->s_occ == EMPTY) {
			    str[b] &= 0xAF;
			    overlap[b * FAREA + a] &= 0xCF;
			} else {
			    str[b] &= 0xF;
			    overlap[b * FAREA + a] &= 0xF;
			}
			intersect[a * FAREA + b] = n = (int)(esp - board);
			intersect[b * FAREA + a] = n;
		    }
		    /* else no change, still multiple overlap */
		}

		/* the other directions can only intersect at spot osp */
		for (int r1 = r; --r1 >= 0; ) {
		    d1 = dd[r1];
		    bmask1 = BFLAG << r1;
		    sp = osp;
		    for (int i = 6; --i >= 0; sp -= d1) { /* for each spot */
			if (sp->s_occ == BORDER)
			    break;
			if ((sp->s_flags & bmask1) != 0)
			    continue;
			b = (int)(sp->s_frame[r1] - frames);
			str[b] = 0;
			overlap[b * FAREA + a] = 0;
		    }
		}
	    }
	}
}
