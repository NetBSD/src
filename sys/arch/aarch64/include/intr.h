/* $NetBSD: intr.h,v 1.1 2014/08/10 05:47:38 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _AARCH64_INTR_H_
#define _AARCH64_INTR_H_

#ifdef __aarch64__

#ifdef _LOCORE
#error use "assym.h"
#endif

#ifdef _KERNEL

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#ifdef MULTIPROCESSOR
#define __HAVE_PREEMPTION	1
#endif

/* Interrupt priority "levels". */
#define	IPL_NONE	0		/* nothing */
#define	IPL_SOFTCLOCK	1		/* clock */
#define	IPL_SOFTBIO	2		/* block I/O */
#define	IPL_SOFTNET	3		/* software network interrupt */
#define	IPL_SOFTSERIAL	4		/* software serial interrupt */
#define	IPL_VM		5		/* memory allocation */
#define	IPL_SCHED	6		/* clock interrupt */
#define	IPL_HIGH	7		/* everything */

#define	NIPL		8

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#define IST_LEVEL_LOW	IST_LEVEL
#define IST_LEVEL_HIGH	4
#define IST_EDGE_FALLING IST_EDGE
#define IST_EDGE_RISING	5
#define IST_EDGE_BOTH	6
#define IST_SOFT	7

#include <arm/pic/picvar.h>

static inline void
spl0(void)
{
	(void)_spllower(IPL_NONE);
}

static inline int
splsoftclock(void)
{
	return _splraise(IPL_SOFTCLOCK);
}

static inline int
splsoftbio(void)
{
	return _splraise(IPL_SOFTBIO);
}

static inline int
splsoftnet(void)
{
	return _splraise(IPL_SOFTNET);
}

static inline int
splsoftserial(void)
{
	return _splraise(IPL_SOFTSERIAL);
}

static inline int
splvm(void)
{
	return _splraise(IPL_VM);
}

static inline int
splsched(void)
{
	return _splraise(IPL_SCHED);
}

static inline int
splhigh(void)
{
	return _splraise(IPL_HIGH);
}

typedef uint8_t ipl_t;
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

	return _splraise(icookie._ipl);
}

#endif /* _KERNEL */

#elif defined(__arm__)

#include <arm/intr.h>

#endif /* __aarch64__/__arm__ */

#endif /* _AARCH64_INTR_H_ */
