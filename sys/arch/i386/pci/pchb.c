/*	$NetBSD: pchb.c,v 1.28 2001/09/17 12:07:32 drochner Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include <dev/pci/agpreg.h>
#include <dev/pci/agpvar.h>

#include <arch/i386/pci/pchbvar.h>

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

int	pchbmatch __P((struct device *, struct cfdata *, void *));
void	pchbattach __P((struct device *, struct device *, void *));

int	pchb_print __P((void *, const char *));
int	agp_print __P((void *, const char *));

struct cfattach pchb_ca = {
	sizeof(struct pchb_softc), pchbmatch, pchbattach
};

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
	pcireg_t bcreg;
	u_char bdnum, pbnum;
	pcitag_t tag;
	int doattach, attachflags, has_agp;

	printf("\n");
	doattach = 0;
	has_agp = 0;
	attachflags = pa->pa_flags;

	/*
	 * Print out a description, and configure certain chipsets which
	 * have auxiliary PCI buses.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_SERVERWORKS:
		pbnum = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x44) & 0xff;

		if (pbnum == 0)
			break;

		/*
		 * This host bridge has a second PCI bus.
		 * Configure it.
		 */
		doattach = 1;
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_SERVERWORKS_XX5:
		case PCI_PRODUCT_SERVERWORKS_CNB20HE:
		case PCI_PRODUCT_SERVERWORKS_CIOB20:
			if ((attachflags &
			    (PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED)) ==
			    PCI_FLAGS_MEM_ENABLED)
				attachflags |= PCI_FLAGS_IO_ENABLED;
			break;
		}
		break;

	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82443BX_AGP:
		case PCI_PRODUCT_INTEL_82443BX_NOAGP:
			/*
			 * BIOS BUG WORKAROUND!  The 82443BX
			 * datasheet indicates that the only
			 * legal setting for the "Idle/Pipeline
			 * DRAM Leadoff Timing (IPLDT)" parameter
			 * (bits 9:8) is 01.  Unfortunately, some
			 * BIOSs do not set these bits properly.
			 */
			bcreg = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    I82443BX_SDRAMC_REG);
			if ((bcreg & 0x0300) != 0x0100) {
				printf("%s: fixing Idle/Pipeline DRAM "
				    "Leadoff Timing\n", self->dv_xname);
				bcreg &= ~0x0300;
				bcreg |=  0x0100;
				pci_conf_write(pa->pa_pc, pa->pa_tag,
				    I82443BX_SDRAMC_REG, bcreg);
			}
			break;

		case PCI_PRODUCT_INTEL_PCI450_PB:
			bcreg = pci_conf_read(pa->pa_pc, pa->pa_tag,
					      PCISET_BUSCONFIG_REG);
			bdnum = PCISET_BRIDGE_NUMBER(bcreg);
			pbnum = PCISET_PCI_BUS_NUMBER(bcreg);
			switch (bdnum & PCISET_BRIDGETYPE_MASK) {
			default:
				printf("%s: bdnum=%x (reserved)\n",
				       self->dv_xname, bdnum);
				break;
			case PCISET_TYPE_COMPAT:
				printf("%s: Compatibility PB (bus %d)\n",
				       self->dv_xname, pbnum);
				break;
			case PCISET_TYPE_AUX:
				printf("%s: Auxiliary PB (bus %d)\n",
				       self->dv_xname, pbnum);
				/*
				 * This host bridge has a second PCI bus.
				 * Configure it.
				 */
				doattach = 1;
				break;
			}
			break;
		case PCI_PRODUCT_INTEL_CDC:
			bcreg = pci_conf_read(pa->pa_pc, pa->pa_tag,
					      I82424_CPU_BCTL_REG);
			if (bcreg & I82424_BCTL_CPUPCI_POSTEN) {
				bcreg &= ~I82424_BCTL_CPUPCI_POSTEN;
				pci_conf_write(pa->pa_pc, pa->pa_tag,
					       I82424_CPU_BCTL_REG, bcreg);
				printf("%s: disabled CPU-PCI write posting\n",
					self->dv_xname);
			}
			break;
		case PCI_PRODUCT_INTEL_82451NX_PXB:
			/*
			 * The NX chipset supports up to 2 "PXB" chips
			 * which can drive 2 PCI buses each. Each bus
			 * shows up as logical PCI device, with fixed
			 * device numbers between 18 and 21.
			 * See the datasheet at
		ftp://download.intel.com/design/chipsets/datashts/24377102.pdf
			 * for details.
			 * (It would be easier to attach all the buses
			 * at the MIOC, but less aesthetical imho.)
			 */
			pbnum = 0;
			switch (pa->pa_device) {
			case 18: /* PXB 0 bus A - primary bus */
				break;
			case 19: /* PXB 0 bus B */
				/* read SUBA0 from MIOC */
				tag = pci_make_tag(pa->pa_pc, 0, 16, 0);
				bcreg = pci_conf_read(pa->pa_pc, tag, 0xd0);
				pbnum = ((bcreg & 0x0000ff00) >> 8) + 1;
				break;
			case 20: /* PXB 1 bus A */
				/* read BUSNO1 from MIOC */
				tag = pci_make_tag(pa->pa_pc, 0, 16, 0);
				bcreg = pci_conf_read(pa->pa_pc, tag, 0xd0);
				pbnum = (bcreg & 0xff000000) >> 24;
				break;
			case 21: /* PXB 1 bus B */
				/* read SUBA1 from MIOC */
				tag = pci_make_tag(pa->pa_pc, 0, 16, 0);
				bcreg = pci_conf_read(pa->pa_pc, tag, 0xd4);
				pbnum = (bcreg & 0x000000ff) + 1;
				break;
			}
			if (pbnum != 0)
				doattach = 1;
			break;

		case PCI_PRODUCT_INTEL_82810_MCH:
		case PCI_PRODUCT_INTEL_82810_DC100_MCH:
		case PCI_PRODUCT_INTEL_82810E_MCH:
		case PCI_PRODUCT_INTEL_82815_FULL_HUB:
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
		apa.apa_busname = "agp";
		apa.apa_pci_args = *pa;
		config_found(self, &apa, agp_print);
	}

	if (doattach) {
		pba.pba_busname = "pci";
		pba.pba_iot = pa->pa_iot;
		pba.pba_memt = pa->pa_memt;
		pba.pba_dmat = pa->pa_dmat;
		pba.pba_bus = pbnum;
		pba.pba_flags = attachflags;
		pba.pba_pc = pa->pa_pc;
		config_found(self, &pba, pchb_print);
	}
}

int
pchb_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

int
agp_print(void *aux, const char *pnp)
{
	struct agpbus_attach_args *apa = aux;
	if (pnp)
		printf("%s at %s", apa->apa_busname, pnp);
	return (UNCONF);
}
