/*	$NetBSD: pickmove.c,v 1.57 2022/05/29 13:49:10 rillig Exp $	*/

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
/*	@(#)pickmove.c	8.2 (Berkeley) 5/3/95	*/
__RCSID("$NetBSD: pickmove.c,v 1.57 2022/05/29 13:49:10 rillig Exp $");

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <limits.h>

#include "gomoku.h"

#define BITS_PER_INT	(sizeof(int) * CHAR_BIT)
#define MAPSZ		(BAREA / BITS_PER_INT)

#define BIT_SET(a, b)	((a)[(unsigned int)(b)/BITS_PER_INT] |= (1 << ((unsigned int)(b) % BITS_PER_INT)))
#define BIT_TEST(a, b)	(((a)[(unsigned int)(b)/BITS_PER_INT] & (1 << ((unsigned int)(b) % BITS_PER_INT))) != 0)

/*
 * This structure is used to store overlap information between frames.
 */
struct overlap_info {
	int		o_intersect;	/* intersection spot */
	u_char		o_off;		/* offset in frame of intersection */
	u_char		o_frameindex;	/* intersection frame index */
};

static struct combostr *hashcombos[FAREA];/* hash list for finding duplicates */
static struct combostr *sortcombos;	/* combos at higher levels */
static int combolen;			/* number of combos in sortcombos */
static int nextcolor;			/* color of next move */
static int elistcnt;			/* count of struct elist allocated */
static int combocnt;			/* count of struct combostr allocated */
static int forcemap[MAPSZ];		/* map for blocking <1,x> combos */
static int tmpmap[MAPSZ];		/* map for blocking <1,x> combos */
static int nforce;			/* count of opponent <1,x> combos */

static bool better(spot_index, spot_index, int);
static void scanframes(int);
static void makecombo2(struct combostr *, struct spotstr *, int, int);
static void addframes(unsigned int);
static void makecombo(struct combostr *, struct spotstr *, int, int);
static void appendcombo(struct combostr *, int);
static void updatecombo(struct combostr *, int);
static void makeempty(struct combostr *);
static int checkframes(struct combostr *, struct combostr *, struct spotstr *,
		    int, struct overlap_info *);
static bool sortcombo(struct combostr **, struct combostr **, struct combostr *);
#if !defined(DEBUG)
static void printcombo(struct combostr *, char *, size_t);
#endif

int
pickmove(int us)
{

	/* first move is easy */
	if (game.nmoves == 0)
		return PT((BSZ + 1) / 2, (BSZ + 1) / 2);

	/* initialize all the board values */
	for (unsigned pos = PT(BSZ, BSZ + 1); pos-- > PT(1, 1); ) {
		struct spotstr *sp = &board[pos];
		sp->s_combo[BLACK].s = 0x601;
		sp->s_combo[WHITE].s = 0x601;
		sp->s_level[BLACK] = 255;
		sp->s_level[WHITE] = 255;
		sp->s_nforce[BLACK] = 0;
		sp->s_nforce[WHITE] = 0;
		sp->s_flags &= ~(FFLAGALL | MFLAGALL);
	}
	nforce = 0;
	memset(forcemap, 0, sizeof(forcemap));

	/* compute new values */
	nextcolor = us;
	/*
	 * TODO: Scanning for both frames misses that after loading the game
	 *  K10 J9 M10 J10 O10 J11 Q10 J8 and playing K9, there are 2
	 *  immediate winning moves J12 and J7. Finding the winning move
	 *  takes too long.
	 */
	scanframes(BLACK);
	scanframes(WHITE);

	/* find the spot with the highest value */
	spot_index s = PT(BSZ, BSZ);
	spot_index s1 = s;
	spot_index s2 = s;
	for ( ; s-- > PT(1, 1); ) {
		struct spotstr *sp = &board[s];
		if (sp->s_occ != EMPTY)
			continue;
		if (debug != 0 && (sp->s_combo[BLACK].cv_force == 1 ||
		    sp->s_combo[WHITE].cv_force == 1)) {
			debuglog("- %s %x/%d %d %x/%d %d %d",
			    stoc(s),
			    sp->s_combo[BLACK].s, sp->s_level[BLACK],
			    sp->s_nforce[BLACK],
			    sp->s_combo[WHITE].s, sp->s_level[WHITE],
			    sp->s_nforce[WHITE],
			    sp->s_wval);
		}
		/* pick the best black move */
		if (better(s, s1, BLACK))
			s1 = s;
		/* pick the best white move */
		if (better(s, s2, WHITE))
			s2 = s;
	}

	if (debug != 0) {
		const struct spotstr *sp1 = &board[s1], *sp2 = &board[s2];
		debuglog("B %s %x/%d %d %x/%d %d %d",
		    stoc(s1),
		    sp1->s_combo[BLACK].s, sp1->s_level[BLACK],
		    sp1->s_nforce[BLACK],
		    sp1->s_combo[WHITE].s, sp1->s_level[WHITE],
		    sp1->s_nforce[WHITE], sp1->s_wval);
		debuglog("W %s %x/%d %d %x/%d %d %d",
		    stoc(s2),
		    sp2->s_combo[WHITE].s, sp2->s_level[WHITE],
		    sp2->s_nforce[WHITE],
		    sp2->s_combo[BLACK].s, sp2->s_level[BLACK],
		    sp2->s_nforce[BLACK], sp2->s_wval);
		/*
		 * Check for more than one force that can't
		 * all be blocked with one move.
		 */
		spot_index m = us == BLACK ? s2 : s1;
		int them = us != BLACK ? BLACK : WHITE;
		if (board[m].s_combo[them].cv_force == 1 &&
		    !BIT_TEST(forcemap, m))
			debuglog("*** Can't be blocked");
	}

	union comboval *Ocp, *Tcp;
	if (us == BLACK) {
		Ocp = &board[s1].s_combo[BLACK];
		Tcp = &board[s2].s_combo[WHITE];
	} else {
		Tcp = &board[s1].s_combo[BLACK];
		Ocp = &board[s2].s_combo[WHITE];

		spot_index tmp = s1;
		s1 = s2;
		s2 = tmp;
	}
	/*
	 * Block their combo only if we have to (i.e., if they are one move
	 * away from completing a force, and we don't have a force that
	 * we can complete which takes fewer moves to win).
	 */
	if (Tcp->cv_force <= 1 &&
	    !(Ocp->cv_force <= 1 &&
	      Tcp->cv_force + Tcp->cv_win >= Ocp->cv_force + Ocp->cv_win))
		return s2;
	return s1;
}

