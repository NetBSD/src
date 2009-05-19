/*	$NetBSD: intr.h,v 1.21 2009/05/19 18:39:27 phx Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

/*
 * machine/intr.h for the Amiga port.
 * Currently, only a wrapper, for most of the stuff, around the old
 * include files.
 */

#ifndef _AMIGA_INTR_H_
#define _AMIGA_INTR_H_

#include <amiga/amiga/isr.h>
#include <m68k/psl.h>

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTBIO	1
#define	IPL_SOFTNET	1
#define	IPL_SOFTSERIAL	1
#define	IPL_VM		2
#define	IPL_SCHED	3
#define	IPL_HIGH	4
#define	_NIPL		5

extern int ipl2spl_table[_NIPL];

typedef int ipl_t;
typedef struct {
	uint16_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(ipl2spl_table[icookie._ipl]);
}

#ifdef _KERNEL_OPT
#include "opt_lev6_defer.h"
#endif

#define	spl0()			_spl0()	/* we have real software interrupts */
#define splsoftclock()		splraise1()
#define splsoftnet()		splraise1()
#define splsoftserial()		splraise1()
#define splsoftbio()		splraise1()
#define	splvm()			splraise4()

#ifndef _LKM

#ifndef LEV6_DEFER
#define splsched()	splraise6()
#define splhigh()	spl7()
#else
#define splsched()	splraise4()
#define splhigh()	splraise4()
#endif

#else	/* _LKM */

extern int _spllkm6(void);
extern int _spllkm7(void);

#define splsched()	_spllkm6()
#define splhigh()	_spllkm7()

#endif /* _LKM */

#define splx(s)		_spl(s)

#endif	/* !_AMIGA_INTR_H_ */
