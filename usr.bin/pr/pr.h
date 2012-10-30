/*	$NetBSD: pr.h,v 1.4.56.1 2012/10/30 19:00:26 yamt Exp $	*/

/*-
 * Copyright (c) 1991 Keith Muller.
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2012
 *	The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
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
 *      from: @(#)pr.h	8.1 (Berkeley) 6/6/93
 *	$NetBSD: pr.h,v 1.4.56.1 2012/10/30 19:00:26 yamt Exp $
 */

/*
 * parameter defaults
 */
#define	CLCNT		1
#define	INCHAR		'\t'
#define	INGAP		8
#define	OCHAR		'\t'
#define OGAP		8
#define	LINES		66
#define	NMWD		5
#define	NMCHAR		'\t'
#define	SCHAR		'\t'
#define	PGWD		72
#define SPGWD		512

/*
 * misc default values
 */
#define	HDFMT		"%s %s Page %d\n\n\n"
#define	HEADLEN		5
#define	TAILLEN		5
#define	TIMEFMT		"%b %e %H:%M %Y"
#define	FNAME		""
#define	LBUF		8192
#define	HDBUF		512

/* when to pause before (for -f and -p options) */
#define NO_PAUSE	0
#define FIRSTPAGE	1
#define ENSUINGPAGES	2
#define EACHPAGE	(FIRSTPAGE | ENSUINGPAGES)

/*
 * structure for vertical columns. Used to balance cols on last page
 */
struct vcol {
	char *pt;		/* ptr to col */
	int cnt;		/* char count */
};
