/*	$NetBSD: extern.h,v 1.5 1997/10/10 02:07:06 lukem Exp $	*/

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

extern char		GAMES[], *file;

extern int		clck, safe_planes, start_time, test_mode;

extern FILE		*filein, *fileout;

extern C_SCREEN		screen, *sp;

extern LIST		air, ground;

extern struct termios	tty_start, tty_new;

extern DISPLACEMENT	displacement[MAXDIR];

int		addplane __P((void));
void		append __P((LIST *, PLANE *));
void		check_adir __P((int, int, int));
void		check_edge __P((int, int));
void		check_edir __P((int, int, int));
void		check_line __P((int, int, int, int));
void		check_linepoint __P((int, int));
void		check_point __P((int, int));
int		checkdefs __P((void));
int		compar __P((const void *, const void *));
void		delete __P((LIST *, PLANE *));
int		dir_deg __P((int));
int		dir_no __P((char));
void		done_screen __P((void));
void		draw_all __P((void));
void		draw_line __P((WINDOW *, int, int, int, int, char *));
void		erase_all __P((void));
int		getAChar __P((void));
int		getcommand __P((void));
int		gettoken __P((void));
void		init_gr __P((void));
void		ioaddstr __P((int, char *));
void		ioclrtobot __P((void));
void		ioclrtoeol __P((int));
void		ioerror __P((int, int, char *));
void		iomove __P((int));
int		list_games __P((void));
int		log_score __P((int));
void		log_score_quit __P((int));
void		loser __P((PLANE *, char *));
int		main __P((int, char *[]));
char		name __P((PLANE *));
int		next_plane __P((void));
void		noise __P((void));
int		number __P((char));
void		planewin __P((void));
int		pop __P((void));
void		push __P((int, int));
void		quit __P((int));
int		read_file __P((char *));
void		redraw __P((void));
void		rezero __P((void));
void		setup_screen __P((C_SCREEN *));
int		too_close __P((PLANE *p1, PLANE *p2, int));
void		update __P((int));
int		yyerror __P((const char *));
int		yylex __P((void));
int		yyparse __P((void));
char	       *Left __P((char));
char	       *Right __P((char));
char	       *airport __P((char));
char	       *beacon __P((char));
char	       *benum __P((char));
char	       *circle __P((char));
char	       *climb __P((char));
char	       *command __P((PLANE *));
char	       *default_game __P((void));
char	       *delayb __P((char));
char	       *descend __P((char));
char	       *ex_it __P((char));
PLANE	       *findplane __P((int));
char	       *ignore __P((char));
char	       *left __P((char));
char	       *mark __P((char));
PLANE	       *newplane __P((void));
char	       *okay_game __P((char *));
char	       *rel_dir __P((char));
char	       *right __P((char));
char	       *setalt __P((char));
char	       *setplane __P((char));
char	       *setrelalt __P((char));
char	       *timestr __P((int));
char	       *to_dir __P((char));
char	       *turn __P((char));
char	       *unmark __P((char));
