/* $NetBSD: hwgpio.c,v 1.1.2.2 2024/02/03 11:47:04 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: hwgpio.c,v 1.1.2.2 2024/02/03 11:47:04 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bitops.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <machine/wii.h>

#include "hollywood.h"

#define PIN(_num, _name, _caps)		\
	{ .pin_num = (_num), 		\
	  .pin_caps = (_caps),		\
	  .pin_defname = (_name),	\
	}
static gpio_pin_t hwgpio_pins[] = {
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
#undef PIN

struct hwgpio_softc {
	struct gpio_chipset_tag	sc_gp;
};

#define	RD4(reg)		in32(reg)
#define	WR4(reg, val)		out32((reg), (val))

static int
hwgpio_pin_read(void *priv, int pin)
{
	return (RD4(HW_GPIOB_IN) & __BIT(pin)) != 0;
}

static void
hwgpio_pin_write(void *priv, int pin, int value)
{
	uint32_t out;
	int s;

	s = splhigh();
	out = RD4(HW_GPIOB_OUT);
	if (value) {
		out |= __BIT(pin);
	} else {
		out &= ~__BIT(pin);
	}
	WR4(HW_GPIOB_OUT, out);
	splx(s);
}

static void
hwgpio_pin_ctl(void *priv, int pin, int flags)
{
	uint32_t dir;
	int s;

	s = splhigh();
	dir = RD4(HW_GPIOB_DIR);
	if (flags & GPIO_PIN_OUTPUT) {
		dir |= __BIT(pin);
	} else {
		dir &= ~__BIT(pin);
	}
	WR4(HW_GPIOB_DIR, dir);
	splx(s);
}

static int
hwgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
hwgpio_attach(device_t parent, device_t self, void *aux)
{
	struct hwgpio_softc * const sc = device_private(self);
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba = {};
	uint32_t in, out, dir;
	u_int n;

	gp->gp_cookie = sc;
	gp->gp_pin_read = hwgpio_pin_read;
	gp->gp_pin_write = hwgpio_pin_write;
	gp->gp_pin_ctl = hwgpio_pin_ctl;

	aprint_naive("\n");
	aprint_normal(": GPIO\n");

	in = RD4(HW_GPIOB_IN);
	out = RD4(HW_GPIOB_OUT);
	dir = RD4(HW_GPIOB_DIR);
	for (n = 0; n < __arraycount(hwgpio_pins); n++) {
		const uint32_t mask = __BIT(hwgpio_pins[n].pin_num);
		if (dir & mask) {
			hwgpio_pins[n].pin_state = (out & mask) != 0;
		} else {
			hwgpio_pins[n].pin_state = (in & mask) != 0;
		}
	}

	gba.gba_gc = &sc->sc_gp;
	gba.gba_pins = hwgpio_pins;
	gba.gba_npins = __arraycount(hwgpio_pins);
	config_found(self, &gba, NULL, CFARGS_NONE);
}

CFATTACH_DECL_NEW(hwgpio, sizeof(struct hwgpio_softc),
    hwgpio_match, hwgpio_attach, NULL, NULL);
