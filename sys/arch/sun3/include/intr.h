/*	$NetBSD: intr.h,v 1.5.6.1 2006/04/22 11:38:05 simonb Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

#ifndef _SUN3_INTR_H_
#define _SUN3_INTR_H_ 1

#include <machine/psl.h>

#if defined(_KERNEL) && !defined(_LOCORE)

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	(PSL_S|PSL_IPL1)
#define	IPL_SOFTNET	(PSL_S|PSL_IPL1)
#define	IPL_BIO		(PSL_S|PSL_IPL2)
#define	IPL_NET		(PSL_S|PSL_IPL3)
#define	IPL_TTY		(PSL_S|PSL_IPL4)
#define	IPL_VM		(PSL_S|PSL_IPL4)
/* Intersil clock hardware interrupts (hard-wired at 5) */
#define	IPL_CLOCK	(PSL_S|PSL_IPL5)
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_SCHED	(PSL_S|PSL_IPL7)
#define	IPL_HIGH	(PSL_S|PSL_IPL7)
#define	IPL_LOCK	(PSL_S|PSL_IPL7)
#define	IPL_SERIAL	(PSL_S|PSL_IPL4)

/*
 * Define inline functions for PSL manipulation.
 * These are as close to macros as one can get.
 * When not optimizing gcc will call the locore.s
 * functions by the same names, so breakpoints on
 * these functions will work normally, etc.
 * (See the GCC extensions info document.)
 */

static __inline int _getsr(void);

/* Get current sr value. */
static __inline int
_getsr(void)
{
	int rv;

	__asm volatile ("clrl %0; movew %%sr,%0" : "=&d" (rv));
	return (rv);
}

/*
 * The rest of this is sun3 specific, because other ports may
 * need to do special things in spl0() (i.e. simulate SIR).
 * Suns have a REAL interrupt register, so spl0() and splx(s)
 * have no need to check for any simulated interrupts, etc.
 */

#define spl0()  _spl0()		/* we have real software interrupts */
#define splx(x)	_spl(x)

/* IPL used by soft interrupts: netintr(), softclock() */
#define	spllowersoftclock() spl1()

#define	splraiseipl(x)	_splraise(x)

#include <sys/spl.h>

#endif	/* KERNEL && !_LOCORE */
#endif	/* _SUN3_INTR_H_ */
