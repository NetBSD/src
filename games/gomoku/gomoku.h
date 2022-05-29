/*	$NetBSD: gomoku.h,v 1.51 2022/05/29 14:37:44 rillig Exp $	*/

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
 *
 *	@(#)gomoku.h	8.2 (Berkeley) 5/3/95
 */

#include <sys/types.h>
#include <sys/endian.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * The board consists of 19x19 spots, the coordinates are 1-based. The board
 * is surrounded by border spots.
 */

#define BSZ	19
#define BAREA	((1 + BSZ + 1) * (BSZ + 1) + 1)

/*
 * A 'frame' is a group of five or six contiguous board locations. An
 * open-ended frame is one with spaces on both ends; otherwise, it is closed.
 */
#define FAREA	(2 * BSZ * (BSZ - 4) + 2 * (BSZ - 4) * (BSZ - 4))

/* values for s_occ */
#define BLACK	0
#define WHITE	1
#define EMPTY	2
#define BORDER	3

/* A spot on the board, or in some cases one of the below special values. */
typedef unsigned short spot_index;
/* return values for makemove, readinput */
#define MOVEOK	0
#define RESIGN	1
#define ILLEGAL	2
#define WIN	3
#define TIE	4
#define SAVE	5
#define END_OF_INPUT 6
#define PT(x, y)	((x) + (BSZ + 1) * (y))

/*
 * A 'combo' is a group of intersecting frames and consists of two numbers:
 * 'F' is the number of moves to make the combo non-blockable.
 * 'W' is the minimum number of moves needed to win once it can't be blocked.
 *
 * A 'force' is a combo that is one move away from being non-blockable.
 *
 * Each time a frame is added to the combo, the number of moves to complete
 * the force is the number of moves needed to 'fill' the frame plus one at
 * the intersection point. The number of moves to win is the number of moves
 * to complete the best frame minus the last move to complete the force.
 * Note that it doesn't make sense to combine a <1,x> with anything since
 * it is already a force. Also, the frames have to be independent so a
 * single move doesn't affect more than one frame making up the combo.
 *
 * Rules for comparing which of two combos (<F1,W1> <F2,W2>) is better:
 * Both the same color:
 *	<F',W'> = (F1 < F2 || F1 == F2 && W1 <= W2) ? <F1,W1> : <F2,W2>
 *	We want to complete the force first, then the combo with the
 *	fewest moves to win.
 * Different colors, <F1,W1> is the combo for the player with the next move:
 *	<F',W'> = F2 <= 1 && (F1 > 1 || F2 + W2 < F1 + W1) ? <F2,W2> : <F1,W1>
 *	We want to block only if we have to (i.e., if they are one move away
 *	from completing a force, and we don't have a force that we can
 *	complete which takes fewer or the same number of moves to win).
 */

/*
 * Single frame combo values:
 *     <F,W>	board values
 *	5,0	. . . . . O
 *	4,1	. . . . . .
 *	4,0	. . . . X O
 *	3,1	. . . . X .
 *	3,0	. . . X X O
 *	2,1	. . . X X .
 *	2,0	. . X X X O
 *	1,1	. . X X X .
 *	1,0	. X X X X O
 *	0,1	. X X X X .
 *	0,0	X X X X X O
 *
 * The rule for combining two combos (<F1,W1> <F2,W2>) with V valid
 * intersection points is:
 *	F' = F1 + F2 - 2 - V
 *	W' = MIN(F1 + W1 - 1, F2 + W2 - 1)
 */
union comboval {
	struct {
#if BYTE_ORDER == BIG_ENDIAN
		u_char	a;
		u_char	b;
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
		u_char	b;
		u_char	a;
#endif
	} c;
	u_short	s;
};
#define cv_force	c.a	/* # moves to complete force */
#define cv_win		c.b	/* # moves to win */

/*
 * This structure is used to record information about single frames (F) and
 * combinations of two more frames (C).
 * For combinations of two or more frames, there is an additional
 * array of pointers to the frames of the combination which is sorted
 * by the index into the frames[] array. This is used to prevent duplication
 * since frame A combined with B is the same as B with A.
 *	struct combostr *c_sort[size c_nframes];
 * The leaves of the tree (frames) are numbered 0 (bottom, leftmost)
 * to c_nframes - 1 (top, right). This is stored in c_frameindex and
 * c_dir if C_LOOP is set.
 */
struct combostr {
	struct combostr	*c_next;	/* list of combos at the same level */
	struct combostr	*c_prev;	/* list of combos at the same level */
	struct combostr	*c_link[2];	/* F: NULL,
					 * C: previous level */
	union comboval	c_linkv[2];	/* C: combo value for link[0, 1] */
	union comboval	c_combo;	/* F: initial combo value (read-only),
					 * C: combo value for this level */
	spot_index	c_vertex;	/* F: frame head,
					 * C: intersection */
	u_char		c_nframes;	/* F: 1,
					 * C: number of frames in the combo */
	u_char		c_dir;		/* F: frame direction,
					 * C: loop frame */
	u_char		c_flags;	/* C: combo flags */
	u_char		c_frameindex;	/* C: intersection frame index */
	u_char		c_framecnt[2];	/* number of frames left to attach */
	u_char		c_emask[2];	/* C: bit mask of completion spots for
					 * link[0] and link[1] */
	u_char		c_voff[2];	/* C: vertex offset within frame */
};

/* flag values for c_flags */
#define C_OPEN_0	0x01		/* link[0] is an open-ended frame */
#define C_OPEN_1	0x02		/* link[1] is an open-ended frame */
#define C_LOOP		0x04		/* link[1] intersects previous frame */

/*
 * This structure is used for recording the completion points of
 * multi frame combos.
 */
struct	elist {
	struct elist	*e_next;	/* list of completion points */
	struct combostr	*e_combo;	/* the whole combo */
	u_char		e_off;		/* offset in frame of this empty spot */
	u_char		e_frameindex;	/* intersection frame index */
	u_char		e_framecnt;	/* number of frames left to attach */
	u_char		e_emask;	/* real value of the frame's emask */
	union comboval	e_fval;		/* frame combo value */
};

/* The index of a frame in the global 'frames'. */
typedef unsigned short frame_index;

/* 0 = right, 1 = down right, 2 = down, 3 = down left. */
typedef unsigned char direction;

/*
 * One spot structure for each location on the board.
 * A frame consists of the combination for the current spot plus the next
 * five spots in the direction.
 */
struct	spotstr {
	short		s_occ;		/* color of occupant */
	short		s_wval;		/* weighted value */
	int		s_flags;	/* flags for graph walks */
	frame_index	s_frame[4];	/* level 1 combo for [dir] */
	union comboval	s_fval[2][4];	/* combo value for [color][dir] */
	union comboval	s_combo[2];	/* minimum combo value for [color] */
	u_char		s_level[2];	/* number of frames in the min combo */
	u_char		s_nforce[2];	/* number of <1,x> combos */
	struct elist	*s_empty;	/* level n combo completion spots */
	struct elist	*s_nempty;	/* level n+1 combo completion spots */
};

/* flag values for s_flags */
#define CFLAG		0x000001	/* frame is part of a combo */
#define CFLAGALL	0x00000F	/* all frame directions marked */
#define IFLAG		0x000010	/* legal intersection point */
#define IFLAGALL	0x0000F0	/* any intersection points? */
#define FFLAG		0x000100	/* frame is part of a <1,x> combo */
#define FFLAGALL	0x000F00	/* all force frames */
#define MFLAG		0x001000	/* frame has already been seen */
#define MFLAGALL	0x00F000	/* all frames seen */
#define BFLAG		0x010000	/* frame intersects border or dead */
#define BFLAGALL	0x0F0000	/* all frames dead */

struct game {
	unsigned int	nmoves;		/* number of played moves */
	spot_index	moves[BSZ * BSZ]; /* log of all played moves */
	spot_index	win_spot;	/* the winning move, or 0 */
	direction	win_dir;
};

extern	const char	letters[];
extern	const char	pdir[];

extern	const int     dd[4];
extern	struct	spotstr	board[BAREA];		/* info for board */
extern	struct	combostr frames[FAREA];		/* storage for single frames */
extern	struct	combostr *sortframes[2];	/* sorted, non-empty frames */
extern	u_char	overlap[FAREA * FAREA];
extern	spot_index intersect[FAREA * FAREA];	/* frame [a][b] intersection */
extern	struct game	game;
extern	int	debug;

extern bool interactive;
extern const char *plyr[];

void	init_board(void);
int	get_coord(void);
int	get_key(const char *);
bool	get_line(char *, int, void (*)(const char *));
void	ask(const char *);
void	dislog(const char *);
void	bdump(FILE *);
void	bdisp(void);
void	bdisp_init(void);
void	cursfini(void);
void	cursinit(void);
void	bdwho(void);
void	panic(const char *, ...) __printflike(1, 2) __dead;
void	debuglog(const char *, ...) __printflike(1, 2);
void	whatsup(int);
const char *stoc(spot_index);
spot_index ctos(const char *);
int	makemove(int, spot_index);
void	clearcombo(struct combostr *, int);
void	markcombo(struct combostr *);
int	pickmove(int);
#if defined(DEBUG)
void	printcombo(struct combostr *, char *, size_t);
#endif
