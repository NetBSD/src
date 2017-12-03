/*	$NetBSD: isa_machdep.c,v 1.32.2.1 2017/12/03 11:36:50 jdolecek Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.32.2.1 2017/12/03 11:36:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/bus_private.h>
#include <machine/pio.h>
#include <machine/cpufunc.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <uvm/uvm_extern.h>

#include "acpica.h"
#include "opt_acpi.h"
#include "ioapic.h"

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#endif

static int _isa_dma_may_bounce(bus_dma_tag_t, bus_dmamap_t, int, int *);

struct x86_bus_dma_tag isa_bus_dma_tag = {
	._tag_needs_free	= 0,
	._bounce_thresh		= ISA_DMA_BOUNCE_THRESHOLD,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= ISA_DMA_BOUNCE_THRESHOLD,
	._may_bounce		= _isa_dma_may_bounce,
};

#define	IDTVEC(name)	__CONCAT(X,name)
typedef void (vector)(void);
extern vector *IDTVEC(intr)[];

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < NUM_LEGACY_IRQS && (x) != 2)

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	int i, tmp, bestirq, count;
	struct intrhand **p, *q;
	struct intrsource *isp;
	struct cpu_info *ci;

	if (type == IST_NONE)
		panic("intr_alloc: bogus type");

	ci = &cpu_info_primary;

	bestirq = -1;
	count = -1;

	/* some interrupts should never be dynamically allocated */
	mask &= 0xdef8;

	/*
	 * XXX some interrupts will be used later (6 for fdc, 12 for pms).
	 * the right answer is to do "breadth-first" searching of devices.
	 */
	mask &= 0xefbf;

	mutex_enter(&cpu_lock);

	for (i = 0; i < NUM_LEGACY_IRQS; i++) {
		if (LEGAL_IRQ(i) == 0 || (mask & (1<<i)) == 0)
			continue;
		isp = ci->ci_isources[i];
		if (isp == NULL) {
			/* if nothing's using the irq, just return it */
			*irq = i;
			mutex_exit(&cpu_lock);
			return 0;
		}

		switch(isp->is_type) {
		case IST_EDGE:
		case IST_LEVEL:
			if (type != isp->is_type)
				continue;
			/*
			 * if the irq is shareable, count the number of other
			 * handlers, and if it's smaller than the last irq like
			 * this, remember it
			 *
			 * XXX We should probably also consider the
			 * interrupt level and stick IPL_TTY with other
			 * IPL_TTY, etc.
			 */
			for (p = &isp->is_handlers, tmp = 0; (q = *p) != NULL;
			     p = &q->ih_next, tmp++)
				;
			if ((bestirq == -1) || (count > tmp)) {
				bestirq = i;
				count = tmp;
			}
			break;
		case IST_PULSE:
			/* this just isn't shareable */
			continue;
		}
	}

	mutex_exit(&cpu_lock);

	if (bestirq == -1)
		return 1;

	*irq = bestirq;

	return 0;
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{
	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
isa_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg)
{
	return isa_intr_establish_xname(ic, irq, type, level,
	    ih_fun, ih_arg, "unknown");
}

void *
isa_intr_establish_xname(isa_chipset_tag_t ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg, const char *xname)
{
	struct pic *pic;
	int pin;
#if NIOAPIC > 0
	intr_handle_t mpih = 0;
	struct ioapic_softc *ioapic = NULL;
#endif

	pin = irq;
	pic = &i8259_pic;

#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, irq, &mpih) == 0 ||
		    intr_find_mpmapping(mp_eisa_bus, irq, &mpih) == 0) {
			if (!APIC_IRQ_ISLEGACY(mpih)) {
				pin = APIC_IRQ_PIN(mpih);
				ioapic = ioapic_find(APIC_IRQ_APIC(mpih));
				if (ioapic == NULL) {
					printf("isa_intr_establish: "
					       "unknown apic %d\n",
					    APIC_IRQ_APIC(mpih));
					return NULL;
				}
				pic = &ioapic->sc_pic;
			}
		} else
			printf("isa_intr_establish: no MP mapping found\n");
	}
#endif
#if defined(XEN)
	KASSERT(APIC_IRQ_ISLEGACY(irq));

	int evtch;
	char evname[16];

	mpih |= APIC_IRQ_LEGACY_IRQ(irq);

	evtch = xen_pirq_alloc((intr_handle_t *)&mpih, type); /* XXX: legacy - xen just tosses irq back at us */
	if (evtch == -1)
		return NULL;
#if NIOAPIC > 0
	if (ioapic)
		snprintf(evname, sizeof(evname), "%s pin %d",
		    device_xname(ioapic->sc_dev), pin);
	else
