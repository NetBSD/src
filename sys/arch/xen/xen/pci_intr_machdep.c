/*      $NetBSD: pci_intr_machdep.c,v 1.1.6.2 2006/04/22 11:38:11 simonb Exp $      */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/bus_private.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/evtchn.h>

#include "locators.h"
#include "opt_ddb.h"

int
pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcireg_t intr;
	int pin;
	int line;

#ifndef XEN3
	physdev_op_t physdev_op;
	/* initialise device, to get the real IRQ */
	physdev_op.cmd = PHYSDEVOP_PCI_INITIALISE_DEVICE;
	physdev_op.u.pci_initialise_device.bus = pa->pa_bus;
	physdev_op.u.pci_initialise_device.dev = pa->pa_device;
	physdev_op.u.pci_initialise_device.func = pa->pa_function;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_PCI_INITIALISE_DEVICE)");
#endif /* !XEN3 */

	intr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG);
	pin = pa->pa_intrpin;
	pa->pa_intrline = line = PCI_INTERRUPT_LINE(intr);
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

	if (line == 0 || line == X86_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		printf("pci_intr_map: no mapping for pin %c (line=%02x)\n",
		    '@' + pin, line);
		goto bad;
	}

	ihp->pirq = line;
	ihp->evtch = bind_pirq_to_evtch(ihp->pirq);
	if (ihp->evtch == -1)
		goto bad;

	return 0;

bad:
	ihp->pirq = -1;
	ihp->evtch = -1;
	return 1;
}
const char
*pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char buf[64];
	snprintf(buf, 64, "irq %d, event channel %d",
	    ih.pirq, ih.evtch);
	return buf;
	
}

const struct evcnt*
pci_intr_evcnt(pci_chipset_tag_t pcitag, pci_intr_handle_t intrh)
{
	return NULL;
}

void *
pci_intr_establish(pci_chipset_tag_t pcitag, pci_intr_handle_t intrh,
    int level, int (*func)(void *), void *arg)
{
	return (void *)pirq_establish(intrh.pirq, intrh.evtch, func, arg, level);
}

void
pci_intr_disestablish(pci_chipset_tag_t pcitag, void *cookie)
{
}
