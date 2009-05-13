/*	$NetBSD: callvec.c,v 1.16.92.1 2009/05/13 17:18:13 jym Exp $	*/

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
	(void *(*)(void *, void *, int))0,
	(void *(*)(void *, int, int))0,
	(char *(*)(char *, char *))DEC_PROM_STRCAT,
	(int (*)(char *, char *))DEC_PROM_STRCMP,
	(char *(*)(char *, char *))DEC_PROM_STRCPY,
	(int (*)(char *))DEC_PROM_STRLEN,
	(char *(*)(char *, char *, int))0,
	(char *(*)(char *, char *, int))0,
	(int (*)(char *, char *, int))0,
	(int (*)(void))DEC_PROM_GETCHAR,
	(char *(*)(char *))DEC_PROM_GETS,
	(int (*)(char *))DEC_PROM_PUTS,
	(int (*)(const char *, ...))DEC_PROM_PRINTF,
	(int (*)(char *, char *, ...))0,
	(int (*)(void))0,
	(long (*)(char *, char **, int))0,
	(psig_t (*)(int, psig_t))0,
	(int (*)(int))0,
	(long (*)(long *))0,
	(int (*)(jmp_buf))0,
	(void (*)(jmp_buf, int))0,
	(int (*)( char *))0,
	(int (*)(int, void *, int))0,
	(int (*)(int, void *, int))0,
	(int (*)(char *, char *))DEC_PROM_SETENV2,
	(char *(*)(const char *))DEC_PROM_GETENV2,
	(int (*)(char *))DEC_PROM_UNSETENV,
	(u_long (*)(int))0,
	(void (*)(void))0,
	(void (*)(int))0,
	(void (*)(int))0,
	(void (*)(char *, int))DEC_PROM_CLEARCACHE,
	(int (*)(void))0,
	(int (*)(memmap *))0,
	(int (*)(int))0,
	(int (*)(int))0,
	(int (*)(int))0,
	(void *)0,
	(int (*)(void))0,
	(void (*)(int *, int))0,
	(void (*)(void))0,
	(tcinfo *(*)(void))0,	/* XXX what are the args for this?*/
	(int (*)(char *))0,
	(void (*)(char))0,
};

const   struct callback *callv = &callvec;
