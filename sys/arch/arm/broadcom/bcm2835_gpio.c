/*	$NetBSD: bcm2835_gpio.c,v 1.5 2017/11/09 21:37:52 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel.
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_gpio.c,v 1.5 2017/11/09 21:37:52 skrll Exp $");

/*
 * Driver for BCM2835 GPIO
 *
 * see: http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 */

#include "gpio.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <sys/bitops.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_gpioreg.h>
#include <arm/broadcom/bcm2835_gpio_subr.h>

/* #define BCM2835_GPIO_DEBUG */
#ifdef BCM2835_GPIO_DEBUG
int bcm2835gpiodebug = 3;
#define DPRINTF(l, x)	do { if (l <= bcm2835gpiodebug) { printf x; } } while (0)
#else
#define DPRINTF(l, x)
#endif

struct bcmgpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[32];
};

static int	bcmgpio_match(device_t, cfdata_t, void *);
static void	bcmgpio_attach(device_t, device_t, void *);

#if NGPIO > 0
static int      bcm2835gpio_gpio_pin_read(void *, int);
static void     bcm2835gpio_gpio_pin_write(void *, int, int);
static void     bcm2835gpio_gpio_pin_ctl(void *, int, int);
#endif

CFATTACH_DECL_NEW(bcmgpio, sizeof(struct bcmgpio_softc),
    bcmgpio_match, bcmgpio_attach, NULL, NULL);

static int
bcmgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amba_attach_args * const aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmgpio") != 0)
		return 0;

	return 1;
}

static void
bcmgpio_attach(device_t parent, device_t self, void *aux)
{
	struct bcmgpio_softc * const sc = device_private(self);
#if NGPIO > 0
	struct amba_attach_args *aaa = aux;
	struct gpiobus_attach_args gba;
	int pin, minpin, maxpin;
	u_int func;
	int error;
#endif

	sc->sc_dev = self;

#if NGPIO > 0
	if (device_unit(sc->sc_dev) > 1) {
		aprint_naive(" NO GPIO\n");
		aprint_normal(": NO GPIO\n");
		return;
	} else if (device_unit(sc->sc_dev) == 1) {
		maxpin = 53;
		minpin = 32;
	} else {
		maxpin = 31;
		minpin = 0;
	}

	aprint_naive("\n");
	aprint_normal(": GPIO [%d...%d]\n", minpin, maxpin);

	sc->sc_iot = aaa->aaa_iot;
	error = bus_space_map(sc->sc_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aaa->aaa_name, error);
		return;
	}

	for (pin = minpin; pin <= maxpin; pin++) {
	        int epin = pin - minpin;

	        sc->sc_gpio_pins[epin].pin_num = epin;
		/*
		 * find out pins still available for GPIO
		 */
		func = bcm2835gpio_function_read(pin);

		if (func == BCM2835_GPIO_IN ||
		    func == BCM2835_GPIO_OUT) {
	                sc->sc_gpio_pins[epin].pin_caps = GPIO_PIN_INPUT |
				GPIO_PIN_OUTPUT |
				GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
				GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
			/* read initial state */
			sc->sc_gpio_pins[epin].pin_state =
				bcm2835gpio_gpio_pin_read(sc, epin);
			DPRINTF(1, ("%s: attach pin %d\n", device_xname(sc->sc_dev), pin));
		} else {
	                sc->sc_gpio_pins[epin].pin_caps = 0;
			sc->sc_gpio_pins[epin].pin_state = 0;
  			DPRINTF(1, ("%s: skip pin %d - func = 0x%x\n", device_xname(sc->sc_dev), pin, func));
		}
	}

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = bcm2835gpio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = bcm2835gpio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = bcm2835gpio_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = maxpin - minpin + 1;

	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
#else
	aprint_normal_dev(sc->sc_dev, "no GPIO configured in kernel");
#endif
}

#if NGPIO > 0
/* GPIO support functions */
static int
bcm2835gpio_gpio_pin_read(void *arg, int pin)
{
	struct bcmgpio_softc *sc = arg;
	int epin = pin + device_unit(sc->sc_dev) * 32;
	uint32_t val;
	int res;

	if (device_unit(sc->sc_dev) > 1) {
		return 0;
	}

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		BCM2835_GPIO_GPLEV(epin / BCM2835_GPIO_GPLEV_PINS_PER_REGISTER));

	res = val & (1 << (epin % BCM2835_GPIO_GPLEV_PINS_PER_REGISTER)) ?
		GPIO_PIN_HIGH : GPIO_PIN_LOW;

	DPRINTF(2, ("%s: gpio_read pin %d->%d\n", device_xname(sc->sc_dev), epin, (res == GPIO_PIN_HIGH)));

	return res;
}

static void
bcm2835gpio_gpio_pin_write(void *arg, int pin, int value)
{
	struct bcmgpio_softc *sc = arg;
	int epin = pin + device_unit(sc->sc_dev) * 32;
	bus_size_t reg;

	if (device_unit(sc->sc_dev) > 1) {
		return;
	}

	if (value == GPIO_PIN_HIGH) {
		reg = BCM2835_GPIO_GPSET(epin / BCM2835_GPIO_GPSET_PINS_PER_REGISTER);
	} else {
		reg = BCM2835_GPIO_GPCLR(epin / BCM2835_GPIO_GPCLR_PINS_PER_REGISTER);
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		reg, 1 << (epin % BCM2835_GPIO_GPSET_PINS_PER_REGISTER));
	DPRINTF(2, ("%s: gpio_write pin %d<-%d\n", device_xname(sc->sc_dev), epin, (value == GPIO_PIN_HIGH)));
}

static void
bcm2835gpio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct bcmgpio_softc *sc = arg;
	uint32_t cmd;
	int epin = pin + device_unit(sc->sc_dev) * 32;

	if (device_unit(sc->sc_dev) > 1) {
		return;
	}

	DPRINTF(2, ("%s: gpio_ctl pin %d flags 0x%x\n", device_xname(sc->sc_dev), epin, flags));

	if (flags & (GPIO_PIN_OUTPUT|GPIO_PIN_INPUT)) {
		if ((flags & GPIO_PIN_INPUT) || !(flags & GPIO_PIN_OUTPUT)) {
			/* for safety INPUT will overide output */
	                bcm2835gpio_function_select(epin, BCM2835_GPIO_IN);
		} else {
	                bcm2835gpio_function_select(epin, BCM2835_GPIO_OUT);
		}
	}

	if (flags & (GPIO_PIN_PULLUP|GPIO_PIN_PULLDOWN)) {
		cmd = (flags & GPIO_PIN_PULLUP) ?
			BCM2835_GPIO_GPPUD_PULLUP : BCM2835_GPIO_GPPUD_PULLDOWN;
	} else {
		cmd = BCM2835_GPIO_GPPUD_PULLOFF;
	}

	/* set up control signal */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		BCM2835_GPIO_GPPUD, cmd);
	delay(1); /* wait 150 cycles */
	/* set clock signal */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		BCM2835_GPIO_GPPUDCLK(device_unit(sc->sc_dev)),
		1 << (epin % BCM2835_GPIO_GPPUD_PINS_PER_REGISTER));
	delay(1); /* wait 150 cycles */
	/* reset control signal and clock */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		BCM2835_GPIO_GPPUD, BCM2835_GPIO_GPPUD_PULLOFF);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		BCM2835_GPIO_GPPUDCLK(device_unit(sc->sc_dev)),
		0);
}
#endif /* NGPIO > 0 */
