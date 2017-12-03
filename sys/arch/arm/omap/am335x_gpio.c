/*	$NetBSD: am335x_gpio.c,v 1.2.18.2 2017/12/03 11:35:55 jdolecek Exp $	*/
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: am335x_gpio.c,v 1.2.18.2 2017/12/03 11:35:55 jdolecek Exp $");

#define _INTR_PRIVATE

#include "locators.h"
#include "gpio.h"
#include "opt_omap.h"
 
#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
 
#include <uvm/uvm_extern.h>
  
#include <machine/intr.h>
 
#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <sys/bus.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_gpio.h>

#include <arm/omap/am335x_prcm.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/sitara_cm.h>
#include <arm/omap/sitara_cmreg.h>

#if NGPIO > 0
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

static const struct omap_module gpio_module[] = {
	{ 0, 0 },
	{ AM335X_PRCM_CM_PER, CM_PER_GPIO1_CLKCTRL },
	{ AM335X_PRCM_CM_PER, CM_PER_GPIO2_CLKCTRL },
	{ AM335X_PRCM_CM_PER, CM_PER_GPIO3_CLKCTRL },
};

#define AM335X_GPIO_REVISION		0x000
#define AM335X_GPIO_SYSCONFIG		0x010
#define AM335X_GPIO_EIO			0x020
#define AM335X_GPIO_IRQSTATUS_RAW_0	0x024
#define AM335X_GPIO_IRQSTATUS_RAW_1	0x028
#define AM335X_GPIO_IRQSTATUS_0		0x02c
#define AM335X_GPIO_IRQSTATUS_1		0x030
#define AM335X_GPIO_IRQSTATUS_SET_0	0x034
#define AM335X_GPIO_IRQSTATUS_SET_1	0x038
#define AM335X_GPIO_IRQSTATUS_CLR_0	0x03c
#define AM335X_GPIO_IRQSTATUS_CLR_1	0x040
#define AM335X_GPIO_IRQWAKEN_0		0x044
#define AM335X_GPIO_IRQWAKEN_1		0x048
#define AM335X_GPIO_SYSSTATUS		0x114
#define AM335X_GPIO_CTRL		0x130
#define AM335X_GPIO_OE			0x134
#define AM335X_GPIO_DATAIN		0x138
#define AM335X_GPIO_DATAOUT		0x13c
#define AM335X_GPIO_LEVELDETECT0	0x140
#define AM335X_GPIO_LEVELDETECT1	0x144
#define AM335X_GPIO_RISINGDETECT	0x148
#define AM335X_GPIO_FALLINGDETECT	0x14c
#define AM335X_GPIO_DEBOUNCEENABLE	0x150
#define AM335X_GPIO_DEBOUNCINGTIME	0x154
#define AM335X_GPIO_CLEARDATAOUT	0x190
#define AM335X_GPIO_SETDATAOUT		0x194

struct gpio_softc {
	device_t gpio_dev;
	bus_space_tag_t gpio_memt;
	bus_space_handle_t gpio_memh;
#if NGPIO > 0
	struct gpio_chipset_tag gpio_chipset;
	gpio_pin_t gpio_pins[32];
#endif
};

#define	GPIO_READ(gpio, reg) \
	bus_space_read_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg))
#define	GPIO_WRITE(gpio, reg, val) \
	bus_space_write_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg), (val))

static int gpio_match(device_t, cfdata_t, void *);
static void gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(omap2gpio,
	sizeof(struct gpio_softc),
	gpio_match, gpio_attach,
	NULL, NULL);

#if NGPIO > 0

static int
omap2gpio_pin_read(void *arg, int pin)
{
	struct gpio_softc * const gpio = arg;

	return (GPIO_READ(gpio, AM335X_GPIO_DATAIN) >> pin) & 1;
}

static void
omap2gpio_pin_write(void *arg, int pin, int value)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	old = GPIO_READ(gpio, AM335X_GPIO_DATAOUT);
	if (value)
		new = old | mask;
	else
		new = old & ~mask;

	if (old != new)
		GPIO_WRITE(gpio, AM335X_GPIO_DATAOUT, new);
}

static void
omap2gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	old = GPIO_READ(gpio, AM335X_GPIO_OE);
	new = old;
	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:	new |= mask; break;
	case GPIO_PIN_OUTPUT:	new &= ~mask; break;
	default:		return;
	}
	if (old != new)
		GPIO_WRITE(gpio, AM335X_GPIO_OE, new);
}

