/*	$NetBSD: extern.h,v 1.7 2003/08/07 09:37:05 agc Exp $	*/

/*-
 * Copyright (c) 1993
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
 *	@(#)extern.h	8.1 (Berkeley) 6/11/93
 */

#include <time.h>

void	 addword __P((const char *));
void	 badword __P((void));
char	*batchword __P((FILE *));
void	 checkdict __P((void));
int	 checkword __P((const char *, int, int *));
void	 cleanup __P((void));
void	 delay __P((int));
long	 dictseek __P((FILE *, long, int));
void	 findword __P((void));
void	 flushin __P((FILE *));
char	*getline __P((char *));
void	 getword __P((char *));
int	 help __P((void));
int	 inputch __P((void));
int	 loaddict __P((FILE *));
int	 loadindex __P((const char *));
void	 newgame __P((const char *));
char	*nextword __P((FILE *));
FILE	*opendict __P((const char *));
void	 playgame __P((void));
void	 prompt __P((const char *));
void	 prtable __P((const char *const [],
	    int, int, int, void (*)(const char *const [], int), int (*)(const char *const [], int)));
void	 putstr __P((const char *));
void	 redraw __P((void));
void	 results __P((void));
int	 setup __P((int, time_t));
void	 showboard __P((const char *));
void	 showstr __P((const char *, int));
void	 showword __P((int));
void	 starttime __P((void));
void	 startwords __P((void));
void	 stoptime __P((void));
int	 timerch __P((void));
void	 usage __P((void)) __attribute__((__noreturn__));
int	 validword __P((const char *));
