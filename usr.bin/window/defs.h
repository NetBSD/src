/*	$NetBSD: defs.h,v 1.6 2002/06/14 01:06:52 wiz Exp $	*/

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
 *
 *	@(#)defs.h	8.1 (Berkeley) 6/6/93
 */

#include <sys/time.h>
#include "value.h"
#include "ww.h"

#ifndef EXTERN
#define EXTERN extern
#endif

#define NWINDOW 9

EXTERN struct timeval starttime;

EXTERN struct ww *window[NWINDOW];	/* the windows */
EXTERN struct ww *selwin;		/* the selected window */
EXTERN struct ww *lastselwin;		/* the last selected window */
EXTERN struct ww *cmdwin;		/* the command window */
EXTERN struct ww *framewin;		/* the window for framing */
EXTERN struct ww *boxwin;		/* the window for the box */
EXTERN struct ww *fgwin;		/* the last foreground window */

#define isfg(w)		((w)->ww_order <= fgwin->ww_order)

EXTERN char *default_shell[128];	/* default shell argv */
EXTERN char *default_shellfile;		/* default shell program */
EXTERN int default_nline;		/* default buffer size for new windows */
EXTERN int default_smooth;		/* default "smooth" parameter */
EXTERN char escapec;			/* the escape character */

/* flags */
EXTERN char quit;			/* quit command issued */
EXTERN char terse;			/* terse mode */
EXTERN char debug;			/* debug mode */
EXTERN char incmd;			/* in command mode */

void		addwin(struct ww *, char);
int		ccinit(void);
void		ccend(void);
void		ccflush(void);
void		ccreset(void);
void		ccstart(void);
void		c_colon(void);
void		c_debug(void);
void		c_help(void);
void		c_move(struct ww *);
void		c_put(void);
void		c_quit(void);
void		c_size(struct ww *);
void		c_window(void);
void		c_yank(void);
void		closeiwin(struct ww *);
void		closewin(struct ww *);
void		closewin1(struct ww *);
int		cx_beginbuf(char *, struct value *, int);
int		cx_beginfile(char *);
void		cx_end(void);
void		deletewin(struct ww *);
void		docmd(void);
int		doconfig(void);
void		dodefault(void);
int		dolongcmd(char *, struct value *, int);
int		dosource(char *);
void		error(const char *, ...);
void		err_end(void);
int		findid(void);
struct ww      *findselwin(void);
void		front(struct ww *, char);
int		getpos(int *, int *, int, int, int, int);
struct ww      *getwin(void);
void		labelwin(struct ww *);
void		mloop(void);
int		more(struct ww *,  char);
void		movewin(struct ww *, int, int);
struct ww      *openwin(int, int, int, int, int, int, char *, int, int,
			char *, char **);
struct ww      *openiwin(int, char *);
void		p_memerror(void);
void		p_start(void);
void		reframe(void);
void		setcmd(char);
void		setescape(char *);
int		setlabel(struct ww *, char *);
void		setselwin(struct ww *);
void		setterse(char);
void		setvars(void);
void		sizewin(struct ww *, int, int);
void		startwin(struct ww *);
void		stopwin(struct ww *);
int		s_gettok(void);
void		verror(const char *, va_list);
void		waitnl(struct ww *);
int		waitnl1(struct ww *, char *);
