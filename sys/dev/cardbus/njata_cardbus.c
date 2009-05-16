/*	$Id: njata_cardbus.c,v 1.6.4.2 2009/05/16 10:41:19 yamt Exp $	*/

/*
 * Copyright (c) 2006 ITOH Yasufumi <itohy@NetBSD.org>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: njata_cardbus.c,v 1.6.4.2 2009/05/16 10:41:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#include <dev/ic/ninjaata32reg.h>
#include <dev/ic/ninjaata32var.h>

#define NJATA32_CARDBUS_BASEADDR_IO	CARDBUS_BASE0_REG
#define NJATA32_CARDBUS_BASEADDR_MEM	CARDBUS_BASE1_REG

struct njata32_cardbus_softc {
	struct njata32_softc	sc_njata32;

	/* CardBus-specific goo */
	cardbus_devfunc_t	sc_ct;		/* our CardBus devfuncs */
	cardbus_intr_line_t	sc_intrline;	/* our interrupt line */
	cardbustag_t		sc_tag;

	bus_space_handle_t	sc_regmaph;
	bus_size_t		sc_regmap_size;
};

static const struct njata32_cardbus_product *njata_cardbus_lookup
		    (const struct cardbus_attach_args *);
static int	njata_cardbus_match(device_t, cfdata_t, void *);
static void	njata_cardbus_attach(device_t, device_t, void *);
static int	njata_cardbus_detach(device_t, int);

CFATTACH_DECL_NEW(njata_cardbus, sizeof(struct njata32_cardbus_softc),
    njata_cardbus_match, njata_cardbus_attach, njata_cardbus_detach, NULL);

static const struct njata32_cardbus_product {
	cardbus_vendor_id_t	p_vendor;
	cardbus_product_id_t	p_product;
	uint8_t			p_flags;
#define NJATA32_FL_IOMAP_ONLY	1	/* registers are only in the I/O map */
} njata32_cardbus_products[] = {
	{ PCI_VENDOR_IODATA,	PCI_PRODUCT_IODATA_CBIDE2,
	  0 },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NJATA32BI,
	  0 },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NJATA32BI_KME,
	  0 },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NPATA32_CF32A,
	  NJATA32_FL_IOMAP_ONLY },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NPATA32_CF32A_BUFFALO,
	  NJATA32_FL_IOMAP_ONLY },
	{ PCI_VENDOR_WORKBIT,	PCI_PRODUCT_WORKBIT_NPATA32_KME,
	  NJATA32_FL_IOMAP_ONLY },

	{ PCI_VENDOR_INVALID,	0,
	  0 }
};

static const struct njata32_cardbus_product *
njata_cardbus_lookup(const struct cardbus_attach_args *ca)
{
	const struct njata32_cardbus_product *p;

	for (p = njata32_cardbus_products;
	    p->p_vendor != PCI_VENDOR_INVALID; p++) {
		if (CARDBUS_VENDOR(ca->ca_id) == p->p_vendor &&
		    CARDBUS_PRODUCT(ca->ca_id) == p->p_product)
			return p;
	}

	return NULL;
}

static int
njata_cardbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (njata_cardbus_lookup(ca))
		return 1;

	return 0;
}

static void
njata_cardbus_attach(device_t parent, device_t self, void *aux)
{
	struct cardbus_attach_args *ca = aux;
	struct njata32_cardbus_softc *csc = device_private(self);
	struct njata32_softc *sc = &csc->sc_njata32;
	const struct njata32_cardbus_product *prod;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t reg;
	int csr;
	uint8_t latency = 0x20;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	if ((prod = njata_cardbus_lookup(ca)) == NULL)
		panic("njata_cardbus_attach");

	aprint_normal(": Workbit NinjaATA-32 IDE controller\n");

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
	if ((prod->p_flags & NJATA32_FL_IOMAP_ONLY) == 0 &&
	    Cardbus_mapreg_map(csc->sc_ct, NJATA32_CARDBUS_BASEADDR_MEM,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &NJATA32_REGT(sc), &csc->sc_regmaph, NULL, &csc->sc_regmap_size)
	    == 0) {
		if (bus_space_subregion(NJATA32_REGT(sc), csc->sc_regmaph,
		    NJATA32_MEMOFFSET_REG, NJATA32_REGSIZE, &NJATA32_REGH(sc))
		    != 0) {
			/* failed -- undo map and try I/O */
			Cardbus_mapreg_unmap(csc->sc_ct,
			    NJATA32_CARDBUS_BASEADDR_MEM, NJATA32_REGT(sc),
			    csc->sc_regmaph, csc->sc_regmap_size);
			goto try_io;
		}
#ifdef NJATA32_DEBUG
		aprint_normal("%s: memory space mapped, size %u\n",
		    NJATA32NAME(sc), (unsigned)csc->sc_regmap_size);
#endif
		csr |= PCI_COMMAND_MEM_ENABLE;
		sc->sc_flags = NJATA32_MEM_MAPPED;
		(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);
	} else {
	try_io:
		if (Cardbus_mapreg_map(csc->sc_ct, NJATA32_CARDBUS_BASEADDR_IO,
		    PCI_MAPREG_TYPE_IO, 0, &NJATA32_REGT(sc),
		    &NJATA32_REGH(sc), NULL, &csc->sc_regmap_size) == 0) {
#ifdef NJATA32_DEBUG
			aprint_normal("%s: io space mapped, size %u\n",
			    NJATA32NAME(sc), (unsigned)csc->sc_regmap_size);
#endif
			csr |= PCI_COMMAND_IO_ENABLE;
			sc->sc_flags = NJATA32_IO_MAPPED;
			(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);
		} else {
			aprint_error("%s: unable to map device registers\n",
			    NJATA32NAME(sc));
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
	    njata32_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: unable to establish interrupt\n",
		    NJATA32NAME(sc));
		return;
	}

	/* attach */
	njata32_attach(sc);
}

static int
njata_cardbus_detach(device_t self, int flags)
{
	struct njata32_cardbus_softc *csc = device_private(self);
	struct njata32_softc *sc = &csc->sc_njata32;
	int rv;

	rv = njata32_detach(sc, flags);
	if (rv)
		return rv;

	if (sc->sc_ih)
		cardbus_intr_disestablish(csc->sc_ct->ct_cc,
		    csc->sc_ct->ct_cf, sc->sc_ih);

	if (sc->sc_flags & NJATA32_IO_MAPPED)
		Cardbus_mapreg_unmap(csc->sc_ct, NJATA32_CARDBUS_BASEADDR_IO,
		    NJATA32_REGT(sc), NJATA32_REGH(sc), csc->sc_regmap_size);
	if (sc->sc_flags & NJATA32_MEM_MAPPED)
		Cardbus_mapreg_unmap(csc->sc_ct, NJATA32_CARDBUS_BASEADDR_MEM,
		    NJATA32_REGT(sc), csc->sc_regmaph, csc->sc_regmap_size);

	return 0;
}