/*
 * Return true if spot 'sp' is better than spot 'sp1' for color 'us'.
 */
static bool
better(spot_index s, spot_index s1, int us)
{
	const struct spotstr *sp = &board[s], *sp1 = &board[s1];

	if (/* .... */ sp->s_combo[us].s != sp1->s_combo[us].s)
		return sp->s_combo[us].s < sp1->s_combo[us].s;
	if (/* .... */ sp->s_level[us] != sp1->s_level[us])
		return sp->s_level[us] < sp1->s_level[us];
	if (/* .... */ sp->s_nforce[us] != sp1->s_nforce[us])
		return sp->s_nforce[us] > sp1->s_nforce[us];

	int them = us != BLACK ? BLACK : WHITE;
	if (BIT_TEST(forcemap, s) != BIT_TEST(forcemap, s1))
		return BIT_TEST(forcemap, s);

	if (/* .... */ sp->s_combo[them].s != sp1->s_combo[them].s)
		return sp->s_combo[them].s < sp1->s_combo[them].s;
	if (/* .... */ sp->s_level[them] != sp1->s_level[them])
		return sp->s_level[them] < sp1->s_level[them];
	if (/* .... */ sp->s_nforce[them] != sp1->s_nforce[them])
		return sp->s_nforce[them] > sp1->s_nforce[them];

	if (/* .... */ sp->s_wval != sp1->s_wval)
		return sp->s_wval > sp1->s_wval;

	return (random() & 1) != 0;
}

static int curcolor;	/* implicit parameter to makecombo() */
static unsigned int curlevel;	/* implicit parameter to makecombo() */

static bool
four_in_a_row(int color, spot_index s, direction r)
{

	struct spotstr *sp = &board[s];
	union comboval cb = { .s = sp->s_fval[color][r].s };
	if (cb.s >= 0x101)
		return false;

	for (int i = 5 + cb.cv_win, d = dd[r]; --i >= 0; sp += d) {
		if (sp->s_occ != EMPTY)
			continue;
		sp->s_combo[color].s = cb.s;
		sp->s_level[color] = 1;
	}
	return true;
}

/*
 * Scan the sorted list of non-empty frames and
 * update the minimum combo values for each empty spot.
 * Also, try to combine frames to find more complex (chained) moves.
 */
static void
scanframes(int color)
{
	struct combostr *ecbp;
	struct spotstr *sp;
	union comboval *cp;
	struct elist *nep;
	int i, r, n;
	union comboval cb;

	curcolor = color;

	/* check for empty list of frames */
	struct combostr *cbp = sortframes[color];
	if (cbp == NULL)
		return;

	if (four_in_a_row(color, cbp->c_vertex, cbp->c_dir))
		return;

	/*
	 * Update the minimum combo value for each spot in the frame
	 * and try making all combinations of two frames intersecting at
	 * an empty spot.
	 */
	n = combolen;
	ecbp = cbp;
	do {
		sp = &board[cbp->c_vertex];
		cp = &sp->s_fval[color][r = cbp->c_dir];
		int delta = dd[r];
		if (cp->cv_win != 0) {
			/*
			 * Since this is the first spot of an open-ended
			 * frame, we treat it as a closed frame.
			 */
			cb.cv_force = cp->cv_force + 1;
			cb.cv_win = 0;
			if (cb.s < sp->s_combo[color].s) {
				sp->s_combo[color].s = cb.s;
				sp->s_level[color] = 1;
			}
			/*
			 * Try combining other frames that intersect
			 * at this spot.
			 */
			makecombo2(cbp, sp, 0, cb.s);
			if (cp->s != 0x101)
				cb.s = cp->s;
			else if (color != nextcolor)
				memset(tmpmap, 0, sizeof(tmpmap));
			sp += delta;
			i = 1;
		} else {
			cb.s = cp->s;
			i = 0;
		}
		for (; i < 5; i++, sp += delta) {	/* for each spot */
			if (sp->s_occ != EMPTY)
				continue;
			if (cp->s < sp->s_combo[color].s) {
				sp->s_combo[color].s = cp->s;
				sp->s_level[color] = 1;
			}
			if (cp->s == 0x101) {
				sp->s_nforce[color]++;
				if (color != nextcolor) {
					/* XXX: suspicious use of 'n' */
					n = (spot_index)(sp - board);
					BIT_SET(tmpmap, n);
				}
			}
			/*
			 * Try combining other frames that intersect
			 * at this spot.
			 */
			makecombo2(cbp, sp, i, cb.s);
		}
		if (cp->s == 0x101 && color != nextcolor) {
			if (nforce == 0)
				memcpy(forcemap, tmpmap, sizeof(tmpmap));
			else {
				for (i = 0; (unsigned int)i < MAPSZ; i++)
					forcemap[i] &= tmpmap[i];
			}
		}
		/* mark frame as having been processed */
		board[cbp->c_vertex].s_flags |= MFLAG << r;
	} while ((cbp = cbp->c_next) != ecbp);

	/*
	 * Try to make new 3rd level combos, 4th level, etc.
	 * Limit the search depth early in the game.
	 */
	for (unsigned int level = 2;
	     level <= 1 + game.nmoves / 2 && combolen > n; level++) {
		if (level >= 9)
			break;	/* Do not think too long. */
		if (debug != 0) {
			debuglog("%cL%u %d %d %d", "BW"[color],
			    level, combolen - n, combocnt, elistcnt);
			refresh();
		}
		n = combolen;
		addframes(level);
	}

	/* scan for combos at empty spots */
	for (unsigned pos = PT(BSZ, BSZ + 1); pos-- > PT(1, 1); ) {
		sp = &board[pos];
		for (struct elist *ep = sp->s_empty; ep != NULL; ep = nep) {
			cbp = ep->e_combo;
			if (cbp->c_combo.s <= sp->s_combo[color].s) {
				if (cbp->c_combo.s != sp->s_combo[color].s) {
					sp->s_combo[color].s = cbp->c_combo.s;
					sp->s_level[color] = cbp->c_nframes;
				} else if (cbp->c_nframes < sp->s_level[color])
					sp->s_level[color] = cbp->c_nframes;
			}
			nep = ep->e_next;
			free(ep);
			elistcnt--;
		}
		sp->s_empty = NULL;
		for (struct elist *ep = sp->s_nempty; ep != NULL; ep = nep) {
			cbp = ep->e_combo;
			if (cbp->c_combo.s <= sp->s_combo[color].s) {
				if (cbp->c_combo.s != sp->s_combo[color].s) {
					sp->s_combo[color].s = cbp->c_combo.s;
					sp->s_level[color] = cbp->c_nframes;
				} else if (cbp->c_nframes < sp->s_level[color])
					sp->s_level[color] = cbp->c_nframes;
			}
			nep = ep->e_next;
			free(ep);
			elistcnt--;
		}
		sp->s_nempty = NULL;
	}

	/* remove old combos */
	if ((cbp = sortcombos) != NULL) {
		struct combostr *ncbp;

		/* scan the list */
		ecbp = cbp;
		do {
			ncbp = cbp->c_next;
			free(cbp);
			combocnt--;
		} while ((cbp = ncbp) != ecbp);
		sortcombos = NULL;
	}
	combolen = 0;

#ifdef DEBUG
	if (combocnt != 0) {
		debuglog("scanframes: %c combocnt %d", "BW"[color],
			combocnt);
		whatsup(0);
	}
	if (elistcnt != 0) {
		debuglog("scanframes: %c elistcnt %d", "BW"[color],
			elistcnt);
		whatsup(0);
	}
#endif
}

