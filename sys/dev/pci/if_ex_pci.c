/*	$NetBSD: if_ex_pci.c,v 1.1 1998/11/04 00:31:05 fvdl Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h" 
 
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

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h> 
#include <netinet/if_inarp.h>
#endif
 
#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif
  
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>
#include <dev/mii/mii.h>

#include <dev/ic/elink3var.h>
#include <dev/ic/elink3reg.h>
#include <dev/ic/elinkxlreg.h>
#include <dev/ic/elinkxlvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI constants.
 * XXX These should be in a common file!
 */
#define PCI_CONN		0x48    /* Connector type */
#define PCI_CBIO		0x10    /* Configuration Base IO Address */
#define PCI_CAPPTR		0x34
#define PCI_CAPID		0xdc
#define PCI_POWERCAP		0xde
#define PCI_POWERCTL		0xe0

int ex_pci_match __P((struct device *, struct cfdata *, void *));
void ex_pci_attach __P((struct device *, struct device *, void *));

struct cfattach ex_pci_ca = {
	sizeof(struct ex_softc), ex_pci_match, ex_pci_attach
};

int
ex_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_3COM)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_3COM_3C900TPO:
	case PCI_PRODUCT_3COM_3C900COMBO:
	case PCI_PRODUCT_3COM_3C905TX:
	case PCI_PRODUCT_3COM_3C905T4:
	case PCI_PRODUCT_3COM_3C900BTPO:
	case PCI_PRODUCT_3COM_3C900BCOMBO:
	case PCI_PRODUCT_3COM_3C905BT4:
	case PCI_PRODUCT_3COM_3C905BTX:
		break;
	default:
		return 0;
	}

	return 1;
}

void
ex_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ex_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	char *model;
	const char *intrstr = NULL;
	int pmode;

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	sc->ex_bustype = EX_BUS_PCI;
	sc->ex_conf = 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_3COM_3C900TPO:
	case PCI_PRODUCT_3COM_3C900COMBO:
		model = "3Com 3C900 Ethernet";
		break;
	case PCI_PRODUCT_3COM_3C905TX:
	case PCI_PRODUCT_3COM_3C905T4:
		sc->ex_conf = EX_CONF_MII;
		model = "3Com 3C905 Ethernet";
		break;
	case PCI_PRODUCT_3COM_3C900BTPO:
	case PCI_PRODUCT_3COM_3C900BCOMBO:
		sc->ex_conf = EX_CONF_90XB;
		model = "3Com 3C900B Ethernet";
		break;
	case PCI_PRODUCT_3COM_3C905BTX:
	case PCI_PRODUCT_3COM_3C905BT4:
		sc->ex_conf = (EX_CONF_MII | EX_CONF_90XB);
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3COM_3C905BTX)
			sc->ex_conf |= EX_CONF_INTPHY;
		model = "3Com 3C905B Ethernet";
		break;
	default:
		model = "unknown model!";
		break;
	}

	printf(": %s\n", model);

	sc->enable = NULL;
	sc->disable = NULL;
	sc->enabled = 1;

	/* Enable the card. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Get it out of power save mode if needed (BIOS bugs) */
	if ((pci_conf_read(pc, pa->pa_tag, PCI_CAPPTR) & 0xff) == PCI_CAPID &&
	    (pci_conf_read(pc, pa->pa_tag, PCI_CAPID) & 0xff) == 0x01) {
		pmode = pci_conf_read(pc, pa->pa_tag, PCI_POWERCTL) & 0x3;
		if (pmode == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake up from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		if (pmode != 0) {
			printf("%s: waking up from power state D%d\n",
			    sc->sc_dev.dv_xname, pmode);
			pci_conf_write(pc, pa->pa_tag, PCI_POWERCTL, 0);
		}
	}

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, ex_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	ex_config(sc);

}
