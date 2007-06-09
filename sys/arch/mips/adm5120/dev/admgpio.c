/* $NetBSD: admgpio.c,v 1.1.8.2 2007/06/09 21:36:53 ad Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: admgpio.c,v 1.1.8.2 2007/06/09 21:36:53 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kernel.h>	/* for hz */

#include <machine/bus.h>

#include <dev/gpio/gpiovar.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_mainbusvar.h>

static inline uint32_t
admgpio_read(struct mainbus_softc *sc)
{
	return bus_space_read_4(sc->sc_obiot, sc->sc_gpioh, ADM5120_GPIO0);
}

static inline void
admgpio_write(struct mainbus_softc *sc, uint32_t val)
{
	bus_space_write_4(sc->sc_obiot, sc->sc_gpioh, ADM5120_GPIO0, val);
}

static void
admgpio_pin_ctl(void *cookie, int pin, int flags)
{
	struct mainbus_softc *sc = cookie;
	uint32_t gpio0, mask;

	KASSERT(flags == GPIO_PIN_INPUT || flags == GPIO_PIN_OUTPUT);

	mask = __SHIFTIN(1 << pin, ADM5120_GPIO0_OE);
	gpio0 = admgpio_read(sc);

	if (flags == GPIO_PIN_OUTPUT)
		admgpio_write(sc, gpio0 | mask);
	else
		admgpio_write(sc, gpio0 & ~mask);
}

static int
admgpio_pin_read(void *cookie, int pin)
{
	struct mainbus_softc *sc = cookie;
	uint32_t gpio0, mask;

	KASSERT(pin >= 0 && pin < 8);

	if (sc->sc_pins[pin].pin_flags == GPIO_PIN_INPUT)
		mask = __SHIFTIN(1 << pin, ADM5120_GPIO0_IV);
	else
		mask = __SHIFTIN(1 << pin, ADM5120_GPIO0_OV);

	gpio0 = admgpio_read(sc);

	return ((gpio0 & mask) != 0) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}

static void
admgpio_pin_write(void *cookie, int pin, int value)
{
	struct mainbus_softc *sc = cookie;
	uint32_t gpio0, mask;

	KASSERT(pin >= 0 && pin < 8);

	mask = __SHIFTIN(1 << pin, ADM5120_GPIO0_OV);
	gpio0 = admgpio_read(sc);

	admgpio_write(sc,
	    (value == GPIO_PIN_HIGH) ? (gpio0 | mask) : (gpio0 & ~mask));
}

device_t
admgpio_attach(struct mainbus_softc *sc)
{
	int pin;
	uint32_t oe;
	struct gpiobus_attach_args gba;

	oe = __SHIFTOUT(admgpio_read(sc), ADM5120_GPIO0_OE);

	for (pin = 0; pin < __arraycount(sc->sc_pins); pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;

		if ((oe & (1 << pin)) != 0)
			sc->sc_pins[pin].pin_flags = GPIO_PIN_OUTPUT;
		else
			sc->sc_pins[pin].pin_flags = GPIO_PIN_INPUT;

		sc->sc_pins[pin].pin_state = admgpio_pin_read(sc, pin);
	}

	sc->sc_gp.gp_cookie = sc;
	sc->sc_gp.gp_pin_read = admgpio_pin_read;
	sc->sc_gp.gp_pin_write = admgpio_pin_write;
	sc->sc_gp.gp_pin_ctl = admgpio_pin_ctl;

	gba.gba_gc = &sc->sc_gp;
	gba.gba_pins = &sc->sc_pins[0];
	gba.gba_npins = __arraycount(sc->sc_pins);

	/* Attach GPIO framework */
	return config_found_ia(&sc->sc_dev, "gpiobus", &gba, gpiobus_print);
}
