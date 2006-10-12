/*	$NetBSD: dpt_pci.c,v 1.18 2006/10/12 01:31:28 christos Exp $	*/

/*
 * Copyright (c) 1999, 2000, 2001 Andrew Doran <ad@NetBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * PCI front-end for DPT EATA SCSI driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dpt_pci.c,v 1.18 2006/10/12 01:31:28 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <dev/ic/dptreg.h>
#include <dev/ic/dptvar.h>

#include <dev/i2o/dptivar.h>

#define	PCI_CBMA	0x14	/* Configuration base memory address */
#define	PCI_CBIO	0x10	/* Configuration base I/O address */

static int	dpt_pci_match(struct device *, struct cfdata *, void *);
static void	dpt_pci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(dpt_pci, sizeof(struct dpt_softc),
    dpt_pci_match, dpt_pci_attach, NULL, NULL);

static int
dpt_pci_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct pci_attach_args *pa;

	pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_DPT &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_DPT_SC_RAID)
		return (1);

	return (0);
}

static void
dpt_pci_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct pci_attach_args *pa;
	struct dpt_softc *sc;
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	bus_space_handle_t ioh;
	const char *intrstr;
	pcireg_t csr;

	aprint_naive(": Storage controller\n");

	sc = (struct dpt_softc *)self;
	pa = (struct pci_attach_args *)aux;
	pc = pa->pa_pc;
	aprint_normal(": ");

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0, &sc->sc_iot,
	    &ioh, NULL, NULL)) {
		aprint_error("can't map i/o space\n");
		return;
	}

	/* Need to map in by 16 registers. */
	if (bus_space_subregion(sc->sc_iot, ioh, 16, 16, &sc->sc_ioh)) {
		aprint_error("can't map i/o subregion\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("can't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, dpt_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("can't establish interrupt");
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}

	/* Read the EATA configuration. */
	if (dpt_readcfg(sc)) {
		aprint_error("%s: readcfg failed - see dpt(4)\n",
		    sc->sc_dv.dv_xname);
		return;
	}

	sc->sc_bustype = SI_PCI_BUS;

	/* Now attach to the bus-independent code. */
	dpt_init(sc, intrstr);
}
