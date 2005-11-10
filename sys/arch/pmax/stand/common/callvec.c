/*	$NetBSD: callvec.c,v 1.13.34.4 2005/11/10 13:58:15 skrll Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)callvec.c	8.1 (Berkeley) 6/10/93
 */

#include <machine/dec_prom.h>

const struct callback callvec = {
	(void *(*) __P((void *, void *, int)))0,
	(void *(*) __P((void *, int, int)))0,
	(char *(*) __P((char *, char *)))DEC_PROM_STRCAT,
	(int (*) __P((char *, char *)))DEC_PROM_STRCMP,
	(char *(*) __P((char *, char *)))DEC_PROM_STRCPY,
	(int (*) __P((char *)))DEC_PROM_STRLEN,
	(char *(*) __P((char *, char *, int)))0,
	(char *(*) __P((char *, char *, int)))0,
	(int (*) __P((char *, char *, int)))0,
	(int (*) __P((void)))DEC_PROM_GETCHAR,
	(char *(*) __P((char *)))DEC_PROM_GETS,
	(int (*) __P((char *)))DEC_PROM_PUTS,
	(int (*) __P((const char *, ...)))DEC_PROM_PRINTF,
	(int (*) __P((char *, char *, ...)))0,
	(int (*) __P((void)))0,
	(long (*) __P((char *, char **, int)))0,
	(psig_t (*) __P((int, psig_t)))0,
	(int (*) __P((int)))0,
	(long (*) __P((long *)))0,
	(int (*) __P((jmp_buf)))0,
	(void (*) __P((jmp_buf, int)))0,
	(int (*) __P(( char *)))0,
	(int (*) __P((int, void *, int)))0,
	(int (*) __P((int, void *, int)))0,
	(int (*) __P((char *, char *)))DEC_PROM_SETENV2,
	(char *(*) __P((const char *)))DEC_PROM_GETENV2,
	(int (*) __P((char *)))DEC_PROM_UNSETENV,
	(u_long (*) __P((int)))0,
	(void (*) __P((void)))0,
	(void (*) __P((int)))0,
	(void (*) __P((int)))0,
	(void (*) __P((char *, int)))DEC_PROM_CLEARCACHE,
	(int (*) __P((void)))0,
	(int (*) __P((memmap *)))0,
	(int (*) __P((int)))0,
	(int (*) __P((int)))0,
	(int (*) __P((int)))0,
	(void *)0,
	(int (*) __P((void)))0,
	(void (*) __P((int *, int)))0,
	(void (*) __P((void)))0,
	(tcinfo *(*) __P((void)))0,	/* XXX what are the args for this?*/
	(int (*) __P((char *)))0,
	(void (*) __P((char)))0,
};

const   struct callback *callv = &callvec;
