/*	$NetBSD: extern.h,v 1.16.12.1 2014/08/20 00:00:21 tls Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ed James.
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
 *	@(#)extern.h	8.1 (Berkeley) 5/31/93
 */

/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

extern char		GAMES[];
extern const char	*filename;

extern int		clck, safe_planes, test_mode;
extern time_t		start_time;

#if 0
extern FILE		*filein, *fileout;
#endif 

extern C_SCREEN		*sp;

extern LIST		air, ground;

extern struct termios	tty_start, tty_new;

extern DISPLACEMENT	displacement[MAXDIR];

void		addplane(void);
void		append(LIST *, PLANE *);
void		check_adir(int, int, int);
void		delete(LIST *, PLANE *);
int		dir_no(int);
void		done_screen(void);
void		draw_all(void);
void		erase_all(void);
int		getAChar(void);
int		getcommand(void);
void		init_gr(void);
void		ioaddstr(int, const char *);
void		ioclrtobot(void);
void		ioclrtoeol(int);
void		ioerror(int, int, const char *);
void		iomove(int);
int		log_score(int);
void		log_score_quit(int) __dead;
void		loser(const PLANE *, const char *) __dead;
int		main(int, char *[]);
char		name(const PLANE *);
int		number(int);
void		open_score_file(void);
void		planewin(void);
void		quit(int);
void		redraw(void);
void		setup_screen(const C_SCREEN *);
void		update(int);
int		yylex(void);
#ifndef YYEMPTY
int		yyparse(void);
#endif
const char     *command(const PLANE *);
PLANE	       *findplane(int);
PLANE	       *newplane(void);
