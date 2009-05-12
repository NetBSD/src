/*	$NetBSD: njs_cardbus.c,v 1.11 2009/05/12 14:17:31 cegger Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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
__KERNEL_RCSID(0, "$NetBSD: njs_cardbus.c,v 1.11 2009/05/12 14:17:31 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/ninjascsi32reg.h>
#include <dev/ic/ninjascsi32var.h>

#define NJSC32_CARDBUS_BASEADDR_IO	CARDBUS_BASE0_REG
#define NJSC32_CARDBUS_BASEADDR_MEM	CARDBUS_BASE1_REG

struct njsc32_cardbus_softc {
	struct njsc32_softc	sc_njsc32;

	/* CardBus-specific goo */
	cardbus_devfunc_t	sc_ct;		/* our CardBus devfuncs */
	cardbus_intr_line_t	sc_intrline;	/* our interrupt line */
	cardbustag_t		sc_tag;

	bus_space_handle_t	sc_regmaph;
	bus_size_t		sc_regmap_size;
};

static int	njs_cardbus_match(device_t, cfdata_t, void *);
static void	njs_cardbus_attach(device_t, device_t, void *);
static int	njs_cardbus_detach(device_t, int);

CFATTACH_DECL_NEW(njs_cardbus, sizeof(struct njsc32_cardbus_softc),
    njs_cardbus_match, njs_cardbus_attach, njs_cardbus_detach, NULL);

static const struct njsc32_cardbus_product {
	cardbus_vendor_id_t	p_vendor;
	cardbus_product_id_t	p_product;
	njsc32_model_t		p_model;
	int			p_clk;		/* one of NJSC32_CLK_* */
} njsc32_cardbus_products[] = {
	{ PCI_VENDOR_IODATA,	PCI_PRODUCT_IODATA_CBSCII,
	  NJSC32_MODEL_32BI,	NJSC32_CLK_40M },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NJSC32BI,
	  NJSC32_MODEL_32BI,	NJSC32_CLK_40M },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NJSC32UDE,
	  NJSC32_MODEL_32UDE | NJSC32_FLAG_DUALEDGE,	NJSC32_CLK_40M },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NJSC32BI_KME,
	  NJSC32_MODEL_32BI,	NJSC32_CLK_40M },

	{ 0,				0,
	  NJSC32_MODEL_INVALID,		0 },
};

static const struct njsc32_cardbus_product *
njs_cardbus_lookup(const struct cardbus_attach_args *ca)
{
	const struct njsc32_cardbus_product *p;

	for (p = njsc32_cardbus_products;
	    p->p_model != NJSC32_MODEL_INVALID; p++) {
		if (CARDBUS_VENDOR(ca->ca_id) == p->p_vendor &&
		    CARDBUS_PRODUCT(ca->ca_id) == p->p_product)
			return p;
	}

	return NULL;
}

static int
njs_cardbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (njs_cardbus_lookup(ca))
		return 1;

	return 0;
}

