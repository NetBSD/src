/*	$NetBSD: pciide_machdep.c,v 1.5 2006/01/15 22:09:52 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pciide_machdep.c,v 1.5 2006/01/15 22:09:52 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <machine/evtchn.h>

void *
pciide_machdep_compat_intr_establish(dev, pa, chan, func, arg)
	struct device *dev;
	struct pci_attach_args *pa;
	int chan;
	int (*func) __P((void *));
	void *arg;
{
	struct pintrhand *ih;
#ifndef XEN3
	physdev_op_t physdev_op;

	physdev_op.cmd = PHYSDEVOP_PCI_INITIALISE_DEVICE;
	physdev_op.u.pci_cfgreg_read.bus = pa->pa_bus;
	physdev_op.u.pci_cfgreg_read.dev = pa->pa_device;
	physdev_op.u.pci_cfgreg_read.func = pa->pa_function;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_PCI_INITIALISE_DEVICE)");
#endif /* !XEN3 */

	ih = pirq_establish(PCIIDE_COMPAT_IRQ(chan),
	    bind_pirq_to_evtch(PCIIDE_COMPAT_IRQ(chan)), func, arg, IPL_BIO);
	if (ih == NULL)
		return NULL;

	printf("%s: %s channel using event channel %d for irq %d\n",
	    dev->dv_xname, PCIIDE_CHANNEL_NAME(chan), ih->evtch, ih->pirq);
	return (void *)ih;
}
