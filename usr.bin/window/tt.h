/*	$NetBSD: tt.h,v 1.8.42.1 2009/05/13 19:20:12 jym Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
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
 *	@(#)tt.h	8.1 (Berkeley) 6/6/93
 */

#include <unistd.h>

#ifndef EXTERN
#define EXTERN extern
#endif

/*
 * Interface structure for the terminal drivers.
 */
struct tt {
		/* startup and cleanup */
	void	(*tt_start)(void);
	void	(*tt_reset)(void);
	void	(*tt_end)(void);

		/* terminal functions */
	void	(*tt_move)(int, int);
	void	(*tt_insline)(int);
	void	(*tt_delline)(int);
	void	(*tt_inschar)(char);
	void	(*tt_insspace)(int);
	void	(*tt_delchar)(int);
	void	(*tt_write)(const char *, int);	/* write a whole block */
	void	(*tt_putc)(char);		/* write one character */
	void	(*tt_clreol)(void);
	void	(*tt_clreos)(void);
	void	(*tt_clear)(void);
	void	(*tt_scroll_down)(int);
	void	(*tt_scroll_up)(int);
	void	(*tt_setscroll)(int, int);/* set scrolling region */
	void	(*tt_setmodes)(int);	/* set display modes */
	void	(*tt_set_token)(int, char *, int);
						/* define a token */
	void	(*tt_put_token)(int, const char *, int);
						/* refer to a defined token */
	void	(*tt_compress)(int);	/* begin, end compression */
	void	(*tt_checksum)(char *, int);
						/* compute checksum */
	void	(*tt_checkpoint)(void);	/* checkpoint protocol */
	int	(*tt_rint)(char *, int);	/* input processing */

		/* internal variables */
	char tt_modes;			/* the current display modes */
	char tt_nmodes;			/* the new modes for next write */
	char tt_insert;			/* currently in insert mode */
	int tt_row;			/* cursor row */
	int tt_col;			/* cursor column */
	int tt_scroll_top;		/* top of scrolling region */
	int tt_scroll_bot;		/* bottom of scrolling region */

		/* terminal info */
	int tt_nrow;			/* number of display rows */
	int tt_ncol;			/* number of display columns */
	char tt_availmodes;		/* the display modes supported */
	char tt_wrap;			/* has auto wrap around */
	char tt_retain;			/* can retain below (db flag) */
	short tt_padc;			/* the pad character */
	int tt_ntoken;			/* number of compression tokens */
	int tt_token_min;		/* minimun token size */
	int tt_token_max;		/* maximum token size */
	int tt_set_token_cost;		/* cost in addition to string */
	int tt_put_token_cost;		/* constant cost */
	int tt_ack;			/* checkpoint ack-nack flag */

		/* the frame characters */
	short *tt_frame;

		/* ttflush() hook */
	void	(*tt_flush)(void);
};
EXTERN struct tt tt;

/*
 * tt_padc is used by the compression routine.
 * It is a short to allow the driver to indicate that there is no padding.
 */
#define TT_PADC_NONE 0x100

/*
 * List of terminal drivers.
 */
struct tt_tab {
	const char *tt_name;
	int tt_len;
	int (*tt_func)(void);
};
EXTERN struct tt_tab tt_tab[];

/*
 * Clean interface to termcap routines.
 * Too may t's.
 */
EXTERN char tt_strings[1024];		/* string buffer */
EXTERN char *tt_strp;			/* pointer for it */

struct tt_str {
	const char *ts_str;
	int ts_n;
};

struct tt_str *tttgetstr(const char *);
struct tt_str *ttxgetstr(const char *);	/* tgetstr() and expand delays */

int	tt_f100(void);
int	tt_generic(void);
int	tt_h19(void);
int	tt_h29(void);
int	tt_tvi925(void);
int	tt_wyse60(void);
int	tt_wyse75(void);
int	tt_zapple(void);
int	tt_zentec(void);
void	ttflush(void);
struct tt_str *tttgetstr(const char *);
int	ttinit(void);
void	ttpgoto(struct tt_str *, int, int, int);
void	ttputs(const char *);
int	ttstrcmp(struct tt_str *, struct tt_str *);
void	tttgoto(struct tt_str *, int, int);
int	tttputc(int);
void	ttwrite(const char *, int);
int	ttxputc(int);

#define tttputs(s, n)	tputs((s)->ts_str, (n), tttputc)
#define ttxputs(s)	ttwrite((s)->ts_str, (s)->ts_n)

/*
 * Buffered output without stdio.
 * These variables have different meanings from the ww_ob* variables.
 * But I'm too lazy to think up different names.
 */
EXTERN char *tt_ob;
EXTERN char *tt_obp;
EXTERN char *tt_obe;
#define ttputc(c)	(tt_obp < tt_obe ? (*tt_obp++ = (c)) \
				: (ttflush(), *tt_obp++ = (c)))

/*
 * Convenience macros for the drivers
 * They require char.h
 */
#define ttctrl(c)	ttputc(ctrl(c))
#define ttesc(c)	(ttctrl('['), ttputc(c))
