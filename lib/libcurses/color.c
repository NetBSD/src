/*	$NetBSD: color.c,v 1.14 2001/01/05 22:57:56 christos Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: color.c,v 1.14 2001/01/05 22:57:56 christos Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/* the following is defined and set up in setterm.c */
extern struct tinfo *_cursesi_genbuf;

/* Maximum colours */
#define	MAX_COLORS	64
/* Maximum colour pairs - determined by number of colour bits in attr_t */
#define	MAX_PAIRS	64	/* PAIR_NUMBER(__COLOR) + 1 */

/* Flags for colours and pairs */
#define	__USED		0x01

/* List of colours */
struct color {
	short	num;
	short	red;
	short	green;
	short	blue;
	int	flags;
} colors[MAX_COLORS];

/* List of colour pairs */
struct pair {
	short	fore;
	short	back;
	int	flags;
} pairs[MAX_PAIRS];

/* Attributes that clash with colours */
attr_t	__nca;

/* Style of colour manipulation */
#define COLOR_NONE	0
#define COLOR_ANSI	1	/* ANSI/DEC-style colour manipulation */
#define COLOR_HP	2	/* HP-style colour manipulation */
#define COLOR_TEK	3	/* Tektronix-style colour manipulation */
#define COLOR_OTHER	4	/* None of the others but can set fore/back */
int	__color_type = COLOR_NONE;

static void
__change_pair __P((short));
/*
 * has_colors --
 *	Check if terminal has colours.
 */
bool
has_colors(void)
{
	if (__tc_Co > 0 && __tc_pa > 0 && ((__tc_AF != NULL &&
	    __tc_AB != NULL) || __tc_Ip != NULL || __tc_Ic != NULL ||
	    (__tc_Sb != NULL && __tc_Sf != NULL)))
		return(TRUE);
	else
		return(FALSE);
}

/*
 * can_change_colors --
 *	Check if terminal can change colours.
 */
bool
can_change_colors(void)
{
	if (__tc_cc)
		return(TRUE);
	else
		return(FALSE);
}

/*
 * start_color --
 *	Initialise colour support.
 */
