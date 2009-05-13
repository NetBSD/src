/*	$NetBSD: interrupt.c,v 1.6.14.1 2009/05/13 17:18:03 jym Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.6.14.1 2009/05/13 17:18:03 jym Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/intr.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/sysconf.h>

void
cpu_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	struct cpu_info *ci;

	ci = curcpu();
	uvmexp.intrs++;

	/* device interrupts */
	ci->ci_idepth++;
	(*platform.iointr)(status, cause, pc, ipending);
	ci->ci_idepth--;

#ifdef __HAVE_FAST_SOFTINTS
	/* software simulated interrupt */
	if ((ipending & MIPS_SOFT_INT_MASK_1)
		    || (ssir && (status & MIPS_SOFT_INT_MASK_1))) {
	    _clrsoftintr(MIPS_SOFT_INT_MASK_1);
	    softintr_dispatch();
	}
#endif
}

static const int ipl_sr_bits[] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_INT_MASK_SPL_SOFT0,
	[IPL_SOFTNET] = MIPS_INT_MASK_SPL_SOFT1,
	[IPL_VM] = MIPS_INT_MASK_SPL2,
	[IPL_SCHED] = MIPS_INT_MASK_SPL2,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._sr = ipl_sr_bits[ipl]};
}
