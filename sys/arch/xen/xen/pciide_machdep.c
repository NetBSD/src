/*	$NetBSD: pciide_machdep.c,v 1.6.28.1 2008/01/09 01:50:20 matt Exp $	*/

/*
 * Copyright (c) 1998 Christopher G. Demetriou.  All rights reserved.
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
 * PCI IDE controller driver (i386 machine-dependent portion).
 *
 * Author: Christopher G. Demetriou, March 2, 1998 (derived from NetBSD
 * sys/dev/pci/ppb.c, revision 1.16).
 *
 * See "PCI IDE Controller Specification, Revision 1.0 3/4/94" from the
 * PCI SIG.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciide_machdep.c,v 1.6.28.1 2008/01/09 01:50:20 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <xen/evtchn.h>

#ifdef XEN3
#include "ioapic.h"
#endif

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#endif  

void *
pciide_machdep_compat_intr_establish(dev, pa, chan, func, arg)
	struct device *dev;
	struct pci_attach_args *pa;
	int chan;
	int (*func) __P((void *));
	void *arg;
{
	struct pintrhand *ih;
	char evname[8];
        struct xen_intr_handle xenih;
#if NIOAPIC > 0
	struct pic *pic = NULL;
#endif
	int evtch;

#ifndef XEN3
	physdev_op_t physdev_op;

	physdev_op.cmd = PHYSDEVOP_PCI_INITIALISE_DEVICE;
	physdev_op.u.pci_cfgreg_read.bus = pa->pa_bus;
	physdev_op.u.pci_cfgreg_read.dev = pa->pa_device;
	physdev_op.u.pci_cfgreg_read.func = pa->pa_function;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_PCI_INITIALISE_DEVICE)");
#endif /* !XEN3 */
	xenih.pirq = PCIIDE_COMPAT_IRQ(chan);
#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, xenih.pirq, &xenih) == 0 ||
		    intr_find_mpmapping(mp_eisa_bus, xenih.pirq, &xenih) == 0) {
			if (!APIC_IRQ_ISLEGACY(xenih.pirq)) {
				pic = (struct pic *)
				    ioapic_find(APIC_IRQ_APIC(xenih.pirq));
				if (pic == NULL) {
					printf("pciide_machdep_compat_intr_establish: "
					    "unknown apic %d\n",
					    APIC_IRQ_APIC(xenih.pirq));
					return NULL;
				}
			}
		} else
			printf("pciide_machdep_compat_intr_establish: "
			    "no MP mapping found\n");
	}
#endif
	evtch = xen_intr_map(&xenih.pirq, IST_EDGE);
	if (evtch == -1)
		return NULL;
#if NIOAPIC > 0
	if (pic)
		snprintf(evname, sizeof(evname), "%s pin %d",
		    pic->pic_name, APIC_IRQ_PIN(xenih.pirq));
	else
#endif
		snprintf(evname, sizeof(evname), "irq%d",
		    PCIIDE_COMPAT_IRQ(chan));

	ih = pirq_establish(PCIIDE_COMPAT_IRQ(chan),
	    evtch, func, arg, IPL_BIO, evname);
	if (ih == NULL)
		return NULL;

	printf("%s: %s channel interrupting at ",
	    dev->dv_xname, PCIIDE_CHANNEL_NAME(chan));
#if NIOAPIC > 0
	if (pic)
		printf("%s pin %d", pic->pic_name, APIC_IRQ_PIN(xenih.pirq));
	else
#endif
		printf("irq %d", ih->pirq);
	printf(", event channel %d\n", ih->evtch);
	return (void *)ih;
}
