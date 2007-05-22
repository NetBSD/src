/*	$NetBSD: intr.h,v 1.8.14.1 2007/05/22 17:26:52 matt Exp $	*/

/*-
 * Copyright (c) 1998, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe, and by Matthew Fredette.
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

#ifndef _HP700_INTR_H_
#define _HP700_INTR_H_

#include <machine/psl.h>

/* Interrupt priority `levels'. */
#define	IPL_NONE	9	/* nothing */
#define	IPL_SOFTCLOCK	8	/* timeouts */
#define	IPL_SOFTNET	7	/* protocol stacks */
#define	IPL_BIO		6	/* block I/O */
#define	IPL_NET		5	/* network */
#define	IPL_SOFTSERIAL	4	/* serial */
#define	IPL_TTY		3	/* terminal */
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		3	/* memory allocation */
#define	IPL_AUDIO	2	/* audio */
#define	IPL_CLOCK	1	/* clock */
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_HIGH	1	/* everything */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define	IPL_SERIAL	0	/* serial */
#define	NIPL		10

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifndef _LOCORE

/* The priority level masks. */
extern int imask[NIPL];

/* The current priority level. */
extern volatile int cpl;

/* The asynchronous system trap flag. */
extern volatile int astpending;

/* The softnet mask. */
extern int softnetmask;

/* 
 * Add a mask to cpl, and return the old value of cpl.
 */
static __inline int  
splraise(register int ncpl)
{
	register int ocpl = cpl;

	cpl = ocpl | ncpl;      

	return (ocpl);  
}

/* spllower() is in locore.S */
void spllower(int);
 
/*
 * Miscellaneous
 */
#define	spl0()		spllower(0)
#define	splx(x)		spllower(x)

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

#define	setsoftast()	(astpending = 1)
#define	setsoftnet()	hp700_intr_schedule(softnetmask)

#endif /* !_LOCORE */

/*
 * Generic software interrupt support.
 */

#define	HP700_SOFTINTR_SOFTCLOCK	0
#define	HP700_SOFTINTR_SOFTNET		1
#define	HP700_SOFTINTR_SOFTSERIAL	2
#define	HP700_NSOFTINTR			3

#ifndef _LOCORE
#include <sys/queue.h>
#include <sys/device.h>

struct hp700_soft_intrhand {
	TAILQ_ENTRY(hp700_soft_intrhand)
		sih_q;
	struct hp700_soft_intr *sih_intrhead;
	void	(*sih_fn)(void *);
	void	*sih_arg;
	int	sih_pending;
};

struct hp700_soft_intr {
	TAILQ_HEAD(, hp700_soft_intrhand)
		softintr_q;
	int softintr_ssir;
};

#define	hp700_softintr_lock(si, s)					\
do {									\
	(s) = splhigh();						\
} while (/*CONSTCOND*/ 0)

#define	hp700_softintr_unlock(si, s)					\
do {									\
	splx((s));							\
} while (/*CONSTCOND*/ 0)

void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_bootstrap(void);
void	softintr_init(void);
int	softintr_dispatch(void *);

#define	softintr_schedule(arg)						\
do {									\
	struct hp700_soft_intrhand *__sih = (arg);			\
	struct hp700_soft_intr *__si = __sih->sih_intrhead;		\
	int __s;							\
									\
	hp700_softintr_lock(__si, __s);					\
	if (__sih->sih_pending == 0) {					\
		TAILQ_INSERT_TAIL(&__si->softintr_q, __sih, sih_q);	\
		__sih->sih_pending = 1;					\
		hp700_intr_schedule(__si->softintr_ssir);		\
	}								\
	hp700_softintr_unlock(__si, __s);				\
} while (/*CONSTCOND*/ 0)

void	hp700_intr_schedule(int);

#endif /* _LOCORE */

#endif /* !_HP700_INTR_H_ */
