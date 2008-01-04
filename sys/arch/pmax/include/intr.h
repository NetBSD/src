/*	$NetBSD: intr.h,v 1.32 2008/01/04 21:54:05 ad Exp $	*/

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

#ifndef _PMAX_INTR_H_
#define _PMAX_INTR_H_

#include <sys/evcnt.h>
#include <sys/queue.h>

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_SOFTCLOCK	1	/* clock software interrupts (SI 0) */
#define	IPL_SOFTBIO	1	/* generic software interrupts (SI 0) */
#define	IPL_SOFTNET	2	/* network software interrupts (SI 1) */
#define	IPL_SOFTSERIAL	2	/* serial software interrupts (SI 1) */
#define	IPL_VM		3
#define	IPL_SCHED	4
#define	IPL_HIGH	5

#define	_IPL_N		6

#define	_IPL_SI0_FIRST	IPL_SOFTCLOCK
#define	_IPL_SI0_LAST	IPL_SOFTBIO

#define	_IPL_SI1_FIRST	IPL_SOFTNET
#define	_IPL_SI1_LAST	IPL_SOFTSERIAL

#ifdef _KERNEL
#ifndef _LOCORE

#include <mips/cpuregs.h>
#include <mips/locore.h>

#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splvm()		splraiseipl(makeiplcookie(IPL_VM))
#define splsched()	splraiseipl(makeiplcookie(IPL_SCHED))
#define splhigh()	_splraise(MIPS_INT_MASK)

#define	_SPL_SOFT	MIPS_SOFT_INT_MASK_0
#define	_SPL_SOFTCLOCK	MIPS_SOFT_INT_MASK_0
#define	_SPL_SOFTNET	(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define	_SPL_SOFTSERIAL	(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)

#define splsoftclock()	_splraise(_SPL_SOFTCLOCK)
#define splsoftbio()	_splraise(_SPL_SOFTBIO)
#define splsoftnet()	_splraise(_SPL_SOFTNET)
#define splsoftserial()	_splraise(_SPL_SOFTSERIAL)

extern const int *ipl2spl_table;

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

/* Conventionals ... */

#define MIPS_SPLHIGH (MIPS_INT_MASK)
#define MIPS_SPL0 (MIPS_INT_MASK_0|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define MIPS_SPL1 (MIPS_INT_MASK_1|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define MIPS_SPL3 (MIPS_INT_MASK_3|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define MIPS_SPL_0_1	 (MIPS_INT_MASK_1|MIPS_SPL0)
#define MIPS_SPL_0_1_2	 (MIPS_INT_MASK_2|MIPS_SPL_0_1)
#define MIPS_SPL_0_1_3	 (MIPS_INT_MASK_3|MIPS_SPL_0_1)
#define MIPS_SPL_0_1_2_3 (MIPS_INT_MASK_3|MIPS_SPL_0_1_2)

struct intrhand {
	int	(*ih_func) __P((void *));
	void	*ih_arg;
	struct evcnt ih_count;
};
extern struct intrhand intrtab[];

#define SYS_DEV_SCC0	0
#define SYS_DEV_SCC1	1
#define SYS_DEV_LANCE	2
#define SYS_DEV_SCSI	3
#define SYS_DEV_OPT0	4
#define SYS_DEV_OPT1	5
#define SYS_DEV_OPT2	6
#define SYS_DEV_DTOP	7
#define SYS_DEV_ISDN	8
#define SYS_DEV_FDC	9
#define SYS_DEV_BOGUS	-1
#define MAX_DEV_NCOOKIES 10

struct pmax_intrhand {
	LIST_ENTRY(pmax_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
};

#include <mips/softintr.h>

extern struct evcnt pmax_clock_evcnt;
extern struct evcnt pmax_fpu_evcnt;
extern struct evcnt pmax_memerr_evcnt;

void intr_init(void);
#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif	/* !_PMAX_INTR_H_ */
