/*	$NetBSD: gpio_ebus.c,v 1.1.8.2 2011/06/06 09:05:16 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: gpio_ebus.c,v 1.1.8.2 2011/06/06 09:05:16 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

/*
 * Device softc
 */
#define GPIO_NPINS 32

struct epio_softc {
	struct device sc_dev;
	struct _Pio *sc_dp;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[GPIO_NPINS];
};

static int	epio_ebus_match (struct device *, struct cfdata *, void *);
static void	epio_ebus_attach (struct device *, struct device *, void *);

CFATTACH_DECL(epio, sizeof (struct epio_softc),
    epio_ebus_match, epio_ebus_attach, NULL, NULL);

static int	epio_pin_read(void *, int);
static void	epio_pin_write(void *, int, int);
static void	epio_pin_ctl(void *, int, int);

static int
epio_ebus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ebus_attach_args *ia = aux;
    struct _Pio *f = (struct _Pio *)ia->ia_vaddr;

	if (strcmp("gpio", ia->ia_name) != 0)
		return (0);
    if ((f == NULL) ||
        (f->Tag != PMTTAG_GPIO))
		return (0);

	return (1);
}

static void
epio_ebus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ebus_attach_args *ia =aux;
	struct epio_softc *sc = (struct epio_softc *)self;
	struct gpiobus_attach_args gba;
    int i;
	uint32_t data;

	sc->sc_dp = (struct _Pio*)ia->ia_vaddr;
	data = sc->sc_dp->PinData;

#if DEBUG
    printf(" virt=%p data=%zx", (void*)sc->sc_dp, data);
#endif
	printf(": GPIO controller\n");

	/* BUGBUG Initialize pins properly */
	for (i = 0 ; i < GPIO_NPINS ; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INOUT
						| GPIO_PIN_OPENDRAIN
						| GPIO_PIN_TRISTATE;

		/* current defaults */
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_INOUT;
		sc->sc_gpio_pins[i].pin_state = (data & (1 << i)) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
		sc->sc_gpio_pins[i].pin_mapped = 0;
	}

	/* Create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = epio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = epio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = epio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = GPIO_NPINS;

	/* Attach GPIO framework */
	(void) config_found(&sc->sc_dev, &gba, gpiobus_print);
}

static int
epio_pin_read(void *arg, int pin)
{
	struct epio_softc *sc = arg;
	uint32_t data = sc->sc_dp->PinData;
	int p;

	p = pin % GPIO_NPINS;
	return (data >> p) & 0x01;
}

static void
epio_pin_write(void *arg, int pin, int value)
{
	struct epio_softc *sc = arg;
	uint32_t data;
	int p;

	p = pin % GPIO_NPINS;
    data = 1 << p;
    if (value)
        sc->sc_dp->PinData = data;
    else
        sc->sc_dp->ClearData = data;
}

static void
epio_pin_ctl(void *arg, int pin, int flags)
{
	struct epio_softc *sc = arg;
	uint32_t data;
	int p;

	p = pin % GPIO_NPINS;
    data = (1 << p);

	if (flags & GPIO_PIN_INOUT) {
        sc->sc_dp->Direction = data;
	}

	if (flags & GPIO_PIN_TRISTATE) {
        sc->sc_dp->OutDisable = data;
	}

	if (flags & GPIO_PIN_OPENDRAIN) {
        sc->sc_dp->Enable = data;
	}
}
