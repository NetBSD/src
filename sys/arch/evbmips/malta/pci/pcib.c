/*	$NetBSD: pcib.c,v 1.2 2002/03/18 10:10:16 simonb Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <evbmips/malta/maltareg.h>
#include <evbmips/malta/maltavar.h>
#include <evbmips/malta/dev/gtreg.h>
#include <evbmips/malta/pci/pcibvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/i8259reg.h>


#define	ICU_LEN		16	/* number of ISA IRQs */

const char *isa_intrnames[ICU_LEN] = {
	"timer",
	"keyboard",
	"reserved",		/* by South Bridge (for cascading) */
	"com1",
	"com0",
	"not used",
	"floppy",
	"centronics",
	"mcclock",
	"i2c",
	"pci A,B",		/* PCI slots 1..4, ethernet */
	"pci C,D",		/* PCI slots 1..4, audio, usb */
	"mouse",
	"reserved",
	"ide primary",
	"ide secondary",	/* and compact flash connector */
};

struct pcib_intrhead {
	LIST_HEAD(, evbmips_intrhand) intr_q;
	struct evcnt intr_count;
	int intr_type;
};

struct pcib_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh_icu1;
	bus_space_handle_t sc_ioh_icu2;
	bus_space_handle_t sc_ioh_elcr;

	struct mips_isa_chipset sc_ic;

	struct pcib_intrhead sc_intrtab[ICU_LEN];

	u_int16_t	sc_imask;
	u_int16_t	sc_elcr;

	u_int16_t	sc_reserved;

	void *sc_ih;
};

/*
 * XXX
 *	There is only one pci-isa bridge, and all external interrupts
 *	are routed through it, so we need to remember the softc when
 *	called from other interrupt handling code.
 */
static struct pcib_softc *my_sc;
struct mips_isa_chipset *pcib_ic;

static int	pcib_match(struct device *, struct cfdata *, void *);
static void	pcib_attach(struct device *, struct device *, void *);
static int	pcib_intr(void *v);
static void	pcib_bridge_callback(struct device *);
static int	pcib_print(void *, const char *);
static void	pcib_set_icus(struct pcib_softc *sc);
static void	pcib_cleanup(void *arg);

static const struct evcnt *
		pcib_isa_intr_evcnt(void *, int);
static void	*pcib_isa_intr_establish(void *, int, int, int,
		    int (*)(void *), void *);
static void	pcib_isa_intr_disestablish(void *, void *);
static void	pcib_isa_attach_hook(struct device *, struct device *,
		    struct isabus_attach_args *);
static int	pcib_isa_intr_alloc(void *, int, int, int *);
static const char *
		pcib_isa_intr_string(void *, int);

struct cfattach pcib_ca = {
	sizeof(struct pcib_softc), pcib_match, pcib_attach
};

static int
pcib_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA)
		return (1);

	return (0);
}

