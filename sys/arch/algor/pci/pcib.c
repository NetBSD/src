/*	$NetBSD: pcib.c,v 1.5 2001/06/15 04:01:41 thorpej Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.5 2001/06/15 04:01:41 thorpej Exp $");

#include "opt_algor_p5064.h" 
#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#ifdef ALGOR_P5064
#include <algor/algor/algor_p5064var.h>
#endif 
 
#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032var.h>
#endif

const char *pcib_intrnames[16] = {
	"irq 0",
	"irq 1",
	"irq 2",
	"irq 3",
	"irq 4",
	"irq 5",
	"irq 6",
	"irq 7",
	"irq 8",
	"irq 9",
	"irq 10",
	"irq 11",
	"irq 12",
	"irq 13",
	"irq 14",
	"irq 15",
};

struct pcib_intrhead {
	LIST_HEAD(, algor_intrhand) intr_q;
	struct evcnt intr_count;
	int intr_type;
};

struct pcib_softc {
	struct device	sc_dev;

	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh_icu1;
	bus_space_handle_t sc_ioh_icu2;
	bus_space_handle_t sc_ioh_elcr;

	struct algor_isa_chipset sc_ic;

	struct pcib_intrhead sc_intrtab[16];

	u_int16_t	sc_inten;
	u_int16_t	sc_elcr;

#if defined(ALGOR_P5064)
	isa_chipset_tag_t sc_parent_ic;
	u_int16_t	sc_reserved;
#endif

	void		*sc_ih;
};

int	pcib_match(struct device *, struct cfdata *, void *);
void	pcib_attach(struct device *, struct device *, void *);

struct cfattach pcib_ca = {
	sizeof(struct pcib_softc), pcib_match, pcib_attach,
};

int	pcib_print(void *, const char *pnp);
void	pcib_isa_attach_hook(struct device *, struct device *,
	    struct isabus_attach_args *);

int	pcib_intr(void *);

void	pcib_bridge_callback(struct device *);

const struct evcnt *pcib_isa_intr_evcnt(void *, int);
void	*pcib_isa_intr_establish(void *, int, int, int,
	    int (*)(void *), void *);
void	pcib_isa_intr_disestablish(void *, void *);
int	pcib_isa_intr_alloc(void *, int, int, int *);

void	pcib_set_icus(struct pcib_softc *);

int
pcib_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA)
		return (1);

	return (0);
}

void
pcib_attach(struct device *parent, struct device *self, void *aux)
{
	struct pcib_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	int i;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_iot = pa->pa_iot;

	/*
	 * Map the PIC/ELCR registers.
	 */
	if (bus_space_map(sc->sc_iot, 0x4d0, 2, 0, &sc->sc_ioh_elcr) != 0)
		printf("%s: unable to map ELCR registers\n",
		    sc->sc_dev.dv_xname);
	if (bus_space_map(sc->sc_iot, IO_ICU1, 2, 0, &sc->sc_ioh_icu1) != 0)
		printf("%s: unable to map ICU1 registers\n",
		    sc->sc_dev.dv_xname);
	if (bus_space_map(sc->sc_iot, IO_ICU2, 2, 0, &sc->sc_ioh_icu2) != 0)
		printf("%s: unable to map ICU2 registers\n",
		    sc->sc_dev.dv_xname);

	/*
	 * Initialize the 8259s.
	 */

	/* ICW1: reset; program device, 4 bytes */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 0, 0x11);

	/* ICW2: vector base address */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 1, 32/*XXX*/);

	/* ICW3: cascade mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 1, 0x04);

	/* ICW4: 8086 mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 1, 0x01);

	/* OCW1: mask all interrupts */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 1,
	    ~sc->sc_inten & 0xff);

	/* OCW3: enable special mask mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 0, 0x68);

	/* OCW3: read IRR by default */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 0, 0x0a);


	/* ICW1: reset; program device, 4 bytes */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 0, 0x11);

	/* ICW2: vector base address */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 1, 32+8/*XXX*/);

	/* ICW3: slave ID code */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 1, 0x02);

	/* ICW4: 8086 mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 1, 0x01);

	/* OCW1: mask all interrupts */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 1,
	    (~sc->sc_inten >> 8) & 0xff);

	/* OCW3: enable special mask mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 0, 0x68);

	/* OCW3: read IRR by default */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 0, 0x0a);

	/*
	 * Default all interrupts to edge-triggered.
	 */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 0,
	    sc->sc_elcr & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 1,
	    (sc->sc_elcr >> 8) & 0xff);

