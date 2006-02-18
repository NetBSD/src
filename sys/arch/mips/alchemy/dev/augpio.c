/* $NetBSD: augpio.c,v 1.2.2.2 2006/02/18 15:38:41 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: augpio.c,v 1.2.2.2 2006/02/18 15:38:41 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kernel.h>

#include <dev/gpio/gpiovar.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/dev/augpioreg.h>
#include <mips/alchemy/dev/augpiovar.h>

struct augpio_softc {
	struct device			sc_dev;
	struct gpio_chipset_tag		sc_gc;
	gpio_pin_t			sc_pins[AUGPIO_NPINS];
	int				sc_npins;
	int				sc_isgpio2;
};

static int augpio_pin_read(void *, int);
static void augpio_pin_write(void *, int, int);
static void augpio_pin_ctl(void *, int, int);
static int augpio_match(struct device *, struct cfdata *, void *);
static void augpio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(augpio, sizeof(struct augpio_softc),
    augpio_match, augpio_attach, NULL, NULL);

#define	GETREG(x)	\
	(*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(x))
#define	PUTREG(x, v)	\
	((*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

static int augpio_found = 0;

int
augpio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct aubus_attach_args *aa = (struct aubus_attach_args *)aux;

	if (strcmp(aa->aa_name, "augpio") != 0)
		return 0;

	if (augpio_found)
		return 0;

	return 1;
}

void
augpio_attach(struct device *parent, struct device *self, void *aux)
{
	int	pin;

	struct augpio_softc *sc = (struct augpio_softc *)self;
	struct aubus_attach_args *aa = aux;
	struct gpiobus_attach_args gba;

	sc->sc_npins = 0;

	printf(": Alchemy GPIO");
	if (aa->aa_addrs[0] == SYS_BASE) {

		for (pin = 0; pin < aa->aa_addrs[1]; pin++) {

			sc->sc_pins[sc->sc_npins].pin_num = pin;
			sc->sc_pins[sc->sc_npins].pin_caps =
			    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
			    GPIO_PIN_TRISTATE;
			sc->sc_pins[sc->sc_npins].pin_flags =
			    au_gpio_ctl(pin, 0);
			sc->sc_pins[sc->sc_npins].pin_state =
			    au_gpio_read(pin);
			sc->sc_npins++;
		}
		sc->sc_isgpio2 = 0;
		printf(", primary block");

	} else if (aa->aa_addrs[0] == GPIO2_BASE) {
		/*
		 * We rely on firmware (or platform init code) to initialize
		 * the GPIO2 block.  We can't do it ourselves, because
		 * resetting the GPIO2 block can have nasty effects (e.g.
		 * reset PCI bus...)
		 */
		for (pin = 0; pin < aa->aa_addrs[1]; pin++) {
			sc->sc_pins[sc->sc_npins].pin_num = pin;
			sc->sc_pins[sc->sc_npins].pin_caps =
			    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
			sc->sc_pins[sc->sc_npins].pin_flags =
			    au_gpio2_ctl(pin, 0);
			sc->sc_pins[sc->sc_npins].pin_state =
			    au_gpio2_read(pin);
			sc->sc_npins++;
		}

		printf(", secondary block");
		sc->sc_isgpio2 = 1;

	} else {
		printf(", unidentified block");
	}
	printf("\n");

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = augpio_pin_read;
	sc->sc_gc.gp_pin_write = augpio_pin_write;
	sc->sc_gc.gp_pin_ctl = augpio_pin_ctl;

	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = sc->sc_npins;

	config_found_ia(&sc->sc_dev, "gpiobus", &gba, gpiobus_print);
}

int
augpio_pin_read(void *arg, int pin)
{
	struct augpio_softc *sc = arg;

	return (sc->sc_isgpio2 ? au_gpio2_read(pin) : au_gpio_read(pin));
}

void
augpio_pin_write(void *arg, int pin, int value)
{
	struct augpio_softc *sc = arg;
	
	if (sc->sc_isgpio2)
		au_gpio2_write(pin, value);
	else
		au_gpio_write(pin, value);
}

void
augpio_pin_ctl(void *arg, int pin, int flags)
{
	struct augpio_softc *sc = arg;

	if (sc->sc_isgpio2)
		(void) au_gpio2_ctl(pin, flags);
	else
		(void) au_gpio_ctl(pin, flags);
}

int
au_gpio_read(int pin)
{

	pin = 1 << pin;
	return ((GETREG(SYS_BASE + AUGPIO_SYS_PINSTATERD) & pin) ?
	    GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

void
au_gpio_write(int pin, int value)
{

	pin = 1 << pin;
	if (value) {
		PUTREG(SYS_BASE + AUGPIO_SYS_OUTPUTSET, pin);
	} else {
		PUTREG(SYS_BASE + AUGPIO_SYS_OUTPUTCLR, pin);
	}
}

int
au_gpio_ctl(int pin, int flags)
{
	uint32_t		tri, out;
	int			old;

	old = 0;

	pin = 1 << pin;
	tri = GETREG(SYS_BASE + AUGPIO_SYS_TRIOUTRD);
	out = GETREG(SYS_BASE + AUGPIO_SYS_OUTPUTRD);
	old = 0;

	if (tri & pin) {
		old |= GPIO_PIN_INPUT;
	} else {
		old |= GPIO_PIN_OUTPUT;
	}

	if (flags & (GPIO_PIN_TRISTATE|GPIO_PIN_INPUT)) {
		PUTREG(SYS_BASE + AUGPIO_SYS_TRIOUTCLR, pin);
	} else if (flags & GPIO_PIN_OUTPUT) {
		if (pin & out) {
			PUTREG(SYS_BASE + AUGPIO_SYS_OUTPUTSET, pin);
		} else {
			PUTREG(SYS_BASE + AUGPIO_SYS_OUTPUTCLR, pin);
		}
	}

	return old;
}

int
au_gpio2_read(int pin)
{

	pin = 1 << pin;

	return ((GETREG(GPIO2_BASE + AUGPIO_GPIO2_PINSTATE) & pin) ?
	    GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

void
au_gpio2_write(int pin, int value)
{

	pin = 1 << pin;

	if (value) {
		PUTREG(GPIO2_BASE + AUGPIO_GPIO2_OUTPUT, (pin | (pin << 16)));
	} else {
		PUTREG(GPIO2_BASE + AUGPIO_GPIO2_OUTPUT, (pin << 16));
	}
}

int
au_gpio2_ctl(int pin, int flags)
{
	uint32_t		dir;
	int			old;

	pin = 1 << pin;

	dir = GETREG(GPIO2_BASE + AUGPIO_GPIO2_DIR);
	old = dir;

	if (dir & pin) {
		old |= GPIO_PIN_OUTPUT;
	} else {
		old |= GPIO_PIN_INPUT;
	}

	if (flags & GPIO_PIN_INPUT) {
		dir |= pin;
		PUTREG(GPIO2_BASE + AUGPIO_GPIO2_DIR, pin);
	} else if (flags & GPIO_PIN_OUTPUT) {
		dir &= ~pin;
		PUTREG(GPIO2_BASE + AUGPIO_GPIO2_DIR, pin);
	}

	return old;
}
