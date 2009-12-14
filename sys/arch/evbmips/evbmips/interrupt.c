/*	$NetBSD: interrupt.c,v 1.12 2009/12/14 00:46:02 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.12 2009/12/14 00:46:02 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <mips/mips3_clock.h>
#include <machine/locore.h>

void
intr_init(void)
{

	evbmips_intr_init();	/* board specific stuff */
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
		/* call the common MIPS3 clock interrupt handler */ 
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
		evbmips_iointr(status, cause, pc, ipending);
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
