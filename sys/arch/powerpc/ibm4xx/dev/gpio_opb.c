/*	$NetBSD: gpio_opb.c,v 1.4.6.1 2006/04/22 11:37:53 simonb Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/pio.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <powerpc/ibm4xx/dcr405gp.h>
#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/gpioreg.h>

struct gpio_opb_softc {
	struct device		sc_dev;		/* device generic */
	/* GPIO interface */
	bus_space_tag_t		sc_gpio_iot;
	bus_space_handle_t	sc_gpio_ioh;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[GPIO_NPINS];
};

static int	gpio_opb_match(struct device *, struct cfdata *, void *);
static void	gpio_opb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(opbgpio, sizeof(struct gpio_opb_softc),
	gpio_opb_match, gpio_opb_attach, NULL, NULL);

static int	gpio_opb_pin_read(void *, int);
static void	gpio_opb_pin_write(void *, int, int);
static void	gpio_opb_pin_ctl(void *, int, int);


static int
gpio_opb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
gpio_opb_attach(struct device *parent, struct device *self, void *aux)
{
	struct gpio_opb_softc *sc = (struct gpio_opb_softc *)self;
	struct opb_attach_args *oaa = aux;
	struct gpiobus_attach_args gba;
	int i;
	uint32_t reg1, reg2, reg3;

	aprint_naive(": GPIO controller\n");
	aprint_normal(": On-Chip GPIO controller\n");

	/* Map GPIO I/O space */
	sc->sc_gpio_iot = oaa->opb_bt;
	bus_space_map(sc->sc_gpio_iot, oaa->opb_addr,
		GPIO_NREG, 0, &sc->sc_gpio_ioh);

	/* Read current register status */
	reg1 = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GPIO_IR);
	reg2 = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GPIO_TCR);
	reg3 = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GPIO_ODR);

	/* Initialize ping array */
	for (i = 0 ; i < GPIO_NPINS ; i++) {
		int p = i + 1;
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INOUT
						| GPIO_PIN_OPENDRAIN
						| GPIO_PIN_TRISTATE;

		/* current defaults */
		sc->sc_gpio_pins[i].pin_flags =
			((reg3 >> GPIO_PIN_SHIFT(p)) & 0x01)
			? GPIO_PIN_OPENDRAIN
			: (((reg2 >> GPIO_PIN_SHIFT(p)) & 0x01)
				? GPIO_PIN_INOUT
				: GPIO_PIN_TRISTATE);
		sc->sc_gpio_pins[i].pin_state =
			((reg1 >> GPIO_PIN_SHIFT(p)) & 0x01);
		sc->sc_gpio_pins[i].pin_mapped = 0;
	}

	/* Create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = gpio_opb_pin_read;
	sc->sc_gpio_gc.gp_pin_write = gpio_opb_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = gpio_opb_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = GPIO_NPINS;

	/* Attach GPIO framework */
	(void) config_found(&sc->sc_dev, &gba, gpiobus_print);
}

static int
gpio_opb_pin_read(void *arg, int pin)
{
	struct gpio_opb_softc *sc = arg;
	uint32_t data;
	int p;

	p = pin % GPIO_NPINS;
	p = p + 1;

	data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GPIO_IR);

	return (data >> GPIO_PIN_SHIFT(p)) & 0x01;
}

static void
gpio_opb_pin_write(void *arg, int pin, int value)
{
	struct gpio_opb_softc *sc = arg;
	uint32_t data;
	int p;

	p = pin % GPIO_NPINS;
	p = p + 1;

	data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GPIO_OR);
	if (value == 0) {
		data &= ~(1 << GPIO_PIN_SHIFT(p));
	} else if (value == 1) {
		data |= (1 << GPIO_PIN_SHIFT(p));
	}

	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, GPIO_OR, data);
}

static void
gpio_opb_pin_ctl(void *arg, int pin, int flags)
{
	struct gpio_opb_softc *sc = arg;
	uint32_t data;
	int p;

	p = pin % GPIO_NPINS;
	p = p + 1;

	if (flags & GPIO_PIN_INOUT) {
		/* GPIOn_ODR register bit is 0 */
		data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_ODR);
		data &= ~(1 << GPIO_PIN_SHIFT(p));
		bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_ODR, data);
		/* GPIOn_TCR register bit is 1 */
		data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_TCR);
		data |= (1 << GPIO_PIN_SHIFT(p));
		bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_TCR, data);
	}

	if (flags & GPIO_PIN_TRISTATE) {
		/* GPIOn_ODR register bit is 0 */
		data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_ODR);
		data &= ~(1 << GPIO_PIN_SHIFT(p));
		bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_ODR, data);
		/* GPIOn_TCR register bit is 0 */
		data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_TCR);
		data &= ~(1 << GPIO_PIN_SHIFT(p));
		bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_TCR, data);
	}

	if (flags & GPIO_PIN_OPENDRAIN) {
		/* GPIOn_ODR register bit is 1 */
		data = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_ODR);
		data |= (1 << GPIO_PIN_SHIFT(p));
		bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh,
					GPIO_ODR, data);
	}
}