/*
 * Compute all level 2 combos of frames intersecting spot 'osp'
 * within the frame 'ocbp' and combo value 'cv'.
 */
static void
makecombo2(struct combostr *ocbp, struct spotstr *osp, int off, int cv)
{
	struct combostr *ncbp;
	int c;
	int baseB, fcnt, emask, n;
	union comboval ocb, fcb;
	struct combostr **scbpp, *fcbp;
	char tmp[128];

	/* try to combine a new frame with those found so far */
	ocb.s = cv;
	baseB = ocb.cv_force + ocb.cv_win - 1;
	fcnt = ocb.cv_force - 2;
	emask = fcnt != 0 ? ((ocb.cv_win != 0 ? 0x1E : 0x1F) & ~(1 << off)) : 0;

	for (direction r = 4; r-- > 0; ) {
	    /* don't include frames that overlap in the same direction */
	    if (r == ocbp->c_dir)
		continue;
	    int d = dd[r];
	    /*
	     * Frame A combined with B is the same value as B combined with A
	     * so skip frames that have already been processed (MFLAG).
	     * Also skip blocked frames (BFLAG) and frames that are <1,x>
	     * since combining another frame with it isn't valid.
	     */
	    int bmask = (BFLAG | FFLAG | MFLAG) << r;
	    struct spotstr *fsp = osp;
	    for (int f = 0; f < 5; f++, fsp -= d) {	/* for each frame */
		if (fsp->s_occ == BORDER)
		    break;
		if ((fsp->s_flags & bmask) != 0)
		    continue;

		/* don't include frames of the wrong color */
		fcb.s = fsp->s_fval[curcolor][r].s;
		if (fcb.cv_force >= 6)
		    continue;

		/*
		 * Get the combo value for this frame.
		 * If this is the end point of the frame,
		 * use the closed ended value for the frame.
		 */
		if ((f == 0 && fcb.cv_win != 0) || fcb.s == 0x101) {
		    fcb.cv_force++;
		    fcb.cv_win = 0;
		}

		/* compute combo value */
		c = fcb.cv_force + ocb.cv_force - 3;
		if (c > 4)
		    continue;
		n = fcb.cv_force + fcb.cv_win - 1;
		if (baseB < n)
		    n = baseB;

		/* make a new combo! */
		ncbp = (struct combostr *)malloc(sizeof(struct combostr) +
		    2 * sizeof(struct combostr *));
		if (ncbp == NULL)
		    panic("Out of memory!");
		scbpp = (void *)(ncbp + 1);
		fcbp = &frames[fsp->s_frame[r]];
		if (ocbp < fcbp) {
		    scbpp[0] = ocbp;
		    scbpp[1] = fcbp;
		} else {
		    scbpp[0] = fcbp;
		    scbpp[1] = ocbp;
		}
		ncbp->c_combo.cv_force = c;
		ncbp->c_combo.cv_win = n;
		ncbp->c_link[0] = ocbp;
		ncbp->c_link[1] = fcbp;
		ncbp->c_linkv[0].s = ocb.s;
		ncbp->c_linkv[1].s = fcb.s;
		ncbp->c_voff[0] = off;
		ncbp->c_voff[1] = f;
		ncbp->c_vertex = (spot_index)(osp - board);
		ncbp->c_nframes = 2;
		ncbp->c_dir = 0;
		ncbp->c_frameindex = 0;
		ncbp->c_flags = ocb.cv_win != 0 ? C_OPEN_0 : 0;
		if (fcb.cv_win != 0)
		    ncbp->c_flags |= C_OPEN_1;
		ncbp->c_framecnt[0] = fcnt;
		ncbp->c_emask[0] = emask;
		ncbp->c_framecnt[1] = fcb.cv_force - 2;
		ncbp->c_emask[1] = ncbp->c_framecnt[1] != 0 ?
		    ((fcb.cv_win != 0 ? 0x1E : 0x1F) & ~(1 << f)) : 0;
		combocnt++;

		if ((c == 1 && debug > 1) || debug > 3) {
		    debuglog("%c c %d %d m %x %x o %d %d",
			"bw"[curcolor],
			ncbp->c_framecnt[0], ncbp->c_framecnt[1],
			ncbp->c_emask[0], ncbp->c_emask[1],
			ncbp->c_voff[0], ncbp->c_voff[1]);
		    printcombo(ncbp, tmp, sizeof(tmp));
		    debuglog("%s", tmp);
		}
		if (c > 1) {
		    /* record the empty spots that will complete this combo */
		    makeempty(ncbp);

		    /* add the new combo to the end of the list */
		    appendcombo(ncbp, curcolor);
		} else {
		    updatecombo(ncbp, curcolor);
		    free(ncbp);
		    combocnt--;
		}
#ifdef DEBUG
		if ((c == 1 && debug > 1) || debug > 5) {
		    markcombo(ncbp);
		    bdisp();
		    whatsup(0);
		    clearcombo(ncbp, 0);
		}
#endif /* DEBUG */
	    }
	}
}

/*
 * Scan the sorted list of frames and try to add a frame to
 * combinations of 'level' number of frames.
 */
