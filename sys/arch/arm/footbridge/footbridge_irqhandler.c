/*	$NetBSD: footbridge_irqhandler.c,v 1.17.24.4 2008/01/01 15:39:26 chris Exp $	*/

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

#ifndef ARM_SPL_NOINLINE
#define	ARM_SPL_NOINLINE
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0,"$NetBSD: footbridge_irqhandler.c,v 1.17.24.4 2008/01/01 15:39:26 chris Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/cpu.h>
#include <arm/arm_intr.h>
#include <arm/footbridge/footbridge_irqhandler.h>
#include <dev/pci/pcivar.h>

#include <arm/footbridge/dc21285mem.h>
#include <arm/footbridge/dc21285reg.h>

extern void isa_intr_init(void);

static irqgroup_t footbridge_irq_group;

static void footbridge_set_irq_mask(irq_hardware_cookie_t, uint32_t intr_enabled);
static uint32_t footbridge_irq_status(void);

const struct evcnt *footbridge_pci_intr_evcnt __P((void *, pci_intr_handle_t));

const struct evcnt *
footbridge_pci_intr_evcnt(void *pcv, pci_intr_handle_t ih)
{
	/* XXX check range is valid */
#if NISA > 0
	if (ih >= 0x80 && ih <= 0x8f) {
		return isa_intr_evcnt(NULL, (ih & 0x0f));
	}
#endif
	return arm_intr_evcnt(footbridge_irq_group, ih);
}

void
footbridge_intr_init(void)
{
	footbridge_irq_group = arm_intr_register_irq_provider("footbridge", FOOTBRIDGE_NIRQ,
		   	footbridge_set_irq_mask,
			NULL,
			NULL);

	/*
	 * Since various PCI interrupts could be routed via the ICU
	 * (for PCI devices in the bridge) we need to set up the ICU
	 * now so that these interrupts can be established correctly
	 * i.e. This is a hack.
	 */
	isa_intr_init();
}

static void
footbridge_set_irq_mask(irq_hardware_cookie_t cookie, uint32_t intr_enabled)
{
    ((volatile uint32_t*)(DC21285_ARMCSR_VBASE))[IRQ_ENABLE_SET>>2] = intr_enabled;
    ((volatile uint32_t*)(DC21285_ARMCSR_VBASE))[IRQ_ENABLE_CLEAR>>2] = ~intr_enabled;
}

static uint32_t
footbridge_irq_status(void)
{
	    return ((volatile uint32_t*)(DC21285_ARMCSR_VBASE))[IRQ_STATUS>>2];
}

void *
footbridge_intr_claim(int irq, int ipl, const char *name, int (*func)(void *), void *arg)
{
	if (irq < 0 || irq >= FOOTBRIDGE_NIRQ)
		panic("footbridge_intr_establish: IRQ %d out of range", irq);

	return arm_intr_claim(footbridge_irq_group, irq, IST_LEVEL, ipl, name, func, arg);
}

void
footbridge_intr_disestablish(void *cookie)
{
	return arm_intr_disestablish(footbridge_irq_group, cookie);
}

void
footbridge_intr_dispatch(struct clockframe * frame)
{
	uint32_t hwpend;

	/* fetch bitmask of pending interrupts */
	hwpend = footbridge_irq_status();

	/* queue up the interrupts for processing */
	arm_intr_queue_irqs(footbridge_irq_group, hwpend);

	/* process interrupts */
	arm_intr_process_pending_ipls(frame, current_ipl_level);
}

