/*	$NetBSD: interrupt.c,v 1.13.18.1 2009/12/31 00:54:08 matt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.13.18.1 2009/12/31 00:54:08 matt Exp $");

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h" 
#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/locore.h>
#include <mips/mips3_clock.h>

#ifdef ALGOR_P4032
#include <algor/algor/algor_p4032var.h>
#endif

#ifdef ALGOR_P5064
#include <algor/algor/algor_p5064var.h>
#endif  
 
#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032var.h>
#endif

void	*(*algor_intr_establish)(int, int (*)(void *), void *);
void	(*algor_intr_disestablish)(void *);

void	(*algor_iointr)(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

u_long	cycles_per_hz;

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
const u_int32_t ipl_sr_bits[_IPL_N] = {
	0,					/* IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_3,		/* IPL_VM */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_3|
		MIPS_INT_MASK_4|
		MIPS_INT_MASK_5,		/* IPL_SCHED */
};

void
intr_init(void)
{

#if defined(ALGOR_P4032)
	algor_p4032_intr_init(&p4032_configuration);
#elif defined(ALGOR_P5064)
	algor_p5064_intr_init(&p5064_configuration);
#elif defined(ALGOR_P6032)
	algor_p6032_intr_init(&p6032_configuration);
#endif
}

void
cpu_intr(u_int32_t status, u_int32_t cause, vaddr_t pc, u_int32_t ipending)
{
	struct clockframe cf;
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;
	uvmexp.intrs++;

	if (ipending & MIPS_INT_MASK_5) {

		cf.pc = pc;
		cf.sr = status;
		mips3_clockintr(&cf);

		/* Re-enable clock interrupts. */
		cause &= ~MIPS_INT_MASK_5;
		_splset(MIPS_SR_INT_IE |
		    ((status & ~cause) & MIPS_HARD_INT_MASK));
	}

	if (ipending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
			MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
		/* Process I/O and error interrupts. */
		(*algor_iointr)(status, cause, pc, ipending);
	}

	ci->ci_idepth--;

#ifdef __HAVE_FAST_SOFTINTS
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;
	_clrsoftintr(ipending);
	softintr_dispatch(ipending);
#endif
}