static void
addframes(unsigned int level)
{
	struct combostr *cbp, *ecbp;
	struct spotstr *fsp;
	struct elist *nep;
	int i, r;
	struct combostr **cbpp, *pcbp;
	union comboval fcb, cb;

	curlevel = level;

	/* scan for combos at empty spots */
	i = curcolor;
	for (unsigned pos = PT(BSZ, BSZ + 1); pos-- > PT(1, 1); ) {
		struct spotstr *sp = &board[pos];
		for (struct elist *ep = sp->s_empty; ep != NULL; ep = nep) {
			cbp = ep->e_combo;
			if (cbp->c_combo.s <= sp->s_combo[i].s) {
				if (cbp->c_combo.s != sp->s_combo[i].s) {
					sp->s_combo[i].s = cbp->c_combo.s;
					sp->s_level[i] = cbp->c_nframes;
				} else if (cbp->c_nframes < sp->s_level[i])
					sp->s_level[i] = cbp->c_nframes;
			}
			nep = ep->e_next;
			free(ep);
			elistcnt--;
		}
		sp->s_empty = sp->s_nempty;
		sp->s_nempty = NULL;
	}

	/* try to add frames to the uncompleted combos at level curlevel */
	cbp = ecbp = sortframes[curcolor];
	do {
		fsp = &board[cbp->c_vertex];
		r = cbp->c_dir;
		/* skip frames that are part of a <1,x> combo */
		if ((fsp->s_flags & (FFLAG << r)) != 0)
			continue;

		/*
		 * Don't include <1,x> combo frames,
		 * treat it as a closed three in a row instead.
		 */
		fcb.s = fsp->s_fval[curcolor][r].s;
		if (fcb.s == 0x101)
			fcb.s = 0x200;

		/*
		 * If this is an open-ended frame, use
		 * the combo value with the end closed.
		 */
		if (fsp->s_occ == EMPTY) {
			if (fcb.cv_win != 0) {
				cb.cv_force = fcb.cv_force + 1;
				cb.cv_win = 0;
			} else
				cb.s = fcb.s;
			makecombo(cbp, fsp, 0, cb.s);
		}

		/*
		 * The next four spots are handled the same for both
		 * open and closed ended frames.
		 */
		int d = dd[r];
		struct spotstr *sp = fsp + d;
		for (i = 1; i < 5; i++, sp += d) {
			if (sp->s_occ != EMPTY)
				continue;
			makecombo(cbp, sp, i, fcb.s);
		}
	} while ((cbp = cbp->c_next) != ecbp);

	/* put all the combos in the hash list on the sorted list */
	cbpp = &hashcombos[FAREA];
	do {
		cbp = *--cbpp;
		if (cbp == NULL)
			continue;
		*cbpp = NULL;
		ecbp = sortcombos;
		if (ecbp == NULL)
			sortcombos = cbp;
		else {
			/* append to sort list */
			pcbp = ecbp->c_prev;
			pcbp->c_next = cbp;
			ecbp->c_prev = cbp->c_prev;
			cbp->c_prev->c_next = ecbp;
			cbp->c_prev = pcbp;
		}
	} while (cbpp != hashcombos);
}

/*
 * Compute all level N combos of frames intersecting spot 'osp'
 * within the frame 'ocbp' and combo value 'cv'.
 */
static void
makecombo(struct combostr *ocbp, struct spotstr *osp, int off, int cv)
{
	struct combostr *cbp;
	struct spotstr *sp;
	struct combostr **scbpp;
	int baseB, fcnt, emask, verts;
	union comboval ocb;
	struct overlap_info vertices[1];
	char tmp[128];

	/*
	 * XXX: when I made functions static gcc started warning about
	 * some members of vertices[0] maybe being used uninitialized.
	 * For now, I'm just going to clear it rather than wade through
	 * the logic to find out whether gcc or the code is wrong. I
	 * wouldn't be surprised if it were the code though. - dholland
	 */
	memset(vertices, 0, sizeof(vertices));

	ocb.s = cv;
	baseB = ocb.cv_force + ocb.cv_win - 1;
	fcnt = ocb.cv_force - 2;
	emask = fcnt != 0 ? ((ocb.cv_win != 0 ? 0x1E : 0x1F) & ~(1 << off)) : 0;
	for (struct elist *ep = osp->s_empty; ep != NULL; ep = ep->e_next) {
	    /* check for various kinds of overlap */
	    cbp = ep->e_combo;
	    verts = checkframes(cbp, ocbp, osp, cv, vertices);
	    if (verts < 0)
		continue;

	    /* check to see if this frame forms a valid loop */
	    if (verts > 0) {
		sp = &board[vertices[0].o_intersect];
#ifdef DEBUG
		if (sp->s_occ != EMPTY) {
		    debuglog("loop: %c %s", "BW"[curcolor],
			stoc((spot_index)(sp - board)));
		    whatsup(0);
		}
#endif
		/*
		 * It is a valid loop if the intersection spot
		 * of the frame we are trying to attach is one
		 * of the completion spots of the combostr
		 * we are trying to attach the frame to.
		 */
		for (struct elist *nep = sp->s_empty;
			nep != NULL; nep = nep->e_next) {
		    if (nep->e_combo == cbp)
			goto fnd;
		    if (nep->e_combo->c_nframes < cbp->c_nframes)
			break;
		}
		/* frame overlaps but not at a valid spot */
		continue;
	    fnd:
		;
	    }

	    /* compute the first half of the combo value */
	    int c = cbp->c_combo.cv_force + ocb.cv_force - verts - 3;
	    if (c > 4)
		continue;

	    /* compute the second half of the combo value */
	    int n = ep->e_fval.cv_force + ep->e_fval.cv_win - 1;
	    if (baseB < n)
		n = baseB;

	    /* make a new combo! */
	    struct combostr *ncbp = malloc(sizeof(struct combostr) +
		(cbp->c_nframes + 1) * sizeof(struct combostr *));
	    if (ncbp == NULL)
		panic("Out of memory!");
	    scbpp = (void *)(ncbp + 1);
	    if (sortcombo(scbpp, (void *)(cbp + 1), ocbp)) {
		free(ncbp);
		continue;
	    }
	    combocnt++;

	    ncbp->c_combo.cv_force = c;
	    ncbp->c_combo.cv_win = n;
	    ncbp->c_link[0] = cbp;
	    ncbp->c_link[1] = ocbp;
	    ncbp->c_linkv[1].s = ocb.s;
	    ncbp->c_voff[1] = off;
	    ncbp->c_vertex = (spot_index)(osp - board);
	    ncbp->c_nframes = cbp->c_nframes + 1;
	    ncbp->c_flags = ocb.cv_win != 0 ? C_OPEN_1 : 0;
	    ncbp->c_frameindex = ep->e_frameindex;
	    /*
	     * Update the completion spot mask of the frame we
	     * are attaching 'ocbp' to so the intersection isn't
	     * listed twice.
	     */
	    ncbp->c_framecnt[0] = ep->e_framecnt;
	    ncbp->c_emask[0] = ep->e_emask;
	    if (verts != 0) {
		ncbp->c_flags |= C_LOOP;
		ncbp->c_dir = vertices[0].o_frameindex;
		ncbp->c_framecnt[1] = fcnt - 1;
		if (ncbp->c_framecnt[1] != 0) {
		    n = (vertices[0].o_intersect - ocbp->c_vertex) /
			dd[ocbp->c_dir];
		    ncbp->c_emask[1] = emask & ~(1 << n);
		} else
		    ncbp->c_emask[1] = 0;
		ncbp->c_voff[0] = vertices[0].o_off;
	    } else {
		ncbp->c_dir = 0;
		ncbp->c_framecnt[1] = fcnt;
		ncbp->c_emask[1] = emask;
		ncbp->c_voff[0] = ep->e_off;
	    }

	    if ((c == 1 && debug > 1) || debug > 3) {
		debuglog("%c v%d i%d d%d c %d %d m %x %x o %d %d",
		    "bw"[curcolor], verts, ncbp->c_frameindex, ncbp->c_dir,
		    ncbp->c_framecnt[0], ncbp->c_framecnt[1],
		    ncbp->c_emask[0], ncbp->c_emask[1],
		    ncbp->c_voff[0], ncbp->c_voff[1]);
		printcombo(ncbp, tmp, sizeof(tmp));
		debuglog("%s", tmp);
	    }
	    if (c > 1) {
		/* record the empty spots that will complete this combo */
		makeempty(ncbp);
		combolen++;
	    } else {
		/* update board values */
		updatecombo(ncbp, curcolor);
	    }
#ifdef DEBUG
	    if ((c == 1 && debug > 1) || debug > 4) {
		markcombo(ncbp);
		bdisp();
		whatsup(0);
		clearcombo(ncbp, 0);
	    }
#endif /* DEBUG */
	}
}

