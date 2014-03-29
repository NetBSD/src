/*	$NetBSD: ssio.c,v 1.2 2014/03/29 19:28:28 christos Exp $	*/

/*	$OpenBSD: ssio.c,v 1.7 2009/03/08 22:19:04 miod Exp $	*/

/*
 * Copyright (c) 2007 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Driver for the National Semiconductor PC87560 Legacy I/O chip.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/iomod.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>

#include <hppa/hppa/machdep.h>
#include <hppa/dev/ssiovar.h>

#include "ukbd.h"
#if NUKBD > 0
#include <dev/usb/ohcireg.h>
#include <dev/usb/ukbdvar.h>
#endif

/* PCI config space. */
#define SSIO_PCI_DMA_RC2	0x64
#define SSIO_PCI_INT_TC1	0x67
#define SSIO_PCI_INT_TC2	0x68
#define SSIO_PCI_INT_RC1	0x69
#define SSIO_PCI_INT_RC2	0x6a
#define SSIO_PCI_INT_RC3	0x6b
#define SSIO_PCI_INT_RC4	0x6c
#define SSIO_PCI_INT_RC5	0x6d
#define SSIO_PCI_INT_RC6	0x6e
#define SSIO_PCI_INT_RC7	0x6f
#define SSIO_PCI_INT_RC8	0x70
#define SSIO_PCI_INT_RC9	0x71
#define SSIO_PCI_SP1BAR		0x94
#define SSIO_PCI_SP2BAR		0x98
#define SSIO_PCI_PPBAR		0x9c

#define SSIO_PCI_INT_TC1_MASK	0xff
#define SSIO_PCI_INT_TC1_SHIFT	24

#define SSIO_PCI_INT_TC2_MASK	0xff
#define SSIO_PCI_INT_TC2_SHIFT	0

#define SSIO_PCI_INT_RC1_MASK	0xff
#define SSIO_PCI_INT_RC1_SHIFT	8

#define SSIO_PCI_INT_RC2_MASK	0xff
#define SSIO_PCI_INT_RC2_SHIFT	16

#define SSIO_PCI_INT_RC3_MASK	0xff
#define SSIO_PCI_INT_RC3_SHIFT	24

#define SSIO_PCI_INT_RC4_MASK	0xff
#define SSIO_PCI_INT_RC4_SHIFT	0

#define SSIO_PCI_INT_RC5_MASK	0xff
#define SSIO_PCI_INT_RC5_SHIFT	8

#define SSIO_PCI_INT_RC6_MASK	0xff
#define SSIO_PCI_INT_RC6_SHIFT	16

#define SSIO_PCI_INT_RC7_MASK	0xff
#define SSIO_PCI_INT_RC7_SHIFT	24

#define SSIO_PCI_INT_RC8_MASK	0xff
#define SSIO_PCI_INT_RC8_SHIFT	0

#define SSIO_PCI_INT_RC9_MASK	0xff
#define SSIO_PCI_INT_RC9_SHIFT	8

/* Cascaded i8259-compatible PICs. */
#define SSIO_PIC1	0x20
#define SSIO_PIC2	0xa0
#define SSIO_NINTS	16

struct ssio_iv {
	int (*handler)(void *);
	void *arg;
};

struct ssio_iv ssio_intr_table[SSIO_NINTS];

struct ssio_softc {
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ic1h;
	bus_space_handle_t sc_ic2h;
	void *sc_ih;
};

int	ssio_match(device_t, cfdata_t, void *);
void	ssio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ssio, sizeof(struct ssio_softc), ssio_match, ssio_attach, NULL,
    NULL);

extern struct cfdriver ssio_cd;

int	ssio_intr(void *);
int	ssio_print(void *, const char *);

