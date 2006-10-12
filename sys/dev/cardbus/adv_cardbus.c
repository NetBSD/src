/*	$NetBSD: adv_cardbus.c,v 1.14 2006/10/12 01:30:55 christos Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * this file was brought from ahc_cardbus.c and adv_pci.c
 * and modified by YAMAMOTO Takashi.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adv_cardbus.c,v 1.14 2006/10/12 01:30:55 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/advlib.h>
#include <dev/ic/adv.h>

#define ADV_CARDBUS_IOBA CARDBUS_BASE0_REG
#define ADV_CARDBUS_MMBA CARDBUS_BASE1_REG

#define ADV_CARDBUS_DEBUG
#define ADV_CARDBUS_ALLOW_MEMIO

#define DEVNAME(sc) sc->sc_dev.dv_xname

struct adv_cardbus_softc {
	struct asc_softc sc_adv;	/* real ADV */

	/* CardBus-specific goo. */
	cardbus_devfunc_t sc_ct;	/* our CardBus devfuncs */
	int	sc_intrline;		/* our interrupt line */
	cardbustag_t sc_tag;

	int	sc_cbenable;		/* what CardBus access type to enable */
	int	sc_csr;			/* CSR bits */
	bus_size_t sc_size;
};

int	adv_cardbus_match(struct device *, struct cfdata *, void *);
void	adv_cardbus_attach(struct device *, struct device *, void *);
int	adv_cardbus_detach(struct device *, int);

CFATTACH_DECL(adv_cardbus, sizeof(struct adv_cardbus_softc),
    adv_cardbus_match, adv_cardbus_attach, adv_cardbus_detach, NULL);

int
adv_cardbus_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (CARDBUS_VENDOR(ca->ca_id) == PCI_VENDOR_ADVSYS &&
	    CARDBUS_PRODUCT(ca->ca_id) == PCI_PRODUCT_ADVSYS_ULTRA)
		return (1);

	return (0);
}

void
adv_cardbus_attach(struct device *parent __unused, struct device *self,
    void *aux)
{
	struct cardbus_attach_args *ca = aux;
	struct adv_cardbus_softc *csc = device_private(self);
	struct asc_softc *sc = &csc->sc_adv;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	pcireg_t reg;
	u_int8_t latency = 0x20;

	sc->sc_flags = 0;

	if (PCI_VENDOR(ca->ca_id) == PCI_VENDOR_ADVSYS) {
		switch (PCI_PRODUCT(ca->ca_id)) {
		case PCI_PRODUCT_ADVSYS_1200A:
			printf(": AdvanSys ASC1200A SCSI adapter\n");
			latency = 0;
			break;

		case PCI_PRODUCT_ADVSYS_1200B:
			printf(": AdvanSys ASC1200B SCSI adapter\n");
			latency = 0;
			break;

		case PCI_PRODUCT_ADVSYS_ULTRA:
			switch (PCI_REVISION(ca->ca_class)) {
			case ASC_PCI_REVISION_3050:
				printf(": AdvanSys ABP-9xxUA SCSI adapter\n");
				break;

			case ASC_PCI_REVISION_3150:
				printf(": AdvanSys ABP-9xxU SCSI adapter\n");
				break;
			}
			break;

		default:
			printf(": unknown model!\n");
			return;
		}
	}

	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;
	csc->sc_intrline = ca->ca_intrline;
	csc->sc_cbenable = 0;

	/*
	 * Map the device.
	 */
	csc->sc_csr = PCI_COMMAND_MASTER_ENABLE;

#ifdef ADV_CARDBUS_ALLOW_MEMIO
	if (Cardbus_mapreg_map(csc->sc_ct, ADV_CARDBUS_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &iot, &ioh, NULL, &csc->sc_size) == 0) {
#ifdef ADV_CARDBUS_DEBUG
		printf("%s: memio enabled\n", DEVNAME(sc));
#endif
		csc->sc_cbenable = CARDBUS_MEM_ENABLE;
		csc->sc_csr |= PCI_COMMAND_MEM_ENABLE;
	} else
#endif
	if (Cardbus_mapreg_map(csc->sc_ct, ADV_CARDBUS_IOBA,
	    PCI_MAPREG_TYPE_IO, 0, &iot, &ioh, NULL, &csc->sc_size) == 0) {
#ifdef ADV_CARDBUS_DEBUG
		printf("%s: io enabled\n", DEVNAME(sc));
#endif
		csc->sc_cbenable = CARDBUS_IO_ENABLE;
		csc->sc_csr |= PCI_COMMAND_IO_ENABLE;
	} else {
		printf("%s: unable to map device registers\n",
		    DEVNAME(sc));
		return;
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, csc->sc_cbenable);
	(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Enable the appropriate bits in the PCI CSR. */
	reg = cardbus_conf_read(cc, cf, ca->ca_tag, PCI_COMMAND_STATUS_REG);
	reg &= ~(PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE);
	reg |= csc->sc_csr;
	cardbus_conf_write(cc, cf, ca->ca_tag, PCI_COMMAND_STATUS_REG, reg);

	/*
	 * Make sure the latency timer is set to some reasonable
	 * value.
	 */
	reg = cardbus_conf_read(cc, cf, ca->ca_tag, PCI_BHLC_REG);
	if (PCI_LATTIMER(reg) < latency) {
		reg &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
		reg |= (latency << PCI_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, ca->ca_tag, PCI_BHLC_REG, reg);
	}

	ASC_SET_CHIP_CONTROL(iot, ioh, ASC_CC_HALT);
	ASC_SET_CHIP_STATUS(iot, ioh, 0);

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = ca->ca_dmat;
	sc->pci_device_id = ca->ca_id;
	sc->bus_type = ASC_IS_PCI;
	sc->chip_version = ASC_GET_CHIP_VER_NO(iot, ioh);

	/*
	 * Initialize the board
	 */
	if (adv_init(sc)) {
		printf("adv_init failed\n");
		return;
	}

	/*
	 * Establish the interrupt.
	 */
	sc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline, IPL_BIO,
	    adv_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt at %d\n",
		    DEVNAME(sc), ca->ca_intrline);
		return;
	}
	printf("%s: interrupting at %d\n", DEVNAME(sc), ca->ca_intrline);

	/*
	 * Attach.
	 */
	adv_attach(sc);
}

int
adv_cardbus_detach(self, flags)
	struct device *self;
	int flags;
{
	struct adv_cardbus_softc *csc = device_private(self);
	struct asc_softc *sc = &csc->sc_adv;

	int rv;

	rv = adv_detach(sc, flags);
	if (rv)
		return rv;

	if (sc->sc_ih) {
		cardbus_intr_disestablish(csc->sc_ct->ct_cc,
		    csc->sc_ct->ct_cf, sc->sc_ih);
		sc->sc_ih = 0;
	}

	if (csc->sc_cbenable) {
#ifdef ADV_CARDBUS_ALLOW_MEMIO
		if (csc->sc_cbenable == CARDBUS_MEM_ENABLE) {
			Cardbus_mapreg_unmap(csc->sc_ct, ADV_CARDBUS_MMBA,
			    sc->sc_iot, sc->sc_ioh, csc->sc_size);
		} else {
#endif
			Cardbus_mapreg_unmap(csc->sc_ct, ADV_CARDBUS_IOBA,
			    sc->sc_iot, sc->sc_ioh, csc->sc_size);
#ifdef ADV_CARDBUS_ALLOW_MEMIO
		}
#endif
		csc->sc_cbenable = 0;
	}

	return 0;
}
