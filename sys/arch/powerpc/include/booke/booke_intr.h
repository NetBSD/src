/*	$NetBSD: booke_intr.h,v 1.1.2.2 2010/03/11 15:02:50 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#ifndef _BOOKE_INTR_H_
#define _BOOKE_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFTCLOCK	1	/* software clock interrupt */
#define	IPL_SOFTBIO	2	/* software block i/o interrupt */
#define	IPL_SOFTNET	3	/* software network interrupt */
#define	IPL_SOFTSERIAL	4	/* software serial interrupt */
#define	IPL_VM		5	/* memory allocation */
#define	IPL_SCHED	6	/* clock */
#define	IPL_HIGH	7	/* everything */
#define	NIPL		8

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifndef _LOCORE

#define	CLKF_BASEPRI(frame)	((frame)->cf_ipl == IPL_NONE)

void 	*intr_establish(int, int, int, int (*)(void *), void *);
void 	intr_disestablish(void *);
void 	intr_init(void);
void 	ext_intr(void); 			/* for machdep */
int 	splraise(int);
int 	spllower(int);
void 	splx(int);
void 	softintr(int);

extern volatile u_int 		imask[NIPL];
extern const int 		mask_clock; 		/* for clock.c */
extern const int 		mask_statclock; 	/* for clock.c */

#define	spllowersoftclock() 	spllower(imask[IPL_SOFTCLOCK])

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

#define	spl0()			spllower(0)

#endif /* !_LOCORE */
#endif /* !_BOOKE_INTR_H_ */