int
ssio_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	pcireg_t bhlc, id;
	pcitag_t tag;

	/*
	 * The firmware doesn't always switch the IDE function into native
	 * mode.  So we do that ourselves since it makes life much simpler.
	 * Note that we have to do this in the match function since the
	 * Legacy I/O function attaches after the IDE function.
	 */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NS &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NS_PC87415) {
		bhlc = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_BHLC_REG);
		if (!PCI_HDRTYPE_MULTIFN(bhlc))
			return (0);

		tag = pci_make_tag(pa->pa_pc, pa->pa_bus, pa->pa_device, 1);
		id = pci_conf_read(pa->pa_pc, tag, PCI_ID_REG);
		if (PCI_VENDOR(id) != PCI_VENDOR_NS ||
		    PCI_PRODUCT(id) != PCI_PRODUCT_NS_PC87560)
			return (0);

		pa->pa_class |= PCIIDE_INTERFACE_PCI(0) << PCI_INTERFACE_SHIFT;
		pa->pa_class |= PCIIDE_INTERFACE_PCI(1) << PCI_INTERFACE_SHIFT;
		pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_CLASS_REG,
		    pa->pa_class);
		return (0);
	}

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NS)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NS_PC87560)
		return 1;

	return 0;
}

void
ssio_attach(device_t parent, device_t self, void *aux)
{
	struct ssio_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct ssio_attach_args saa;
	pci_intr_handle_t ih;
	char devinfo[256];
	const char *intrstr;
	pcireg_t reg;
	int revision;
#if NUKBD > 0
	pcitag_t tag;
	int pagezero_cookie;
#endif
	char buf[PCI_INTRSTR_LEN];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof devinfo);
	revision = PCI_REVISION(pa->pa_class);
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo, revision);

	sc->sc_iot = pa->pa_iot;
	if (bus_space_map(sc->sc_iot, SSIO_PIC1, 2, 0, &sc->sc_ic1h)) {
		aprint_error_dev(self, "unable to map PIC1 registers\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, SSIO_PIC2, 2, 0, &sc->sc_ic2h)) {
		aprint_error_dev(self, "unable to map PIC2 registers\n");
		goto unmap_ic1;
	}

	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "unable to map interrupt\n");
		goto unmap_ic2;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih, buf, sizeof(buf));
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_TTY, ssio_intr,
	    sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "could not establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto unmap_ic2;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	/*
	 * We use the following interrupt mapping:
	 *
	 * USB (INTD#)		IRQ 1
	 * IDE Channel 1	IRQ 5
	 * Serial Port 1	IRQ 4
	 * Serial Port 2	IRQ 3
	 * Parallel Port	IRQ 7
	 *
	 * USB and IDE are set to level triggered, all others to edge
	 * triggered.
	 *
	 * We disable all other interrupts since we don't need them.
	 */
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, SSIO_PCI_DMA_RC2);
	reg &= ~(SSIO_PCI_INT_TC1_MASK << SSIO_PCI_INT_TC1_SHIFT);
	reg |= 0x22 << SSIO_PCI_INT_TC1_SHIFT;
	pci_conf_write(pa->pa_pc, pa->pa_tag, SSIO_PCI_DMA_RC2, reg);

	reg = 0;
	reg |= 0x34 << SSIO_PCI_INT_RC1_SHIFT;	/* SP1, SP2 */
	reg |= 0x07 << SSIO_PCI_INT_RC2_SHIFT;	/* PP */
	reg |= 0x05 << SSIO_PCI_INT_RC3_SHIFT;	/* IDE1 */
	pci_conf_write(pa->pa_pc, pa->pa_tag, SSIO_PCI_INT_TC2, reg);

	reg = 0;
	reg |= 0x10 << SSIO_PCI_INT_RC5_SHIFT;	/* INTD# (USB) */
	pci_conf_write(pa->pa_pc, pa->pa_tag, SSIO_PCI_INT_RC4, reg);

	/* Program PIC1. */
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 0, 0x11);
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 1, 0x00);
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 1, 0x04);
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 1, 0x01);

	/* Priority (3-7,0-2). */
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 0, 0xc2);

	/* Program PIC2. */
	bus_space_write_1(sc->sc_iot, sc->sc_ic2h, 0, 0x11);
	bus_space_write_1(sc->sc_iot, sc->sc_ic2h, 1, 0x00);
	bus_space_write_1(sc->sc_iot, sc->sc_ic2h, 1, 0x02);
	bus_space_write_1(sc->sc_iot, sc->sc_ic2h, 1, 0x01);

	/* Unmask all interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 1, 0x00);
	bus_space_write_1(sc->sc_iot, sc->sc_ic2h, 1, 0x00);

	/* Serial Port 1. */
	saa.saa_name = "com";
	saa.saa_iot = sc->sc_iot;
	saa.saa_iobase = pci_conf_read(pa->pa_pc, pa->pa_tag, SSIO_PCI_SP1BAR);
	saa.saa_iobase &= 0xfffffffe;
	saa.saa_irq = 4;
	config_found(self, &saa, ssio_print);

	/* Serial Port 2. */
	saa.saa_name = "com";
	saa.saa_iot = sc->sc_iot;
	saa.saa_iobase = pci_conf_read(pa->pa_pc, pa->pa_tag, SSIO_PCI_SP2BAR);
	saa.saa_iobase &= 0xfffffffe;
	saa.saa_irq = 3;
	config_found(self, &saa, ssio_print);

	/* Parallel Port. */
	saa.saa_name = "lpt";
	saa.saa_iot = sc->sc_iot;
	saa.saa_iobase = pci_conf_read(pa->pa_pc, pa->pa_tag, SSIO_PCI_PPBAR);
	saa.saa_iobase &= 0xfffffffe;
	saa.saa_irq = 7;
	config_found(self, &saa, ssio_print);

