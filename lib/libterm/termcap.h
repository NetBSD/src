/*	$NetBSD: termcap.h,v 1.8 1999/10/04 23:16:52 lukem Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christos Zoulas of Cornell University.
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
 *	@(#)termcap.h	8.1 (Berkeley) 6/4/93
 */

#include <sys/types.h>

#ifndef _TERMCAP_H_
#define _TERMCAP_H_

struct tinfo;

__BEGIN_DECLS
int   tgetent	__P((char *, const char *));
char *tgetstr	__P((const char *, char **));
int   tgetflag	__P((const char *));
int   tgetnum	__P((const char *));
char *tgoto	__P((const char *, int, int));
void  tputs	__P((const char *, int, int (*)(int)));

/*
 * New interface
 */
int   t_getent	__P((struct tinfo **, const char *));
int   t_getnum  __P((struct tinfo *, const char *));
int   t_getflag __P((struct tinfo *, const char *));
char *t_getstr  __P((struct tinfo *, const char *, char **, size_t *));
int   t_goto    __P((struct tinfo *, const char *, int, int, char *, size_t));
int   t_puts    __P((struct tinfo *, const char *, int,
		     void (*)(char, void *), void *));
void  t_freent  __P((struct tinfo *));

extern	char PC;
extern	char *BC;
extern	char *UP;
extern	short ospeed;
__END_DECLS

#endif /* _TERMCAP_H_ */
