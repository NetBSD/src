/*	$NetBSD: intr.h,v 1.25.32.1 2009/05/13 17:18:18 jym Exp $	*/

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
#define	IPL_SOFTCLOCK	1	/* generic software interrupts */
#define	IPL_SOFTBIO	1	/* serial software interrupts */
#define	IPL_SOFTNET	1	/* network software interrupts */
#define	IPL_SOFTSERIAL	1	/* clock software interrupts */
#define	IPL_VM		2
#define	IPL_SCHED	3
#define	IPL_HIGH	4

#define NIPL		5

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

#ifdef _KERNEL
#ifndef _LOCORE

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/evcnt.h>
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

#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splvm()		_splraise(ipl2spl_table[IPL_VM])
#define splsched()	_splraise(ipl2spl_table[IPL_SCHED])
#define splhigh()	_splraise(MIPS_INT_MASK)

#define splsoftclock()	_splraise(MIPS_SOFT_INT_MASK_1)
#define splsoftbio()	splsoftclock()
#define splsoftnet()	splsoftclock()
#define splsoftserial()	splsoftclock()

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