#if defined(ALGOR_P5064)
	/*
	 * Some "ISA" interrupts are a little wacky, wired up directly
	 * to the P-5064 interrupt controller.
	 */
	sc->sc_reserved =
		(1 << 1) |
		(1 << 3) |
		(1 << 4) |
		(1 << 6) |
		(1 << 7) |
		(1 << 8) |
		(1 << 12) |
		(1 << 14) |
		(1 << 15);

	sc->sc_parent_ic = &p5064_configuration.ac_ic;
#endif /* ALGOR_P5064 */

	/* Set up our ISA chipset. */
	sc->sc_ic.ic_v = sc;
	sc->sc_ic.ic_intr_evcnt = pcib_isa_intr_evcnt;
	sc->sc_ic.ic_intr_establish = pcib_isa_intr_establish;
	sc->sc_ic.ic_intr_disestablish = pcib_isa_intr_disestablish;
	sc->sc_ic.ic_intr_alloc = pcib_isa_intr_alloc;

	/* Initialize our interrupt table. */
	for (i = 0; i < 16; i++) {
		LIST_INIT(&sc->sc_intrtab[i].intr_q);
		evcnt_attach_dynamic(&sc->sc_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "pcib", pcib_intrnames[i]);
		sc->sc_intrtab[i].intr_type = IST_NONE;
	}

	/* Hook up our interrupt handler. */
#if defined(ALGOR_P5064)
	sc->sc_ih = (*algor_intr_establish)(P5064_IRQ_ISABRIDGE,
	    pcib_intr, sc);
#elif defined(ALGOR_P6032)
	sc->sc_ih = (*algor_intr_establish)(XXX,
	    pcib_intr, sc);
#endif
	if (sc->sc_ih == NULL)
		printf("%s: WARNING: unable to register interrupt handler\n",
		    sc->sc_dev.dv_xname);

	config_defer(self, pcib_bridge_callback);
}

void
pcib_bridge_callback(self)
	struct device *self;
{
	struct pcib_softc *sc = (struct pcib_softc *)self;
	struct isabus_attach_args iba;

	memset(&iba, 0, sizeof(iba));

	iba.iba_busname = "isa";
#if defined(ALGOR_P5064)
	    {
		struct p5064_config *acp = &p5064_configuration;

		iba.iba_iot = &acp->ac_iot;
		iba.iba_memt = &acp->ac_memt;
		iba.iba_dmat = &acp->ac_isa_dmat;
		iba.iba_ic = &acp->ac_ic;
	    }
#elif defined(ALGOR_P6032)
	    {
		/* XXX */
	    }
#endif

	iba.iba_ic->ic_attach_hook = pcib_isa_attach_hook;

	(void) config_found(&sc->sc_dev, &iba, pcib_print);
}

int
pcib_print(void *aux, const char *pnp)
{
	struct isabus_attach_args *iba;

	if (pnp)
		printf("%s at %s", iba->iba_busname, pnp);
	return (UNCONF);
}

void
pcib_isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}

void
pcib_set_icus(struct pcib_softc *sc)
{

	/* Enable the cascade IRQ (2) if 8-15 is enabled. */
	if (sc->sc_inten & 0xff00)
		sc->sc_inten |= (1 << 2);
	else
		sc->sc_inten &= ~(1 << 2);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 1,
	    ~sc->sc_inten & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, 1,
	    (~sc->sc_inten >> 8) & 0xff);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 0,
	    sc->sc_elcr & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 1,
	    (sc->sc_elcr >> 8) & 0xff);
}

int
pcib_intr(void *v)
{
	struct pcib_softc *sc = v;
	struct algor_intrhand *ih;
	u_int16_t ipending;
	int i;

	ipending = bus_space_read_1(sc->sc_iot, sc->sc_ioh_icu1, 0) |
	    (bus_space_read_1(sc->sc_iot, sc->sc_ioh_icu2, 0) << 8);
	ipending &= (sc->sc_inten & ~0x02);
	if (ipending == 0)
		return (0);

	for (i = 0; i < 16; i++) {
		if ((ipending & (1 << i)) == 0)
			continue;
		sc->sc_intrtab[i].intr_count.ev_count++;
		for (ih = LIST_FIRST(&sc->sc_intrtab[i].intr_q);
		     ih != NULL; ih = LIST_NEXT(ih, ih_q)) {
			(*ih->ih_func)(ih->ih_arg);
		}

		/* Send a specific EOI to the 8259. */
		if (i > 7)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2,
			    0, 0x20 | (i & 0x07));
		bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, 0,
		    0x20 | (i > 7 ? 2 : i));
	}

	return (1);
}

