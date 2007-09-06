/*	$NetBSD: pchb.c,v 1.7.22.2 2007/09/06 22:12:53 jmcneill Exp $	*/

/*-
 * Copyright (c) 1996, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pchb.c,v 1.7.22.2 2007/09/06 22:12:53 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include <arch/x86/pci/pchbvar.h>

#include <dev/pci/agpreg.h>
#include <dev/pci/agpvar.h>

#include "rnd.h"

#define PCISET_BRIDGETYPE_MASK	0x3
#define PCISET_TYPE_COMPAT	0x1
#define PCISET_TYPE_AUX		0x2

#define PCISET_BUSCONFIG_REG	0x48
#define PCISET_BRIDGE_NUMBER(reg)	(((reg) >> 8) & 0xff)
#define PCISET_PCI_BUS_NUMBER(reg)	(((reg) >> 16) & 0xff)

/* XXX should be in dev/ic/i82443reg.h */
#define	I82443BX_SDRAMC_REG	0x76

/* XXX should be in dev/ic/i82424{reg.var}.h */
#define I82424_CPU_BCTL_REG		0x53
#define I82424_PCI_BCTL_REG		0x54

#define I82424_BCTL_CPUMEM_POSTEN	0x01
#define I82424_BCTL_CPUPCI_POSTEN	0x02
#define I82424_BCTL_PCIMEM_BURSTEN	0x01
#define I82424_BCTL_PCI_BURSTEN		0x02

int	pchbmatch(struct device *, struct cfdata *, void *);
void	pchbattach(struct device *, struct device *, void *);

CFATTACH_DECL(pchb, sizeof(struct pchb_softc),
    pchbmatch, pchbattach, NULL, NULL);

int
pchbmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_HOST) {
		return (1);
	}

	return (0);
}

void
pchbattach(struct device *parent, struct device *self, void *aux)
{
#if NRND > 0
	struct pchb_softc *sc = (void *) self;
#endif
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	struct pcibus_attach_args pba;
	struct agpbus_attach_args apa;
	u_char pbnum = 0; /* XXX: gcc */
	int doattach, attachflags, has_agp;

	printf("\n");

	doattach = 0;
	has_agp = 0;
	attachflags = pa->pa_flags;

	/*
	 * Print out a description, and configure certain chipsets which
	 * have auxiliary PCI buses.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82810_MCH:
		case PCI_PRODUCT_INTEL_82810_DC100_MCH:
		case PCI_PRODUCT_INTEL_82810E_MCH:
		case PCI_PRODUCT_INTEL_82815_FULL_HUB:
		case PCI_PRODUCT_INTEL_82830MP_IO_1:
		case PCI_PRODUCT_INTEL_82845G_DRAM:
		case PCI_PRODUCT_INTEL_82855GM_MCH:
		case PCI_PRODUCT_INTEL_82865_HB:
		case PCI_PRODUCT_INTEL_82915G_HB:
		case PCI_PRODUCT_INTEL_82915GM_HB:
		case PCI_PRODUCT_INTEL_82945P_MCH:
		case PCI_PRODUCT_INTEL_82945GM_HB:
		case PCI_PRODUCT_INTEL_82965Q_HB:
		case PCI_PRODUCT_INTEL_82965PM_HB:
		case PCI_PRODUCT_INTEL_82G33_HB:
			/*
			 * The host bridge is either in GFX mode (internal
			 * graphics) or in AGP mode. In GFX mode, we pretend
			 * to have AGP because the graphics memory access
			 * is very similar and the AGP GATT code will
			 * deal with this. In the latter case, the
			 * pci_get_capability(PCI_CAP_AGP) test below will
			 * fire, so we do no harm by already setting the flag.
			 */
			has_agp = 1;
			break;
		}
		break;
	}

#if NRND > 0
	/*
	 * Attach a random number generator, if there is one.
	 */
	pchb_attach_rnd(sc, pa);
#endif

	/*
	 * If we haven't detected AGP yet (via a product ID),
	 * then check for AGP capability on the device.
	 */
	if (has_agp ||
	    pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_AGP,
			       NULL, NULL) != 0) {
		apa.apa_pci_args = *pa;
		config_found_ia(self, "agpbus", &apa, agpbusprint);
	}

	if (doattach) {
		pba.pba_iot = pa->pa_iot;
		pba.pba_memt = pa->pa_memt;
		pba.pba_dmat = pa->pa_dmat;
		pba.pba_dmat64 = pa->pa_dmat64;
		pba.pba_pc = pa->pa_pc;
		pba.pba_flags = attachflags;
		pba.pba_bus = pbnum;
		pba.pba_bridgetag = NULL;
		pba.pba_pc = pa->pa_pc;
		pba.pba_intrswiz = 0;
		memset(&pba.pba_intrtag, 0, sizeof(pba.pba_intrtag));
		config_found_ia(self, "pcibus", &pba, pcibusprint);
	}
}
