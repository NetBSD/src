/*	$NetBSD: i80200_icu.c,v 1.3.2.2 2002/02/11 20:07:22 jdolecek Exp $	*/

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

/*
 * Intel i80200 Interrupt Controller Unit support.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <arm/cpufunc.h>

#include <arm/xscale/i80200reg.h>
#include <arm/xscale/i80200var.h>

/* Software shadow copy of INTCTL. */
static __volatile uint32_t intctl;

/* Pointer to board-specific external IRQ dispatcher. */
void	(*i80200_extirq_dispatch)(struct clockframe *);

static void
i80200_default_extirq_dispatch(struct clockframe *framep)
{

	panic("external IRQ with no dispatch routine");
}

/*
 * i80200_intr_init:
 *
 *	Initialize the i80200 ICU.
 */
void
i80200_intr_init(void)
{

	/* Disable all interrupt sources. */
	intctl = 0;
	__asm __volatile("mcr p13, 0, %0, c0, c0"
		:
		: "r" (intctl));

	/* Steer PMU and BMU to IRQ. */
	__asm __volatile("mcr p13, 0, %0, c2, c0"
		:
		: "r" (0));

	i80200_extirq_dispatch = i80200_default_extirq_dispatch;
}

/*
 * i80200_intr_enable:
 *
 *	Enable an interrupt source in the i80200 ICU.
 */
void
i80200_intr_enable(uint32_t intr)
{
	u_int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit|F32_bit);

	intctl |= intr;
	__asm __volatile("mcr p13, 0, %0, c0, c0"
		:
		: "r" (intctl));

	restore_interrupts(oldirqstate);
}

/*
 * i80200_intr_disable:
 *
 *	Disable an interrupt source in the i80200 ICU.
 */
void
i80200_intr_disable(uint32_t intr)
{
	u_int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit|F32_bit);

	intctl &= ~intr;
	__asm __volatile("mcr p13, 0, %0, c0, c0"
		:
		: "r" (intctl));

	restore_interrupts(oldirqstate);
}
