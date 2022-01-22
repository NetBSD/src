/*	$NetBSD: pcib.c,v 1.24 2022/01/22 15:10:31 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.24 2022/01/22 15:10:31 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
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

const char * const isa_intrnames[ICU_LEN] = {
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
	device_t sc_dev;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh_icu1;
	bus_space_handle_t sc_ioh_icu2;
	bus_space_handle_t sc_ioh_elcr;

	bus_dma_tag_t sc_dmat;

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

static int	pcib_match(device_t, cfdata_t, void *);
static void	pcib_attach(device_t, device_t, void *);
static int	pcib_intr(void *v);
static void	pcib_bridge_callback(device_t);
static void	pcib_set_icus(struct pcib_softc *sc);
static void	pcib_cleanup(void *arg);

static const struct evcnt *
		pcib_isa_intr_evcnt(void *, int);
static void	*pcib_isa_intr_establish(void *, int, int, int,
		    int (*)(void *), void *);
static void	pcib_isa_intr_disestablish(void *, void *);
static void	pcib_isa_attach_hook(device_t, device_t,
		    struct isabus_attach_args *);
static void	pcib_isa_detach_hook(isa_chipset_tag_t, device_t);
static int	pcib_isa_intr_alloc(void *, int, int, int *);
static const char *
		pcib_isa_intr_string(void *, int, char *, size_t);

CFATTACH_DECL_NEW(pcib, sizeof(struct pcib_softc),
    pcib_match, pcib_attach, NULL, NULL);

static int
malta_isa_dma_may_bounce(bus_dma_tag_t t, bus_dmamap_t map, int flags,
	int *cookieflagsp)
{
        if (((map->_dm_size / PAGE_SIZE) + 1) > map->_dm_segcnt)
                *cookieflagsp |= _BUS_DMA_MIGHT_NEED_BOUNCE;

	return 0;
}

static int
pcib_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA)
		return (1);

	return (0);
}

static void
pcib_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args * const pa = aux;
	struct pcib_softc * const sc = device_private(self);
	const char * const xname = device_xname(self);
	char devinfo[256];
	int error;

	printf("\n");

	if (my_sc != NULL)
		panic("pcib_attach: already attached!");
	my_sc = sc;
	sc->sc_dev = self;

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf("%s: %s, (rev . 0x%02x)\n", xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;

	/*
	 * Initialize the DMA tag used for ISA DMA.
	 */
	error = bus_dmatag_subregion(pa->pa_dmat, MALTA_DMA_ISA_PHYSBASE,
	    MALTA_DMA_ISA_PHYSBASE + MALTA_DMA_ISA_SIZE - 1, &sc->sc_dmat, 0);
	if (error)
		panic("malta_dma_init: failed to create ISA dma tag: %d",
		    error);
	sc->sc_dmat->_may_bounce = malta_isa_dma_may_bounce;

	/*
	 * Map the PIC/ELCR registers.
	 */
	if (bus_space_map(sc->sc_iot, 0x4d0, 2, 0, &sc->sc_ioh_elcr) != 0)
		printf("%s: unable to map ELCR registers\n", xname);
	if (bus_space_map(sc->sc_iot, IO_ICU1, 2, 0, &sc->sc_ioh_icu1) != 0)
		printf("%s: unable to map ICU1 registers\n", xname);
	if (bus_space_map(sc->sc_iot, IO_ICU2, 2, 0, &sc->sc_ioh_icu2) != 0)
		printf("%s: unable to map ICU2 registers\n", xname);

	/* All interrupts default to "masked off". */
	sc->sc_imask = 0xffff;

	/* All interrupts default to edge-triggered. */
	sc->sc_elcr = 0;

	/*
	 * Initialize the 8259s.
	 */
	/* reset, program device, 4 bytes */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_ICW1,
	    ICW1_SELECT | ICW1_IC4);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_ICW2,
	    ICW2_VECTOR(0)/*XXX*/);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_ICW3,
	    ICW3_CASCADE(2));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_ICW4,
	    ICW4_8086);

	/* mask all interrupts */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW1,
	    sc->sc_imask & 0xff);

	/* enable special mask mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW3,
	    OCW3_SELECT | OCW3_SSMM | OCW3_SMM);

	/* read IRR by default */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW3,
	    OCW3_SELECT | OCW3_RR);

	/* reset, program device, 4 bytes */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_ICW1,
	    ICW1_SELECT | ICW1_IC4);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_ICW2,
	    ICW2_VECTOR(0)/*XXX*/);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_ICW3,
	    ICW3_CASCADE(2));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_ICW4,
	    ICW4_8086);

	/* mask all interrupts */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_OCW1,
	    sc->sc_imask & 0xff);

	/* enable special mask mode */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_OCW3,
	    OCW3_SELECT | OCW3_SSMM | OCW3_SMM);

	/* read IRR by default */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu2, PIC_OCW3,
	    OCW3_SELECT | OCW3_RR);

	/*
	 * Default all interrupts to edge-triggered.
	 */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 0,
	    sc->sc_elcr & 0xff);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh_elcr, 1,
	    (sc->sc_elcr >> 8) & 0xff);

	/*
	 * Some ISA interrupts are reserved for devices that
	 * we know are hard-wired to certain IRQs.
	 */
	sc->sc_reserved =
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
	sc->sc_ic.ic_v = sc;
	sc->sc_ic.ic_intr_evcnt = pcib_isa_intr_evcnt;
	sc->sc_ic.ic_intr_establish = pcib_isa_intr_establish;
	sc->sc_ic.ic_intr_disestablish = pcib_isa_intr_disestablish;
	sc->sc_ic.ic_intr_alloc = pcib_isa_intr_alloc;
	sc->sc_ic.ic_intr_string = pcib_isa_intr_string;

	pcib_ic = &sc->sc_ic;	/* XXX for external use */

	/* Initialize our interrupt table. */
	for (size_t i = 0; i < ICU_LEN; i++) {
#if 0
		char irqstr[8];		/* 4 + 2 + NULL + sanity */

		snprintf(irqstr, sizeof(irqstr), "irq %d", i);
		evcnt_attach_dynamic(&sc->sc_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "pcib", irqstr);
#else
		evcnt_attach_dynamic(&sc->sc_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "pcib", isa_intrnames[i]);
#endif
		LIST_INIT(&sc->sc_intrtab[i].intr_q);
		sc->sc_intrtab[i].intr_type = IST_NONE;
	}

	/* Hook up our interrupt handler. */
	sc->sc_ih = evbmips_intr_establish(MALTA_SOUTHBRIDGE_INTR, pcib_intr, sc);
	if (sc->sc_ih == NULL)
		printf("%s: WARNING: unable to register interrupt handler\n",
		    xname);


	/*
	 * Disable ISA interrupts before returning to YAMON.
	 */
	if (shutdownhook_establish(pcib_cleanup, sc) == NULL)
		panic("pcib_attach: could not establish shutdown hook");

	config_defer(self, pcib_bridge_callback);
}

