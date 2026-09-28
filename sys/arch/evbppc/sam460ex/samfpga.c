/*	$NetBSD: samfpga.c,v 1.1 2026/09/28 20:41:00 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * ACube Sam460ex board FPGA.
 * GPIO pins via J22 connector, LEDs, soft power off.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: samfpga.c,v 1.1 2026/09/28 20:41:00 rkujawa Exp $");

#include "locators.h"
#include "ioconf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>
#include <dev/led.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/ebcvar.h>
#include <evbppc/sam460ex/samfpgareg.h>

#define	SAMFPGA_NLED	3

struct samfpga_softc;

struct samfpga_led {
	struct samfpga_softc	*led_sc;
	uint16_t		led_bit;
};

struct samfpga_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	kmutex_t		sc_lock;
	struct gpio_chipset_tag	sc_gc;
	gpio_pin_t		sc_pins[SAMFPGA_GPIO_NPINS];
	struct samfpga_led	sc_leds[SAMFPGA_NLED];
};

static int	samfpga_match(device_t, cfdata_t, void *);
static void	samfpga_attach(device_t, device_t, void *);

static void	samfpga_powerdown(void);

static int	samfpga_led_get(void *);
static void	samfpga_led_set(void *, int);

static int	samfpga_gpio_pin_read(void *, int);
static void	samfpga_gpio_pin_write(void *, int, int);
static void	samfpga_gpio_pin_ctl(void *, int, int);

CFATTACH_DECL_NEW(samfpga, sizeof(struct samfpga_softc),
    samfpga_match, samfpga_attach, NULL, NULL);

/* J22 pad names. */
static const char samfpga_j22[SAMFPGA_GPIO_NPINS][4] = {
	"B6", "B4", "B8", "B7", "C6", "A4", "A6", "C5",
	"C4", "B5", "D10", "D8", "A8", "D7", "D4", "D5",
	"A5", "B9", "C8", "D6", "C7", "B10", "D9", "C9",
	"A9", "A7", "D11", "D16", "A15", "C10", "B15", "A10",
	"A11", "B14", "C15", "D15", "A14", "C14", "D14", "C13",
	"A13", "D13", "A12", "B13", "D12", "C12", "B12", "C21",
	"B20", "A20", "C20", "D20", "B19", "A19", "C19", "A18",
	"C18", "D19", "C17", "C16", "D18", "A17", "B17", "D17",
	"B16", "A16", "B18", "C22", "A22", "B22", "A23", "B23",
	"B11", "C11", "D21", "A21", "D22", "B21", "D23", "C23",
};

static const struct {
	const char	*name;
	uint16_t	bit;
} samfpga_led_names[SAMFPGA_NLED] = {
	{ "red",	SAMFPGA_CTL_LED_RED },
	{ "yellow",	SAMFPGA_CTL_LED_YELLOW },
	{ "amber",	SAMFPGA_CTL_LED_AMBER },
};

