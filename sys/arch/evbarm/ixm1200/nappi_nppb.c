/*	$NetBSD: nappi_nppb.c,v 1.6 2003/03/25 06:53:16 igy Exp $ */
/*
 * Copyright (c) 2002, 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nappi_nppb.c,v 1.6 2003/03/25 06:53:16 igy Exp $");

#include "pci.h"
#include "opt_pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

static int	nppbmatch(struct device *, struct cfdata *, void *);
static void	nppbattach(struct device *, struct device *, void *);

int	nppb_intr(void *); /* XXX into i21555var.h */

CFATTACH_DECL(nppb, sizeof(struct device),
    nppbmatch, nppbattach, NULL, NULL);

#define NPPB_MMBA	0x10
#define NPPB_IOBA	0x14

#define CSR_READ_1(sc, reg)	\
	bus_space_read_1(sc->sc_st, sc->sc_sh, reg)
#define CSR_READ_2(sc, reg)	\
	bus_space_read_2(sc->sc_st, sc->sc_sh, reg)
#define CSR_READ_4(sc, reg)	\
	bus_space_read_4(sc->sc_st, sc->sc_sh, reg)

#define CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1(sc->sc_st, sc->sc_sh, reg, val)
#define CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2(sc->sc_st, sc->sc_sh, reg, val)
#define CSR_WRITE_4(sc, reg, val)	\
	bus_space_write_4(sc->sc_st, sc->sc_sh, reg, val)

struct nppb_softc {  /* XXX into i21555var.h */
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */

	void *sc_ih;			/* interrupt handler cookie */
};

struct nppb_pci_softc {
	struct nppb_softc psc_nppb;

	pci_chipset_tag_t psc_pc;	/* pci chipset tag */
	pcitag_t psc_tag;		/* pci register tag */
};

static int
nppbmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;
	u_int32_t class, id;

	class = pa->pa_class;
	id = pa->pa_id;

	if (PCI_CLASS(class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_MISC) {
		switch (PCI_VENDOR(id)) {
		case PCI_VENDOR_INTEL:
			switch (PCI_PRODUCT(id)) {
			case PCI_PRODUCT_INTEL_21555:
			    return(1);
			}
			break;
		}
	}
	return(0);
}

static void
nppbattach(struct device *parent, struct device *self, void *aux)
{
	struct nppb_pci_softc *psc = (struct nppb_pci_softc *)self;
	struct nppb_softc *sc = (struct nppb_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	char devinfo[256];

	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	int ioh_valid, memh_valid;

	psc->psc_pc = pc;
	psc->psc_tag = pa->pa_tag;

	sprintf(devinfo, "21555 Non-Transparent PCI-PCI Bridge");
	aprint_normal(": %s, rev %d\n", devinfo, PCI_REVISION(pa->pa_class));

	/* Make sure bus-mastering is enabled. */
	pci_conf_write(psc->psc_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Chip Reset */
	pci_conf_write(psc->psc_pc, pa->pa_tag, 0xD8, 0x03);

	/* Map control/status registers */
	ioh_valid = (pci_mapreg_map(pa, NPPB_IOBA,
			PCI_MAPREG_TYPE_IO, 0,
			&iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, NPPB_MMBA,
			PCI_MAPREG_TYPE_MEM |
			PCI_MAPREG_MEM_TYPE_32BIT,
			0, &memt, &memh, NULL, NULL) == 0);

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

	/* Map and establish our interrupt */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, nppb_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

}

/* XXX */
int
nppb_intr(void *arg)
{
#if 0
	struct nppb_softc *sc = arg;
#endif
#ifdef PCI_DEBUG
	printf("nppb_intr assert\n");
#endif
	return(0);
}