static void
pcib_bridge_callback(device_t self)
{
	struct pcib_softc *sc = device_private(self);
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));

	iba.iba_iot = sc->sc_iot;
	iba.iba_memt = sc->sc_memt;
	iba.iba_dmat = sc->sc_dmat;

	iba.iba_ic = &sc->sc_ic;
	iba.iba_ic->ic_attach_hook = pcib_isa_attach_hook;
	iba.iba_ic->ic_detach_hook = pcib_isa_detach_hook;

	config_found(self, &iba, isabusprint, CFARGS_NONE);
}

static void
pcib_isa_attach_hook(device_t parent, device_t self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}

static void
pcib_isa_detach_hook(isa_chipset_tag_t ic, device_t self)
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
			    PIC_OCW2, OCW2_SELECT | OCW2_EOI | OCW2_SL |
			    OCW2_ILS(irq & 7));
			irq = 2;
		}

		bus_space_write_1(sc->sc_iot, sc->sc_ioh_icu1, PIC_OCW2,
		    OCW2_SELECT | OCW2_EOI | OCW2_SL | OCW2_ILS(irq));
	}
}

const char *
pcib_isa_intr_string(void *v, int irq, char *buf, size_t len)
{
	if (irq == 0 || irq >= ICU_LEN || irq == 2)
		panic("%s: bogus isa irq 0x%x", __func__, irq);

	snprintf(buf, len, "isa irq %d", irq);
	return buf;
}

const struct evcnt *
pcib_isa_intr_evcnt(void *v, int irq)
{

	if (irq == 0 || irq >= ICU_LEN || irq == 2)
		panic("pcib_isa_intr_evcnt: bogus isa irq 0x%x", irq);

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

	ih = kmem_alloc(sizeof(*ih), KM_SLEEP);
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

	kmem_free(ih, sizeof(*ih));
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
