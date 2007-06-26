/*	$NetBSD: intr.h,v 1.23 2007/06/26 12:55:38 tsutsui Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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

#ifndef	_SGIMIPS_INTR_H_
#define	_SGIMIPS_INTR_H_

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_SOFT	1	/* generic software interrupts */
#define	IPL_SOFTSERIAL	2	/* serial software interrupts */
#define	IPL_SOFTNET	3	/* network software interrupts */
#define	IPL_SOFTCLOCK	4	/* clock software interrupts */
#define	IPL_BIO		5	/* Disable block I/O interrupts. */
#define	IPL_NET		6	/* Disable network interrupts. */
#define	IPL_TTY		7	/* Disable terminal interrupts. */
#define	IPL_SERIAL	IPL_TTY
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		IPL_TTY
#define	IPL_CLOCK	8	/* Disable clock interrupts. */
#define	IPL_STATCLOCK	IPL_CLOCK /* Disable profiling interrupts. */
#define	IPL_HIGH	9	/* Disable all interrupts. */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define NIPL		10

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

/* Soft interrupt numbers */
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

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/device.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>

#define NINTR	32

struct sgimips_intrhand {
	LIST_ENTRY(sgimips_intrhand)
		ih_q;
	int	(*ih_fun) (void *);
	void	 *ih_arg;
	struct	sgimips_intr *ih_intrhead;
	struct	sgimips_intrhand *ih_next;
	int	ih_pending;
};

struct sgimips_intr {
	LIST_HEAD(,sgimips_intrhand)
		intr_q;
	struct	evcnt ih_evcnt;
	unsigned long intr_ipl;
};

extern struct sgimips_intrhand intrtab[];

extern const int *ipl2spl_table;

#define splhigh()	_splraise(MIPS_INT_MASK)
#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splbio()	_splraise(ipl2spl_table[IPL_BIO])
#define splnet()	_splraise(ipl2spl_table[IPL_NET])
#define spltty()	_splraise(ipl2spl_table[IPL_TTY])
#define splvm()		spltty()
#define splclock()	_splraise(ipl2spl_table[IPL_CLOCK])
#define splstatclock()	splclock()

#define	splsched()	splhigh()
#define	spllock()	splhigh()
#define splserial()	spltty()
#define spllpt()	spltty()

#define splsoft()	_splraise(MIPS_SOFT_INT_MASK_1)
#define splsoftclock()	splsoft()
#define splsoftnet()	splsoft()
#define splsoftserial()	splsoft()

extern void *		cpu_intr_establish(int, int, int (*)(void *), void *);

typedef int ipl_t;
typedef struct {
	int _spl;
} ipl_cookie_t;

ipl_cookie_t makeiplcookie(ipl_t);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._spl);
}

#include <mips/softintr.h>

#endif /* _LOCORE */
#endif /* !_KERNEL */

#endif	/* !_SGIMIPS_INTR_H_ */
