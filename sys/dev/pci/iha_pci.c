/*	$NetBSD: iha_pci.c,v 1.1.6.1 2001/10/01 12:45:56 fvdl Exp $ */
/*
 * Initio INI-9xxxU/UW SCSI Device Driver
 *
 * Copyright (c) 2000 Ken Westerback
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *-------------------------------------------------------------------------
 *
 * Ported from i91u.c, provided by Initio Corporation, which credits:
 *
 * Device driver for the INI-9XXXU/UW or INIC-940/950  PCI SCSI Controller.
 *
 * FreeBSD
 *
 *  Written for 386bsd and FreeBSD by
 *	Winston Hung		<winstonh@initio.com>
 *
 * Copyright (c) 1997-99 Initio Corp.  All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/*
 * Ported to NetBSD by Izumi Tsutsui <tsutsui@ceres.dti.ne.jp> from OpenBSD:
 * $OpenBSD: iha_pci.c,v 1.2 2001/03/29 23:53:38 krw Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ihavar.h>

static int  iha_pci_probe(struct device *, struct cfdata *, void *);
static void iha_pci_attach(struct device *, struct device *, void *);

struct cfattach iha_pci_ca = {
	sizeof(struct iha_softc), iha_pci_probe, iha_pci_attach
};

static int
iha_pci_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INITIO)
		return (0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_INITIO_I940:
	case PCI_PRODUCT_INITIO_I950:
		return (1);
	}

	return (0);
}

static void
iha_pci_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	pci_intr_handle_t ih;
	struct iha_softc *sc = (struct iha_softc *)self;
	const char *intrstr;
	pcireg_t command;
	int ioh_valid;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	command  = pci_conf_read(pa->pa_pc,pa->pa_tag,PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_PARITY_ENABLE;

	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

	/*
	 * XXX - Tried memory mapping (using code from adw and ahc)
	 *	 rather that IO mapping, but it didn't work at all..
	 */
	ioh_valid = pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL);

	if (ioh_valid != 0) {
		printf("%s: unable to map registers\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot  = iot;
	sc->sc_ioh  = ioh;
	sc->sc_dmat = pa->pa_dmat;

	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);

	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, iha_intr, sc);

	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt", sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	iha_attach(sc);
}
