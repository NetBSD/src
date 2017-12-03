/*	$NetBSD: algor_intr.c,v 1.1.12.1 2017/12/03 11:35:45 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: algor_intr.c,v 1.1.12.1 2017/12/03 11:35:45 jdolecek Exp $");

#define	__INTR_PRIVATE
#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h" 
#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/cpu.h>

#include <algor/autoconf.h>
#include <mips/locore.h>
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

void	(*algor_iointr)(int, vaddr_t, uint32_t);

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const struct ipl_sr_map algor_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE]	=	0,
	[IPL_SOFTCLOCK]	=	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTBIO]	=	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET]	=	MIPS_SOFT_INT_MASK,
	[IPL_SOFTSERIAL] =	MIPS_SOFT_INT_MASK,
	[IPL_VM]	=	MIPS_SOFT_INT_MASK
				|MIPS_INT_MASK_0|MIPS_INT_MASK_1
				|MIPS_INT_MASK_2|MIPS_INT_MASK_3,
	[IPL_SCHED]	=	MIPS_INT_MASK,
	[IPL_HIGH]	=	MIPS_INT_MASK,
    },
};

void
#ifdef evbmips
evbmips_intr_init(void)
#else
intr_init(void)
#endif
{
	ipl_sr_map = algor_ipl_sr_map;

#if defined(ALGOR_P4032)
	algor_p4032_intr_init(&p4032_configuration);
#elif defined(ALGOR_P5064)
	algor_p5064_intr_init(&p5064_configuration);
#elif defined(ALGOR_P6032)
	algor_p6032_intr_init(&p6032_configuration);
#endif
}

#ifdef evbmips
void
evbmips_iointr(int ipl, uint32_t pending, struct clockframe *cf)
{
	(*algor_iointr)(ipl, cf->pc, pending);
}

void *
evbmips_intr_establish(int irq, int (*func)(void *), void *arg)
{
	return (*algor_intr_establish)(irq, func, arg);
}

void
evbmips_intr_disestablish(void *cookie)
{
	(*algor_intr_disestablish)(cookie);
}
#else
void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	uint32_t pending;
	int ipl;

	curcpu()->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&pending))) {
		splx(ipl);
		if (pending & MIPS_INT_MASK_5) {
			struct clockframe cf;

			cf.pc = pc;
			cf.sr = status;
			cf.intr = (curcpu()->ci_idepth > 1);
			mips3_clockintr(&cf);
		}

		if (pending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
				MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
			/* Process I/O and error interrupts. */
			(*algor_iointr)(ipl, pc, pending);
		}
		(void)splhigh();
	}
}
#endif
