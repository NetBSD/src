/* $NetBSD: p64h2apic.c,v 1.8.6.1 2006/04/22 11:37:34 simonb Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Driver for P64H2 IOxAPIC
 *
 * This is an ioapic which appears in PCI space.
 * This driver currently does nothing useful but will eventually
 * thwak the ioapic into working correctly.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: p64h2apic.c,v 1.8.6.1 2006/04/22 11:37:34 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static int	p64h2match(struct device *, struct cfdata *, void *);
static void	p64h2attach(struct device *, struct device *, void *);

struct p64h2apic_softc {
	struct device sc_dev;
	pcitag_t sc_tag;
};

CFATTACH_DECL(p64h2apic, sizeof(struct p64h2apic_softc),
    p64h2match, p64h2attach, NULL, NULL);

int	p64h2print(void *, const char *pnp);


static int
p64h2match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82870P2_IOxAPIC)
		return (1);

	return (0);
}

static void
p64h2attach(struct device *parent, struct device *self, void *aux)
{
	struct p64h2apic_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	aprint_naive("\n");
	aprint_normal("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal("%s: %s (rev. 0x%02x)\n", sc->sc_dev.dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_tag = pa->pa_tag;
}