#define MAXDEPTH	100
static struct elist einfo[MAXDEPTH];
static struct combostr *ecombo[MAXDEPTH];	/* separate from elist to save space */

/*
 * Add the combostr 'ocbp' to the empty spots list for each empty spot
 * in 'ocbp' that will complete the combo.
 */
static void
makeempty(struct combostr *ocbp)
{
	struct combostr *cbp, **cbpp;
	struct elist *ep, *nep;
	struct spotstr *sp;
	int d, emask, i;
	int nframes;
	char tmp[128];

	if (debug > 2) {
		printcombo(ocbp, tmp, sizeof(tmp));
		debuglog("E%c %s", "bw"[curcolor], tmp);
	}

	/* should never happen but check anyway */
	if ((nframes = ocbp->c_nframes) >= MAXDEPTH)
		return;

	/*
	 * The lower level combo can be pointed to by more than one
	 * higher level 'struct combostr' so we can't modify the
	 * lower level. Therefore, higher level combos store the
	 * real mask of the lower level frame in c_emask[0] and the
	 * frame number in c_frameindex.
	 *
	 * First we traverse the tree from top to bottom and save the
	 * connection info. Then we traverse the tree from bottom to
	 * top overwriting lower levels with the newer emask information.
	 */
	ep = &einfo[nframes];
	cbpp = &ecombo[nframes];
	for (cbp = ocbp; cbp->c_link[1] != NULL; cbp = cbp->c_link[0]) {
		ep--;
		ep->e_combo = cbp;
		*--cbpp = cbp->c_link[1];
		ep->e_off = cbp->c_voff[1];
		ep->e_frameindex = cbp->c_frameindex;
		ep->e_fval.s = cbp->c_linkv[1].s;
		ep->e_framecnt = cbp->c_framecnt[1];
		ep->e_emask = cbp->c_emask[1];
	}
	cbp = ep->e_combo;
	ep--;
	ep->e_combo = cbp;
	*--cbpp = cbp->c_link[0];
	ep->e_off = cbp->c_voff[0];
	ep->e_frameindex = 0;
	ep->e_fval.s = cbp->c_linkv[0].s;
	ep->e_framecnt = cbp->c_framecnt[0];
	ep->e_emask = cbp->c_emask[0];

	/* now update the emask info */
	int n = 0;
	for (i = 2, ep += 2; i < nframes; i++, ep++) {
		cbp = ep->e_combo;
		nep = &einfo[ep->e_frameindex];
		nep->e_framecnt = cbp->c_framecnt[0];
		nep->e_emask = cbp->c_emask[0];

		if ((cbp->c_flags & C_LOOP) != 0) {
			n++;
			/*
			 * Account for the fact that this frame connects
			 * to a previous one (thus forming a loop).
			 */
			nep = &einfo[cbp->c_dir];
			if (--nep->e_framecnt != 0)
				nep->e_emask &= ~(1 << cbp->c_voff[0]);
			else
				nep->e_emask = 0;
		}
	}

	/*
	 * We only need to update the emask values of "complete" loops
	 * to include the intersection spots.
	 */
	if (n != 0 && ocbp->c_combo.cv_force == 2) {
		/* process loops from the top down */
		ep = &einfo[nframes];
		do {
			ep--;
			cbp = ep->e_combo;
			if ((cbp->c_flags & C_LOOP) == 0)
				continue;

			/*
			 * Update the emask values to include the
			 * intersection spots.
			 */
			nep = &einfo[cbp->c_dir];
			nep->e_framecnt = 1;
			nep->e_emask = 1 << cbp->c_voff[0];
			ep->e_framecnt = 1;
			ep->e_emask = 1 << ep->e_off;
			ep = &einfo[ep->e_frameindex];
			do {
				ep->e_framecnt = 1;
				ep->e_emask = 1 << ep->e_off;
				ep = &einfo[ep->e_frameindex];
			} while (ep > nep);
		} while (ep != einfo);
	}

	/* check all the frames for completion spots */
	for (i = 0, ep = einfo, cbpp = ecombo; i < nframes; i++, ep++, cbpp++) {
		/* skip this frame if there are no incomplete spots in it */
		if ((emask = ep->e_emask) == 0)
			continue;
		cbp = *cbpp;
		sp = &board[cbp->c_vertex];
		d = dd[cbp->c_dir];
		for (int off = 0, m = 1; off < 5; off++, sp += d, m <<= 1) {
			if (sp->s_occ != EMPTY || (emask & m) == 0)
				continue;

			/* add the combo to the list of empty spots */
			nep = (struct elist *)malloc(sizeof(struct elist));
			if (nep == NULL)
				panic("Out of memory!");
			nep->e_combo = ocbp;
			nep->e_off = off;
			nep->e_frameindex = i;
			if (ep->e_framecnt > 1) {
				nep->e_framecnt = ep->e_framecnt - 1;
				nep->e_emask = emask & ~m;
			} else {
				nep->e_framecnt = 0;
				nep->e_emask = 0;
			}
			nep->e_fval.s = ep->e_fval.s;
			if (debug > 2) {
				debuglog("e %s o%d i%d c%d m%x %x",
				    stoc((spot_index)(sp - board)),
				    nep->e_off,
				    nep->e_frameindex,
				    nep->e_framecnt,
				    nep->e_emask,
				    nep->e_fval.s);
			}

			/* sort by the number of frames in the combo */
			nep->e_next = sp->s_nempty;
			sp->s_nempty = nep;
			elistcnt++;
		}
	}
}

