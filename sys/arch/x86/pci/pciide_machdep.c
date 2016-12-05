/*	$NetBSD: pciide_machdep.c,v 1.13.4.3 2016/12/05 10:54:59 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pciide_machdep.c,v 1.13.4.3 2016/12/05 10:54:59 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <dev/isa/isavar.h>

#include "ioapic.h"

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#endif

void *
pciide_machdep_compat_intr_establish(device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *),
    void *arg)
{
	int irq;
	void *cookie;
#if NIOAPIC > 0
	intr_handle_t mpih;
	char buf[PCI_INTRSTR_LEN];
#endif
	char intr_xname[64];

	snprintf(intr_xname, sizeof(intr_xname), "%s %s",
	    device_xname(dev), PCIIDE_CHANNEL_NAME(chan));

	irq = PCIIDE_COMPAT_IRQ(chan);
	cookie = isa_intr_establish_xname(NULL, irq, IST_EDGE, IPL_BIO,
	    func, arg, intr_xname);
	if (cookie == NULL)
		return NULL;
#if NIOAPIC > 0
	if (mp_busses != NULL &&
	    (intr_find_mpmapping(mp_isa_bus, irq, &mpih) == 0 ||
	     intr_find_mpmapping(mp_eisa_bus, irq, &mpih) == 0)) {
		mpih |= irq;
		aprint_normal_dev(dev, "%s channel interrupting at %s\n",
		    PCIIDE_CHANNEL_NAME(chan),
		    intr_string(mpih, buf, sizeof(buf)));
	} else
#endif
	aprint_normal_dev(dev, "%s channel interrupting at irq %d\n",
	    PCIIDE_CHANNEL_NAME(chan), irq);
	return cookie;
}

void
pciide_machdep_compat_intr_disestablish(device_t dev, pci_chipset_tag_t pc,
    int chan, void *cookie)
{
	isa_intr_disestablish(NULL, cookie);
	return;
}