#endif
		snprintf(evname, sizeof(evname), "irq%d", irq);

	aprint_debug("irq: %d requested on pic: %s.\n", irq, pic->pic_name);

	return (void *)pirq_establish(irq, evtch, ih_fun, ih_arg, level,
	    evname);
#else /* defined(XEN) */
	return intr_establish_xname(irq, pic, pin, type, level, ih_fun, ih_arg,
	    false, xname);
#endif

}

/* Deregister an interrupt handler. */
void
isa_intr_disestablish(isa_chipset_tag_t ic, void *arg)
{
#if !defined(XEN)
	struct intrhand *ih = arg;

	if (!LEGAL_IRQ(ih->ih_pin))
		panic("intr_disestablish: bogus irq");

	intr_disestablish(ih);
#endif	
}

void
isa_attach_hook(device_t parent, device_t self, struct isabus_attach_args *iba)
{
	extern struct x86_isa_chipset x86_isa_chipset;
	extern int isa_has_been_seen;

	/*
	 * Notify others that might need to know that the ISA bus
	 * has now been attached.
	 */
	if (isa_has_been_seen)
		panic("isaattach: ISA bus already seen!");
	isa_has_been_seen = 1;

	/*
	 * Since we can only have one ISA bus, we just use a single
	 * statically allocated ISA chipset structure.  Pass it up
	 * now.
	 */
	iba->iba_ic = &x86_isa_chipset;
}

void
isa_detach_hook(isa_chipset_tag_t ic, device_t self)
{
	extern int isa_has_been_seen;

	isa_has_been_seen = 0;
}

int
isa_mem_alloc(bus_space_tag_t t, bus_size_t size, bus_size_t align,
    bus_addr_t boundary, int flags, bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	/* Allocate physical address space in the ISA hole. */
	return bus_space_alloc(t, IOM_BEGIN, IOM_END - 1, size, align,
	    boundary, flags, addrp, bshp);
}

void
isa_mem_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	bus_space_free(t, bsh, size);
}

/*
 * ISA only has 24-bits of address space.  This means
 * we can't DMA to pages over 16M.  In order to DMA to
 * arbitrary buffers, we use "bounce buffers" - pages
 * in memory below the 16M boundary.  On DMA reads,
 * DMA happens to the bounce buffers, and is copied into
 * the caller's buffer.  On writes, data is copied into
 * but bounce buffer, and the DMA happens from those
 * pages.  To software using the DMA mapping interface,
 * this looks simply like a data cache.
 *
 * If we have more than 16M of RAM in the system, we may
 * need bounce buffers.  We check and remember that here.
 *
 * There are exceptions, however.  VLB devices can do
 * 32-bit DMA, and indicate that here.
 *
 * ...or, there is an opposite case.  The most segments
 * a transfer will require is (maxxfer / PAGE_SIZE) + 1.  If
 * the caller can't handle that many segments (e.g. the
 * ISA DMA controller), we may have to bounce it as well.
 */
static int
_isa_dma_may_bounce(bus_dma_tag_t t, bus_dmamap_t map, int flags,
    int *cookieflagsp)
{
	if ((flags & ISABUS_DMA_32BIT) != 0)
		map->_dm_bounce_thresh = 0;

	if (((map->_dm_size / PAGE_SIZE) + 1) > map->_dm_segcnt)
		*cookieflagsp |= X86_DMA_MIGHT_NEED_BOUNCE;
	return 0;
}

device_t
device_isa_register(device_t dev, void *aux)
{
	/*
	 * Handle network interfaces here, the attachment information is
	 * not available driver-independently later.
	 *
	 * For disks, there is nothing useful available at attach time.
	 */
	if (device_class(dev) == DV_IFNET) {
		struct btinfo_netif *bin = lookup_bootinfo(BTINFO_NETIF);
		if (bin == NULL)
			return NULL;

		/*
		 * We don't check the driver name against the device name
		 * passed by the boot ROM.  The ROM should stay usable if
		 * the driver becomes obsolete.  The physical attachment
		 * information (checked below) must be sufficient to
		 * identify the device.
		 */
		if (bin->bus == BI_BUS_ISA &&
		    device_is_a(device_parent(dev), "isa")) {
			struct isa_attach_args *iaa = aux;

			/* Compare IO base address */
			/* XXXJRT What about multiple IO addrs? */
			if (iaa->ia_nio > 0 &&
			    bin->addr.iobase == iaa->ia_io[0].ir_addr)
			    	return dev;
		}
	}
#if NACPICA > 0
#if notyet
	if (device_is_a(dev, "isa") && acpi_active) {
		if (!(AcpiGbl_FADT.BootFlags & ACPI_FADT_LEGACY_DEVICES))
			prop_dictionary_set_bool(device_properties(dev),
			    "no-legacy-devices", true);
	}
#endif
#endif /* NACPICA > 0 */
	return NULL;
}
