/*	$NetBSD: if_epic_pci.c,v 1.7.12.3 2000/07/15 22:48:01 tron Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
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
 * PCI bus front-end for the Standard Microsystems Corp. 83C170
 * Ethernet PCI Integrated Controller (EPIC/100) driver.
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
 
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#ifdef INET
#include <netinet/in.h> 
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/ic/smc83c170reg.h>
#include <dev/ic/smc83c170var.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI configuration space registers used by the EPIC.
 */
#define	EPIC_PCI_IOBA		0x10	/* i/o mapped base */
#define	EPIC_PCI_MMBA		0x14	/* memory mapped base */

struct epic_pci_softc {
	struct epic_softc sc_epic;	/* real EPIC softc */

	/* PCI-specific goo. */
	void	*sc_ih;			/* interrupt handle */
};

int	epic_pci_match __P((struct device *, struct cfdata *, void *));
void	epic_pci_attach __P((struct device *, struct device *, void *));

struct cfattach epic_pci_ca = {
	sizeof(struct epic_pci_softc), epic_pci_match, epic_pci_attach,
};

const struct epic_pci_product {
	u_int32_t	epp_prodid;	/* PCI product ID */
	const char	*epp_name;	/* device name */
} epic_pci_products[] = {
	{ PCI_PRODUCT_SMC_83C170,	"SMC 83c170 Fast Ethernet" },
	{ PCI_PRODUCT_SMC_83C175,	"SMC 83c175 Fast Ethernet" },
	{ 0,				NULL },
};

const struct epic_pci_product *epic_pci_lookup
    __P((const struct pci_attach_args *));

const struct epic_pci_product *
epic_pci_lookup(pa)
	const struct pci_attach_args *pa;
{
	const struct epic_pci_product *epp;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_SMC)
		return (NULL);

	for (epp = epic_pci_products; epp->epp_name != NULL; epp++)
		if (PCI_PRODUCT(pa->pa_id) == epp->epp_prodid)
			return (epp);

	return (NULL);
}

int
epic_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (epic_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
epic_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct epic_pci_softc *psc = (struct epic_pci_softc *)self;
	struct epic_softc *sc = &psc->sc_epic;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	const struct epic_pci_product *epp;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	pcireg_t reg;
	int pmreg, ioh_valid, memh_valid;

	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, &pmreg, 0)) {
		reg = pci_conf_read(pc, pa->pa_tag, pmreg + 4) & 0x3;
		if (reg == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf(": unable to wake up from power state D3\n");
			return;
		}
		if (reg != 0) {
			printf(": waking up from power state D%d\n%s",
			    reg, sc->sc_dev.dv_xname);
			pci_conf_write(pc, pa->pa_tag, pmreg + 4, 0);
		}
	}

	/*
	 * Map the device.
	 */
	ioh_valid = (pci_mapreg_map(pa, EPIC_PCI_IOBA,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, EPIC_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);

	if (memh_valid) {
		sc->sc_st = memt;
		sc->sc_sh = memh;
	} else if (ioh_valid) {
		sc->sc_st = iot;
		sc->sc_sh = ioh;
	} else {
		printf(": unable to map device registers\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	epp = epic_pci_lookup(pa);
	if (epp == NULL) {
		printf("\n");
		panic("epic_pci_attach: impossible");
	}

	printf(": %s, rev. %d\n", epp->epp_name, PCI_REVISION(pa->pa_class));

	/* Make sure bus mastering is enabled. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih); 
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, epic_intr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * Finish off the attach.
	 */
	epic_attach(sc);
}
