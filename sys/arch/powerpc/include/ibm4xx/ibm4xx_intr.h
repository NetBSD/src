/*	$NetBSD: ibm4xx_intr.h,v 1.13 2006/09/27 09:11:47 freza Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#include <powerpc/softintr.h>

/* Interrupt priority `levels'. */
#define	IPL_NONE	12	/* nothing */
#define	IPL_SOFTCLOCK	11	/* software clock interrupt */
#define	IPL_SOFTNET	10	/* software network interrupt */
#define	IPL_BIO		9	/* block I/O */
#define	IPL_NET		8	/* network */
#define	IPL_SOFTSERIAL	7	/* software serial interrupt */
#define	IPL_TTY		6	/* terminal */
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		5	/* memory allocation */
#define	IPL_AUDIO	4	/* audio */
#define	IPL_CLOCK	3	/* clock */
#define	IPL_STATCLOCK	2
#define	IPL_HIGH	1	/* everything */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define	IPL_SERIAL	0	/* serial */
#define	NIPL		13

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
#define	splraiseipl(x)		splraise(imask[x])

#include <sys/spl.h>

/* Note the constants here are indices to softintr()'s private table. */
#define	setsoftclock()		softintr(0);
#define	setsoftnet()		softintr(1);
#define	setsoftserial()		softintr(2);

#define	spl0()			spllower(0)

#endif /* !_LOCORE */
#endif /* !_IBM4XX_INTR_H_ */