/*
 * Update the board value based on the combostr.
 * This is called only if 'cbp' is a <1,x> combo.
 * We handle things differently depending on whether the next move
 * would be trying to "complete" the combo or trying to block it.
 */
static void
updatecombo(struct combostr *cbp, int color)
{
	struct combostr *tcbp;
	int nframes, flags;
	union comboval cb;

	flags = 0;
	/* save the top level value for the whole combo */
	cb.cv_force = cbp->c_combo.cv_force;
	nframes = cbp->c_nframes;

	if (color != nextcolor)
		memset(tmpmap, 0, sizeof(tmpmap));

	for (; (tcbp = cbp->c_link[1]) != NULL; cbp = cbp->c_link[0]) {
		flags = cbp->c_flags;
		cb.cv_win = cbp->c_combo.cv_win;
		if (color == nextcolor) {
			/* update the board value for the vertex */
			struct spotstr *sp = &board[cbp->c_vertex];
			sp->s_nforce[color]++;
			if (cb.s <= sp->s_combo[color].s) {
				if (cb.s != sp->s_combo[color].s) {
					sp->s_combo[color].s = cb.s;
					sp->s_level[color] = nframes;
				} else if (nframes < sp->s_level[color])
					sp->s_level[color] = nframes;
			}
		} else {
			/* update the board values for each spot in frame */
			spot_index s = tcbp->c_vertex;
			struct spotstr *sp = &board[s];
			int d = dd[tcbp->c_dir];
			int i = (flags & C_OPEN_1) != 0 ? 6 : 5;
			for (; --i >= 0; sp += d, s += d) {
				if (sp->s_occ != EMPTY)
					continue;
				sp->s_nforce[color]++;
				if (cb.s <= sp->s_combo[color].s) {
					if (cb.s != sp->s_combo[color].s) {
						sp->s_combo[color].s = cb.s;
						sp->s_level[color] = nframes;
					} else if (nframes < sp->s_level[color])
						sp->s_level[color] = nframes;
				}
				BIT_SET(tmpmap, s);
			}
		}

		/* mark the frame as being part of a <1,x> combo */
		board[tcbp->c_vertex].s_flags |= FFLAG << tcbp->c_dir;
	}

	if (color != nextcolor) {
		/* update the board values for each spot in frame */
		spot_index s = cbp->c_vertex;
		struct spotstr *sp = &board[s];
		int d = dd[cbp->c_dir];
		int i = (flags & C_OPEN_0) != 0 ? 6 : 5;
		for (; --i >= 0; sp += d, s += d) {
			if (sp->s_occ != EMPTY)
				continue;
			sp->s_nforce[color]++;
			if (cb.s <= sp->s_combo[color].s) {
				if (cb.s != sp->s_combo[color].s) {
					sp->s_combo[color].s = cb.s;
					sp->s_level[color] = nframes;
				} else if (nframes < sp->s_level[color])
					sp->s_level[color] = nframes;
			}
			BIT_SET(tmpmap, s);
		}
		if (nforce == 0)
			memcpy(forcemap, tmpmap, sizeof(tmpmap));
		else {
			for (i = 0; (unsigned int)i < MAPSZ; i++)
				forcemap[i] &= tmpmap[i];
		}
		nforce++;
	}

	/* mark the frame as being part of a <1,x> combo */
	board[cbp->c_vertex].s_flags |= FFLAG << cbp->c_dir;
}

/*
 * Add combo to the end of the list.
 */
static void
appendcombo(struct combostr *cbp, int color __unused)
{
	struct combostr *pcbp, *ncbp;

	combolen++;
	ncbp = sortcombos;
	if (ncbp == NULL) {
		sortcombos = cbp;
		cbp->c_next = cbp;
		cbp->c_prev = cbp;
		return;
	}
	pcbp = ncbp->c_prev;
	cbp->c_next = ncbp;
	cbp->c_prev = pcbp;
	ncbp->c_prev = cbp;
	pcbp->c_next = cbp;
}

/*
 * Return zero if it is valid to combine frame 'fcbp' with the frames
 * in 'cbp' and forms a linked chain of frames (i.e., a tree; no loops).
 * Return positive if combining frame 'fcbp' to the frames in 'cbp'
 * would form some kind of valid loop. Also return the intersection spots
 * in 'vertices[]' beside the known intersection at spot 'osp'.
 * Return -1 if 'fcbp' should not be combined with 'cbp'.
 * 'cv' is the combo value for frame 'fcbp'.
 */
