/*	$NetBSD: pchb.c,v 1.13.12.1 2007/03/29 19:27:29 reinoud Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pchb.c,v 1.13.12.1 2007/03/29 19:27:29 reinoud Exp $");

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
#include "agp.h"

int	pchbmatch __P((struct device *, struct cfdata *, void *));
void	pchbattach __P((struct device *, struct device *, void *));

CFATTACH_DECL(pchb, sizeof(struct device),
    pchbmatch, pchbattach, NULL, NULL);

int
pchbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match all known PCI host chipsets.
	 */

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_APPLE:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_APPLE_BANDIT:
		case PCI_PRODUCT_APPLE_BANDIT2:
		case PCI_PRODUCT_APPLE_UNINORTH1:
		case PCI_PRODUCT_APPLE_UNINORTH2:
		case PCI_PRODUCT_APPLE_UNINORTH3:
		case PCI_PRODUCT_APPLE_UNINORTH4:
		case PCI_PRODUCT_APPLE_UNINORTH5:
		case PCI_PRODUCT_APPLE_UNINORTH6:
		case PCI_PRODUCT_APPLE_UNINORTH_AGP:
		case PCI_PRODUCT_APPLE_UNINORTH_AGP2:
		case PCI_PRODUCT_APPLE_UNINORTH_AGP3:
		case PCI_PRODUCT_APPLE_PANGEA_PCI1:
		case PCI_PRODUCT_APPLE_PANGEA_PCI2:
		case PCI_PRODUCT_APPLE_PANGEA_AGP:
		case PCI_PRODUCT_APPLE_U3_PPB1:
		case PCI_PRODUCT_APPLE_U3_PPB2:
		case PCI_PRODUCT_APPLE_U3_PPB3:
		case PCI_PRODUCT_APPLE_U3_PPB4:
		case PCI_PRODUCT_APPLE_U3_PPB5:
		case PCI_PRODUCT_APPLE_INTREPID2_AGP:
		case PCI_PRODUCT_APPLE_INTREPID2_PCI1:
		case PCI_PRODUCT_APPLE_INTREPID2_PCI2:
			return 1;
		}
		break;

	case PCI_VENDOR_MOT:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_MOT_MPC106:
			return 1;
		}
		break;
	}

	return 0;
}

void
pchbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
#if NAGP > 0
	struct agpbus_attach_args apa;
#endif
	char devinfo[256];

	printf("\n");

	/*
	 * All we do is print out a description.  Eventually, we
	 * might want to add code that does something that's
	 * possibly chipset-specific.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));
#if NAGP > 0
	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_AGP,
			       NULL, NULL) != 0) {
		apa.apa_pci_args = *pa;
		config_found_ia(self, "agpbus", &apa, agpbusprint);
	}
#endif
}
