/* $NetBSD: pl061.c,v 1.1 2018/10/15 23:50:48 jmcneill Exp $ */

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl061.c,v 1.1 2018/10/15 23:50:48 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>
#include "gpio.h"

#include <dev/ic/pl061reg.h>
#include <dev/ic/pl061var.h>

void
plgpio_attach(struct plgpio_softc *sc)
{
	struct gpiobus_attach_args gba;
	u_int pin;

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = plgpio_pin_read;
	sc->sc_gc.gp_pin_write = plgpio_pin_write;
	sc->sc_gc.gp_pin_ctl = plgpio_pin_ctl;

	const uint32_t cnf = PLGPIO_READ(sc, PL061_GPIOAFSEL_REG);

	for (pin = 0; pin < 8; pin++) {
		sc->sc_pins[pin].pin_num = pin;
		/* skip pins in hardware control mode */
		if ((cnf & __BIT(pin)) != 0)
			continue;
		sc->sc_pins[pin].pin_caps =
		    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_TRISTATE;
		sc->sc_pins[pin].pin_state =
		    plgpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = 8;

#if NGPIO > 0
	(void)config_found_ia(sc->sc_dev, "gpiobus", &gba,
	    gpiobus_print);
#endif
}

int
plgpio_pin_read(void *priv, int pin)
{
	struct plgpio_softc * const sc = priv;

	const uint32_t v = PLGPIO_READ(sc, PL061_GPIODATA_REG(1<<pin));

	return (v >> pin) & 1;
}

void
plgpio_pin_write(void *priv, int pin, int val)
{
	struct plgpio_softc * const sc = priv;

	PLGPIO_WRITE(sc, PL061_GPIODATA_REG(1 << pin), val << pin);
}

void
plgpio_pin_ctl(void *priv, int pin, int flags)
{
	struct plgpio_softc * const sc = priv;
	uint32_t v;

	if (flags & GPIO_PIN_INPUT) {
		v = PLGPIO_READ(sc, PL061_GPIODIR_REG);
		v &= ~(1 << pin);
		PLGPIO_WRITE(sc, PL061_GPIODIR_REG, v);
	} else if (flags & GPIO_PIN_OUTPUT) {
		v = PLGPIO_READ(sc, PL061_GPIODIR_REG);
		v |= (1 << pin);
		PLGPIO_WRITE(sc, PL061_GPIODIR_REG, v);
	}
}