static int
checkframes(struct combostr *cbp, struct combostr *fcbp, struct spotstr *osp,
	    int cv, struct overlap_info *vertices)
{
	struct combostr *tcbp, *lcbp;
	int i, n, mask, flags, verts, myindex, fcnt;
	union comboval cb;
	u_char *str;

	lcbp = NULL;
	flags = 0;

	cb.s = cv;
	fcnt = cb.cv_force - 2;
	verts = 0;
	myindex = cbp->c_nframes;
	n = (frame_index)(fcbp - frames) * FAREA;
	str = &overlap[n];
	spot_index *ip = &intersect[n];
	/*
	 * i == which overlap bit to test based on whether 'fcbp' is
	 * an open or closed frame.
	 */
	i = cb.cv_win != 0 ? 2 : 0;
	for (; (tcbp = cbp->c_link[1]) != NULL;
	    lcbp = cbp, cbp = cbp->c_link[0]) {
		if (tcbp == fcbp)
			return -1;	/* fcbp is already included */

		/* check for intersection of 'tcbp' with 'fcbp' */
		myindex--;
		mask = str[tcbp - frames];
		flags = cbp->c_flags;
		n = i + ((flags & C_OPEN_1) != 0 ? 1 : 0);
		if ((mask & (1 << n)) != 0) {
			/*
			 * The two frames are not independent if they
			 * both lie in the same line and intersect at
			 * more than one point.
			 */
			if (tcbp->c_dir == fcbp->c_dir &&
			    (mask & (0x10 << n)) != 0)
				return -1;
			/*
			 * If this is not the spot we are attaching
			 * 'fcbp' to, and it is a reasonable intersection
			 * spot, then there might be a loop.
			 */
			spot_index s = ip[tcbp - frames];
			if (osp != &board[s]) {
				/* check to see if this is a valid loop */
				if (verts != 0)
					return -1;
				if (fcnt == 0 || cbp->c_framecnt[1] == 0)
					return -1;
				/*
				 * Check to be sure the intersection is not
				 * one of the end points if it is an
				 * open-ended frame.
				 */
				if ((flags & C_OPEN_1) != 0 &&
				    (s == tcbp->c_vertex ||
				     s == tcbp->c_vertex + 5 * dd[tcbp->c_dir]))
					return -1;	/* invalid overlap */
				if (cb.cv_win != 0 &&
				    (s == fcbp->c_vertex ||
				     s == fcbp->c_vertex + 5 * dd[fcbp->c_dir]))
					return -1;	/* invalid overlap */

				vertices->o_intersect = s;
				vertices->o_off = (s - tcbp->c_vertex) /
					dd[tcbp->c_dir];
				vertices->o_frameindex = myindex;
				verts++;
			}
		}
		n = i + ((flags & C_OPEN_0) != 0 ? 1 : 0);
	}
	if (cbp == fcbp)
		return -1;	/* fcbp is already included */

	/* check for intersection of 'cbp' with 'fcbp' */
	mask = str[cbp - frames];
	if ((mask & (1 << n)) != 0) {
		/*
		 * The two frames are not independent if they
		 * both lie in the same line and intersect at
		 * more than one point.
		 */
		if (cbp->c_dir == fcbp->c_dir && (mask & (0x10 << n)) != 0)
			return -1;
		/*
		 * If this is not the spot we are attaching
		 * 'fcbp' to, and it is a reasonable intersection
		 * spot, then there might be a loop.
		 */
		spot_index s = ip[cbp - frames];
		if (osp != &board[s]) {
			/* check to see if this is a valid loop */
			if (verts != 0)
				return -1;
			if (fcnt == 0 || lcbp->c_framecnt[0] == 0)
				return -1;
			/*
			 * Check to be sure the intersection is not
			 * one of the end points if it is an open-ended
			 * frame.
			 */
			if ((flags & C_OPEN_0) != 0 &&
			    (s == cbp->c_vertex ||
			     s == cbp->c_vertex + 5 * dd[cbp->c_dir]))
				return -1;	/* invalid overlap */
			if (cb.cv_win != 0 &&
			    (s == fcbp->c_vertex ||
			     s == fcbp->c_vertex + 5 * dd[fcbp->c_dir]))
				return -1;	/* invalid overlap */

			vertices->o_intersect = s;
			vertices->o_off = (s - cbp->c_vertex) /
				dd[cbp->c_dir];
			vertices->o_frameindex = 0;
			verts++;
		}
	}
	return verts;
}

/*
 * Merge sort the frame 'fcbp' and the sorted list of frames 'cbpp' and
 * store the result in 'scbpp'. 'curlevel' is the size of the 'cbpp' array.
 * Return true if this list of frames is already in the hash list.
 * Otherwise, add the new combo to the hash list.
 */
static bool
sortcombo(struct combostr **scbpp, struct combostr **cbpp,
	  struct combostr *fcbp)
{
	struct combostr **spp, **cpp;
	struct combostr *cbp, *ecbp;
	int inx;

#ifdef DEBUG
	if (debug > 3) {
		char buf[128];
		size_t pos;

		debuglog("sortc: %s%c l%u", stoc(fcbp->c_vertex),
			pdir[fcbp->c_dir], curlevel);
		pos = 0;
		for (cpp = cbpp; cpp < cbpp + curlevel; cpp++) {
			snprintf(buf + pos, sizeof(buf) - pos, " %s%c",
				stoc((*cpp)->c_vertex), pdir[(*cpp)->c_dir]);
			pos += strlen(buf + pos);
		}
		debuglog("%s", buf);
	}
#endif /* DEBUG */

	/* first build the new sorted list */
	unsigned int n = curlevel + 1;
	spp = scbpp + n;
	cpp = cbpp + curlevel;
	do {
		cpp--;
		if (fcbp > *cpp) {
			*--spp = fcbp;
			do {
				*--spp = *cpp;
			} while (cpp-- != cbpp);
			goto inserted;
		}
		*--spp = *cpp;
	} while (cpp != cbpp);
	*--spp = fcbp;
inserted:

	/* now check to see if this list of frames has already been seen */
	cbp = hashcombos[inx = (frame_index)(*scbpp - frames)];
	if (cbp == NULL) {
		/*
		 * Easy case, this list hasn't been seen.
		 * Add it to the hash list.
		 */
		fcbp = (void *)((char *)scbpp - sizeof(struct combostr));
		hashcombos[inx] = fcbp;
		fcbp->c_next = fcbp->c_prev = fcbp;
		return false;
	}
	ecbp = cbp;
	do {
		cbpp = (void *)(cbp + 1);
		cpp = cbpp + n;
		spp = scbpp + n;
		cbpp++;	/* first frame is always the same */
		do {
			if (*--spp != *--cpp)
				goto next;
		} while (cpp != cbpp);
		/* we found a match */
#ifdef DEBUG
		if (debug > 3) {
			char buf[128];
			size_t pos;

			debuglog("sort1: n%u", n);
			pos = 0;
			for (cpp = scbpp; cpp < scbpp + n; cpp++) {
				snprintf(buf + pos, sizeof(buf) - pos, " %s%c",
					stoc((*cpp)->c_vertex),
					pdir[(*cpp)->c_dir]);
				pos += strlen(buf + pos);
			}
			debuglog("%s", buf);
			printcombo(cbp, buf, sizeof(buf));
			debuglog("%s", buf);
			cbpp--;
			pos = 0;
			for (cpp = cbpp; cpp < cbpp + n; cpp++) {
				snprintf(buf + pos, sizeof(buf) - pos, " %s%c",
					stoc((*cpp)->c_vertex),
					pdir[(*cpp)->c_dir]);
				pos += strlen(buf + pos);
			}
			debuglog("%s", buf);
		}
#endif /* DEBUG */
		return true;
	next:
		;
	} while ((cbp = cbp->c_next) != ecbp);
	/*
	 * This list of frames hasn't been seen.
	 * Add it to the hash list.
	 */
	ecbp = cbp->c_prev;
	fcbp = (void *)((char *)scbpp - sizeof(struct combostr));
	fcbp->c_next = cbp;
	fcbp->c_prev = ecbp;
	cbp->c_prev = fcbp;
	ecbp->c_next = fcbp;
	return false;
}

