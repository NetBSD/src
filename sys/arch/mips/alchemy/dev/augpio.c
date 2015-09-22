/* $NetBSD: augpio.c,v 1.7.24.1 2015/09/22 12:05:46 skrll Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: augpio.c,v 1.7.24.1 2015/09/22 12:05:46 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <dev/gpio/gpiovar.h>

#include <mips/locore.h>

#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/dev/augpioreg.h>
#include <mips/alchemy/dev/augpiovar.h>

struct augpio_softc {
	device_t			sc_dev;
	struct gpio_chipset_tag		sc_gc;
	gpio_pin_t			sc_pins[AUGPIO_NPINS];
	int				sc_npins;
	bus_space_tag_t			sc_bst;
	int				sc_caps;
	const char 			*sc_name;
	int				(*sc_getctl)(void *, int);
};

static int augpio_match(device_t, struct cfdata *, void *);
static void augpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(augpio, sizeof(struct augpio_softc),
    augpio_match, augpio_attach, NULL, NULL);

#define	GETREG(x)	\
	(*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(x))
#define	PUTREG(x, v)	\
	((*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

#define	GETGPIO(x)	GETREG(GPIO_BASE + (x))
#define	PUTGPIO(x,v)	PUTREG(GPIO_BASE + (x), (v))
#define	GETGPIO2(x)	GETREG(GPIO2_BASE + (x))
#define	PUTGPIO2(x,v)	PUTREG(GPIO2_BASE + (x), (v))

int
augpio_match(device_t parent, struct cfdata *match, void *aux)
{
	struct aubus_attach_args *aa = (struct aubus_attach_args *)aux;

	if (strcmp(aa->aa_name, "augpio") != 0)
		return 0;

	return 1;
}

void
augpio_attach(device_t parent, device_t self, void *aux)
{
	int	pin;

	struct augpio_softc *sc = device_private(self);
	struct aubus_attach_args *aa = aux;
	struct gpiobus_attach_args gba;

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_st;
	sc->sc_npins = aa->aa_addrs[1];
	sc->sc_gc.gp_cookie = sc;

	if (aa->aa_addrs[0] == GPIO_BASE) {

		sc->sc_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_TRISTATE;
		sc->sc_gc.gp_pin_read = augpio_read;
		sc->sc_gc.gp_pin_write = augpio_write;
		sc->sc_gc.gp_pin_ctl = augpio_ctl;
		sc->sc_getctl = augpio_getctl;
		sc->sc_name = "primary block";

	} else if (aa->aa_addrs[0] == GPIO2_BASE) {
		/*
		 * We rely on firmware (or platform init code) to initialize
		 * the GPIO2 block.  We can't do it ourselves, because
		 * resetting the GPIO2 block can have nasty effects (e.g.
		 * reset PCI bus...)
		 */
		sc->sc_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_gc.gp_pin_read = augpio2_read;
		sc->sc_gc.gp_pin_write = augpio2_write;
		sc->sc_gc.gp_pin_ctl = augpio2_ctl;
		sc->sc_getctl = augpio2_getctl;
		sc->sc_name = "secondary block";

	} else {
		aprint_error(": unidentified block\n");
		return;
	}

	for (pin = 0; pin < sc->sc_npins; pin++) {
		gpio_pin_t	*pp = &sc->sc_pins[pin];

		pp->pin_num = pin;
		pp->pin_caps = sc->sc_caps;
		pp->pin_flags = sc->sc_getctl(sc, pin);
		pp->pin_state = sc->sc_gc.gp_pin_read(sc, pin);
	}

	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = sc->sc_npins;

	aprint_normal(": Alchemy GPIO, %s\n", sc->sc_name);
	aprint_naive("\n");
	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}

int
augpio_read(void *arg, int pin)
{

	pin = 1 << pin;

	if (GETGPIO(AUGPIO_PINSTATERD) & pin)
		return GPIO_PIN_HIGH;
	else
		return GPIO_PIN_LOW;
}

void
augpio_write(void *arg, int pin, int value)
{

	pin = 1 << pin;
	PUTGPIO(value ? AUGPIO_OUTPUTSET : AUGPIO_OUTPUTCLR, pin);
}

void
augpio_ctl(void *arg, int pin, int flags)
{
	bus_addr_t		reg;

	pin = 1 << pin;

	if (flags & (GPIO_PIN_TRISTATE|GPIO_PIN_INPUT)) {
		reg = AUGPIO_TRIOUTCLR;
	} else if (flags & GPIO_PIN_OUTPUT) {
		uint32_t		out;
		out = GETGPIO(AUGPIO_OUTPUTRD);
		reg = pin & out ? AUGPIO_OUTPUTSET : AUGPIO_OUTPUTCLR;
	} else {
		return;
	}
	
	PUTGPIO(reg, pin);
}

int
augpio_getctl(void *arg, int pin)
{

	if (GETGPIO(AUGPIO_TRIOUTRD) & pin)
		return GPIO_PIN_OUTPUT;
	else
		return GPIO_PIN_INPUT;
}

int
augpio2_read(void *arg, int pin)
{

	pin = 1 << pin;
	
	if (GETGPIO2(AUGPIO2_PINSTATE) & pin)
		return GPIO_PIN_HIGH;
	else
		return GPIO_PIN_LOW;
}

void
augpio2_write(void *arg, int pin, int value)
{

	pin = 1 << pin;

	if (value) {
		pin = pin | (pin << 16);
	} else {
		pin = (pin << 16);
	}

	PUTGPIO2(AUGPIO2_OUTPUT, pin);
}

void
augpio2_ctl(void *arg, int pin, int flags)
{
	uint32_t		dir;

	pin = 1 << pin;

	dir = GETGPIO2(AUGPIO2_DIR);

	if (flags & GPIO_PIN_INPUT) {
		dir |= pin;
	} else if (flags & GPIO_PIN_OUTPUT) {
		dir &= ~pin;
	}
	PUTGPIO2(AUGPIO2_DIR, dir);
}

int
augpio2_getctl(void *arg, int pin)
{
	uint32_t		dir;

	pin = 1 << pin;

	dir = GETGPIO2(AUGPIO2_DIR);
	if (dir & (uint32_t)pin) {
		return GPIO_PIN_OUTPUT;
	} else {
		return GPIO_PIN_INPUT;
	}
}

