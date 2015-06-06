/*	$NetBSD: eisa_machdep.c,v 1.39.6.1 2015/06/06 14:40:00 skrll Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-specific functions for EISA autoconfiguration.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eisa_machdep.c,v 1.39.6.1 2015/06/06 14:40:00 skrll Exp $");

#include "ioapic.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/bus_private.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/eisa/eisavar.h>

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#endif

/*
 * EISA doesn't have any special needs; just use the generic versions
 * of these funcions.
 *
 * XXX really doesn't use bounce buffers? --dyoung
 */
struct x86_bus_dma_tag eisa_bus_dma_tag = {
	._tag_needs_free	= 0,
	._bounce_thresh		= 0,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= 0,
	._may_bounce		= NULL,
};

void
eisa_attach_hook(device_t parent, device_t self,
    struct eisabus_attach_args *eba)
{
	extern int eisa_has_been_seen;

	/*
	 * Notify others that might need to know that the EISA bus
	 * has now been attached.
	 */
	if (eisa_has_been_seen)
		panic("eisaattach: EISA bus already seen!");
	eisa_has_been_seen = 1;
}

int
eisa_maxslots(eisa_chipset_tag_t ec)
{

	/*
	 * Always try 16 slots.
	 */
	return (16);
}

int
eisa_intr_map(eisa_chipset_tag_t ec, u_int irq,
    eisa_intr_handle_t *ihp)
{
	if (irq >= NUM_LEGACY_IRQS) {
		aprint_error("eisa_intr_map: bad IRQ %d\n", irq);
		*ihp = -1;
		return 1;
	}
	if (irq == 2) {
		aprint_verbose("eisa_intr_map: changed IRQ 2 to IRQ 9\n");
		irq = 9;
	}

#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_eisa_bus, irq, ihp) == 0 ||
		    intr_find_mpmapping(mp_isa_bus, irq, ihp) == 0) {
			*ihp |= irq;
			return 0;
		} else
			aprint_verbose("eisa_intr_map: no MP mapping found\n");
	}
#endif

	*ihp = irq;
	return 0;
}

const char *
eisa_intr_string(eisa_chipset_tag_t ec, eisa_intr_handle_t ih, char *buf,
    size_t len)
{
	if (ih == 0 || APIC_IRQ_LEGACY_IRQ(ih) >= NUM_LEGACY_IRQS || ih == 2)
		panic("eisa_intr_string: bogus handle 0x%" PRIx64, ih);

#if NIOAPIC > 0
	if (ih & APIC_INT_VIA_APIC)
		snprintf(buf, len, "apic %d int %d (irq %d)",
		    APIC_IRQ_APIC(ih),
		    APIC_IRQ_PIN(ih),
		    APIC_IRQ_LEGACY_IRQ(ih));
	else
		snprintf(buf, len, "irq %d",  APIC_IRQ_LEGACY_IRQ(ih));
#else
	snprintf(buf, len, "irq %d", APIC_IRQ_LEGACY_IRQ(ih));
#endif
	return buf;
}

const struct evcnt *
eisa_intr_evcnt(eisa_chipset_tag_t ec, eisa_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
eisa_intr_establish(eisa_chipset_tag_t ec, eisa_intr_handle_t ih,
    int type, int level, int (*func)(void *), void *arg)
{
	int pin, irq;
	struct pic *pic;

	pic = &i8259_pic;
	pin = irq = ih;

#if NIOAPIC > 0
	if (ih & APIC_INT_VIA_APIC) {
		struct ioapic_softc * const ioapic = ioapic_find(APIC_IRQ_APIC(ih));
		if (ioapic == NULL) {
			aprint_normal("eisa_intr_establish: bad ioapic %d\n",
			    APIC_IRQ_APIC(ih));
			return NULL;
		}
		pic = &ioapic->sc_pic;
		pin = APIC_IRQ_PIN(ih);
		irq = APIC_IRQ_LEGACY_IRQ(ih);
		if (irq < 0 || irq >= NUM_LEGACY_IRQS)
			irq = -1;
	}
#endif

	return intr_establish(irq, pic, pin, type, level, func, arg, false);
}

void
eisa_intr_disestablish(eisa_chipset_tag_t ec, void *cookie)
{

	intr_disestablish(cookie);
}

int
eisa_conf_read_mem(eisa_chipset_tag_t ec, int slot,
    int func, int entry, struct eisa_cfg_mem *ecm)
{

	/* XXX XXX XXX */
	return (ENOENT);
}

int
eisa_conf_read_irq(eisa_chipset_tag_t ec, int slot,
    int func, int entry, struct eisa_cfg_irq *eci)
{

	/* XXX XXX XXX */
	return (ENOENT);
}

int
eisa_conf_read_dma(eisa_chipset_tag_t ec, int slot,
    int func, int entry, struct eisa_cfg_dma *ecd)
{

	/* XXX XXX XXX */
	return (ENOENT);
}

int
eisa_conf_read_io(eisa_chipset_tag_t ec, int slot,
    int func, int entry, struct eisa_cfg_io *ecio)
{

	/* XXX XXX XXX */
	return (ENOENT);
}
