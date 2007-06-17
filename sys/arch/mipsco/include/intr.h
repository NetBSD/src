/*	$NetBSD: intr.h,v 1.14 2007/06/17 06:04:28 tsutsui Exp $	*/

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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_SOFT	1	/* generic software interrupts */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts */
#define	IPL_SOFTNET	3	/* network software interrupts */
#define	IPL_SOFTSERIAL	4	/* serial software interrupts */
#define	IPL_BIO		5	/* disable block I/O interrupts */
#define	IPL_NET		6	/* disable network interrupts */
#define	IPL_TTY		7	/* disable terminal interrupts */
#define	IPL_SERIAL	IPL_TTY
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		IPL_TTY
#define	IPL_CLOCK	8	/* disable clock interrupts */
#define	IPL_STATCLOCK	9	/* disable profiling interrupts */
#define	IPL_SCHED	IPL_CLOCK
#define	IPL_HIGH	10	/* disable all interrupts */
#define	IPL_LOCK	IPL_HIGH

#define	IPL_N		11

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

#define	SI_SOFT		0
#define	SI_SOFTCLOCK	1
#define	SI_SOFTNET	2
#define	SI_SOFTSERIAL	3

#define	SI_NQUEUES	4

#define	SI_QUEUENAMES {							\
	"misc",								\
	"clock",							\
	"net",								\
	"serial",							\
}

#ifdef _KERNEL
#ifndef _LOCORE
#include <sys/types.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <mips/locore.h>

/*
 * software simulated interrupt
 */
#define setsoft(x)	do {			\
	extern u_int ssir;			\
	int _s;					\
						\
	_s = splhigh();				\
	ssir |= 1 << (x);			\
	_setsoftintr(MIPS_SOFT_INT_MASK_1);	\
	splx(_s);				\
} while (0)

#define softintr_schedule(arg)						\
do {									\
	struct mipsco_intrhand *__ih = (arg);				\
	__ih->ih_pending = 1;						\
	setsoft(__ih->ih_intrhead->intr_siq);				\
} while (0)

extern struct mipsco_intrhand *softnet_intrhand;

#define	setsoftnet()	softintr_schedule(softnet_intrhand)

/*
 * nesting interrupt masks.
 */
#define MIPS_INT_MASK_SPL_SOFT0	MIPS_SOFT_INT_MASK_0
#define MIPS_INT_MASK_SPL_SOFT1	(MIPS_SOFT_INT_MASK_1|MIPS_INT_MASK_SPL_SOFT0)
#define MIPS_INT_MASK_SPL0	(MIPS_INT_MASK_0|MIPS_INT_MASK_SPL_SOFT1)
#define MIPS_INT_MASK_SPL1	(MIPS_INT_MASK_1|MIPS_INT_MASK_SPL0)
#define MIPS_INT_MASK_SPL2	(MIPS_INT_MASK_2|MIPS_INT_MASK_SPL1)
#define MIPS_INT_MASK_SPL3	(MIPS_INT_MASK_3|MIPS_INT_MASK_SPL2)
#define MIPS_INT_MASK_SPL4	(MIPS_INT_MASK_4|MIPS_INT_MASK_SPL3)
#define MIPS_INT_MASK_SPL5	(MIPS_INT_MASK_5|MIPS_INT_MASK_SPL4)

#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splbio()	_splraise(MIPS_INT_MASK_SPL1)
#define splnet()	_splraise(MIPS_INT_MASK_SPL0)
#define spltty()	_splraise(MIPS_INT_MASK_SPL0)
#define splvm()		_splraise(MIPS_INT_MASK_SPL2)
#define splclock()	_splraise(MIPS_INT_MASK_SPL2)
#define splstatclock()	_splraise(MIPS_INT_MASK_SPL2)
#define splhigh()	_splraise(MIPS_INT_MASK_SPL2)
#define	splsched()	splhigh()
#define	spllock()	splhigh()
#define splserial()	spltty()
#define spllpt()	spltty()

#define splsoft()	_splraise(MIPS_INT_MASK_SPL_SOFT1)
#define splsoftclock()	_splraise(MIPS_INT_MASK_SPL_SOFT0)
#define	splsoftnet()	splsoft()
#define	splsoftserial()	splsoft()

typedef int ipl_t;
typedef struct {
	int _sr;
} ipl_cookie_t;

ipl_cookie_t makeiplcookie(ipl_t ipl);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._sr);
}

struct mipsco_intrhand {
	LIST_ENTRY(mipsco_intrhand)
		ih_q;
	int	(*ih_fun) __P((void *));
	void	 *ih_arg;
	struct	mipsco_intr *ih_intrhead;
	int	ih_pending;
};

struct mipsco_intr {
	LIST_HEAD(,mipsco_intrhand)
		intr_q;
	struct	evcnt ih_evcnt;
	unsigned long intr_siq;
};


extern struct mipsco_intrhand intrtab[];

#define SYS_INTR_LEVEL0	0
#define SYS_INTR_LEVEL1	1
#define SYS_INTR_LEVEL2	2
#define SYS_INTR_LEVEL3	3
#define SYS_INTR_LEVEL4	4
#define SYS_INTR_LEVEL5	5
#define SYS_INTR_SCSI	6
#define SYS_INTR_TIMER	7
#define SYS_INTR_ETHER	8
#define SYS_INTR_SCC0	9
#define SYS_INTR_FDC	10
#define SYS_INTR_ATBUS	11

#define MAX_INTR_COOKIES 16

#define	CALL_INTR(lev)	((*intrtab[lev].ih_fun)(intrtab[lev].ih_arg))

void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_init(void);
void	softintr_dispatch(void);

#endif /* !_LOCORE */
#endif /* _KERNEL */
#endif /* _MACHINE_INTR_H_ */
