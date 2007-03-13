/*	$NetBSD: intr.h,v 1.17.2.1 2007/03/13 16:50:02 ad Exp $	*/

/*
 *
 * Copyright (c) 1998 NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto and Jason R. Thorpe.
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
 *      This product includes software developed by The NetBSD Foundation
 *	Inc. and its contributers.
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef _NEWS68K_INTR_H_
#define	_NEWS68K_INTR_H_

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/psl.h>
#include <m68k/asm_single.h>

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTNET	2
#define	IPL_SOFTSERIAL	3
#define	IPL_SOFT	4
#define	IPL_BIO		5
#define	IPL_NET		6
#define	IPL_TTY		7
#define	IPL_VM		8
#define	IPL_SERIAL	9
#define	IPL_CLOCK	10
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_HIGH	11
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define	NIPL		12

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

/*
 * news68k can handle software interrupts by its own hardware
 * so has no need to check for any simulated interrupts, etc.
 */
#define	spl0()		_spl0()

#define	splsoft()	splraise2()
#define	splsoftclock()	splsoft()
#define	splsoftnet()	splsoft()
#define	splsoftserial()	splsoft()
#define	splbio()	splraise4()
#define	splnet()	splraise4()
#define	spltty()	splraise5()
#define	splvm()		splraise5()
#define	splserial()	splraise5()
#define	splclock()	splraise6()
#define	splstatclock()	splclock()
#define	splhigh()	spl7()
#define	splsched()	spl7()
#define	spllock()	spl7()

/*
 * simulated software interrupt register
 */
#define SOFTINTR_IPL	2
extern volatile uint8_t *ctrl_int2;

#define	setsoft(x)		(x = 0)
#define	softintr_assert()	(*ctrl_int2 = 0xff)
#define	softintr_clear()	(*ctrl_int2 = 0)

struct news68k_soft_intrhand {
	LIST_ENTRY(news68k_soft_intrhand) sih_q;
	struct news68k_soft_intr *sih_intrhead;
	void (*sih_fn)(void *);
	void *sih_arg;
	volatile int sih_pending;
};

struct news68k_soft_intr {
	LIST_HEAD(, news68k_soft_intrhand) nsi_q;
	struct evcnt nsi_evcnt;
	volatile unsigned char nsi_ssir;
};

void *softintr_establish(int, void (*)(void *), void *);
void softintr_disestablish(void *);
void softintr_init(void);
void softintr_dispatch(void);

#define	softintr_schedule(arg)					\
	do {							\
		struct news68k_soft_intrhand *__sih = (arg);	\
		__sih->sih_pending = 1;				\
		setsoft(__sih->sih_intrhead->nsi_ssir);		\
		softintr_assert();				\
	} while (/*CONSTCOND*/ 0)

/* XXX For legacy software interrupts */
extern struct news68k_soft_intrhand *softnet_intrhand;

#define	setsoftnet()	softintr_schedule(softnet_intrhand)

#endif /* _NEWS68K_INTR_H_ */
