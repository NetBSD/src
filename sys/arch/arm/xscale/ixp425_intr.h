/*	$NetBSD: ixp425_intr.h,v 1.6.52.2 2008/01/09 01:45:27 matt Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IXP425_INTR_H_
#define _IXP425_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(ixp425_intr_dispatch)

#ifndef _LOCORE

#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/cpu.h>

#include <arm/xscale/ixp425reg.h>

#define IXPREG(reg)     *((volatile u_int32_t*) (reg))

static inline void __attribute__((__unused__))
ixp425_set_intrmask(void)
{
	extern volatile uint32_t intr_enabled;

	IXPREG(IXP425_INT_ENABLE) = intr_enabled & IXP425_INT_HWMASK;
}

#ifdef __HAVE_FAST_SOFTINTS
void ixp425_do_pending(void);

#define INT_SWMASK						\
	((1U << IXP425_INT_bit31) | (1U << IXP425_INT_bit30) |	\
	 (1U << IXP425_INT_bit14) | (1U << IXP425_INT_bit11))
#endif

static inline void __attribute__((__unused__))
ixp425_splx(int new)
{
	extern volatile uint32_t intr_enabled;
	extern volatile int ixp425_ipending;
	extern void ixp425_do_pending(void);
	int oldirqstate, hwpend;

	/* Don't let the compiler re-order this code with preceding code */
	__insn_barrier();

	set_curcpl(new);

	hwpend = (ixp425_ipending & IXP425_INT_HWMASK) & ~new;
	if (hwpend != 0) {
		oldirqstate = disable_interrupts(I32_bit);
		intr_enabled |= hwpend;
		ixp425_set_intrmask();
		restore_interrupts(oldirqstate);
	}

#ifdef __HAVE_FAST_SOFTINTS
	if ((ixp425_ipending & INT_SWMASK) & ~new)
		ixp425_do_pending();
#endif
}

static inline int __attribute__((__unused__))
ixp425_splraise(int ipl)
{
	extern int ixp425_imask[];
	int old = curcpl();

	set_curcpl(old | ixp425_imask[ipl]);

	/* Don't let the compiler re-order this code with subsequent code */
	__insn_barrier();

	return (old);
}

static inline int __attribute__((__unused__))
ixp425_spllower(int ipl)
{
	extern int ixp425_imask[];
	int old = curcpl();

	ixp425_splx(ixp425_imask[ipl]);
	return(old);
}

#endif /* __PROG32 */

#if !defined(EVBARM_SPL_NOINLINE)

#define splx(new)		ixp425_splx(new)
#define	_spllower(ipl)		ixp425_spllower(ipl)
#define	_splraise(ipl)		ixp425_splraise(ipl)
#ifdef __HAVE_FAST_SOFTINTS
void	_setsoftintr(int);
#endif

#else

int	_splraise(int);
int	_spllower(int);
void	splx(int);
#ifdef __HAVE_FAST_SOFTINTS
void	_setsoftintr(int);
#endif

#endif /* ! EVBARM_SPL_NOINLINE */

#endif /* _LOCORE */

#endif /* _IXP425_INTR_H_ */
