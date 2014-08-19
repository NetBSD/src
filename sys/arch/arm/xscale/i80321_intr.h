/*	$NetBSD: i80321_intr.h,v 1.10.44.1 2014/08/20 00:02:48 tls Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2006 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Steve C. Woodford for Wasabi Systems, Inc.
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

#ifndef _I80321_INTR_H_
#ifndef _NO_INTR_H_
#define _I80321_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(i80321_intr_dispatch)

#ifndef _LOCORE

#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/cpu.h>

#include <arm/xscale/i80321reg.h>

#ifdef __PROG32
static inline void __attribute__((__unused__))
i80321_set_intrmask(void)
{
	extern volatile uint32_t intr_enabled;

	__asm volatile("mcr p6, 0, %0, c0, c0, 0"
		:
		: "r" (intr_enabled & ICU_INT_HWMASK));
}

#define INT_HPIMASK	(1u << ICU_INT_HPI)
extern volatile uint32_t intr_enabled;
extern volatile int i80321_ipending;
extern int i80321_imask[];

static inline void __attribute__((__unused__))
i80321_splx(int new)
{
	int oldirqstate, hwpend;

	/* Don't let the compiler re-order this code with preceding code */
	__insn_barrier();

	set_curcpl(new);

	hwpend = (i80321_ipending & ICU_INT_HWMASK) & ~i80321_imask[new];
	if (hwpend != 0) {
		oldirqstate = disable_interrupts(I32_bit);
		intr_enabled |= hwpend;
		i80321_set_intrmask();
#ifdef I80321_HPI_ENABLED
		if (__predict_false(hwpend & INT_HPIMASK))
			oldirqstate &= ~I32_bit;
#endif
		restore_interrupts(oldirqstate);
	}

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

static inline int __attribute__((__unused__))
i80321_splraise(int ipl)
{
	int old = curcpl();
	set_curcpl(ipl);

	/* Don't let the compiler re-order this code with subsequent code */
	__insn_barrier();

	return (old);
}

static inline int __attribute__((__unused__))
i80321_spllower(int ipl)
{
	int old = curcpl();
	i80321_splx(ipl);
	return(old);
}

#endif /* __PROG32 */

#if !defined(EVBARM_SPL_NOINLINE)

#define splx(new)		i80321_splx(new)
#define	_spllower(ipl)		i80321_spllower(ipl)
#define	_splraise(ipl)		i80321_splraise(ipl)

#else

int	_splraise(int);
int	_spllower(int);
void	splx(int);

#endif /* ! EVBARM_SPL_NOINLINE */

#endif /* _LOCORE */

#endif /* _NO_INTR_H_ */
#endif /* _I80321_INTR_H_ */