/*
 * Print the combo into string buffer 'buf'.
 */
#if !defined(DEBUG)
static
#endif
void
printcombo(struct combostr *cbp, char *buf, size_t max)
{
	struct combostr *tcbp;
	size_t pos = 0;

	snprintf(buf + pos, max - pos, "%x/%d",
	    cbp->c_combo.s, cbp->c_nframes);
	pos += strlen(buf + pos);

	for (; (tcbp = cbp->c_link[1]) != NULL; cbp = cbp->c_link[0]) {
		snprintf(buf + pos, max - pos, " %s%c%x",
		    stoc(tcbp->c_vertex), pdir[tcbp->c_dir], cbp->c_flags);
		pos += strlen(buf + pos);
	}
	snprintf(buf + pos, max - pos, " %s%c",
	    stoc(cbp->c_vertex), pdir[cbp->c_dir]);
}

#ifdef DEBUG
void
markcombo(struct combostr *ocbp)
{
	struct combostr *cbp, **cbpp;
	struct elist *ep, *nep;
	struct spotstr *sp;
	int d, m, i;
	int nframes;
	int cmask, omask;

	/* should never happen but check anyway */
	if ((nframes = ocbp->c_nframes) >= MAXDEPTH)
		return;

	/*
	 * The lower level combo can be pointed to by more than one
	 * higher level 'struct combostr' so we can't modify the
	 * lower level. Therefore, higher level combos store the
	 * real mask of the lower level frame in c_emask[0] and the
	 * frame number in c_frameindex.
	 *
	 * First we traverse the tree from top to bottom and save the
	 * connection info. Then we traverse the tree from bottom to
	 * top overwriting lower levels with the newer emask information.
	 */
	ep = &einfo[nframes];
	cbpp = &ecombo[nframes];
	for (cbp = ocbp; cbp->c_link[1] != NULL; cbp = cbp->c_link[0]) {
		ep--;
		ep->e_combo = cbp;
		*--cbpp = cbp->c_link[1];
		ep->e_off = cbp->c_voff[1];
		ep->e_frameindex = cbp->c_frameindex;
		ep->e_fval.s = cbp->c_linkv[1].s;
		ep->e_framecnt = cbp->c_framecnt[1];
		ep->e_emask = cbp->c_emask[1];
	}
	cbp = ep->e_combo;
	ep--;
	ep->e_combo = cbp;
	*--cbpp = cbp->c_link[0];
	ep->e_off = cbp->c_voff[0];
	ep->e_frameindex = 0;
	ep->e_fval.s = cbp->c_linkv[0].s;
	ep->e_framecnt = cbp->c_framecnt[0];
	ep->e_emask = cbp->c_emask[0];

	/* now update the emask info */
	int n = 0;
	for (i = 2, ep += 2; i < nframes; i++, ep++) {
		cbp = ep->e_combo;
		nep = &einfo[ep->e_frameindex];
		nep->e_framecnt = cbp->c_framecnt[0];
		nep->e_emask = cbp->c_emask[0];

		if ((cbp->c_flags & C_LOOP) != 0) {
			n++;
			/*
			 * Account for the fact that this frame connects
			 * to a previous one (thus forming a loop).
			 */
			nep = &einfo[cbp->c_dir];
			if (--nep->e_framecnt != 0)
				nep->e_emask &= ~(1 << cbp->c_voff[0]);
			else
				nep->e_emask = 0;
		}
	}

	/*
	 * We only need to update the emask values of "complete" loops
	 * to include the intersection spots.
	 */
	if (n != 0 && ocbp->c_combo.cv_force == 2) {
		/* process loops from the top down */
		ep = &einfo[nframes];
		do {
			ep--;
			cbp = ep->e_combo;
			if ((cbp->c_flags & C_LOOP) == 0)
				continue;

			/*
			 * Update the emask values to include the
			 * intersection spots.
			 */
			nep = &einfo[cbp->c_dir];
			nep->e_framecnt = 1;
			nep->e_emask = 1 << cbp->c_voff[0];
			ep->e_framecnt = 1;
			ep->e_emask = 1 << ep->e_off;
			ep = &einfo[ep->e_frameindex];
			do {
				ep->e_framecnt = 1;
				ep->e_emask = 1 << ep->e_off;
				ep = &einfo[ep->e_frameindex];
			} while (ep > nep);
		} while (ep != einfo);
	}

	/* mark all the frames with the completion spots */
	for (i = 0, ep = einfo, cbpp = ecombo; i < nframes; i++, ep++, cbpp++) {
		m = ep->e_emask;
		cbp = *cbpp;
		sp = &board[cbp->c_vertex];
		d = dd[cbp->c_dir];
		cmask = CFLAG << cbp->c_dir;
		omask = (IFLAG | CFLAG) << cbp->c_dir;
		int off = ep->e_fval.cv_win != 0 ? 6 : 5;
		/* LINTED 117: bitwise '>>' on signed value possibly nonportable */
		for (; --off >= 0; sp += d, m >>= 1)
			sp->s_flags |= (m & 1) != 0 ? omask : cmask;
	}
}

void
clearcombo(struct combostr *cbp, int open)
{
	struct combostr *tcbp;

	for (; (tcbp = cbp->c_link[1]) != NULL; cbp = cbp->c_link[0]) {
		clearcombo(tcbp, cbp->c_flags & C_OPEN_1);
		open = cbp->c_flags & C_OPEN_0;
	}

	struct spotstr *sp = &board[cbp->c_vertex];
	int d = dd[cbp->c_dir];
	int mask = ~((IFLAG | CFLAG) << cbp->c_dir);
	int n = open != 0 ? 6 : 5;
	for (; --n >= 0; sp += d)
		sp->s_flags &= mask;
}
#endif /* DEBUG */