int
start_color(void)
{
	int	i;
	attr_t	temp_nc;

	if (has_colors() == FALSE)
		return(ERR);

	/* Max colours and colour pairs */
	if (__tc_Co == -1)
		COLORS = 0;
	else {
		COLORS = __tc_Co > MAX_COLORS ? MAX_COLORS : __tc_Co;
		if (__tc_pa == -1) {
			COLOR_PAIRS = 0;
			COLORS = 0;
		} else {
			COLOR_PAIRS = __tc_pa > MAX_PAIRS ? MAX_PAIRS : __tc_pa;
		}
	}
	if (!COLORS)
		return (ERR);

	/* Reset terminal colour and colour pairs. */
	if (__tc_oc != NULL)
		tputs(__tc_oc, 0, __cputchar);
	if (__tc_op != NULL) {
		tputs(__tc_op, 0, __cputchar);
		curscr->wattr &= __mask_op;
	}

	/* Type of colour manipulation - ANSI/TEK/HP/other */
	if (__tc_AF != NULL && __tc_AB != NULL)
		__color_type = COLOR_ANSI;
	else if (__tc_Ip != NULL)
		__color_type = COLOR_HP;
	else if (__tc_Ic != NULL)
		__color_type = COLOR_TEK;
	else if (__tc_Sb != NULL && __tc_Sf != NULL)
		__color_type = COLOR_OTHER;
	else
		return(ERR);		/* Unsupported colour method */

#ifdef DEBUG
	__CTRACE("start_color: COLORS = %d, COLOR_PAIRS = %d",
	    COLORS, COLOR_PAIRS);
	switch (__color_type) {
	case COLOR_ANSI:
		__CTRACE(" (ANSI style)\n");
		break;
	case COLOR_HP:
		__CTRACE(" (HP style)\n");
		break;
	case COLOR_TEK:
		__CTRACE(" (Tektronics style)\n");
		break;
	case COLOR_OTHER:
		__CTRACE(" (Other style)\n");
		break;
	}
#endif
	
	/*
	 * Attributes that cannot be used with color.
	 * Store these in an attr_t for wattrset()/wattron().
	 */
	__nca = __NORMAL;
	if (__tc_NC != -1) {
		temp_nc = (attr_t) t_getnum(_cursesi_genbuf, "NC");
		if (temp_nc & 0x0001)
			__nca |= __STANDOUT;
		if (temp_nc & 0x0002)
			__nca |= __UNDERSCORE;
		if (temp_nc & 0x0004)
			__nca |= __REVERSE;
		if (temp_nc & 0x0008)
			__nca |= __BLINK;
		if (temp_nc & 0x0010)
			__nca |= __DIM;
		if (temp_nc & 0x0020)
			__nca |= __BOLD;
		if (temp_nc & 0x0040)
			__nca |= __BLANK;
		if (temp_nc & 0x0080)
			__nca |= __PROTECT;
		if (temp_nc & 0x0100)
			__nca |= __ALTCHARSET;
	}
#ifdef DEBUG
	__CTRACE ("start_color: __nca = %d\n", __nca);
#endif

	/* Set up initial 8 colours */
	if (COLORS >= COLOR_BLACK)
		(void) init_color(COLOR_BLACK, 0, 0, 0);
	if (COLORS >= COLOR_RED)
		(void) init_color(COLOR_RED, 1000, 0, 0);
	if (COLORS >= COLOR_GREEN)
		(void) init_color(COLOR_GREEN, 0, 1000, 0);
	if (COLORS >= COLOR_YELLOW)
		(void) init_color(COLOR_YELLOW, 1000, 1000, 0);
	if (COLORS >= COLOR_BLUE)
		(void) init_color(COLOR_BLUE, 0, 0, 1000);
	if (COLORS >= COLOR_MAGENTA)
		(void) init_color(COLOR_MAGENTA, 1000, 0, 1000);
	if (COLORS >= COLOR_CYAN)
		(void) init_color(COLOR_CYAN, 0, 1000, 1000);
	if (COLORS >= COLOR_WHITE)
		(void) init_color(COLOR_WHITE, 1000, 1000, 1000);

	/* Initialise other colours */
	for (i = 8; i < COLORS; i++) {
		colors[i].red = 0;
		colors[i].green = 0;
		colors[i].blue = 0;
		colors[i].flags = 0;
	}

	/* Initialise colour pairs to default (white on black) */
	for (i = 0; i < COLOR_PAIRS; i++) {
		pairs[i].fore = COLOR_WHITE;
		pairs[i].back = COLOR_BLACK;
		pairs[i].flags = 0;
	}

	return(OK);
}

/*
 * init_pair --
 *	Set pair foreground and background colors.
 */
int
init_pair(short pair, short fore, short back)
{
	int	changed;

#ifdef DEBUG
	__CTRACE("init_pair: %d, %d, %d\n", pair, fore, back);
#endif

	if (pair < 0 || pair >= COLOR_PAIRS)
		return (ERR);
	if (fore < 0 || fore >= COLORS)
		return (ERR);
	if (back < 0 || back >= COLORS)
		return (ERR);

	if ((pairs[pair].flags & __USED) && (fore != pairs[pair].fore ||
	    back != pairs[pair].back))
		changed = 1;
	else
		changed = 0;

	pairs[pair].flags |= __USED;
	pairs[pair].fore = fore;
	pairs[pair].back = back;

	/* XXX: need to initialise HP style (Ip) */

	if (changed)
		__change_pair(pair);
	return (OK);
}

/*
 * pair_content --
 *	Get pair foreground and background colours.
 */
int
pair_content(short pair, short *forep, short *backp)
{
	if (pair < 0 || pair >= COLOR_PAIRS)
		return(ERR);

	*forep = pairs[pair].fore;
	*backp = pairs[pair].back;
	return(OK);
}

/*
 * init_color --
 *	Set colour red, green and blue values.
 */
