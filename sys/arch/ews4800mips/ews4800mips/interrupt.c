/*	$NetBSD: interrupt.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $");

#include <sys/param.h>
#include <uvm/uvm_extern.h>	/* uvmexp.intrs */

#include <machine/intr.h>
#include <machine/sbdvar.h>

const uint32_t *ipl_sr_bits;
static void (*platform_intr)(uint32_t, uint32_t, uint32_t, uint32_t);

const uint32_t mips_ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,		/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,		/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTSERIAL */
};

void
intr_init(void)
{

	softintr_init();

	platform_intr = platform.intr;
	(*platform.intr_init)();
}

void
intr_establish(int irq, int (*func)(void *), void *arg)
{

	(*platform.intr_establish)(irq, func, arg);
}

void
intr_disestablish(void *arg)
{

	(*platform.intr_disestablish)(arg);
}

void
cpu_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{

	uvmexp.intrs++;

	(*platform_intr)(status, cause, pc, ipending);

	ipending &= (MIPS_SOFT_INT_MASK_1 | MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	softintr_dispatch(ipending);
}