static inline uint16_t
samfpga_read(struct samfpga_softc *sc, bus_size_t o)
{
	return bus_space_read_2(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
samfpga_write(struct samfpga_softc *sc, bus_size_t o, uint16_t v)
{
	bus_space_write_2(sc->sc_bst, sc->sc_bsh, o, v);
}

static inline unsigned
samfpga_bcd(unsigned v)
{
	return (v >> 4) * 10 + (v & 0xf);
}

static int
samfpga_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebc_attach_args * const eaa = aux;

	if (eaa->ebc_width != 16 || eaa->ebc_usage != EBC_USAGE_RW ||
	    eaa->ebc_size < SAMFPGA_SIZE)
		return 0;

	return 1;
}

static void
samfpga_attach(device_t parent, device_t self, void *aux)
{
	struct samfpga_softc * const sc = device_private(self);
	struct ebc_attach_args * const eaa = aux;
	struct gpiobus_attach_args gba;
	uint16_t date, year;
	int i;

	sc->sc_dev = self;
	sc->sc_bst = eaa->ebc_bt;

	if (bus_space_map(sc->sc_bst, eaa->ebc_addr, SAMFPGA_SIZE, 0,
	    &sc->sc_bsh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	date = samfpga_read(sc, SAMFPGA_REV_DATE);
	year = samfpga_read(sc, SAMFPGA_REV_YEAR);

	aprint_naive(": board FPGA\n");
	aprint_normal(": board FPGA, revision %u (20%02u-%02u-%02u)\n",
	    samfpga_bcd(__SHIFTOUT(year, SAMFPGA_REV_YEAR_REV)),
	    samfpga_bcd(__SHIFTOUT(year, SAMFPGA_REV_YEAR_YEAR)),
	    samfpga_bcd(__SHIFTOUT(date, SAMFPGA_REV_DATE_MONTH)),
	    samfpga_bcd(__SHIFTOUT(date, SAMFPGA_REV_DATE_DAY)));
	aprint_verbose_dev(self, "rev 0x%04x 0x%04x ctl 0x%04x usb 0x%04x\n",
	    date, year, samfpga_read(sc, SAMFPGA_CTL),
	    samfpga_read(sc, SAMFPGA_USB));
	for (i = 0; i < SAMFPGA_GPIO_NBANK; i++)
		aprint_verbose_dev(self,
		    "bank %d: dir 0x%04x out 0x%04x in 0x%04x\n", i,
		    samfpga_read(sc, SAMFPGA_GPIO_DIR(i)),
		    samfpga_read(sc, SAMFPGA_GPIO_OUT(i)),
		    samfpga_read(sc, SAMFPGA_GPIO_IN(i)));

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	for (i = 0; i < SAMFPGA_GPIO_NPINS; i++) {
		gpio_pin_t * const pin = &sc->sc_pins[i];
		const int bank = SAMFPGA_GPIO_BANK(i);
		const uint16_t bit = SAMFPGA_GPIO_BIT(i);

		pin->pin_num = i;
		pin->pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		pin->pin_flags =
		    (samfpga_read(sc, SAMFPGA_GPIO_DIR(bank)) & bit) ?
		    GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		pin->pin_state =
		    (samfpga_read(sc, SAMFPGA_GPIO_IN(bank)) & bit) ?
		    GPIO_PIN_HIGH : GPIO_PIN_LOW;
		snprintf(pin->pin_defname, sizeof(pin->pin_defname),
		    "J22-%s", samfpga_j22[i]);
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = samfpga_gpio_pin_read;
	sc->sc_gc.gp_pin_write = samfpga_gpio_pin_write;
	sc->sc_gc.gp_pin_ctl = samfpga_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = SAMFPGA_GPIO_NPINS;

	config_found(self, &gba, gpiobus_print, CFARGS_NONE);

	for (i = 0; i < SAMFPGA_NLED; i++) {
		sc->sc_leds[i].led_sc = sc;
		sc->sc_leds[i].led_bit = samfpga_led_names[i].bit;
		if (led_attach(samfpga_led_names[i].name, &sc->sc_leds[i],
		    samfpga_led_get, samfpga_led_set) == NULL)
			aprint_error_dev(self, "can't attach led %s\n",
			    samfpga_led_names[i].name);
	}

	md_powerdown = samfpga_powerdown;
}

static int
samfpga_led_get(void *arg)
{
	struct samfpga_led * const led = arg;

	return (samfpga_read(led->led_sc, SAMFPGA_CTL) & led->led_bit) ?
	    LED_STATE_ON : LED_STATE_OFF;
}

static void
samfpga_led_set(void *arg, int state)
{
	struct samfpga_led * const led = arg;
	struct samfpga_softc * const sc = led->led_sc;
	uint16_t v;

	mutex_enter(&sc->sc_lock);
	v = samfpga_read(sc, SAMFPGA_CTL);
	if (state == LED_STATE_ON)
		v |= led->led_bit;
	else
		v &= ~led->led_bit;
	samfpga_write(sc, SAMFPGA_CTL, v);
	mutex_exit(&sc->sc_lock);
}

static void
samfpga_powerdown(void)
{
	struct samfpga_softc * const sc = device_lookup_private(&samfpga_cd, 0);
	int i;

	if (sc == NULL)
		return;

	/* Flash x3, then hold RESET. */
	for (i = 0; i < 3; i++) {
		samfpga_write(sc, SAMFPGA_CTL, (i & 1) ? 0 : SAMFPGA_CTL_LEDS);
		delay(300 * 1000);
	}
	samfpga_write(sc, SAMFPGA_CTL, SAMFPGA_CTL_RESET);
	delay(1000 * 1000);
}

static int
samfpga_gpio_pin_read(void *arg, int pin)
{
	struct samfpga_softc * const sc = arg;
	const int bank = SAMFPGA_GPIO_BANK(pin);

	return (samfpga_read(sc, SAMFPGA_GPIO_IN(bank)) &
	    SAMFPGA_GPIO_BIT(pin)) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}

static void
samfpga_gpio_pin_write(void *arg, int pin, int value)
{
	struct samfpga_softc * const sc = arg;
	const bus_size_t reg = SAMFPGA_GPIO_OUT(SAMFPGA_GPIO_BANK(pin));
	const uint16_t bit = SAMFPGA_GPIO_BIT(pin);
	uint16_t v;

	mutex_enter(&sc->sc_lock);
	v = samfpga_read(sc, reg);
	if (value == GPIO_PIN_LOW)
		v &= ~bit;
	else
		v |= bit;
	samfpga_write(sc, reg, v);
	mutex_exit(&sc->sc_lock);
}

static void
samfpga_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct samfpga_softc * const sc = arg;
	const bus_size_t reg = SAMFPGA_GPIO_DIR(SAMFPGA_GPIO_BANK(pin));
	const uint16_t bit = SAMFPGA_GPIO_BIT(pin);
	uint16_t v;

	if ((flags & (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT)) == 0)
		return;

	mutex_enter(&sc->sc_lock);
	v = samfpga_read(sc, reg);
	if (flags & GPIO_PIN_OUTPUT)
		v |= bit;
	else
		v &= ~bit;
	samfpga_write(sc, reg, v);
	mutex_exit(&sc->sc_lock);
}

