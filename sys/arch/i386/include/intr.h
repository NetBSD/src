/*	$NetBSD: intr.h,v 1.18 2001/04/13 23:30:00 thorpej Exp $	*/

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

#ifndef _I386_INTR_H_
#define _I386_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE	9	/* nothing */
#define	IPL_SOFTCLOCK	8	/* timeouts */
#define	IPL_SOFTNET	7	/* protocol stacks */
#define	IPL_BIO		6	/* block I/O */
#define	IPL_NET		5	/* network */
#define	IPL_SOFTSERIAL	4	/* serial */
#define	IPL_TTY		3	/* terminal */
#define	IPL_IMP		3	/* memory allocation */
#define	IPL_AUDIO	2	/* audio */
#define	IPL_CLOCK	1	/* clock */
#define	IPL_HIGH	1	/* everything */
#define	IPL_SERIAL	0	/* serial */
#define	NIPL		10

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/* Soft interrupt masks. */
#define	SIR_CLOCK	31
#define	SIR_NET		30
#define	SIR_SERIAL	29

/* Hack for CLKF_INTR(). */
#define	IPL_TAGINTR	28

#ifndef _LOCORE

volatile int cpl, ipending, astpending;
int imask[NIPL];

extern void Xspllower __P((void));

static __inline int splraise __P((int));
static __inline void spllower __P((int));
static __inline void softintr __P((int));

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
static __inline int
splraise(ncpl)
	register int ncpl;
{
	register int ocpl = cpl;

	cpl = ocpl | ncpl;
	return (ocpl);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
static __inline void
spllower(ncpl)
	register int ncpl;
{

	cpl = ncpl;
	if (ipending & ~ncpl)
		Xspllower();
}

/*
 * Hardware interrupt masks
 */
#define	splbio()	splraise(imask[IPL_BIO])
#define	splnet()	splraise(imask[IPL_NET])
#define	spltty()	splraise(imask[IPL_TTY])
#define	splaudio()	splraise(imask[IPL_AUDIO])
#define	splclock()	splraise(imask[IPL_CLOCK])
#define	splstatclock()	splclock()
#define	splserial()	splraise(imask[IPL_SERIAL])

#define spllpt()	spltty()

/*
 * Software interrupt masks
 *
 * NOTE: splsoftclock() is used by hardclock() to lower the priority from
 * clock to softclock before it calls softclock().
 */
#define	spllowersoftclock() spllower(imask[IPL_SOFTCLOCK])
#define	splsoftclock()	splraise(imask[IPL_SOFTCLOCK])
#define	splsoftnet()	splraise(imask[IPL_SOFTNET])
#define	splsoftserial()	splraise(imask[IPL_SOFTSERIAL])

/*
 * Miscellaneous
 */
#define	splvm()		splraise(imask[IPL_IMP])
#define	splhigh()	splraise(imask[IPL_HIGH])
#define	splsched()	splhigh()
#define	spllock()	splhigh()
#define	spl0()		spllower(0)
#define	splx(x)		spllower(x)

/*
 * Software interrupt registration
 *
 * We hand-code this to ensure that it's atomic.
 */
static __inline void
softintr(mask)
	register int mask;
{
	__asm __volatile("orl %1, %0" : "=m"(ipending) : "ir" (1 << mask));
}

#define	setsoftast()	(astpending = 1)
#define	setsoftclock()	softintr(SIR_CLOCK)
#define	setsoftnet()	softintr(SIR_NET)
#define	setsoftserial()	softintr(SIR_SERIAL)

#endif /* !_LOCORE */

#endif /* !_I386_INTR_H_ */
