/*	$NetBSD: intr.h,v 1.15.4.1 2010/05/30 05:16:49 rmind Exp $	*/
/*	$OpenBSD: intr.h,v 1.26 2009/12/29 13:11:40 jsing Exp $	*/

/*-
 * Copyright (c) 1998, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe, and by Matthew Fredette.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HP700_INTR_H_
#define _HP700_INTR_H_

#include <machine/psl.h>

/* Interrupt priority `levels'. */
#define	IPL_NONE	7	/* nothing */
#define	IPL_SOFTCLOCK	6	/* timeouts */
#define	IPL_SOFTBIO	5	/* block I/o */
#define	IPL_SOFTNET	4	/* protocol stacks */
#define	IPL_SOFTSERIAL	3	/* serial */
#define	IPL_VM		2	/* memory allocation, low I/O */
#define	IPL_SCHED	1	/* clock, medium I/O */
#define	IPL_HIGH	0	/* everything */
#define	NIPL		8

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifndef _LOCORE

/* The priority level masks. */
extern int imask[NIPL];

/* splraise()/spllower() are in locore.S */
int splraise(int);
void spllower(int);

/*
 * Miscellaneous
 */
#define	spl0()		spllower(0)
#define	splx(x)		spllower(x)

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(imask[icookie._ipl]);
}

#include <sys/spl.h>

#define	setsoftast(l)	((l)->l_md.md_astpending = 1)

#endif /* !_LOCORE */

#endif /* !_HP700_INTR_H_ */
