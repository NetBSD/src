/* $NetBSD: tcu.c,v 1.2.4.2 2016/10/05 20:55:57 skrll Exp $ */

/*-
 * Copyright (c) 2016, Felix Deichmann
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

/*
 * flxd TC-USB - TURBOchannel USB host option
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcu.c,v 1.2.4.2 2016/10/05 20:55:57 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <sys/bus.h>

#include <dev/tc/tcvar.h>

#include <dev/gpio/gpiovar.h>

#include "gpio.h"
#include "slhci_tcu.h"

#define TCU_GPIO_NPINS	8

#define TCU_CPLD_OFFS	0x80
#define TCU_CPLD_SIZE	(4 * 4)

#define TCU_CFG		0x0
#define   TCU_CFG_RUN	__BIT(7)	/* write-only */
#define TCU_GPIO_DIR	0x4
#define TCU_GPIO_IN	0x8
#define TCU_GPIO_OUT	0xc

struct tcu_softc {
#if NGPIO > 0
	kmutex_t		sc_gpio_mtx;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[TCU_GPIO_NPINS];
	bus_space_tag_t		sc_gpio_iot;
	bus_space_handle_t	sc_gpio_ioh;
#endif /* NGPIO > 0 */
};

static int  tcu_match(device_t, cfdata_t, void *);
static void tcu_attach(device_t, device_t, void *);

#if NSLHCI_TCU > 0
static int  tcu_print(void *, const char *);
#endif /* NSLHCI_TCU > 0 */
#if NGPIO > 0
static void tcu_gpio_attach(device_t, device_t, void *);
static int  tcu_gpio_read(void *, int);
static void tcu_gpio_write(void *, int, int);
static void tcu_gpio_ctl(void *, int, int);
#endif /* NGPIO > 0 */

CFATTACH_DECL_NEW(tcu, sizeof(struct tcu_softc),
    tcu_match, tcu_attach, NULL, NULL);

static int
tcu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tc_attach_args *ta = aux;

	if (strncmp("TC-USB  ", ta->ta_modname, TC_ROM_LLEN))
		return 0;

	return 1;
}

static void
tcu_attach(device_t parent, device_t self, void *aux)
{
	struct tc_attach_args *ta = aux;
	bus_space_tag_t iot = ta->ta_memt;
	bus_space_handle_t ioh;
	int error;
	uint8_t cfg;
	char buf[30];

	printf(": TC-USB\n");

	error = bus_space_map(iot, ta->ta_addr + TCU_CPLD_OFFS, TCU_CPLD_SIZE,
	    0, &ioh);
	if (error) {
		aprint_error_dev(self, "bus_space_map() failed (%d)\n", error);
		return;
	}

	/*
	 * Force reset in case system didn't. SL811 reset pulse and hold time
	 * must be min. 16 clocks long (at 48 MHz clock) each.
	 */
	bus_space_write_1(iot, ioh, TCU_CFG, 0);
	DELAY(1000);
	bus_space_write_1(iot, ioh, TCU_CFG, TCU_CFG_RUN);
	DELAY(1000);

	cfg = bus_space_read_1(iot, ioh, TCU_CFG);

	bus_space_unmap(iot, ioh, TCU_CPLD_SIZE);

	/* Display DIP switch configuration. */
	(void)snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\3S1-1\0"
	    "b\2S1-2\0"
	    "b\1S1-3\0"
	    "b\0S1-4\0"
	    "\0", cfg);
	aprint_normal_dev(self, "config %s\n", buf);

#if NSLHCI_TCU > 0
	/* Attach slhci. */
	(void)config_found_ia(self, "tcu", aux, tcu_print);
#endif /* NSLHCI_TCU > 0 */
#if NGPIO > 0
	/* Attach gpio(bus). */
	tcu_gpio_attach(parent, self, aux);
#endif /* NGPIO > 0 */
}

#if NSLHCI_TCU > 0
static int
tcu_print(void *aux, const char *pnp)
{

	/* This function is only used for "slhci at tcu". */
	if (pnp)
		aprint_normal("slhci at %s", pnp);

	return UNCONF;
}
#endif /* NSLHCI_TCU > 0 */

#if NGPIO > 0
static void
tcu_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct tcu_softc *sc = device_private(self);
	struct tc_attach_args *ta = aux;
	struct gpiobus_attach_args gba;
	bus_space_tag_t iot = ta->ta_memt;
	uint32_t v;
	int error;

	sc->sc_gpio_iot = iot;

	error = bus_space_map(iot, ta->ta_addr + TCU_CPLD_OFFS, TCU_CPLD_SIZE,
	    0, &sc->sc_gpio_ioh);
	if (error) {
		aprint_error_dev(self, "bus_space_map() failed (%d)\n", error);
		return;
	}

	mutex_init(&sc->sc_gpio_mtx, MUTEX_DEFAULT, IPL_NONE);

	v = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, TCU_GPIO_DIR);

	for (int pin = 0; pin < TCU_GPIO_NPINS; pin++) {
		sc->sc_gpio_pins[pin].pin_num = pin;
		sc->sc_gpio_pins[pin].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT;
		sc->sc_gpio_pins[pin].pin_flags = (v & (UINT32_C(1) << pin)) ?
		    GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		sc->sc_gpio_pins[pin].pin_state = tcu_gpio_read(sc, pin);
	}

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = tcu_gpio_read;
	sc->sc_gpio_gc.gp_pin_write = tcu_gpio_write;
	sc->sc_gpio_gc.gp_pin_ctl = tcu_gpio_ctl;

	memset(&gba, 0, sizeof(gba));

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = TCU_GPIO_NPINS;

	(void)config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}

static int
tcu_gpio_read(void *arg, int pin)
{
	struct tcu_softc *sc = arg;
	uint32_t v;

	mutex_enter(&sc->sc_gpio_mtx);
	v = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, TCU_GPIO_IN);
	mutex_exit(&sc->sc_gpio_mtx);

	return (v & (UINT32_C(1) << pin)) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}

static void
tcu_gpio_write(void *arg, int pin, int val)
{
	struct tcu_softc *sc = arg;
	uint32_t v;

	mutex_enter(&sc->sc_gpio_mtx);

	v = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, TCU_GPIO_OUT);

	if (val == GPIO_PIN_LOW)
		v &= ~(UINT32_C(1) << pin);
	else if (val == GPIO_PIN_HIGH)
		v |= (UINT32_C(1) << pin);

	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, TCU_GPIO_OUT, v);

	mutex_exit(&sc->sc_gpio_mtx);
}

static void
tcu_gpio_ctl(void *arg, int pin, int flags)
{
	struct tcu_softc *sc = arg;
	uint32_t v;

	mutex_enter(&sc->sc_gpio_mtx);

	v = bus_space_read_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, TCU_GPIO_DIR);

	if (flags & GPIO_PIN_INPUT)
		v &= ~(UINT32_C(1) << pin);
	if (flags & GPIO_PIN_OUTPUT)
		v |= (UINT32_C(1) << pin);

	bus_space_write_4(sc->sc_gpio_iot, sc->sc_gpio_ioh, TCU_GPIO_DIR, v);

	mutex_exit(&sc->sc_gpio_mtx);
}
#endif /* NGPIO > 0 */
