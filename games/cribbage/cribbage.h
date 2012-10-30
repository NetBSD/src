/*	$NetBSD: cribbage.h,v 1.16.2.1 2012/10/30 18:58:19 yamt Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)cribbage.h	8.1 (Berkeley) 5/31/93
 */

extern  CARD		deck[ CARDS ];		/* a deck */
extern  CARD		phand[ FULLHAND ];	/* player's hand */
extern  CARD		chand[ FULLHAND ];	/* computer's hand */
extern  CARD		crib[ CINHAND ];	/* the crib */
extern  CARD		turnover;		/* the starter */

extern  CARD		known[ CARDS ];		/* cards we have seen */
extern  int		knownum;		/* # of cards we know */

extern  int		pscore;			/* player's score */
extern  int		cscore;			/* comp's score */
extern  int		glimit;			/* points to win game */

extern  int		pgames;			/* player's games won */
extern  int		cgames;			/* comp's games won */
extern  int		gamecount;		/* # games played */
extern	int		Lastscore[2];		/* previous score for each */

extern  BOOLEAN		iwon;			/* if comp won last */
extern  BOOLEAN		explain;		/* player mistakes explained */
extern  BOOLEAN		rflag;			/* if all cuts random */
extern  BOOLEAN		quiet;			/* if suppress random mess */

extern  char		explan[];		/* string for explanation */

void	 addmsg(const char *, ...) __printflike(1, 2);
int	 adjust(const CARD [], CARD);
int	 anymove(const CARD [], int, int);
void	 bye(void);
int	 cchose(const CARD [], int, int);
void	 cdiscard(BOOLEAN);
int	 chkscr(int *, int);
int	 comphand(const CARD [], const char *);
void	 cremove(CARD, CARD [], int);
void	 do_wait(void);
void	 endmsg(void);
char	*get_line(void);
int	 getuchar(void);
int	 infrom(const CARD [], int, const char *);
void	 instructions(void);
int	 is_one(CARD, const CARD [], int);
void	 makedeck(CARD []);
void	 makeknown(const CARD [], int);
void	 msg(const char *, ...) __printflike(1, 2);
int	 msgcard(CARD, BOOLEAN);
int	 number(int, int, const char *);
int	 pegscore(CARD, const CARD [], unsigned, int);
int	 plyrhand(const CARD [], const char *);
void	 prcard(WINDOW *, int, int, CARD, BOOLEAN);
void	 prhand(const CARD [], unsigned, WINDOW *, BOOLEAN);
void	 receive_intr(int) __dead;
int	 scorehand(const CARD [], CARD, int, BOOLEAN, BOOLEAN);
void	 shuffle(CARD []);
void	 sorthand(CARD [], int);
