/*	$NetBSD: becc_intr.h,v 1.2 2005/12/24 20:06:52 perry Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
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

#ifndef _BECC_INTR_H_
#define	_BECC_INTR_H_

#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/becc_csrvar.h>

static inline void __attribute__((__unused__))
becc_set_intrmask(void)
{
	extern volatile uint32_t intr_enabled;

	/*
	 * The bits in the ICMR indicate which interrupts are masked
	 * (disabled), so we must invert our intr_enabled mask.
	 */

	BECC_CSR_WRITE(BECC_ICMR, ~intr_enabled & ICU_VALID_MASK);
	(void) BECC_CSR_READ(BECC_ICMR);
}

static inline int __attribute__((__unused__))
becc_splraise(int ipl)
{
	extern volatile uint32_t current_spl_level;
	extern uint32_t becc_imask[];
	uint32_t old;

	old = current_spl_level;
	current_spl_level |= becc_imask[ipl];

	return (old);
}

static inline void __attribute__((__unused__))
becc_splx(int new)
{
	extern volatile uint32_t intr_enabled, becc_ipending;
	extern volatile uint32_t current_spl_level;
	uint32_t oldirqstate, hwpend;

	current_spl_level = new;

	/*
	 * If there are pending HW interrupts which are being
	 * unmasked, then enable them in the ICMR register.
	 * This will cause them to come flooding in.  This
	 * includes soft interrupts.
	 */
	hwpend = becc_ipending & ~new;
	if (hwpend != 0) {
		oldirqstate = disable_interrupts(I32_bit); 
		intr_enabled |= hwpend;
		becc_set_intrmask();
		restore_interrupts(oldirqstate);
	}
}

static inline int __attribute__((__unused__))
becc_spllower(int ipl)
{
	extern volatile uint32_t current_spl_level;
	extern uint32_t becc_imask[];
	uint32_t old = current_spl_level;

	becc_splx(becc_imask[ipl]);
	return (old);
}

static inline void __attribute__((__unused__))
becc_setsoftintr(int si)
{
	extern volatile uint32_t	becc_sipending;

	becc_sipending |= (1 << si);
	BECC_CSR_WRITE(BECC_ICSR, (1U << ICU_SOFT));
}

int	becc_softint(void *arg);

#if !defined(EVBARM_SPL_NOINLINE)

#define	_splraise(ipl)		becc_splraise(ipl)
#define	splx(new)		becc_splx(new)
#define	_spllower(ipl)		becc_spllower(ipl)
#define	_setsoftintr(si)	becc_setsoftintr(si)

#else

int	_splraise(int);
void	splx(int);
int	_spllower(int);
void	_setsoftintr(int);

#endif /* ! EVBARM_SPL_NOINLINE */

#endif /* _BECC_INTR_H_ */
