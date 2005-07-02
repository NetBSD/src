/*	$NetBSD: cribbage.h,v 1.13 2005/07/02 08:32:32 jmc Exp $	*/

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

void	 addmsg(const char *, ...)
    __attribute__((__format__(__printf__, 1, 2)));
int	 adjust(const CARD [], CARD);
int	 anymove(const CARD [], int, int);
int	 anysumto(const CARD [], int, int, int);
void	 bye(void);
int	 cchose(const CARD [], int, int);
void	 cdiscard(BOOLEAN);
int	 chkscr(int *, int);
int	 comphand(const CARD [], const char *);
void	 cremove(CARD, CARD [], int);
int	 cut(BOOLEAN, int);
int	 deal(BOOLEAN);
void	 discard(BOOLEAN);
void	 do_wait(void);
void	 endmsg(void);
int	 eq(CARD, CARD);
int	 fifteens(const CARD [], int);
void	 game(void);
void	 gamescore(void);
char	*getline(void);
int	 getuchar(void);
int	 incard(CARD *);
int	 infrom(const CARD [], int, const char *);
void	 instructions(void);
int	 is_one(CARD, const CARD [], int);
void	 makeboard(void);
void	 makedeck(CARD []);
void	 makeknown(const CARD [], int);
void	 msg(const char *, ...)
    __attribute__((__format__(__printf__, 1, 2)));
int	 msgcard(CARD, BOOLEAN);
int	 msgcrd(CARD, BOOLEAN, const char *, BOOLEAN);
int	 number(int, int, const char *);
int	 numofval(const CARD [], int, int);
int	 pairuns(const CARD [], int);
int	 peg(BOOLEAN);
int	 pegscore(CARD, const CARD [], int, int);
int	 playhand(BOOLEAN);
int	 plyrhand(const CARD [], const char *);
void	 prcard(WINDOW *, int, int, CARD, BOOLEAN);
void	 prcrib(BOOLEAN, BOOLEAN);
void	 prhand(const CARD [], int, WINDOW *, BOOLEAN);
void	 printcard(WINDOW *, int, CARD, BOOLEAN);
void	 prpeg(int, int, BOOLEAN);
void	 prtable(int);
int	 readchar(void);
void	 receive_intr(int) __attribute__((__noreturn__));
int	 score(BOOLEAN);
int	 scorehand(const CARD [], CARD, int, BOOLEAN, BOOLEAN);
void	 shuffle(CARD []);
void	 sorthand(CARD [], int);
void	 wait_for(int);
