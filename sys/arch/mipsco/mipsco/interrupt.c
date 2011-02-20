/*	$NetBSD: interrupt.c,v 1.11 2011/02/20 07:56:16 matt Exp $	*/

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

#define __INTR_PRIVATE
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.11 2011/02/20 07:56:16 matt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/intr.h>
#include <sys/lwp.h>
#include <sys/cpu.h>

#include <machine/sysconf.h>

void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	uint32_t ipending;
	int ipl;

	curcpu()->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&ipending))) {
		/* device interrupts */
		(*platform.iointr)(status, pc, ipending);
	}

}

const struct ipl_sr_map mipsco_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_INT_MASK_SPL_SOFT0,
	[IPL_SOFTNET] = MIPS_INT_MASK_SPL_SOFT1,
	[IPL_VM] = MIPS_INT_MASK_SPL2,
	[IPL_SCHED] = MIPS_INT_MASK_SPL2,
	[IPL_HIGH] = MIPS_INT_MASK,
    },
};