static void
gpio_attach1(device_t self)
{
	struct gpio_softc * const gpio = device_private(self);
	struct gpio_chipset_tag * const gp = &gpio->gpio_chipset;
	struct gpiobus_attach_args gba;
	gpio_pin_t *pins;
	uint32_t mask, dir, valueout, valuein;
	int pin;

	gp->gp_cookie = gpio;
	gp->gp_pin_read = omap2gpio_pin_read;
	gp->gp_pin_write = omap2gpio_pin_write;
	gp->gp_pin_ctl = omap2gpio_pin_ctl;

	gba.gba_gc = gp;
	gba.gba_pins = gpio->gpio_pins;
	gba.gba_npins = __arraycount(gpio->gpio_pins);

	dir = GPIO_READ(gpio, AM335X_GPIO_OE);
	valueout = GPIO_READ(gpio, AM335X_GPIO_DATAOUT);
	valuein = GPIO_READ(gpio, AM335X_GPIO_DATAIN);
	for (pin = 0, mask = 1, pins = gpio->gpio_pins;
	     pin < 32; pin++, mask <<= 1, pins++) {
		pins->pin_num = pin;
		pins->pin_caps = GPIO_PIN_INPUT|GPIO_PIN_OUTPUT;
		pins->pin_flags =
		    (dir & mask) ? GPIO_PIN_INPUT : GPIO_PIN_OUTPUT;
		pins->pin_state =
		    (((dir & mask) ? valuein : valueout) & mask)
			? GPIO_PIN_HIGH
			: GPIO_PIN_LOW;
	}

	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}
#endif /* NGPIO > 0 */

int
gpio_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct obio_attach_args *oa = aux;

#ifdef TI_AM335X
	if (oa->obio_addr == GPIO0_BASE_TI_AM335X
	    || oa->obio_addr == GPIO1_BASE_TI_AM335X
	    || oa->obio_addr == GPIO2_BASE_TI_AM335X
	    || oa->obio_addr == GPIO3_BASE_TI_AM335X)
		return 1;
#endif

	return 0;
}

void
gpio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args * const oa = aux;
	struct gpio_softc * const gpio = device_private(self);
	int error;

	gpio->gpio_dev = self;

	if (oa->obio_size == OBIOCF_SIZE_DEFAULT)
		panic("\n%s: no size assigned", device_xname(self));

	gpio->gpio_memt = oa->obio_iot;
	error = bus_space_map(oa->obio_iot, oa->obio_addr, oa->obio_size,
	    0, &gpio->gpio_memh);

	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    oa->obio_size, oa->obio_addr, error);
		return;
	}

	aprint_normal("\n");

#ifdef TI_AM335X
	switch (oa->obio_addr) {
	case GPIO0_BASE_TI_AM335X:
		break;
	case GPIO1_BASE_TI_AM335X:
		prcm_module_enable(&gpio_module[1]);
		break;
	case GPIO2_BASE_TI_AM335X:
		prcm_module_enable(&gpio_module[2]);
		break;
	case GPIO3_BASE_TI_AM335X:
		prcm_module_enable(&gpio_module[3]);
		break;
	}
#endif

#if NGPIO > 0
	gpio_attach1(self);
#endif
}

#if NGPIO > 0

extern struct cfdriver omapgpio_cd;

#define	GPIO_MODULE(pin)	((pin) / 32)
#define GPIO_PIN(pin)		((pin) % 32)

u_int
omap2_gpio_read(u_int gpio)
{
	struct gpio_softc *sc;

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL)
		panic("omap2gpio: GPIO Module for pin %d not configured.", gpio);

	return omap2gpio_pin_read(sc, GPIO_PIN(gpio));
}

void
omap2_gpio_write(u_int gpio, u_int value)
{
	struct gpio_softc *sc;

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL)
		panic("omap2gpio: GPIO Module for pin %d not configured.", gpio);

	omap2gpio_pin_write(sc, GPIO_PIN(gpio), value);
}

void
omap2_gpio_ctl(u_int gpio, int flags)
{
	struct gpio_softc *sc;

	sc = device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio));
	if (sc == NULL)
		panic("omap2gpio: GPIO Module for pin %d not configured.", gpio);

	omap2gpio_pin_ctl(sc, GPIO_PIN(gpio), flags);
}

bool
omap2_gpio_has_pin(u_int gpio)
{
	return device_lookup_private(&omapgpio_cd, GPIO_MODULE(gpio)) != NULL;
}

#endif
