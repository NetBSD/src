/*	$NetBSD: intr.h,v 1.9 2002/02/11 10:57:58 wiz Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

/*
 * machine/intr.h for the Amiga port.
 * Currently, only a wrapper, for most of the stuff, around the old
 * include files.
 */

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#include <amiga/amiga/isr.h>
#include <amiga/include/mtpr.h>

/* ADAM: commented out
#define IPL_SOFTSERIAL 1
#define IPL_SOFTNET 1
*/

#ifdef splaudio
#undef splaudio
#define splaudio spl6
#endif

#define spllpt()	spl6()

/* ADAM: from macppc/intr.h */
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

#ifndef _LOCORE

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun) __P((void *));
	void	*ih_arg;
	u_long	ih_count;
	struct	intrhand *ih_next;
	int	ih_level;
	int	ih_irq;
};

void clearsoftclock __P((void));
int  splsoftclock __P((void));
/*
void setsoftnet   __P((void));
*/
void clearsoftnet __P((void));
int  splsoftnet   __P((void));

void do_pending_int __P((void));

static __inline int splraise __P((int));
static __inline int spllower __P((int));
static __inline void splx __P((int));
static __inline void softintr __P((int));

extern volatile int cpl, ipending, astpending, tickspending;
extern int imask[];

/*
 *  Reorder protection in the following inline functions is
 * achieved with the "eieio" instruction which the assembler
 * seems to detect and then doen't move instructions past....
 */
static __inline int
splraise(ncpl)
	int ncpl;
{
	int ocpl;

	__asm__ volatile("sync; eieio\n");	/* don't reorder.... */
	ocpl = cpl;
	cpl = ocpl | ncpl;
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	return (ocpl);
}

static __inline void
splx(ncpl)
	int ncpl;
{
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	cpl = ncpl;
	if (ipending & ~ncpl)
		do_pending_int();
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
}

static __inline int
spllower(ncpl)
	int ncpl;
{
	int ocpl;

	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	ocpl = cpl;
	cpl = ncpl;
	if (ipending & ~ncpl)
		do_pending_int();
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	return (ocpl);
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
static __inline void
softintr(ipl)
	int ipl;
{
	int msrsave;

	__asm__ volatile("mfmsr %0" : "=r"(msrsave));
	__asm__ volatile("mtmsr %0" :: "r"(msrsave & ~PSL_EE));
	ipending |= 1 << ipl;
	__asm__ volatile("mtmsr %0" :: "r"(msrsave));
}

#define	ICU_LEN		32

/* Soft interrupt masks. */
/*
#define SIR_CLOCK	28
#define SIR_NET		29
#define SIR_SERIAL	30
*/
#define SPL_CLOCK	31

/*
 * Hardware interrupt masks
 */
#define splbio()	splraise(imask[IPL_BIO])
#define splnet()	splraise(imask[IPL_NET])
#define spltty()	splraise(imask[IPL_TTY])
#define	splaudio()	splraise(imask[IPL_AUDIO])
#define splclock()	splraise(imask[IPL_CLOCK])
#define splstatclock()	splclock()
#define	splserial()	splraise(imask[IPL_SERIAL])

/* ADAM: see above
#define spllpt()	spltty()
*/

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
#define splvm()		splraise(imask[IPL_IMP])
#define	splhigh()	splraise(imask[IPL_HIGH])
#define	splsched()	splhigh()
#define	spllock()	splhigh()
#define	spl0()		spllower(0)

/*
#define	setsoftnet()	softintr(SIR_NET)
#define	setsoftserial()	softintr(SIR_SERIAL)
*/
extern long intrcnt[];

#define CNT_IRQ0	0
#define CNT_CLOCK	64
#define CNT_SOFTCLOCK	65
#define CNT_SOFTNET	66
#define CNT_SOFTSERIAL	67

#endif /* !_LOCORE */

#endif /* !_MACPPC_INTR_H_ */
