/* $NetBSD: intr.h,v 1.14.8.1 2014/08/20 00:03:10 tls Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#ifndef _MACHINE_INTR_H
#define _MACHINE_INTR_H

#ifdef _KERNEL

/*
 * spl functions
 */
#include <machine/psl.h>

#define spl0()		_spl0()
#define splnone()	spl0()
#define splsoftclock()	splraise1()
#define splsoftbio()	splraise1()
#define splsoftnet()	splraise1()
#define splsoftserial()	splraise1()
#define splvm()		splraise4()
#define splsched()	splraise5()
#define splhigh()	spl7()

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTBIO	2
#define	IPL_SOFTNET	3
#define	IPL_SOFTSERIAL	4
#define	IPL_VM		5
#define	IPL_SCHED	6
#define	IPL_HIGH	7
#define	NIPL		8

extern const uint16_t ipl2psl_table[NIPL];

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = ipl2psl_table[ipl]};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

static inline void
splx(int sr)
{

	__asm volatile("movew %0,%%sr" : : "di" (sr));
}

#endif /* _KERNEL */

#endif	/* _MACHINE_INTR_H */
