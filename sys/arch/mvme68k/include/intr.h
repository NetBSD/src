/*	$NetBSD: intr.h,v 1.15.6.1 2007/03/13 16:50:02 ad Exp $	*/

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

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_SOFT	1	/* other software interrupts */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts */
#define	IPL_SOFTNET	3	/* network software interrupts */
#define	IPL_SOFTSERIAL	4	/* serial software interrupts */
#define	IPL_BIO		5	/* disable block I/O interrupts */
#define	IPL_NET		6	/* disable network interrupts */
#define	IPL_TTY		7	/* disable terminal interrupts */
#define	IPL_LPT		IPL_TTY
#define	IPL_TTYNOBUF	8	/* IPL_TTY + higher ISR priority */
#define	IPL_VM		9
#define	IPL_SERIAL	10	/* disable serial interrupts */
#define	IPL_CLOCK	11	/* disable clock interrupts */
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_HIGH	12	/* disable all interrupts */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH

#define	SI_SOFTSERIAL	0
#define	SI_SOFTNET	1
#define	SI_SOFTCLOCK	2
#define	SI_SOFT		3

#define	SI_NQUEUES	4

#define	SI_QUEUENAMES {							\
	"serial",							\
	"net",								\
	"clock",							\
	"misc",								\
}

#ifdef _KERNEL
#define spl0()			_spl0()
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

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

ipl_cookie_t makeiplcookie(ipl_t);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

static __inline void
splx(int sr)
{

	__asm volatile("movw %0,%%sr" : : "di" (sr));
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
extern void (*_softintr_chipset_assert)(void);

#define softintr_schedule(arg)						\
		do {							\
			struct mvme68k_soft_intrhand *__sih = (arg);	\
			__sih->sih_pending = 1;				\
			setsoft(__sih->sih_intrhead->msi_ssir);		\
			_softintr_chipset_assert();			\
		} while (0)

/* XXX For legacy software interrupts */
extern struct mvme68k_soft_intrhand *softnet_intrhand;

#define setsoftnet()	softintr_schedule(softnet_intrhand)

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* _MVME68K_INTR_H */
