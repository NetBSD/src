/*	$NetBSD: interrupt.c,v 1.8 2003/07/15 02:54:40 lukem Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.8 2003/07/15 02:54:40 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <mips/psl.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/intr.h>

const u_int32_t mips_ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTSERIAL */
};

struct evcnt pmax_clock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "clock", "intr");
struct evcnt pmax_fpu_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "fpu", "intr");
struct evcnt pmax_memerr_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "memerr", "intr");

extern void MachFPInterrupt(unsigned, unsigned, unsigned, struct frame *);

/*
 * pmax uses standard mips1 convention, wiring FPU to hard interrupt 5.
 */
void
cpu_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{

	uvmexp.intrs++;

	/* device interrupts */
	if (ipending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
			MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
		(*platform.iointr)(status, cause, pc, ipending);
	}
	/* FPU notification */
	if (ipending & MIPS_INT_MASK_5) {
		if (!USERMODE(status))
			goto kerneltouchedFPU;
		pmax_fpu_evcnt.ev_count++;
#if !defined(SOFTFLOAT)
		MachFPInterrupt(status, cause, pc, curlwp->l_md.md_regs);
#endif
	}

	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	softintr_dispatch(ipending);

	return;
kerneltouchedFPU:
	panic("kernel used FPU: PC %x, CR %x, SR %x", pc, cause, status);
}
