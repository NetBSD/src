/*	$NetBSD: options.h,v 1.28 2024/07/12 07:30:30 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
 *	@(#)options.h	8.2 (Berkeley) 5/4/95
 */

struct shparam {
	int nparam;		/* # of positional parameters (without $0) */
	unsigned char malloc;	/* if parameter list dynamically allocated */
	unsigned char reset;	/* if getopts has been reset */
	char **p;		/* parameter list */
	char **optnext;		/* next parameter to be processed by getopts */
	char *optptr;		/* used by getopts */
};

/*
 * Note that option default values can be changed at shell startup
 * depending upon the environment in which the shell is running.
 */
struct optent {
	const char *name;		/* for set -o <name> */
	const char letter;		/* set [+/-]<letter> and $- */
	const char opt_set;		/* mutually exclusive option set */
	unsigned char val;		/* value of <letter>flag */
	unsigned char dflt;		/* default value of flag */
};

#include "optinit.h"

extern char *minusc;		/* argument to -c option */
extern char *arg0;		/* $0 */
extern struct shparam shellparam;  /* $@ */
extern char **argptr;		/* argument list for builtin commands */
extern char *optionarg;		/* set by nextopt */
extern char *optptr;		/* used by nextopt */

void procargs(int, char **);
void optschanged(void);
void setparam(char **);
void freeparam(volatile struct shparam *);
int nextopt(const char *);
void getoptsreset(char *, int);
