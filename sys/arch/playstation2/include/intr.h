/*	$NetBSD: intr.h,v 1.2.2.3 2002/02/28 04:11:20 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, UCHIYAMA Yasushi.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _PLAYSTATION2_INTR_H_
#define _PLAYSTATION2_INTR_H_
#ifdef _KERNEL

#include <sys/device.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* Interrupt sharing types. */
#define	IST_NONE		0	/* none */
#define	IST_PULSE		1	/* pulsed */
#define	IST_EDGE		2	/* edge-triggered */
#define	IST_LEVEL		3	/* level-triggered */

/* Interrupt priority levels */
#define	IPL_NONE		0	/* nothing */

#define IPL_SOFT		1	/* generic */
#define	IPL_SOFTCLOCK		2	/* timeouts */
#define	IPL_SOFTNET		3	/* protocol stacks */
#define IPL_SOFTSERIAL		4	/* serial */

#define	IPL_BIO			5	/* block I/O */
#define	IPL_NET			6	/* network */
#define	IPL_TTY			7	/* terminal */
#define	IPL_SERIAL		7	/* serial */
#define	IPL_CLOCK		8	/* clock */
#define	IPL_HIGH		8	/* everything */

#define	_IPL_NSOFT	4
#define	_IPL_N		9
#define	IPL_SOFTNAMES {							\
	"misc",								\
	"clock",							\
	"net",								\
	"serial",							\
}

/*
 * Hardware interrupt masks
 */
extern u_int32_t __icu_mask[_IPL_N];

#define	splbio()		splraise(__icu_mask[IPL_BIO])
#define	splnet()		splraise(__icu_mask[IPL_NET])
#define	spltty()		splraise(__icu_mask[IPL_TTY])
#define	splserial()		splraise(__icu_mask[IPL_SERIAL])
#define	splclock()		splraise(__icu_mask[IPL_CLOCK])

#define	splstatclock()		splclock()
#define splvm()			spltty()

/*
 * Software interrupt masks
 */
#define splsoft()		splraise(__icu_mask[IPL_SOFT])
#define	splsoftclock()		splraise(__icu_mask[IPL_SOFTCLOCK])
#define	splsoftnet()		splraise(__icu_mask[IPL_SOFTNET])
#define	splsoftserial()		splraise(__icu_mask[IPL_SOFTSERIAL])

#define	spllowersoftclock()	splset(__icu_mask[IPL_SOFTCLOCK])
void	spllowersofthigh(void);

/*
 * Miscellaneous
 */
#define	splx(s)			splset(s)
#define	splhigh()		splraise(__icu_mask[IPL_HIGH])
#define	splsched()		splhigh()
#define	spllock()		splhigh()

/*
 * software simulated interrupt
 */
struct playstation2_soft_intrhand {
	TAILQ_ENTRY(playstation2_soft_intrhand) sih_q;
	struct playstation2_soft_intr *sih_intrhead;
	void	(*sih_fn)(void *);
	void	*sih_arg;
	int	sih_pending;
};

struct playstation2_soft_intr {
	TAILQ_HEAD(, playstation2_soft_intrhand) softintr_q;
	struct evcnt softintr_evcnt;
	struct simplelock softintr_slock;
	unsigned long softintr_ipl;
};

#define	softintr_schedule(arg)						\
do {									\
	struct playstation2_soft_intrhand *__sih = (arg);		\
	struct playstation2_soft_intr *__si = __sih->sih_intrhead;	\
	int __s;							\
									\
	__s = _intr_suspend();						\
	simple_lock(&__si->softintr_slock);				\
	if (__sih->sih_pending == 0) {					\
		TAILQ_INSERT_TAIL(&__si->softintr_q, __sih, sih_q);	\
		__sih->sih_pending = 1;					\
		setsoft(__si->softintr_ipl);				\
	}								\
	simple_unlock(&__si->softintr_slock);				\
	_intr_resume(__s);						\
} while (0)

void *softintr_establish(int, void (*)(void *), void *);
void softintr_disestablish(void *);
void setsoft(int);

/* XXX For legacy software interrupts. */
extern struct playstation2_soft_intrhand *softnet_intrhand;

#define	setsoftnet()	softintr_schedule(softnet_intrhand)

extern int splraise(int);
extern void splset(int);
extern void spl0(void);
extern int _splset(int);

/* R5900 EI/DI instruction */
extern int _intr_suspend(void);
extern void _intr_resume(int);

#endif /* _KERNEL */
#endif /* _PLAYSTATION2_INTR_H_ */
