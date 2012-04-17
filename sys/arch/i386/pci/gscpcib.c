/*	$NetBSD: gscpcib.c,v 1.17.2.1 2012/04/17 00:06:29 yamt Exp $	*/
/*	$OpenBSD: gscpcib.c,v 1.3 2004/10/05 19:02:33 grange Exp $	*/
/*
 * Copyright (c) 2004 Alexander Yurchenko <grange@openbsd.org>
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
 * Special driver for the National Semiconductor Geode SC1100 PCI-ISA bridge
 * that attaches instead of pcib(4). In addition to the core pcib(4)
 * functionality this driver provides support for the GPIO interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gscpcib.c,v 1.17.2.1 2012/04/17 00:06:29 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kernel.h>

#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/gpio/gpiovar.h>

#include <i386/pci/gscpcibreg.h>
#include <arch/x86/pci/pcibvar.h>

#include "gpio.h"

struct gscpcib_softc {
	struct pcib_softc sc_pcib;

	bool sc_gpio_present;
	device_t sc_gpiobus;

	/* GPIO interface */
	bus_space_tag_t sc_gpio_iot;
	bus_space_handle_t sc_gpio_ioh;
	struct gpio_chipset_tag sc_gpio_gc;
	gpio_pin_t sc_gpio_pins[GSCGPIO_NPINS];
};

int	gscpcib_match(device_t, cfdata_t, void *);
void	gscpcib_attach(device_t, device_t, void *);
int	gscpcib_detach(device_t, int);
int	gscpcib_rescan(device_t, const char *, const int *);
void	gscpcib_childdetached(device_t, device_t);

int	gscpcib_gpio_pin_read(void *, int);
void	gscpcib_gpio_pin_write(void *, int, int);
void	gscpcib_gpio_pin_ctl(void *, int, int);

CFATTACH_DECL3_NEW(gscpcib, sizeof(struct gscpcib_softc),
	gscpcib_match, gscpcib_attach, gscpcib_detach, NULL, gscpcib_rescan,
	gscpcib_childdetached, DVF_DETACH_SHUTDOWN);

extern struct cfdriver gscpcib_cd;

void
gscpcib_childdetached(device_t self, device_t child)
{
	struct gscpcib_softc *sc = device_private(self);

	if (sc->sc_gpiobus == child)
		sc->sc_gpiobus = NULL;
	else
		pcibchilddet(self, child);
}

int
gscpcib_rescan(device_t self, const char *ifattr, const int *loc)
{
#if NGPIO > 0
	struct gscpcib_softc *sc = device_private(self);

	/* Attach GPIO framework */
	if (sc->sc_gpio_present && ifattr_match(ifattr, "gpiobus") &&
	    sc->sc_gpiobus == NULL) {
		struct gpiobus_attach_args gba;

		gba.gba_gc = &sc->sc_gpio_gc;
		gba.gba_pins = sc->sc_gpio_pins;
		gba.gba_npins = GSCGPIO_NPINS;

		sc->sc_gpiobus = config_found_sm_loc(self, "gpiobus", loc,
		    &gba, gpiobus_print, NULL);
		return 0;
	}
#endif

	return pcibrescan(self, ifattr, loc);
}

int
gscpcib_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return (0);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NS &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NS_SC1100_ISA)
		return (2);	/* supersede pcib(4) */

	return (0);
}

void
gscpcib_attach(device_t parent, device_t self, void *aux)
{
	struct gscpcib_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t gpiobase;
	int i;

	/* Map GPIO I/O space */
	gpiobase = pci_conf_read(pa->pa_pc, pa->pa_tag, GSCGPIO_BASE);
	sc->sc_gpio_iot = pa->pa_iot;
	if (bus_space_map(sc->sc_gpio_iot, PCI_MAPREG_IO_ADDR(gpiobase),
	    GSCGPIO_SIZE, 0, &sc->sc_gpio_ioh)) {
		printf(": failed to map GPIO I/O space");
		goto corepcib;
	}

	/* Initialize pins array */
	for (i = 0; i < GSCGPIO_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN |
		    GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
		    GPIO_PIN_PULLUP;

		/* safe defaults */
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_TRISTATE;
		sc->sc_gpio_pins[i].pin_state = GPIO_PIN_LOW;
		gscpcib_gpio_pin_ctl(sc, i, sc->sc_gpio_pins[i].pin_flags);
		gscpcib_gpio_pin_write(sc, i, sc->sc_gpio_pins[i].pin_state);
	}

	/* Create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = gscpcib_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = gscpcib_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = gscpcib_gpio_pin_ctl;

	sc->sc_gpio_present = true;

corepcib:
	/* Provide core pcib(4) functionality */
	pcibattach(parent, self, aux);

	gscpcib_rescan(self, "gpiobus", NULL);
}

int
gscpcib_detach(device_t self, int flags)
{
	int rc;
	struct gscpcib_softc *sc = device_private(self);

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

	if ((rc = pcibdetach(self, flags)) != 0)
		return rc;

	if (sc->sc_gpio_present)
		bus_space_unmap(sc->sc_gpio_iot, sc->sc_gpio_ioh, GSCGPIO_SIZE);

	return rc;
}

static inline void
gscpcib_gpio_pin_select(struct gscpcib_softc *sc, int pin)
{
	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GSCGPIO_SEL, pin);
}

int
gscpcib_gpio_pin_read(void *arg, int pin)
{
	struct gscpcib_softc *sc = arg;
	int reg, shift;
	uint32_t data;

	reg = (pin < 32 ? GSCGPIO_GPDI0 : GSCGPIO_GPDI1);
	shift = pin % 32;
	data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, reg);

	return ((data >> shift) & 0x1);
}

void
gscpcib_gpio_pin_write(void *arg, int pin, int value)
{
	struct gscpcib_softc *sc = arg;
	int reg, shift;
	uint32_t data;

	reg = (pin < 32 ? GSCGPIO_GPDO0 : GSCGPIO_GPDO1);
	shift = pin % 32;
	data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, reg);
	if (value == 0)
		data &= ~(1 << shift);
	else if (value == 1)
		data |= (1 << shift);

	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, reg, data);
}

void
gscpcib_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct gscpcib_softc *sc = arg;
	uint32_t conf;

	gscpcib_gpio_pin_select(sc, pin);
	conf = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
	    GSCGPIO_CONF);

	conf &= ~(GSCGPIO_CONF_OUTPUTEN | GSCGPIO_CONF_PUSHPULL |
	    GSCGPIO_CONF_PULLUP);
	if ((flags & GPIO_PIN_TRISTATE) == 0)
		conf |= GSCGPIO_CONF_OUTPUTEN;
	if (flags & GPIO_PIN_PUSHPULL)
		conf |= GSCGPIO_CONF_PUSHPULL;
	if (flags & GPIO_PIN_PULLUP)
		conf |= GSCGPIO_CONF_PULLUP;
	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
	    GSCGPIO_CONF, conf);
}
