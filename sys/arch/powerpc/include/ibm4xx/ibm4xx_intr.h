/*	$NetBSD: ibm4xx_intr.h,v 1.14.24.1 2008/01/09 01:47:50 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _IBM4XX_INTR_H_
#define _IBM4XX_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE	12	/* nothing */
#define	IPL_SOFTCLOCK	11	/* software clock interrupt */
#define	IPL_SOFTNET	10	/* software network interrupt */
#define	IPL_VM		5	/* memory allocation */
#define	IPL_SCHED	3	/* clock */
#define	IPL_HIGH	1	/* everything */
#define	NIPL		13

#define	IPL_SOFTBIO	IPL_SOFTNET
#define	IPL_SOFTSERIAL	IPL_SOFTNET

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifndef _LOCORE

#define	CLKF_BASEPRI(frame)	((frame)->pri == 0)

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
#endif /* !_IBM4XX_INTR_H_ */