const struct evcnt *
pcib_isa_intr_evcnt(void *v, int irq)
{
	struct pcib_softc *sc = v;

#if defined(ALGOR_P5064)
	if (sc->sc_reserved & (1 << irq))
		return (isa_intr_evcnt(sc->sc_parent_ic, irq));
#endif

	return (&sc->sc_intrtab[irq].intr_count);
}

void *
pcib_isa_intr_establish(void *v, int irq, int type, int level,
    int (*func)(void *), void *arg)
{
	struct pcib_softc *sc = v;
	struct algor_intrhand *ih;
	int s;

	if (irq > 15 || irq == 2 || type == IST_NONE)
		panic("pcib_isa_intr_establish: bad irq or type");

#if defined(ALGOR_P5064)
	if (sc->sc_reserved & (1 << irq)) {
		ih = isa_intr_establish(sc->sc_parent_ic, irq, type,
		    level, func, arg);
		if (ih != NULL)
			ih->ih_irq = irq;
	}
#endif

	switch (sc->sc_intrtab[irq].intr_type) {
	case IST_NONE:
		sc->sc_intrtab[irq].intr_type = type;
		break;

	case IST_EDGE:
	case IST_LEVEL:
		if (type == sc->sc_intrtab[irq].intr_type)
			break;
		/* FALLTHROUGH */
	case IST_PULSE:
		/*
		 * We can't share interrupts in this case.
		 */
		return (NULL);
	}

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_irqmap = NULL;

	s = splhigh();

	/* Insert the handler into the table. */
	LIST_INSERT_HEAD(&sc->sc_intrtab[irq].intr_q, ih, ih_q);
	sc->sc_intrtab[irq].intr_type = type;

	/* Enable it, set trigger mode. */
	sc->sc_inten |= (1 << irq);
	if (sc->sc_intrtab[irq].intr_type == IST_LEVEL)
		sc->sc_elcr |= (1 << irq);
	else
		sc->sc_elcr &= ~(1 << irq);

	pcib_set_icus(sc);

	splx(s);

	return (ih);
}

void
pcib_isa_intr_disestablish(void *v, void *arg)
{
	struct pcib_softc *sc = v;
	struct algor_intrhand *ih = arg;
	int s;

#if defined(ALGOR_P5064)
	if (sc->sc_reserved & (1 << ih->ih_irq)) {
		isa_intr_disestablish(sc->sc_parent_ic, ih);
		return;
	}
#endif

	s = splhigh();

	LIST_REMOVE(ih, ih_q);

	/* If there are no more handlers on this IRQ, disable it. */
	if (LIST_FIRST(&sc->sc_intrtab[ih->ih_irq].intr_q) == NULL) {
		sc->sc_inten &= ~(1 << ih->ih_irq);
		pcib_set_icus(sc);
	}

	splx(s);

	free(ih, M_DEVBUF);
}

int
pcib_isa_intr_alloc(void *v, int mask, int type, int *irq)
{
	struct pcib_softc *sc = v;
	int i, tmp, bestirq, count;
	struct algor_intrhand *ih;

	if (type == IST_NONE)
		panic("pcib_intr_alloc: bogus type");

	bestirq = -1;
	count = -1;

#if defined(ALGOR_P5064)
	mask &= ~sc->sc_reserved;
#endif

	for (i = 0; i < 16; i++) {
		if (i == 2 || (mask & (1 << i)) == 0)
			continue;

		switch (sc->sc_intrtab[i].intr_type) {
		case IST_NONE:
			/*
			 * If nothing's using the IRQ, just return it.
			 */
			*irq = i;
			return (0);

		case IST_EDGE:
		case IST_LEVEL:
			if (type != sc->sc_intrtab[i].intr_type)
				continue;
			/*
			 * If the IRQ is sharable, count the number of
			 * other handlers, and if it's smaller than the
			 * last IRQ like this, remember it.
			 */
			tmp = 0;
			for (ih = LIST_FIRST(&sc->sc_intrtab[i].intr_q);
			     ih != NULL; ih = LIST_NEXT(ih, ih_q))
				tmp++;
			if (bestirq == -1 || count > tmp) {
				bestirq = i;
				count = tmp;
			}
			break;

		case IST_PULSE:
			/* This just isn't sharable. */
			continue;
		}
	}

	if (bestirq == -1)
		return (1);

	*irq = bestirq;
	return (0);
}
