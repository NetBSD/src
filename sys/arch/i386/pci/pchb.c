/*	$NetBSD: pchb.c,v 1.7 1997/08/03 23:42:05 fvdl Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
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

#define PCISET_BRIDGETYPE_MASK	0x3
#define PCISET_TYPE_COMPAT	0x1
#define PCISET_TYPE_AUX		0x2

#define PCISET_BUSCONFIG_REG	0x48
#define PCISET_BRIDGE_NUMBER(reg)	(((reg) >> 8) & 0xff)
#define PCISET_PCI_BUS_NUMBER(reg)	(((reg) >> 16) & 0xff)

int	pchbmatch __P((struct device *, void *, void *));
void	pchbattach __P((struct device *, struct device *, void *));

int	pchb_print __P((void *, const char *));

struct cfattach pchb_ca = {
	sizeof(struct device), pchbmatch, pchbattach
};

struct cfdriver pchb_cd = {
	NULL, "pchb", DV_DULL
};

int
pchbmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match all known PCI host chipsets.
	 */
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_PCMC:
		case PCI_PRODUCT_INTEL_82437:
		case PCI_PRODUCT_INTEL_82437MX:
		case PCI_PRODUCT_INTEL_82439:
		case PCI_PRODUCT_INTEL_82440FX:
		case PCI_PRODUCT_INTEL_82430VX:
		case PCI_PRODUCT_INTEL_CDC:
		case PCI_PRODUCT_INTEL_PCI450_PB:
		case PCI_PRODUCT_INTEL_PCI450_MC:
			return (1);
		}
		break;
	case PCI_VENDOR_UMC:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_UMC_UM8881F:
			return (1);
		}
		break;
	case PCI_VENDOR_ACC:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ACC_2188:
			return (1);
		}
		break;
	case PCI_VENDOR_ACER:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ACER_M1435:
			return (1);
		}
		break;
	case PCI_VENDOR_ALI:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ALI_M1445:
		case PCI_PRODUCT_ALI_M1451:
		case PCI_PRODUCT_ALI_M1461:
			return (1);
		}
		break;
	case PCI_VENDOR_COMPAQ:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_COMPAQ_TRIFLEX1:
		case PCI_PRODUCT_COMPAQ_TRIFLEX2:
		case PCI_PRODUCT_COMPAQ_TRIFLEX4:
			return (1);
		}
		break;
	case PCI_VENDOR_NEXGEN:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_NEXGEN_NX82C501:
			return (1);
		}
		break;
	case PCI_VENDOR_NKK:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_NKK_NDR4600:
			return (1);
		}
		break;
	case PCI_VENDOR_TOSHIBA:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_TOSHIBA_R4X00:
			return (1);
		}
		break;
	case PCI_VENDOR_VIATECH:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_VIATECH_VT82C570M:
		case PCI_PRODUCT_VIATECH_VT82C595:
			return (1);
		}
		break;
	}

	return (0);
}

void
pchbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	struct pcibus_attach_args pba;
	pcireg_t bcreg;
	u_char bdnum, pbnum;

	printf("\n");

	/*
	 * Print out a description, and configure certain chipsets which
	 * have auxiliary PCI buses.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
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
				pba.pba_busname = "pci";
				pba.pba_iot = pa->pa_iot;
				pba.pba_memt = pa->pa_memt;
				pba.pba_dmat = pa->pa_dmat;
				pba.pba_bus = pbnum;
				pba.pba_flags = pa->pa_flags;
				pba.pba_pc = pa->pa_pc;
				config_found(self, &pba, pchb_print);
				break;
			}
			break;
		}
	}
}

int
pchb_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}
