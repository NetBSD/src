/* $NetBSD: sio.c,v 1.31 2000/07/12 20:50:00 thorpej Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sio.c,v 1.31 2000/07/12 20:50:00 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/bus.h>
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
	struct device	sc_dv;

	pci_chipset_tag_t sc_pc;

	bus_space_tag_t sc_iot, sc_memt;
	bus_dma_tag_t	sc_parent_dmat;
	int		sc_haseisa;
	int		sc_is82c693;

	/* ISA chipset must persist; it's used after autoconfig. */
	isa_chipset_tag_t sc_ic;
};

int	siomatch __P((struct device *, struct cfdata *, void *));
void	sioattach __P((struct device *, struct device *, void *));

struct cfattach sio_ca = {
	sizeof(struct sio_softc), siomatch, sioattach,
};

int	pcebmatch __P((struct device *, struct cfdata *, void *));

struct cfattach pceb_ca = {
	sizeof(struct sio_softc), pcebmatch, sioattach,
};

union sio_attach_args {
	const char *sa_name;			/* XXX should be common */
	struct isabus_attach_args sa_iba;
	struct eisabus_attach_args sa_eba;
};

int	sioprint __P((void *, const char *pnp));
void	sio_isa_attach_hook __P((struct device *, struct device *,
	    struct isabus_attach_args *));
void	sio_eisa_attach_hook __P((struct device *, struct device *,
	    struct eisabus_attach_args *));
int	sio_eisa_maxslots __P((void *));
int	sio_eisa_intr_map __P((void *, u_int, eisa_intr_handle_t *));

void	sio_bridge_callback __P((struct device *));

int
siomatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
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
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ALI_M1543)
		return (1);

	return (0);
}

int
pcebmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PCEB)
		return (1);

	return (0);
}

void
sioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sio_softc *sc = (struct sio_softc *)self;
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_pc = pa->pa_pc;
	sc->sc_iot = pa->pa_iot;
	sc->sc_memt = pa->pa_memt;
	sc->sc_parent_dmat = pa->pa_dmat;
	sc->sc_haseisa = (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PCEB);
	sc->sc_is82c693 = (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CONTAQ &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CONTAQ_82C693);

	config_defer(self, sio_bridge_callback);
}

void
sio_bridge_callback(self)
	struct device *self;
{
	struct sio_softc *sc = (struct sio_softc *)self;
	struct alpha_eisa_chipset ec;
	union sio_attach_args sa;

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

		sa.sa_eba.eba_busname = "eisa";
		sa.sa_eba.eba_iot = sc->sc_iot;
		sa.sa_eba.eba_memt = sc->sc_memt;
		sa.sa_eba.eba_dmat =
		    alphabus_dma_get_tag(sc->sc_parent_dmat, ALPHA_BUS_EISA);
		sa.sa_eba.eba_ec = &ec;
		config_found(&sc->sc_dv, &sa.sa_eba, sioprint);
	}

	/*
	 * Deal with platforms which have Odd ISA DMA needs.
	 */
	switch (cputype) {
#ifdef DEC_2100_A500
	case ST_DEC_2100_A500:
		sc->sc_ic = sableio_pickisa();
		break;
#endif
	default:
		sc->sc_ic = malloc(sizeof(*sc->sc_ic), M_DEVBUF, M_WAITOK);
		memset(sc->sc_ic, 0, sizeof(*sc->sc_ic));
	}

	sc->sc_ic->ic_v = NULL;
	sc->sc_ic->ic_attach_hook = sio_isa_attach_hook;

	/*
	 * Deal with platforms that hook up ISA interrupts differently.
	 */
	switch (cputype) {
#ifdef DEC_2100_A500
	case ST_DEC_2100_A500:
		pci_2100_a500_isa_pickintr(sc->sc_pc, sc->sc_ic);
		break;
#endif
	default:
		sc->sc_ic->ic_intr_evcnt = sio_intr_evcnt;
		sc->sc_ic->ic_intr_establish = sio_intr_establish;
		sc->sc_ic->ic_intr_disestablish = sio_intr_disestablish;
		sc->sc_ic->ic_intr_alloc = sio_intr_alloc;
	}

	sa.sa_iba.iba_busname = "isa";
	sa.sa_iba.iba_iot = sc->sc_iot;
	sa.sa_iba.iba_memt = sc->sc_memt;
	sa.sa_iba.iba_dmat =
	    alphabus_dma_get_tag(sc->sc_parent_dmat, ALPHA_BUS_ISA);
	sa.sa_iba.iba_ic = sc->sc_ic;
	config_found(&sc->sc_dv, &sa.sa_iba, sioprint);
}

int
sioprint(aux, pnp)
	void *aux;
	const char *pnp;
{
        register union sio_attach_args *sa = aux;

        if (pnp)
                printf("%s at %s", sa->sa_name, pnp);
        return (UNCONF);
}

void
sio_isa_attach_hook(parent, self, iba)
	struct device *parent, *self;
	struct isabus_attach_args *iba;
{

	/* Nothing to do. */
}

void
sio_eisa_attach_hook(parent, self, eba)
	struct device *parent, *self;
	struct eisabus_attach_args *eba;
{

	/* Nothing to do. */
}

int
sio_eisa_maxslots(v)
	void *v;
{

	return 16;		/* as good a number as any.  only 8, maybe? */
}

int
sio_eisa_intr_map(v, irq, ihp)
	void *v;
	u_int irq;
	eisa_intr_handle_t *ihp;
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