static void
pcib_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	int i;

	printf("\n");

	if (my_sc != NULL)
		panic("pcib_attach: already attached!\n");
	my_sc = (void *)self;

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s, (rev . 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	my_sc->sc_iot = pa->pa_iot;

	/*
	 * Map the PIC/ELCR registers.
	 */
	if (bus_space_map(my_sc->sc_iot, 0x4d0, 2, 0, &my_sc->sc_ioh_elcr) != 0)
		printf("%s: unable to map ELCR registers\n",
		    my_sc->sc_dev.dv_xname);
	if (bus_space_map(my_sc->sc_iot, IO_ICU1, 2, 0, &my_sc->sc_ioh_icu1) != 0)
		printf("%s: unable to map ICU1 registers\n",
		    my_sc->sc_dev.dv_xname);
	if (bus_space_map(my_sc->sc_iot, IO_ICU2, 2, 0, &my_sc->sc_ioh_icu2) != 0)
		printf("%s: unable to map ICU2 registers\n",
		    my_sc->sc_dev.dv_xname);

	/* All interrupts default to "masked off". */
	my_sc->sc_imask = 0xffff;

	/* All interrupts default to edge-triggered. */
	my_sc->sc_elcr = 0;

	/*
	 * Initialize the 8259s.
	 */
	/* reset, program device, 4 bytes */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_ICW1,
	    ICW1_SELECT | ICW1_IC4);
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_ICW2,
	    ICW2_VECTOR(0)/*XXX*/);
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_ICW3,
	    ICW3_CASCADE(2));
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_ICW4,
	    ICW4_8086);

	/* mask all interrupts */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_OCW1,
	    my_sc->sc_imask & 0xff);

	/* enable special mask mode */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_OCW3,
	    OCW3_SELECT | OCW3_SSMM | OCW3_SMM);

	/* read IRR by default */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu1, PIC_OCW3,
	    OCW3_SELECT | OCW3_RR);

	/* reset, program device, 4 bytes */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_ICW1,
	    ICW1_SELECT | ICW1_IC4);
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_ICW2,
	    ICW2_VECTOR(0)/*XXX*/);
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_ICW3,
	    ICW3_CASCADE(2));
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_ICW4,
	    ICW4_8086);

	/* mask all interrupts */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_OCW1,
	    my_sc->sc_imask & 0xff);

	/* enable special mask mode */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_OCW3,
	    OCW3_SELECT | OCW3_SSMM | OCW3_SMM);

	/* read IRR by default */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_icu2, PIC_OCW3,
	    OCW3_SELECT | OCW3_RR);

	/*
	 * Default all interrupts to edge-triggered.
	 */
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_elcr, 0,
	    my_sc->sc_elcr & 0xff);
	bus_space_write_1(my_sc->sc_iot, my_sc->sc_ioh_elcr, 1,
	    (my_sc->sc_elcr >> 8) & 0xff);

	/*
	 * Some ISA interrupts are reserved for devices that
	 * we know are hard-wired to certain IRQs.
	 */
	my_sc->sc_reserved =
		(1U << 0) |     /* timer */
		(1U << 1) |     /* keyboard controller (keyboard) */
		(1U << 2) |     /* PIC cascade */
		(1U << 3) |     /* COM 2 */
		(1U << 4) |     /* COM 1 */
		(1U << 6) |     /* floppy */
		(1U << 7) |     /* centronics */
		(1U << 8) |     /* RTC */
		(1U << 9) |	/* I2C */
		(1U << 12) |    /* keyboard controller (mouse) */
		(1U << 14) |    /* IDE primary */
		(1U << 15);     /* IDE secondary */

	/* Set up our ISA chipset. */
	my_sc->sc_ic.ic_v = my_sc;
	my_sc->sc_ic.ic_intr_evcnt = pcib_isa_intr_evcnt;
	my_sc->sc_ic.ic_intr_establish = pcib_isa_intr_establish;
	my_sc->sc_ic.ic_intr_disestablish = pcib_isa_intr_disestablish;
	my_sc->sc_ic.ic_intr_alloc = pcib_isa_intr_alloc;
	my_sc->sc_ic.ic_intr_string = pcib_isa_intr_string;

	pcib_ic = &my_sc->sc_ic;	/* XXX for external use */

	/* Initialize our interrupt table. */
	for (i = 0; i < ICU_LEN; i++) {
#if 0
		char irqstr[8];		/* 4 + 2 + NULL + sanity */

		sprintf(irqstr, "irq %d", i);
		evcnt_attach_dynamic(&my_sc->sc_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "pcib", irqstr);
#else
		evcnt_attach_dynamic(&my_sc->sc_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "pcib", isa_intrnames[i]);
#endif
		LIST_INIT(&my_sc->sc_intrtab[i].intr_q);
		my_sc->sc_intrtab[i].intr_type = IST_NONE;
	}

	/* Hook up our interrupt handler. */
	my_sc->sc_ih = evbmips_intr_establish(MALTA_SOUTHBRIDGE_INTR, pcib_intr, my_sc);
	if (my_sc->sc_ih == NULL)
		printf("%s: WARNING: unable to register interrupt handler\n",
		    my_sc->sc_dev.dv_xname);


	/*
	 * Disable ISA interrupts before returning to YAMON.
	 */
	if (shutdownhook_establish(pcib_cleanup, my_sc) == NULL)
		panic("pcib_attach: could not establish shutdown hook");

	config_defer(self, pcib_bridge_callback);
}

