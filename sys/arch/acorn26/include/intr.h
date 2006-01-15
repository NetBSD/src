/* $NetBSD: intr.h,v 1.5.2.1 2006/01/15 10:02:36 yamt Exp $ */
/*-
 * Copyright (c) 1998, 2000 Ben Harris
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * intr.h - Interrupt stuff for the consumption of MI code.
 *
 * arm26-specific functions are in irq.h
 */

#ifndef _ARM26_INTR_H_
#define _ARM26_INTR_H_

/*
 * These are the different SPL states
 *
 * Each state has an interrupt mask associated with it which
 * indicate which interrupts are allowed.
 */

#define IPL_NONE	0
#define IPL_SOFTCLOCK	1
#define IPL_SOFTNET	2
#define IPL_SOFTSERIAL	IPL_SOFTNET
#define IPL_BIO		3
#define IPL_NET		4
#define IPL_TTY		5
#define IPL_LPT		IPL_TTY
#define IPL_VM		6
#define IPL_AUDIO	7
#define IPL_SERIAL	8
#define IPL_CLOCK	9
#define IPL_STATCLOCK	10
#define IPL_SCHED	11
#define IPL_HIGH	12
#define	IPL_LOCK	IPL_HIGH
#define NIPL		IPL_HIGH + 1

#if defined(_KERNEL) && !defined(_LOCORE)

#define splsoftnet()	raisespl(IPL_SOFTNET)
#define splsoft()	splsoftnet()
#define splsoftserial()	splsoftnet()
#define splsoftclock()	raisespl(IPL_SOFTCLOCK)
#define splbio()	raisespl(IPL_BIO)
#define splnet()	raisespl(IPL_NET)
#define spltty()	raisespl(IPL_TTY)
#define spllpt()	raisespl(IPL_LPT)
#define splvm()		raisespl(IPL_VM)
#define	splaudio()	raisespl(IPL_AUDIO)
#define splserial()	raisespl(IPL_SERIAL)
#define splclock()	raisespl(IPL_CLOCK)
#define splstatclock()	raisespl(IPL_STATCLOCK)
#define splsched()	raisespl(IPL_SCHED)

/* #define	splsched()	splhigh() */
#define spllock()	splhigh()

#define	splraiseipl(x)	(((x) == IPL_HIGH) ? splhigh() : raisespl(x))

#define spl0()			lowerspl(IPL_NONE)
#define spllowersoftclock()	lowerspl(IPL_SOFTCLOCK)
#define splx(s)			lowerspl(s)

extern int splhigh(void);
extern int raisespl(int);
extern void lowerspl(int);
extern int hardsplx(int);

/*
 * Interrupt sharing types
 * (not currently used on arm26)
 */

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/*
 * Soft Interrupts
 */

/* Old-fashioned soft interrupts */
extern void setsoftnet(void);

/* New-fangled generic soft interrupts */
extern void *softintr_establish(int, void (*)(void *), void *);
extern void softintr_disestablish(void *);
extern void softintr_schedule(void *);

/* Machine-dependent soft-interrupt servicing routines */
extern void softintr_init(void);
extern void dosoftints(int);

#endif /* _KERNEL && !ASSEMBLER */
#endif
