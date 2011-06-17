/*	$NetBSD: gpio_opb.c,v 1.8 2011/06/17 19:03:02 matt Exp $	*/

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

#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/gpioreg.h>

struct gpio_opb_softc {
	device_t		sc_dev;		/* device generic */
	/* GPIO interface */
	bus_space_tag_t		sc_gpio_iot;
	bus_space_handle_t	sc_gpio_ioh;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[GPIO_NPINS];
};

static int	gpio_opb_match(device_t, cfdata_t, void *);
static void	gpio_opb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(opbgpio, sizeof(struct gpio_opb_softc),
	gpio_opb_match, gpio_opb_attach, NULL, NULL);

static int	gpio_opb_pin_read(void *, int);
static void	gpio_opb_pin_write(void *, int, int);
static void	gpio_opb_pin_ctl(void *, int, int);

static inline uint32_t
gpio_read(struct gpio_opb_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, o);
}

static inline void
gpio_write(struct gpio_opb_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, o, v);
}

static inline void
gpio_set(struct gpio_opb_softc *sc, bus_size_t o, uint32_t v)
{
	gpio_write(sc, o, gpio_read(sc, o) | v);
}

static inline void
gpio_clear(struct gpio_opb_softc *sc, bus_size_t o, uint32_t v)
{
	gpio_write(sc, o, gpio_read(sc, o) & ~v);
}

static int
gpio_opb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct opb_attach_args * const oaa = aux;

	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
gpio_opb_attach(device_t parent, device_t self, void *aux)
{
	struct gpio_opb_softc * const sc = device_private(self);
	struct opb_attach_args * const oaa = aux;
	struct gpiobus_attach_args gba;
	uint32_t reg_ir, reg_tcr, reg_odr;

	aprint_naive(": GPIO controller\n");
	aprint_normal(": On-Chip GPIO controller\n");

	sc->sc_dev = self;

	/* Map GPIO I/O space */
	sc->sc_gpio_iot = oaa->opb_bt;
	bus_space_map(sc->sc_gpio_iot, oaa->opb_addr,
		GPIO_NREG, 0, &sc->sc_gpio_ioh);

	/* Read current register status */
	reg_ir  = gpio_read(sc, GPIO_IR);
	reg_tcr = gpio_read(sc, GPIO_TCR);
	reg_odr = gpio_read(sc, GPIO_ODR);

	/* Initialize pins array */
	gpio_pin_t *pin = sc->sc_gpio_pins;
	for (u_int i = 0 ; i < GPIO_NPINS ; i++, pin++) {
		const uint32_t pin_mask = 1 << GPIO_PIN_SHIFT(i + 1);
		pin->pin_num = i;
		pin->pin_caps = GPIO_PIN_INOUT
				 | GPIO_PIN_OPENDRAIN
				 | GPIO_PIN_TRISTATE;

		/* current defaults */
		pin->pin_flags =
		    (reg_odr & pin_mask)
			? GPIO_PIN_OPENDRAIN
			: ((reg_tcr & pin_mask)
			    ? GPIO_PIN_INOUT
			    : GPIO_PIN_TRISTATE);
		pin->pin_state = (reg_ir & pin_mask) != 0;
		pin->pin_mapped = 0;
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
	(void) config_found(self, &gba, gpiobus_print);
}

static int
gpio_opb_pin_read(void *arg, int pin)
{
	struct gpio_opb_softc * const sc = arg;
	const u_int p = (pin % GPIO_NPINS) + 1;
	uint32_t reg_ir = gpio_read(sc, GPIO_IR);

	return (reg_ir >> GPIO_PIN_SHIFT(p)) & 0x01;
}

static void
gpio_opb_pin_write(void *arg, int pin, int value)
{
	struct gpio_opb_softc * const sc = arg;
	const u_int p = (pin % GPIO_NPINS) + 1;
	const uint32_t pin_mask = 1 << GPIO_PIN_SHIFT(p);

	if (value == 0) {
		gpio_clear(sc, GPIO_OR, pin_mask);
	} else if (value == 1) {
		gpio_set(sc, GPIO_OR, pin_mask);
	}
}

static void
gpio_opb_pin_ctl(void *arg, int pin, int flags)
{
	struct gpio_opb_softc * const sc = arg;
	const u_int p = (pin % GPIO_NPINS) + 1;
	const uint32_t pin_mask = 1 << GPIO_PIN_SHIFT(p);

	if (flags & GPIO_PIN_INOUT) {
		/* GPIOn_ODR register bit is 0 */
		gpio_clear(sc, GPIO_ODR, pin_mask);

		/* GPIOn_TCR register bit is 1 */
		gpio_set(sc, GPIO_TCR, pin_mask);
	}

	if (flags & GPIO_PIN_TRISTATE) {
		/* GPIOn_ODR register bit is 0 */
		gpio_clear(sc, GPIO_ODR, pin_mask);

		/* GPIOn_TCR register bit is 0 */
		gpio_clear(sc, GPIO_TCR, pin_mask);
	}

	if (flags & GPIO_PIN_OPENDRAIN) {
		/* GPIOn_ODR register bit is 1 */
		gpio_set(sc, GPIO_ODR, pin_mask);
	}
}
