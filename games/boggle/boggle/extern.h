/*	$NetBSD: extern.h,v 1.11 2009/08/12 05:29:40 dholland Exp $	*/

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

void	 addword(const char *);
void	 badword(void);
int	 checkword(const char *, int, int *);
void	 cleanup(void);
void	 delay(int);
long	 dictseek(FILE *, long, int);
void	 findword(void);
void	 flushin(FILE *);
char	*get_line(char *);
int	 help(void);
int	 inputch(void);
int	 loaddict(FILE *);
int	 loadindex(const char *);
char	*nextword(FILE *);
FILE	*opendict(const char *);
void	 prompt(const char *);
void	 prtable(const char *const [],
	    int, int, int, void (*)(const char *const [], int), 
	    int (*)(const char *const [], int));
void	 redraw(void);
void	 results(void);
int	 setup(int, time_t);
void	 showboard(const char *);
void	 showstr(const char *, int);
void	 showword(int);
void	 startwords(void);
int	 timerch(void);