int
init_color(short color, short red, short green, short blue)
{
#ifdef DEBUG
	__CTRACE("init_color: %d, %d, %d, %d\n", color, red, green, blue);
#endif
	if (color < 0 || color >= COLORS)
		return(ERR);

	colors[color].red = red;
	colors[color].green = green;
	colors[color].blue = blue;
	/* XXX Not yet implemented */
	return(ERR);
	/* XXX: need to initialise Tek style (Ic) and support HLS */
}

/*
 * color_content --
 *	Get colour red, green and blue values.
 */
int
color_content(short color, short *redp, short *greenp, short *bluep)
{
	if (color < 0 || color >= COLORS)
		return(ERR);

	*redp = colors[color].red;
	*greenp = colors[color].green;
	*bluep = colors[color].blue;
	return(OK);
}

/*
 * __set_color --
 *	Set terminal foreground and background colours.
 */
void
__set_color(attr_t attr)
{
	short	pair;

	pair = PAIR_NUMBER((u_int32_t)attr);
#ifdef DEBUG
	__CTRACE("__set_color: %d, %d, %d\n", pair, pairs[pair].fore,
	    pairs[pair].back);
#endif
	switch (__color_type) {
	/* Set ANSI forground and background colours */
	case COLOR_ANSI:
		tputs(__parse_cap(__tc_AF, pairs[pair].fore), 0, __cputchar);
		tputs(__parse_cap(__tc_AB, pairs[pair].back), 0, __cputchar);
		break;
	case COLOR_HP:
		/* XXX: need to support HP style */
		break;
	case COLOR_TEK:
		/* XXX: need to support Tek style */
		break;
	case COLOR_OTHER:
		tputs(__parse_cap(__tc_Sf, pairs[pair].fore), 0, __cputchar);
		tputs(__parse_cap(__tc_Sb, pairs[pair].back), 0, __cputchar);
		break;
	}
}

/*
 * __restore_colors --
 *	Redo color definitions after restarting 'curses' mode.
 */
void
__restore_colors(void)
{
	if (__tc_cc != NULL)
		switch (__color_type) {
		case COLOR_HP:
			/* XXX: need to re-initialise HP style (Ip) */
			break;
		case COLOR_TEK:
			/* XXX: need to re-initialise Tek style (Ic) */
			break;
		}
}

/*
 * __change_pair --
 *	Mark dirty all positions using pair.
 */
void
__change_pair(short pair)
{
	struct __winlist	*wlp;
	WINDOW			*win;
	int			 y, x;

	
	for (wlp = __winlistp; wlp != NULL; wlp = wlp->nextp) {
#ifdef DEBUG
		__CTRACE("__change_pair: win = %0.2o\n", wlp->winp);
#endif
		if (wlp->winp == curscr) {
			/* Reset colour attribute on curscr */
#ifdef DEBUG
			__CTRACE("__change_pair: win == curscr\n");
#endif
			for (y = 0; y < curscr->maxy; y++)
				for (x = 0; x < curscr->maxx; x++)
					if ((curscr->lines[y]->line[x].attr &
					    __COLOR) == COLOR_PAIR(pair))
						curscr->lines[y]->line[x].attr
						    &= ~__COLOR;
		} else {
			/* Mark dirty those positions with color pair "pair" */
			win = wlp->winp;
			for (y = 0; y < win->maxy; y++) {
				for (x = 0; x < win->maxx; x++)
					if ((win->lines[y]->line[x].attr &
					    __COLOR) == COLOR_PAIR(pair)) {
						if (!(win->lines[y]->flags &
						    __ISDIRTY))
							win->lines[y]->flags |=
							    __ISDIRTY;
						/*
						 * firstchp/lastchp are shared
						 * between parent window and
						 * sub-window.
						 */
						if (*win->lines[y]->firstchp >
						    x)
							*win->lines[y]->firstchp
							    = x;
						if (*win->lines[y]->lastchp < x)
							*win->lines[y]->lastchp
							    = x;
					}
#ifdef DEBUG
				if ((win->lines[y]->flags & __ISDIRTY))
					__CTRACE("__change_pair: first = %d, last = %d\n", *win->lines[y]->firstchp, *win->lines[y]->lastchp);
#endif
			}
		}
	}
}
