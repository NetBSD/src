/*	$NetBSD: defs.h,v 1.5 1998/10/14 00:58:47 wsanchez Exp $	*/

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

void		addwin __P((struct ww *, char));
int		ccinit __P((void));
void		ccend __P((void));
void		ccflush __P((void));
void		ccreset __P((void));
void		ccstart __P((void));
void		c_colon __P((void));
void		c_debug __P((void));
void		c_help __P((void));
void		c_move __P((struct ww *));
void		c_put __P((void));
void		c_quit __P((void));
void		c_size __P((struct ww *));
void		c_window __P((void));
void		c_yank __P((void));
void		closeiwin __P((struct ww *));
void		closewin __P((struct ww *));
void		closewin1 __P((struct ww *));
int		cx_beginbuf __P((char *, struct value *, int));
int		cx_beginfile __P((char *));
void		cx_end __P((void));
void		deletewin __P((struct ww *));
void		docmd __P((void));
int		doconfig __P((void));
void		dodefault __P((void));
int		dolongcmd __P((char *, struct value *, int));
int		dosource __P((char *));
void		error __P((const char *, ...));
void		err_end __P((void));
int		findid __P((void));
struct ww      *findselwin __P((void));
void		front __P((struct ww *, char));
int		getpos __P((int *, int *, int, int, int, int));
struct ww      *getwin __P((void));
void		labelwin __P((struct ww *));
void		mloop __P((void));
int		more __P((struct ww *,  char));
void		movewin __P((struct ww *, int, int));
struct ww      *openwin __P((int, int, int, int, int, int, char *, int, int,
			    char *, char **));
struct ww      *openiwin __P((int, char *));
void		p_memerror __P((void));
void		p_start __P((void));
void		reframe __P((void));
void		setcmd __P((char));
void		setescape __P((char *));
int		setlabel __P((struct ww *, char *));
void		setselwin __P((struct ww *));
void		setterse __P((char));
void		setvars __P((void));
void		sizewin __P((struct ww *, int, int));
void		startwin __P((struct ww *));
void		stopwin __P((struct ww *));
int		s_gettok __P((void));
void		verror __P((const char *, va_list));
void		waitnl __P((struct ww *));
int		waitnl1 __P((struct ww *, char *));
