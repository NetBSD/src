/*	$NetBSD: intr.h,v 1.5.2.5 2001/04/21 17:54:08 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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

#ifndef _MVME68K_INTR_H
#define _MVME68K_INTR_H

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/psl.h>

/*
 * These are identical to the values used by hp300, but are not meaningful
 * to mvme68k code at this time.
 */
#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_BIO		1	/* disable block I/O interrupts */
#define	IPL_NET		2	/* disable network interrupts */
#define	IPL_TTY		3	/* disable terminal interrupts */
#define	IPL_TTYNOBUF	4	/* IPL_TTY + higher ISR priority */
#define	IPL_SERIAL	4	/* disable serial interrupts */
#define	IPL_CLOCK	5	/* disable clock interrupts */
#define	IPL_HIGH	6	/* disable all interrupts */

/* Copied from alpha/include/intr.h */
#define	IPL_SOFTSERIAL	0	/* serial software interrupts */
#define	IPL_SOFTNET	1	/* network software interrupts */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts */
#define	IPL_SOFT	3	/* other software interrupts */
#define	IPL_NSOFT	4

#define	IPL_SOFTNAMES {							\
	"serial",							\
	"net",								\
	"clock",							\
	"misc",								\
}

#ifdef _KERNEL
/* spl0 requires checking for software interrupts */

#define spllowersoftclock()	spl1()
#define splsoft()		splraise1()
#define splsoftclock()		splsoft()
#define splsoftnet()		splsoft()
#define splsoftserial()		splsoft()
#define splbio()		splraise2()
#define splnet()		splraise3()
#define spltty()		splraise3()
#define splvm()			splraise3()
#define splserial()		splraise4()
#define splclock()		splraise5()
#define splstatclock()		splraise5()
#define splhigh()		spl7()
#define splsched()		spl7()
#define spllock()		spl7()

#ifndef _LOCORE

/*
 * Simulated software interrupt register
 * This is cleared to zero to indicate a soft interrupt is pending
 * (Yes, it's a bit bizarre, but it allows the use of the m68k's `tas'
 * instruction so we can avoid masking interrupts elsewhere.)
 */
extern volatile unsigned char ssir;

extern void mvme68k_dossir(void);

static __inline void
splx(int sr)
{

	if ((u_int16_t)sr < (u_int16_t)(PSL_IPL1|PSL_S) && ssir == 0)
		mvme68k_dossir();
	else
		__asm __volatile("movw %0,%%sr" : : "di" (sr));
}

static __inline int
spl0(void)
{
	int sr;

	__asm __volatile("movw %%sr,%0" : "=d" (sr));

	if (ssir == 0)
		mvme68k_dossir();
	else
		__asm __volatile("movw %0,%%sr" : : "i" (PSL_LOWIPL));

	return sr;
}


#define setsoft(x)		x = 0

struct mvme68k_soft_intrhand {
	LIST_ENTRY(mvme68k_soft_intrhand) sih_q;
	struct mvme68k_soft_intr *sih_intrhead;
	void (*sih_fn)(void *);
	void *sih_arg;
	volatile int sih_pending;
};

struct mvme68k_soft_intr {
	LIST_HEAD(, mvme68k_soft_intrhand) msi_q;
	struct evcnt msi_evcnt;
	volatile unsigned char msi_ssir;
};

void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_init(void);
void	softintr_dispatch(void);

#define softintr_schedule(arg)						\
		do {							\
			struct mvme68k_soft_intrhand *__sih = (arg);	\
			__sih->sih_pending = 1;				\
			setsoft(__sih->sih_intrhead->msi_ssir);		\
			setsoft(ssir);					\
		} while (0)

/* XXX For legacy software interrupts */
extern struct mvme68k_soft_intrhand *softnet_intrhand;

#define setsoftnet()	softintr_schedule(softnet_intrhand)

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* _MVME68K_INTR_H */
