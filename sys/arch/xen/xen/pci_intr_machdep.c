/*      $NetBSD: pci_intr_machdep.c,v 1.16.6.1 2015/04/06 15:18:04 skrll Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_intr_machdep.c,v 1.16.6.1 2015/04/06 15:18:04 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <machine/bus_private.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <xen/evtchn.h>

#include "locators.h"
#include "opt_ddb.h"
#include "ioapic.h"
#include "acpica.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpconfig.h>
#include <machine/pic.h>
#endif

#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

#if NACPICA > 0
#include <machine/mpacpi.h>
#endif

int
pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin;
	int line;

#if NIOAPIC > 0
	int rawpin = pa->pa_rawintrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int bus, dev, func;
#endif

	pin = pa->pa_intrpin;
	line = pa->pa_intrline;
#if 0 /* XXXX why is it always 0 ? */
	if (pin == 0) {
		/* No IRQ used */
		goto bad;
	}
#endif
	if (pin > PCI_INTERRUPT_PIN_MAX) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}
	ihp->pirq = 0;

#if NIOAPIC > 0
	pci_decompose_tag(pc, pa->pa_tag, &bus, &dev, &func);
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(bus, (dev<<2)|(rawpin-1), ihp) == 0) {
			if (ihp->pirq & APIC_INT_VIA_APIC) {
				/* make sure a new IRQ will be allocated */
				ihp->pirq &= ~0xff;
			} else {
				ihp->pirq |= line;
			}
			goto end;
		}
		/*
		 * No explicit PCI mapping found. This is not fatal,
		 * we'll try the ISA (or possibly EISA) mappings next.
		 */
	}
#endif

	if (line == 0 || line == X86_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		printf("pci_intr_map: no mapping for pin %c (line=%02x)\n",
		    '@' + pin, line);
		goto bad;
	}
#ifdef DOM0OPS
	if (line >= NUM_LEGACY_IRQS) {
		printf("pci_intr_map: bad interrupt line %d\n", line);
		goto bad;
	}
#endif
	if (line == 2) {
		printf("pci_intr_map: changed line 2 to line 9\n");
		line = 9;
	}
#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, line, ihp) == 0) {
			if ((ihp->pirq & 0xff) == 0)
				ihp->pirq |= line;
			goto end;
		}
		printf("pci_intr_map: bus %d dev %d func %d pin %d; line %d\n",
		    bus, dev, func, pin, line);
		printf("pci_intr_map: no MP mapping found\n");
	}
#endif /* NIOAPIC */

	ihp->pirq = line;

#if NIOAPIC > 0
end:
#endif
	ihp->evtch = xen_intr_map(&ihp->pirq, IST_LEVEL);
	if (ihp->evtch == -1)
		goto bad;

	return 0;

bad:
	ihp->pirq = -1;
	ihp->evtch = -1;
	return 1;
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
#if NIOAPIC > 0
	struct ioapic_softc *pic;
	if (ih.pirq & APIC_INT_VIA_APIC) {
		pic = ioapic_find(APIC_IRQ_APIC(ih.pirq));
		if (pic == NULL) {
			printf("%s: bad ioapic %d\n", __func__,
			    APIC_IRQ_APIC(ih.pirq));
			return NULL;
		}
		snprintf(buf, len, "%s pin %d, event channel %d",
		    device_xname(pic->sc_dev), APIC_IRQ_PIN(ih.pirq),
		    ih.evtch);
		return buf;
	}
#endif
	snprintf(buf, len, "irq %d, event channel %d",
	    ih.pirq, ih.evtch);
	return buf;
}

const struct evcnt*
pci_intr_evcnt(pci_chipset_tag_t pcitag, pci_intr_handle_t intrh)
{
	return NULL;
}

int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ih,
		 int attr, uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		return 0;
	default:
		return ENODEV;
	}
}

void *
pci_intr_establish(pci_chipset_tag_t pcitag, pci_intr_handle_t intrh,
    int level, int (*func)(void *), void *arg)
{
	char evname[16];
#if NIOAPIC > 0
	struct ioapic_softc *pic;
	if (intrh.pirq & APIC_INT_VIA_APIC) {
		pic = ioapic_find(APIC_IRQ_APIC(intrh.pirq));
		if (pic == NULL) {
			printf("pci_intr_establish: bad ioapic %d\n",
			    APIC_IRQ_APIC(intrh.pirq));
			return NULL;
		}
		snprintf(evname, sizeof(evname), "%s pin %d",
		    device_xname(pic->sc_dev), APIC_IRQ_PIN(intrh.pirq));
	} else
#endif
		snprintf(evname, sizeof(evname), "irq%d", intrh.pirq);

	return (void *)pirq_establish(intrh.pirq & 0xff,
	    intrh.evtch, func, arg, level, evname);
}

void
pci_intr_disestablish(pci_chipset_tag_t pcitag, void *cookie)
{
	pirq_disestablish(cookie);
}
