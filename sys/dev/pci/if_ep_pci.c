/*	$NetBSD: if_ep_pci.c,v 1.33.2.2 2001/08/24 00:10:03 nathanw Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1997 Jonathan Stone <jonathan@NetBSD.org>
 * Copyright (c) 1994 Herb Peyerl <hpeyerl@beer.org>
 * All rights reserved.
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
 *      This product includes software developed by Herb Peyerl.
 * 4. The name of Herb Peyerl may not be used to endorse or promote products
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h> 
#include <sys/socket.h> 
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI constants.
 */
#define PCI_CBIO		0x10    /* Configuration Base IO Address */

int ep_pci_match __P((struct device *, struct cfdata *, void *));
void ep_pci_attach __P((struct device *, struct device *, void *));

struct cfattach ep_pci_ca = {
	sizeof(struct ep_softc), ep_pci_match, ep_pci_attach
};

struct ep_pci_product {
	u_int32_t	epp_prodid;	/* PCI product ID */
	u_short		epp_chipset;	/* 3Com chipset used */
	int		epp_flags;	/* initial softc flags */
	const char	*epp_name;	/* device name */
} ep_pci_products[] = {
	{ PCI_PRODUCT_3COM_3C590,	ELINK_CHIPSET_VORTEX,
	  0,
	  "3c590 Ethernet" },

	/*
	 * Note: The 3c595-MII is an MII connector for an
	 * external PHY.  We treat it as `manual' in the
	 * core driver.
	 */
	{ PCI_PRODUCT_3COM_3C595TX,	ELINK_CHIPSET_VORTEX,
	  0,
	  "3c595-TX 10/100 Ethernet" },
	{ PCI_PRODUCT_3COM_3C595T4,	ELINK_CHIPSET_VORTEX,
	  0,
	  "3c595-T4 10/100 Ethernet" },
	{ PCI_PRODUCT_3COM_3C595MII,	ELINK_CHIPSET_VORTEX,
	  0,
	  "3c595-MII 10/100 Ethernet" },

	{ PCI_PRODUCT_3COM_3C900TPO,	ELINK_CHIPSET_BOOMERANG,
	  0,
	  "3c900-TPO Ethernet" },
	{ PCI_PRODUCT_3COM_3C900COMBO,	ELINK_CHIPSET_BOOMERANG,
	  0,
	  "3c900-COMBO Ethernet" },

	{ PCI_PRODUCT_3COM_3C905TX,	ELINK_CHIPSET_BOOMERANG,
	  ELINK_FLAGS_MII,
	  "3c905-TX 10/100 Ethernet" },
	{ PCI_PRODUCT_3COM_3C905T4,	ELINK_CHIPSET_BOOMERANG,
	  ELINK_FLAGS_MII,
	  "3c905-T4 10/100 Ethernet" },

	{ 0,				0,
	  0,				NULL },
};

const struct ep_pci_product *ep_pci_lookup
    __P((const struct pci_attach_args *));

const struct ep_pci_product *
ep_pci_lookup(pa)
	const struct pci_attach_args *pa;
{
	struct ep_pci_product *epp;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_3COM)
		return (NULL);

	for (epp = ep_pci_products; epp->epp_name != NULL; epp++)
		if (PCI_PRODUCT(pa->pa_id) == epp->epp_prodid)
			return (epp);

	return (NULL);
}

int
ep_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (ep_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
ep_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ep_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const struct ep_pci_product *epp;
	const char *intrstr = NULL;

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		printf(": can't map i/o space\n");
		return;
	}

	epp = ep_pci_lookup(pa);
	if (epp == NULL) {
		printf("\n");
		panic("ep_pci_attach: impossible");
	}

	printf(": 3Com %s\n", epp->epp_name);

	sc->enable = NULL;
	sc->disable = NULL;
	sc->enabled = 1;

	sc->bustype = ELINK_BUS_PCI;
	sc->ep_flags = epp->epp_flags;

	/* Enable the card. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, epintr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	epconfig(sc, epp->epp_chipset, NULL);
}
