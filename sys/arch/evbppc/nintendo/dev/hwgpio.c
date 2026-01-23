/* $NetBSD: hwgpio.c,v 1.2 2026/01/23 09:58:55 jmcneill Exp $ */

/*-
 * Copyright (c) 2024-2026 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hwgpio.c,v 1.2 2026/01/23 09:58:55 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>
#include <sys/gpio.h>
#include <sys/mutex.h>

#include <dev/gpio/gpiovar.h>

#include <machine/wii.h>
#include <machine/wiiu.h>
#include "ahb.h"

#define PIN(_num, _name, _caps)		\
	{ .pin_num = (_num), 		\
	  .pin_caps = (_caps),		\
	  .pin_defname = (_name),	\
	}

static gpio_pin_t hwgpio0_pins_wii[] = {
	PIN( 0, "POWER",	GPIO_PIN_INPUT),
	PIN( 1, "SHUTDOWN",	GPIO_PIN_OUTPUT),
	PIN( 2, "FAN",		GPIO_PIN_OUTPUT),
	PIN( 3, "DC_DC",	GPIO_PIN_OUTPUT),
	PIN( 4, "DI_SPIN",	GPIO_PIN_OUTPUT),
	PIN( 5, "SLOT_LED",	GPIO_PIN_OUTPUT),
	PIN( 6, "EJECT_BTN",	GPIO_PIN_INPUT),
	PIN( 7, "SLOT_IN",	GPIO_PIN_INPUT),
	PIN( 8, "SENSOR_BAR",	GPIO_PIN_OUTPUT),
	PIN( 9, "DO_EJECT",	GPIO_PIN_OUTPUT),
	PIN(10, "EEP_CS",	GPIO_PIN_OUTPUT),
	PIN(11, "EEP_CLK",	GPIO_PIN_OUTPUT),
	PIN(12, "EEP_MOSI",	GPIO_PIN_OUTPUT),
	PIN(13, "EEP_MISO",	GPIO_PIN_INPUT),
	PIN(14, "AVE_SCL",	GPIO_PIN_OUTPUT),
	PIN(15, "AVE_SDA",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(16, "DEBUG0",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(17, "DEBUG1",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(18, "DEBUG2",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(19, "DEBUG3",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(20, "DEBUG4",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(21, "DEBUG5",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(22, "DEBUG6",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(23, "DEBUG7",	GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
};

static gpio_pin_t hwgpio0_pins_wiiu[] = {
	PIN( 0, "RtcSysInt",		GPIO_PIN_INPUT),
	PIN( 1, "DwifiMode",		GPIO_PIN_OUTPUT),
	PIN( 2, "FanPower",		GPIO_PIN_OUTPUT),
	PIN( 3, "DcdcPowerControl",	GPIO_PIN_OUTPUT),
	PIN( 5, "Esp10Workaround",	GPIO_PIN_OUTPUT),
	PIN( 6, "DRCPWRREQ",		GPIO_PIN_OUTPUT),
	PIN( 7, "JIG",			GPIO_PIN_INPUT),
	PIN( 8, "PadPd",		GPIO_PIN_OUTPUT),
	PIN(10, "EepromCs",		GPIO_PIN_OUTPUT),
	PIN(11, "EepromSk",		GPIO_PIN_OUTPUT),
	PIN(12, "EepromDo",		GPIO_PIN_OUTPUT),
	PIN(13, "EepromDi",		GPIO_PIN_INPUT),
	PIN(14, "Av0I2cClock",		GPIO_PIN_OUTPUT),
	PIN(15, "Av0I2cData",		GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(24, "Av1I2cClock",		GPIO_PIN_OUTPUT),
	PIN(25, "Av1I2cData",		GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN(26, "MuteLamp",		GPIO_PIN_OUTPUT),
	PIN(27, "BluetoothMode",	GPIO_PIN_OUTPUT),
	PIN(28, "CcrhReset",		GPIO_PIN_OUTPUT),
	PIN(29, "WifiMode",		GPIO_PIN_OUTPUT),
	PIN(30, "Sdcc0s0Power",		GPIO_PIN_OUTPUT),
};

static gpio_pin_t hwgpio1_pins_wiiu[] = {
	PIN( 0, "FanSpeed",		GPIO_PIN_OUTPUT),
	PIN( 1, "SmcScl",		GPIO_PIN_OUTPUT),
	PIN( 2, "SmcSda",		GPIO_PIN_INPUT | GPIO_PIN_OUTPUT),
	PIN( 3, "DcdcPowerControl2",	GPIO_PIN_OUTPUT),
	PIN( 4, "AvInt",		GPIO_PIN_INPUT),
	PIN( 5, "Ccrhlo12",		GPIO_PIN_OUTPUT),
	PIN( 6, "AvReset",		GPIO_PIN_OUTPUT),
};

#undef PIN

struct hwgpio_softc {
	struct gpio_chipset_tag	sc_gp;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
};

#define GPIO0_ADDR		0x0d0000c0
#define GPIO1_ADDR		0x0d000520
#define GPIO_SIZE		0x40

#define GPIOB_OUT		0x00
#define GPIOB_DIR		0x04
#define GPIOB_IN		0x08

#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
hwgpio_pin_read(void *priv, int pin)
{
	struct hwgpio_softc * const sc = priv;

	return (RD4(sc, GPIOB_IN) & __BIT(pin)) != 0;
}

static void
hwgpio_pin_write(void *priv, int pin, int value)
{
	struct hwgpio_softc * const sc = priv;
	uint32_t out;

	mutex_enter(&sc->sc_lock);
	out = RD4(sc, GPIOB_OUT);
	if (value) {
		out |= __BIT(pin);
	} else {
		out &= ~__BIT(pin);
	}
	WR4(sc, GPIOB_OUT, out);
	mutex_exit(&sc->sc_lock);
}

static void
hwgpio_pin_ctl(void *priv, int pin, int flags)
{
	struct hwgpio_softc * const sc = priv;
	uint32_t dir;

	mutex_enter(&sc->sc_lock);
	dir = RD4(sc, GPIOB_DIR);
	if (flags & GPIO_PIN_OUTPUT) {
		dir |= __BIT(pin);
	} else {
		dir &= ~__BIT(pin);
	}
	WR4(sc, GPIOB_DIR, dir);
	mutex_exit(&sc->sc_lock);
}

static int
hwgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ahb_attach_args * const aaa = aux;

	if (wiiu_plat) {
		return aaa->aaa_addr == GPIO0_ADDR ||
		       aaa->aaa_addr == GPIO1_ADDR;
	} else {
		return aaa->aaa_addr == GPIO0_ADDR;
	}
}

static void
hwgpio_attach(device_t parent, device_t self, void *aux)
{
	struct hwgpio_softc * const sc = device_private(self);
	struct ahb_attach_args * const aaa = aux;
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba = {};
	uint32_t in, out, dir;
	gpio_pin_t *pins;
	int error;
	u_int n, npins;

	sc->sc_bst = aaa->aaa_bst;
	error = bus_space_map(sc->sc_bst, aaa->aaa_addr, GPIO_SIZE, 0,
	    &sc->sc_bsh);
	if (error != 0) {
		aprint_error(": couldn't map registers (%d)\n", error);
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	gp->gp_cookie = sc;
	gp->gp_pin_read = hwgpio_pin_read;
	gp->gp_pin_write = hwgpio_pin_write;
	gp->gp_pin_ctl = hwgpio_pin_ctl;

	aprint_naive("\n");
	aprint_normal(": GPIO\n");

	if (wiiu_plat) {
		if (aaa->aaa_addr == GPIO0_ADDR) {
			pins = hwgpio0_pins_wiiu;
			npins = __arraycount(hwgpio0_pins_wiiu);
		} else {
			KASSERT(aaa->aaa_addr == GPIO1_ADDR);
			pins = hwgpio1_pins_wiiu;
			npins = __arraycount(hwgpio1_pins_wiiu);
		}
	} else {
		KASSERT(aaa->aaa_addr == GPIO0_ADDR);
		pins = hwgpio0_pins_wii;
		npins = __arraycount(hwgpio0_pins_wii);
	}

	in = RD4(sc, GPIOB_IN);
	out = RD4(sc, GPIOB_OUT);
	dir = RD4(sc, GPIOB_DIR);
	for (n = 0; n < npins; n++) {
		const uint32_t mask = __BIT(pins[n].pin_num);
		if (dir & mask) {
			pins[n].pin_state = (out & mask) != 0;
		} else {
			pins[n].pin_state = (in & mask) != 0;
		}
	}

	gba.gba_gc = &sc->sc_gp;
	gba.gba_pins = pins;
	gba.gba_npins = npins;
	config_found(self, &gba, NULL, CFARGS_NONE);
}

CFATTACH_DECL_NEW(hwgpio, sizeof(struct hwgpio_softc),
    hwgpio_match, hwgpio_attach, NULL, NULL);