static void
pcib_bridge_callback(struct device *self)
{
	struct pcib_softc *sc = (void *)self;
	struct malta_config *mcp = &malta_configuration;
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));

	iba.iba_busname = "isa";
	iba.iba_iot = &mcp->mc_iot;
	iba.iba_memt = &mcp->mc_memt;
	iba.iba_dmat = &mcp->mc_isa_dmat;

	iba.iba_ic = &sc->sc_ic;
	iba.iba_ic->ic_attach_hook = pcib_isa_attach_hook;

	config_found(&sc->sc_dev, &iba, pcib_print);
}

static int
pcib_print(void *aux, const char *pnp)
{

	/* Only ISAs can attach to pcib's; easy. */
	if (pnp)
		printf("isa at %s", pnp);
	return (UNCONF);
}

static void
pcib_isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}

static void
pcib_set_icus(struct pcib_softc *sc)
{

	/* Enable the cascade IRQ (2) if 8-15 is enabled. */
	if ((sc->sc_imask & 0xff00) != 0xff00)
		sc->sc_imask &= ~(1U << 2);
	else
		sc->sc_imask |= (1U << 2);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW1,
	    sc->sc_imask & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_OCW1,
	    (sc->sc_imask >> 8) & 0xff);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 0,
	    sc->sc_elcr & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 1,
	    (sc->sc_elcr >> 8) & 0xff);
}

static int
pcib_intr(void *v)
{
	struct pcib_softc *sc = v;
	struct evbmips_intrhand *ih;
	int irq;

	for (;;) {
#if 1
		bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW3,
		    OCW3_SELECT | OCW3_POLL);
		irq = bus_space_read_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW3);
		if ((irq & OCW3_POLL_PENDING) == 0)
			return (1);

		irq = OCW3_POLL_IRQ(irq);

		if (irq == 2) {
			bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2,
			    PIC_OCW3, OCW3_SELECT | OCW3_POLL);
			irq = bus_space_read_1(sc->sc_iot, sc->sc_ioh_icu2,
			    PIC_OCW3);
			if (irq & OCW3_POLL_PENDING)
				irq = OCW3_POLL_IRQ(irq) + 8;
			else
				irq = 2;
		}
#else
		/* XXX - should be a function call to gt.c? */
		irq = GT_REGVAL(GT_PCI0_INTR_ACK) & 0xff;

		/*
		 * From YAMON source code:
		 *
		 * IRQ7 is used to detect spurious interrupts.
		 * The interrupt acknowledge cycle returns IRQ7, if no 
		 * interrupts is requested.
		 * We can differentiate between this situation and a
		 * "Normal" IRQ7 by reading the ISR.
		 */

		if (irq == 7) {
			int reg;

			bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW3,
			    OCW3_SELECT | OCW3_RR | OCW3_RIS);
			reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh_icu1,
			    PIC_OCW3);
			if (!(reg & (1 << 7)))
				break;	/* spurious interrupt */
		}
#endif

		sc->sc_intrtab[irq].intr_count.ev_count++;
		LIST_FOREACH(ih, &sc->sc_intrtab[irq].intr_q, ih_q)
			(*ih->ih_func)(ih->ih_arg);

		/* Send a specific EOI to the 8259. */
		if (irq > 7) {
			bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2,
			    PIC_OCW2, OCW2_SELECT | OCW3_EOI | OCW3_SL |
			    OCW2_ILS(irq & 7));
			irq = 2;
		}

		bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW2,
		    OCW2_SELECT | OCW3_EOI | OCW3_SL | OCW2_ILS(irq));
	}
}