static void
njs_cardbus_attach(device_t parent, device_t self, void *aux)
{
	struct cardbus_attach_args *ca = aux;
	struct njsc32_cardbus_softc *csc = device_private(self);
	struct njsc32_softc *sc = &csc->sc_njsc32;
	const struct njsc32_cardbus_product *prod;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t reg;
	int csr;
	u_int8_t latency = 0x20;

	if ((prod = njs_cardbus_lookup(ca)) == NULL)
		panic("njs_cardbus_attach");

	printf(": Workbit NinjaSCSI-32 SCSI adapter\n");
	sc->sc_dev = self;
	sc->sc_model = prod->p_model;
	sc->sc_clk = prod->p_clk;

	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;
	csc->sc_intrline = ca->ca_intrline;

	/*
	 * Map the device.
	 */
	csr = PCI_COMMAND_MASTER_ENABLE;

	/*
	 * Map registers.
	 * Try memory map first, and then try I/O.
	 */
	if (Cardbus_mapreg_map(csc->sc_ct, NJSC32_CARDBUS_BASEADDR_MEM,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_regt, &csc->sc_regmaph, NULL, &csc->sc_regmap_size) == 0) {
		if (bus_space_subregion(sc->sc_regt, csc->sc_regmaph,
		    NJSC32_MEMOFFSET_REG, NJSC32_REGSIZE, &sc->sc_regh) != 0) {
			/* failed -- undo map and try I/O */
			Cardbus_mapreg_unmap(csc->sc_ct,
			    NJSC32_CARDBUS_BASEADDR_MEM,
			    sc->sc_regt, csc->sc_regmaph, csc->sc_regmap_size);
			goto try_io;
		}
#ifdef NJSC32_DEBUG
		printf("%s: memory space mapped\n", device_xname(self));
#endif
		csr |= PCI_COMMAND_MEM_ENABLE;
		sc->sc_flags = NJSC32_MEM_MAPPED;
		(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);
	} else {
	try_io:
		if (Cardbus_mapreg_map(csc->sc_ct, NJSC32_CARDBUS_BASEADDR_IO,
		    PCI_MAPREG_TYPE_IO, 0, &sc->sc_regt, &sc->sc_regh,
		    NULL, &csc->sc_regmap_size) == 0) {
#ifdef NJSC32_DEBUG
			printf("%s: io space mapped\n", device_xname(self));
#endif
			csr |= PCI_COMMAND_IO_ENABLE;
			sc->sc_flags = NJSC32_IO_MAPPED;
			(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);
		} else {
			aprint_error_dev(self, "unable to map device registers\n");
			return;
		}
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Enable the appropriate bits in the PCI CSR. */
	reg = cardbus_conf_read(cc, cf, ca->ca_tag, PCI_COMMAND_STATUS_REG);
	reg &= ~(PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE);
	reg |= csr;
	cardbus_conf_write(cc, cf, ca->ca_tag, PCI_COMMAND_STATUS_REG, reg);

	/*
	 * Make sure the latency timer is set to some reasonable
	 * value.
	 */
	reg = cardbus_conf_read(cc, cf, ca->ca_tag, CARDBUS_BHLC_REG);
	if (CARDBUS_LATTIMER(reg) < latency) {
		reg &= ~(CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
		reg |= (latency << CARDBUS_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_BHLC_REG, reg);
	}

	sc->sc_dmat = ca->ca_dmat;

	/*
	 * Establish the interrupt.
	 */
	sc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline, IPL_BIO,
	    njsc32_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
				 "unable to establish interrupt\n");
		return;
	}

	/* CardBus device cannot supply termination power. */
	sc->sc_flags |= NJSC32_CANNOT_SUPPLY_TERMPWR;

	/* attach */
	njsc32_attach(sc);
}

static int
njs_cardbus_detach(device_t self, int flags)
{
	struct njsc32_cardbus_softc *csc = device_private(self);
	struct njsc32_softc *sc = &csc->sc_njsc32;
	int rv;

	rv = njsc32_detach(sc, flags);
	if (rv)
		return rv;

	if (sc->sc_ih)
		cardbus_intr_disestablish(csc->sc_ct->ct_cc,
		    csc->sc_ct->ct_cf, sc->sc_ih);

	if (sc->sc_flags & NJSC32_IO_MAPPED)
		Cardbus_mapreg_unmap(csc->sc_ct, NJSC32_CARDBUS_BASEADDR_IO,
		    sc->sc_regt, sc->sc_regh, csc->sc_regmap_size);
	if (sc->sc_flags & NJSC32_MEM_MAPPED)
		Cardbus_mapreg_unmap(csc->sc_ct, NJSC32_CARDBUS_BASEADDR_MEM,
		    sc->sc_regt, csc->sc_regmaph, csc->sc_regmap_size);

	return 0;
}
