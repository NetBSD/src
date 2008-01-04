/*	$NetBSD: intr.h,v 1.30 2008/01/04 22:55:25 ad Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_COBALT_INTR_H_
#define	_COBALT_INTR_H_

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_SOFTCLOCK	1	/* generic software interrupts */
#define	IPL_SOFTBIO	1	/* clock software interrupts */
#define	IPL_SOFTNET	2	/* network software interrupts */
#define	IPL_SOFTSERIAL	2	/* serial software interrupts */
#define	IPL_VM		3	/* Memory allocation */
#define	IPL_SCHED	4	/* Disable clock interrupts. */
#define	IPL_HIGH	4	/* Disable all interrupts. */
#define NIPL		5

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

#ifdef _KERNEL
#ifndef _LOCORE

#include <sys/evcnt.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>

#define SPLVM		(MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 | \
			MIPS_INT_MASK_1 | MIPS_INT_MASK_2 | \
			MIPS_INT_MASK_3 | MIPS_INT_MASK_4)
#define SPLSCHED	(SPLVM | MIPS_INT_MASK_5)

#define spl0()          (void)_spllower(0)
#define splx(s)         (void)_splset(s)
#define splvm()		_splraise(SPLVM)
#define splsched()	_splraise(SPLSCHED)
#define splhigh()       _splraise(MIPS_INT_MASK)

#define splsoftclock()	_splraise(MIPS_SOFT_INT_MASK_0)
#define splsoftbio()	_splraise(MIPS_SOFT_INT_MASK_0)
#define splsoftnet()	_splraise(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define splsoftserial()	_splraise(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)

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

struct cobalt_intrhand {
	LIST_ENTRY(cobalt_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_type;
	int ih_cookie_type;
#define	COBALT_COOKIE_TYPE_CPU	0x1
#define	COBALT_COOKIE_TYPE_ICU	0x2

	struct evcnt ih_evcnt;
	char ih_evname[32];
};

#include <mips/softintr.h>

void *cpu_intr_establish(int, int, int (*)(void *), void *);
void *icu_intr_establish(int, int, int, int (*)(void *), void *);
void cpu_intr_disestablish(void *);
void icu_intr_disestablish(void *);
void icu_init(void);

#endif /* !_LOCORE */
#endif /* _LOCORE */

#endif	/* !_COBALT_INTR_H_ */