const char *
pcib_isa_intr_string(void *v, int irq)
{
	static char irqstr[12];		/* 8 + 2 + NULL + sanity */

	if (irq == 0 || irq >= ICU_LEN || irq == 2)
		panic("pcib_isa_intr_string: bogus isa irq 0x%x\n", irq);

	sprintf(irqstr, "isa irq %d", irq);
	return (irqstr);
}

const struct evcnt *
pcib_isa_intr_evcnt(void *v, int irq)
{

	if (irq == 0 || irq >= ICU_LEN || irq == 2)
		panic("pcib_isa_intr_evcnt: bogus isa irq 0x%x\n", irq);

	return (&my_sc->sc_intrtab[irq].intr_count);
}

void *
pcib_isa_intr_establish(void *v, int irq, int type, int level,
    int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	int s;

	if (irq >= ICU_LEN || irq == 2 || type == IST_NONE)
		panic("pcib_isa_intr_establish: bad irq or type");

	switch (my_sc->sc_intrtab[irq].intr_type) {
	case IST_NONE:
		my_sc->sc_intrtab[irq].intr_type = type;
		break;

	case IST_EDGE:
	case IST_LEVEL:
		if (type == my_sc->sc_intrtab[irq].intr_type)
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

	s = splhigh();

	/* Insert the handler into the table. */
	LIST_INSERT_HEAD(&my_sc->sc_intrtab[irq].intr_q, ih, ih_q);
	my_sc->sc_intrtab[irq].intr_type = type;

	/* Enable it, set trigger mode. */
	my_sc->sc_imask &= ~(1 << irq);
	if (my_sc->sc_intrtab[irq].intr_type == IST_LEVEL)
		my_sc->sc_elcr |= (1 << irq);
	else
		my_sc->sc_elcr &= ~(1 << irq);

	pcib_set_icus(my_sc);

	splx(s);

	return (ih);
}

void
pcib_isa_intr_disestablish(void *v, void *arg)
{
	struct evbmips_intrhand *ih = arg;
	int s;

	s = splhigh();

	LIST_REMOVE(ih, ih_q);

	/* If there are no more handlers on this IRQ, disable it. */
	if (LIST_FIRST(&my_sc->sc_intrtab[ih->ih_irq].intr_q) == NULL) {
		my_sc->sc_imask |= (1 << ih->ih_irq);
		pcib_set_icus(my_sc);
	}

	splx(s);

	free(ih, M_DEVBUF);
}

static int
pcib_isa_intr_alloc(void *v, int mask, int type, int *irq)
{
	int i, tmp, bestirq, count;
	struct evbmips_intrhand *ih;

	if (type == IST_NONE)
		panic("pcib_intr_alloc: bogus type");

	bestirq = -1;
	count = -1;

	mask &= ~my_sc->sc_reserved;

	for (i = 0; i < ICU_LEN; i++) {
		if ((mask & (1 << i)) == 0)
			continue;

		switch (my_sc->sc_intrtab[i].intr_type) {
		case IST_NONE:
			/*
			 * If nothing's using the IRQ, just return it.
			 */
			*irq = i;
			return (0);

		case IST_EDGE:
		case IST_LEVEL:
			if (type != my_sc->sc_intrtab[i].intr_type)
				continue;
			/*
			 * If the IRQ is sharable, count the number of
			 * other handlers, and if it's smaller than the
			 * last IRQ like this, remember it.
			 */
			tmp = 0;
			for (ih = LIST_FIRST(&my_sc->sc_intrtab[i].intr_q);
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

static void
pcib_cleanup(void *arg)
{

	my_sc->sc_imask = 0xffff;
	pcib_set_icus(my_sc);
}
