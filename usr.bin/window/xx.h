/*	$NetBSD: xx.h,v 1.7 2003/08/07 11:17:47 agc Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)xx.h	8.1 (Berkeley) 6/6/93
 */

#ifndef EXTERN
#define EXTERN extern
#endif

struct xx {
	enum { xc_move, xc_scroll, xc_inschar, xc_insspace, xc_delchar,
		xc_clear, xc_clreos, xc_clreol, xc_write } cmd;
	int arg0;
	int arg1;
	int arg2;
	int arg3;
	char *buf;
	struct xx *link;
};

EXTERN struct xx *xx_head, *xx_tail;
EXTERN struct xx *xx_freelist;

EXTERN char *xxbuf, *xxbufp, *xxbufe;
EXTERN int xxbufsize;

#define char_sep '\0'

struct xx *xxalloc(void);
void	xxclear(void);
void	xxclreol(int, int);
void	xxclreos(int, int);
void	xxdelchar(int, int);
void	xxend(void);
void	xxflush(int);
void	xxflush_scroll(struct xx *);
void	xxfree(struct xx *);
int	xxinit(void);
void	xxinschar(int, int, int, int);
void	xxinsspace(int, int);
void	xxmove(int, int);
void	xxreset(void);
void	xxreset1(void);
void	xxscroll(int, int, int);
void	xxstart(void);
void	xxwrite(int, int, char *, int, int);
