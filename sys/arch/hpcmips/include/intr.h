/*	$NetBSD: intr.h,v 1.13 2001/09/15 19:51:39 uch Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
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

#ifndef _HPCMIPS_INTR_H_
#define _HPCMIPS_INTR_H_

#include <sys/device.h>
#include <sys/lock.h>
#include <sys/queue.h>

#define	IPL_NONE	0	/* disable only this interrupt */

#define	IPL_SOFT	1	/* generic software interrupts (SI 0) */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts (SI 0) */
#define	IPL_SOFTNET	3	/* network software interrupts (SI 1) */
#define	IPL_SOFTSERIAL	4	/* serial software interrupts (SI 1) */

#define	IPL_BIO		5	/* disable block I/O interrupts */
#define	IPL_NET		6	/* disable network interrupts */
#define	IPL_TTY		7	/* disable terminal interrupts */
#define	IPL_SERIAL	7	/* disable serial hardware interrupts */
#define	IPL_CLOCK	8	/* disable clock interrupts */
#define	IPL_STATCLOCK	8	/* disable profiling interrupts */
#define	IPL_HIGH	8	/* disable all interrupts */

#define	_IPL_NSOFT	4
#define	_IPL_N		9

#define	_IPL_SI0_FIRST	IPL_SOFT
#define	_IPL_SI0_LAST	IPL_SOFTCLOCK

#define	_IPL_SI1_FIRST	IPL_SOFTNET
#define	_IPL_SI1_LAST	IPL_SOFTSERIAL

#define	IPL_SOFTNAMES {							\
	"misc",								\
	"clock",							\
	"net",								\
	"serial",							\
}

/* Interrupt sharing types. */
#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifdef _KERNEL
#ifndef _LOCORE
#include <mips/cpuregs.h>

extern const u_int32_t *ipl_sr_bits;
extern const u_int32_t ipl_si_to_sr[_IPL_NSOFT];

void	intr_init(void);
int	_splraise(int);
int	_spllower(int);
int	_splset(int);
int	_splget(void);
void	_splnone(void);
void	_setsoftintr(int);
void	_clrsoftintr(int);

#define	splhigh()	_splraise(ipl_sr_bits[IPL_HIGH])
#define	spl0()		(void) _spllower(0)
#define	splx(s)		(void) _splset(s)
#define	splbio()	_splraise(ipl_sr_bits[IPL_BIO])
#define	splnet()	_splraise(ipl_sr_bits[IPL_NET])
#define	spltty()	_splraise(ipl_sr_bits[IPL_TTY])
#define	splserial()	_splraise(ipl_sr_bits[IPL_SERIAL])
#define	splvm()		spltty()
#define	splclock()	_splraise(ipl_sr_bits[IPL_CLOCK])
#define	splstatclock()	splclock()

#define	splsched()	splclock()
#define	spllock()	splhigh()
#define	spllpt()	spltty()

#define	splsoft()	_splraise(ipl_sr_bits[IPL_SOFT])
#define	splsoftclock()	_splraise(ipl_sr_bits[IPL_SOFTCLOCK])
#define	splsoftnet()	_splraise(ipl_sr_bits[IPL_SOFTNET])
#define	splsoftserial()	_splraise(ipl_sr_bits[IPL_SOFTSERIAL])

#define	spllowersoftclock() _spllower(ipl_sr_bits[IPL_SOFTCLOCK])

/*
 * Index into intrcnt[], which is defined in locore
 */
extern u_long intrcnt[];

#define	SOFTCLOCK_INTR	0
#define	SOFTNET_INTR	1
#define	HARDCLOCK	8
#define	STRAY_INTR	16

/*
 * software simulated interrupt
 */
#define	setsoft(x)							\
do {									\
	_setsoftintr(ipl_si_to_sr[(x) - IPL_SOFT]);			\
} while (0)

struct hpcmips_soft_intrhand {
	TAILQ_ENTRY(hpcmips_soft_intrhand)
		sih_q;
	struct hpcmips_soft_intr *sih_intrhead;
	void	(*sih_fn)(void *);
	void	*sih_arg;
	int	sih_pending;
};

struct hpcmips_soft_intr {
	TAILQ_HEAD(, hpcmips_soft_intrhand)
		softintr_q;
	struct evcnt softintr_evcnt;
	struct simplelock softintr_slock;
	unsigned long softintr_ipl;
};

void	softintr_init(void);
void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_dispatch(void);

#define	softintr_schedule(arg)						\
do {									\
	struct hpcmips_soft_intrhand *__sih = (arg);			\
	struct hpcmips_soft_intr *__si = __sih->sih_intrhead;		\
	int __s;							\
									\
	__s = splhigh();						\
	simple_lock(&__si->softintr_slock);				\
	if (__sih->sih_pending == 0) {					\
		TAILQ_INSERT_TAIL(&__si->softintr_q, __sih, sih_q);	\
		__sih->sih_pending = 1;					\
		setsoft(__si->softintr_ipl);				\
	}								\
	simple_unlock(&__si->softintr_slock);				\
	splx(__s);							\
} while (0)

/* XXX For legacy software interrupts. */
extern struct hpcmips_soft_intrhand *softnet_intrhand;

#define	setsoftnet()	softintr_schedule(softnet_intrhand)

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* !_HPCMIPS_INTR_H_ */
