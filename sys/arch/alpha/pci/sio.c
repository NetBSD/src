/* $NetBSD: sio.c,v 1.51.2.1 2012/04/17 00:05:57 yamt Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "opt_dec_2100_a500.h"
#include "opt_dec_2100a_a500.h"
#include "eisa.h"
#include "sio.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sio.c,v 1.51.2.1 2012/04/17 00:05:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <sys/bus.h>
#include <machine/rpb.h>

#include <dev/isa/isavar.h>
#include <dev/eisa/eisavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <alpha/pci/siovar.h>

#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
#include <alpha/pci/pci_2100_a500.h>
#include <alpha/sableio/sableiovar.h>
#endif

struct sio_softc {
	device_t	sc_dev;

	pci_chipset_tag_t sc_pc;

	bus_space_tag_t sc_iot, sc_memt;
	bus_dma_tag_t	sc_parent_dmat;
#if NPCEB > 0
	int		sc_haseisa;
#endif
	int		sc_is82c693;

	/* ISA chipset must persist; it's used after autoconfig. */
	isa_chipset_tag_t sc_ic;
};

int	siomatch(device_t, cfdata_t, void *);
void	sioattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sio, sizeof(struct sio_softc),
    siomatch, sioattach, NULL, NULL);

#if NPCEB > 0
int	pcebmatch(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(pceb, sizeof(struct sio_softc),
    pcebmatch, sioattach, NULL, NULL);
#endif

union sio_attach_args {
	struct isabus_attach_args sa_iba;
	struct eisabus_attach_args sa_eba;
};

void	sio_isa_attach_hook(device_t, device_t,
	    struct isabus_attach_args *);
void	sio_isa_detach_hook(isa_chipset_tag_t, device_t);
#if NPCEB > 0
void	sio_eisa_attach_hook(device_t, device_t,
	    struct eisabus_attach_args *);
int	sio_eisa_maxslots(void *);
int	sio_eisa_intr_map(void *, u_int, eisa_intr_handle_t *);
#endif

void	sio_bridge_callback(device_t);

int
siomatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * The Cypress 82C693 is more-or-less an SIO, but with
	 * indirect register access.  (XXX for everything, or
	 * just the ELCR?)
	 */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CONTAQ &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CONTAQ_82C693 &&
	    pa->pa_function == 0)
		return (1);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_SIO)
		return (1);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_ALI &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ALI_M1533)
		return (1);

	return (0);
}

#if NPCEB > 0
int
pcebmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PCEB)
		return (1);

	return (0);
}
#endif

void
sioattach(device_t parent, device_t self, void *aux)
{
	struct sio_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_iot = pa->pa_iot;
	sc->sc_memt = pa->pa_memt;
	sc->sc_parent_dmat = pa->pa_dmat;
#if NPCEB > 0
	sc->sc_haseisa = (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PCEB);
#endif
	sc->sc_is82c693 = (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CONTAQ &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CONTAQ_82C693);

	config_defer(self, sio_bridge_callback);
}

void
sio_bridge_callback(device_t self)
{
	struct sio_softc *sc = device_private(self);
	union sio_attach_args sa;
#if NPCEB > 0
	struct alpha_eisa_chipset ec;

	if (sc->sc_haseisa) {
		ec.ec_v = NULL;
		ec.ec_attach_hook = sio_eisa_attach_hook;
		ec.ec_maxslots = sio_eisa_maxslots;

		/*
		 * Deal with platforms that hook EISA interrupts
		 * up differently.
		 */
		switch (cputype) {
#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
		case ST_DEC_2100_A500:
		case ST_DEC_2100A_A500:
			pci_2100_a500_eisa_pickintr(sc->sc_pc, &ec);
			break;
#endif
		default:
			ec.ec_intr_map = sio_eisa_intr_map;
			ec.ec_intr_string = sio_intr_string;
			ec.ec_intr_evcnt = sio_intr_evcnt;
			ec.ec_intr_establish = sio_intr_establish;
			ec.ec_intr_disestablish = sio_intr_disestablish;
		}

		sa.sa_eba.eba_iot = sc->sc_iot;
		sa.sa_eba.eba_memt = sc->sc_memt;
		sa.sa_eba.eba_dmat =
		    alphabus_dma_get_tag(sc->sc_parent_dmat, ALPHA_BUS_EISA);
		sa.sa_eba.eba_ec = &ec;
		config_found_ia(sc->sc_dev, "eisabus", &sa.sa_eba,
				eisabusprint);
	}
#endif /* NPCEB */

	/*
	 * Deal with platforms which have Odd ISA DMA needs.
	 */
	switch (cputype) {
#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
	case ST_DEC_2100_A500:
	case ST_DEC_2100A_A500:
		sc->sc_ic = sableio_pickisa();
		break;
#endif
	default:
		sc->sc_ic = malloc(sizeof(*sc->sc_ic), M_DEVBUF, M_WAITOK);
		memset(sc->sc_ic, 0, sizeof(*sc->sc_ic));
	}

	sc->sc_ic->ic_v = NULL;
	sc->sc_ic->ic_attach_hook = sio_isa_attach_hook;
	sc->sc_ic->ic_detach_hook = sio_isa_detach_hook;

	/*
	 * Deal with platforms that hook up ISA interrupts differently.
	 */
	switch (cputype) {
#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
	case ST_DEC_2100_A500:
	case ST_DEC_2100A_A500:
		pci_2100_a500_isa_pickintr(sc->sc_pc, sc->sc_ic);
		break;
#endif
	default:
		sc->sc_ic->ic_intr_evcnt = sio_intr_evcnt;
		sc->sc_ic->ic_intr_establish = sio_intr_establish;
		sc->sc_ic->ic_intr_disestablish = sio_intr_disestablish;
		sc->sc_ic->ic_intr_alloc = sio_intr_alloc;
	}

	sa.sa_iba.iba_iot = sc->sc_iot;
	sa.sa_iba.iba_memt = sc->sc_memt;
	sa.sa_iba.iba_dmat =
	    alphabus_dma_get_tag(sc->sc_parent_dmat, ALPHA_BUS_ISA);
	sa.sa_iba.iba_ic = sc->sc_ic;
	config_found_ia(sc->sc_dev, "isabus", &sa.sa_iba, isabusprint);
}

void
sio_isa_attach_hook(device_t parent, device_t self, struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}

void
sio_isa_detach_hook(isa_chipset_tag_t ic, device_t self)
{

	/* Nothing to do. */
}

#if NPCEB > 0

void
sio_eisa_attach_hook(device_t parent, device_t self, struct eisabus_attach_args *eba)
{

#if NEISA > 0
	eisa_init(eba->eba_ec);
#endif
}

int
sio_eisa_maxslots(void *v)
{

	return 16;		/* as good a number as any.  only 8, maybe? */
}

int
sio_eisa_intr_map(void *v, u_int irq, eisa_intr_handle_t *ihp)
{

#define	ICU_LEN		16	/* number of ISA IRQs (XXX) */

	if (irq >= ICU_LEN) {
		printf("sio_eisa_intr_map: bad IRQ %d\n", irq);
		*ihp = -1;
		return 1;
	}
	if (irq == 2) {
		printf("sio_eisa_intr_map: changed IRQ 2 to IRQ 9\n");
		irq = 9;
	}

	*ihp = irq;
	return 0;
}

#endif /* NPCEB */
