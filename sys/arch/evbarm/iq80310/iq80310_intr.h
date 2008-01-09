/*	$NetBSD: iq80310_intr.h,v 1.5.50.1 2008/01/09 01:45:47 matt Exp $	*/

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

#ifndef _IQ80310_INTR_H_
#define _IQ80310_INTR_H_

#include "opt_iop310.h"

#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/xscale/i80200reg.h>
#include <arm/xscale/i80200var.h>

#if defined(IOP310_TEAMASA_NPWR)
/*
 * We have 5 interrupt source bits -- all in XINT3.  All interrupts
 * can be masked in the CPLD.
 */
#define	IRQ_BITS		0x1f
#define	IRQ_BITS_ALWAYS_ON	0x00
#else /* Default to stock IQ80310 */
/*
 * We have 8 interrupt source bits -- 5 in the XINT3 register, and 3
 * in the XINT0 register (the upper 3).  Note that the XINT0 IRQs
 * (SPCI INTA, INTB, and INTC) are always enabled, since they can not
 * be masked out in the CPLD (it provides only status, not masking,
 * for those interrupts).
 */
#define	IRQ_BITS		0xff
#define	IRQ_BITS_ALWAYS_ON	0xe0
#define	IRQ_READ_XINT0		1	/* XXX only if board rev >= F */
#endif /* list of IQ80310-based designs */

#ifdef __HAVE_FAST_SOFTINTS
void	iq80310_do_soft(void);
#endif

static inline int __attribute__((__unused__))
iq80310_splraise(int ipl)
{
	extern int iq80310_imask[];
	int old;

	old = curcpl();
	set_curcpl(old | iq80310_imask[ipl]);

	/* Don't let the compiler re-order this code with subsequent code */
	__insn_barrier();

	return (old);
}

static inline void __attribute__((__unused__))
iq80310_splx(int new)
{
	extern volatile int iq80310_ipending;
	int old;

	/* Don't let the compiler re-order this code with preceding code */
	__insn_barrier();

	old = curcpl();
	set_curcpl(new);

#ifdef __HAVE_FAST_SOFTINTS
	/* If there are software interrupts to process, do it. */
	if ((iq80310_ipending & ~IRQ_BITS) & ~new)
		iq80310_do_soft();
#endif

	/*
	 * If there are pending hardware interrupts (i.e. the
	 * external interrupt is disabled in the ICU), and all
	 * hardware interrupts are being unblocked, then re-enable
	 * the external hardware interrupt.
	 *
	 * XXX We have to wait for ALL hardware interrupts to
	 * XXX be unblocked, because we currently lose if we
	 * XXX get nested interrupts, and I don't know why yet.
	 */
	if ((new & IRQ_BITS) == 0 && (iq80310_ipending & IRQ_BITS))
		i80200_intr_enable(INTCTL_IM | INTCTL_PM);
}

static inline int __attribute__((__unused__))
iq80310_spllower(int ipl)
{
	extern int iq80310_imask[];
	const int old = curcpl();

	iq80310_splx(iq80310_imask[ipl]);
	return (old);
}

#if !defined(EVBARM_SPL_NOINLINE)

#define _splraise(ipl)		iq80310_splraise(ipl)
#define	_spllower(ipl)		iq80310_spllower(ipl)
#define	splx(spl)		iq80310_splx(spl)
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

#endif /* _IQ80310_INTR_H_ */
