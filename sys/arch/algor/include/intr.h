/*	$NetBSD: intr.h,v 1.13.18.1 2010/02/06 02:57:43 matt Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _ALGOR_INTR_H_
#define _ALGOR_INTR_H_

#include <sys/evcnt.h>
#include <sys/queue.h>

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_SOFTCLOCK	1	/* generic software interrupts (SI 0) */
#define	IPL_SOFTBIO	1	/* clock software interrupts (SI 0) */
#define	IPL_SOFTNET	2	/* network software interrupts (SI 1) */
#define	IPL_SOFTSERIAL	2	/* serial software interrupts (SI 1) */
#define	IPL_VM		3
#define	IPL_SCHED	4
#define	IPL_HIGH	4	/* disable all interrupts */

#define	_IPL_N		5

#define	_IPL_SI0_FIRST	IPL_SOFTCLOCK
#define	_IPL_SI0_LAST	IPL_SOFTBIO

#define	_IPL_SI1_FIRST	IPL_SOFTNET
#define	_IPL_SI1_LAST	IPL_SOFTSERIAL

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifdef	_KERNEL

#include <mips/locore.h>

extern const u_int32_t ipl_sr_bits[_IPL_N];

#define	spl0()		(void) _spllower(0)
#define	splx(s)		(void) _splset(s)

typedef int ipl_t;
typedef struct {
	ipl_t _sr;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._sr = ipl_sr_bits[ipl]};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._sr);
}

#include <sys/spl.h>

struct algor_intrhand {
	LIST_ENTRY(algor_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;		/* mostly for ISA */
	const void *ih_irqmap;
};

extern struct evcnt mips_int5_evcnt;

void	intr_init(void);
void	*(*algor_intr_establish)(int, int (*)(void *), void *);
void	(*algor_intr_disestablish)(void *);

#endif /* _KERNEL */
#endif /* ! _ALGOR_INTR_H_ */