#if NUKBD > 0
	/*
	 * If a USB keybard is used for console input, the firmware passes
	 * the mmio address of the USB controller the keyboard is attached
	 * to.  Since we know the USB controller is function 2 on the same
	 * device and comes right after us (we're function 1 remember),
	 * this is a convenient spot to mark the USB keyboard as console
	 * if the address matches.
	 */
	tag = pci_make_tag(pa->pa_pc, pa->pa_bus, pa->pa_device, 2);
	reg = pci_conf_read(pa->pa_pc, tag, PCI_CBMEM);

	pagezero_cookie = hppa_pagezero_map();
	if (PAGE0->mem_kbd.pz_class == PCL_KEYBD &&
	    PAGE0->mem_kbd.pz_hpa == (struct iomod *)reg)
		ukbd_cnattach();
	hppa_pagezero_unmap(pagezero_cookie);
#endif

	return;

unmap_ic2:
	bus_space_unmap(sc->sc_iot, sc->sc_ic2h, 2);
unmap_ic1:
	bus_space_unmap(sc->sc_iot, sc->sc_ic1h, 2);
}

int
ssio_intr(void *v)
{
	struct ssio_softc *sc = v;
	struct ssio_iv *iv;
	int claimed = 0;
	int irq, isr;

	/* Poll for interrupt. */
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 0, 0x0c);
	irq = bus_space_read_1(sc->sc_iot, sc->sc_ic1h, 0);
	irq &= 0x07;

	if (irq == 7) {
		bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 0, 0x0b);
		isr = bus_space_read_1(sc->sc_iot, sc->sc_ic1h, 0);
		if ((isr & 0x80) == 0)
			/* Spurious interrupt. */
			return (0);
	}

	iv = &ssio_intr_table[irq];
	if (iv->handler)
		claimed = iv->handler(iv->arg);

	/* Signal EOI. */
	bus_space_write_1(sc->sc_iot, sc->sc_ic1h, 0, 0x60 | (irq & 0x0f));

	return (claimed);
}

void *
ssio_intr_establish(int pri, int irq, int (*handler)(void *), void *arg,
    const char *name)
{
	struct ssio_iv *iv;

	if (irq < 0 || irq >= SSIO_NINTS || ssio_intr_table[irq].handler)
		return (NULL);

	iv = &ssio_intr_table[irq];
	iv->handler = handler;
	iv->arg = arg;

	return (iv);
}

int
ssio_print(void *aux, const char *pnp)
{
	struct ssio_attach_args *saa = aux;

	if (pnp)
		printf("%s at %s", saa->saa_name, pnp);
	if (saa->saa_iobase) {
		printf(" offset %lx", saa->saa_iobase);
		if (!pnp && saa->saa_irq >= 0)
			printf(" irq %d", saa->saa_irq);
	}

	return (UNCONF);
}
