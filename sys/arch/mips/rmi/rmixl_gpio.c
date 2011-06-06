/*	$NetBSD: rmixl_gpio.c,v 1.1.6.2 2011/06/06 09:06:09 jruoho Exp $	*/
/*	$NetBSD: rmixl_gpio.c,v 1.1.6.2 2011/06/06 09:06:09 jruoho Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: rmixl_gpio.c,v 1.1.6.2 2011/06/06 09:06:09 jruoho Exp $");

#define _INTR_PRIVATE

#include "locators.h"
#include "gpio.h"
 
#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
 
#include <uvm/uvm_extern.h>
  
#include <machine/intr.h>
 
#include <mips/cpu.h>

#include <machine/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_obiovar.h>

#if NGPIO > 0
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

struct gpio_softc {
	device_t gpio_dev;
	void *gpio_ih;
	bus_space_tag_t gpio_memt;
	bus_space_handle_t gpio_memh;
	uint32_t gpio_enable_mask;
	uint32_t gpio_edge_mask;
	uint32_t gpio_edge_falling_mask;
	uint32_t gpio_edge_rising_mask;
	uint32_t gpio_level_mask;
	uint32_t gpio_level_hi_mask;
	uint32_t gpio_level_lo_mask;
	uint32_t gpio_inuse_mask;
#if NGPIO > 0
	struct gpio_chipset_tag gpio_chipset;
	gpio_pin_t gpio_pins[RMIXL_GPIO_NSIGNALS];
#endif
};

#define	GPIO_READ(gpio, reg) \
	bus_space_read_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg))
#define	GPIO_WRITE(gpio, reg, val) \
	bus_space_write_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg), (val))

static int gpio_match(device_t, cfdata_t, void *);
static void gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rmixl_gpio,
	sizeof(struct gpio_softc),
	gpio_match, gpio_attach,
	NULL, NULL);

#if NGPIO > 0

static int
rmixl_gpio_pin_read(void *arg, int pin)
{
	struct gpio_softc * const gpio = arg;

	KASSERT(((1 << pin) & RMIXL_GPIO_INPUT_MASK) != 0);
	return (GPIO_READ(gpio, RMIXL_GPIO_INPUT) >> pin) & 1;
}

static void
rmixl_gpio_pin_write(void *arg, int pin, int value)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	KASSERT(((1 << pin) & RMIXL_GPIO_OUTPUT_MASK) != 0);

	old = GPIO_READ(gpio, RMIXL_GPIO_OUTPUT);
	if (value)
		new = old | mask; 
	else
		new = old & ~mask;

	if (old != new)
		GPIO_WRITE(gpio, RMIXL_GPIO_OUTPUT, new);
}

static void
rmixl_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	KASSERT((flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) != (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT));
	old = GPIO_READ(gpio, RMIXL_GPIO_IO_DIR);
	new = old;
	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:	new &= ~mask; break;
	case GPIO_PIN_OUTPUT:	new |= mask; break;
	default:		return;
	}
	if (old != new)
		GPIO_WRITE(gpio, RMIXL_GPIO_IO_DIR, new);
}

static void
gpio_defer(device_t self)
{
	struct gpio_softc * const gpio = device_private(self);
	struct gpio_chipset_tag * const gp = &gpio->gpio_chipset;
	struct gpiobus_attach_args gba;
	gpio_pin_t *pins;
	uint32_t mask, dir, valueout, valuein;
	int pin;

	gp->gp_cookie = gpio;
	gp->gp_pin_read = rmixl_gpio_pin_read;
	gp->gp_pin_write = rmixl_gpio_pin_write;
	gp->gp_pin_ctl = rmixl_gpio_pin_ctl;

	gba.gba_gc = gp;
	gba.gba_pins = gpio->gpio_pins;
	gba.gba_npins = __arraycount(gpio->gpio_pins);

	dir = GPIO_READ(gpio, RMIXL_GPIO_IO_DIR);
	valueout = GPIO_READ(gpio, RMIXL_GPIO_OUTPUT);
	valuein = GPIO_READ(gpio, RMIXL_GPIO_INPUT);
	for (pin = 0, mask = 1, pins = gpio->gpio_pins;
	     pin < RMIXL_GPIO_NSIGNALS; pin++, mask <<= 1, pins++) {
		pins->pin_num = pin;
		if (gpio->gpio_inuse_mask & mask) {
			if ((gpio->gpio_inuse_mask & RMIXL_GPIO_INPUT_MASK))
				pins->pin_caps = GPIO_PIN_INPUT;
		} else {
			int caps = 0;
			if ((gpio->gpio_inuse_mask & RMIXL_GPIO_INPUT_MASK))
				caps |= GPIO_PIN_INPUT;
			if ((gpio->gpio_inuse_mask & RMIXL_GPIO_OUTPUT_MASK))
				caps |= GPIO_PIN_INPUT;
			pins->pin_caps = caps;
		}
		pins->pin_flags =
		    (dir & mask) ? GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		pins->pin_state =
		    (((dir & mask) ? valueout : valuein) & mask)
			? GPIO_PIN_LOW
			: GPIO_PIN_HIGH;
	}

	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}
#endif /* NGPIO > 0 */

int
gpio_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct obio_attach_args *oa = aux;

	if (oa->obio_addr == RMIXL_IO_DEV_GPIO)	/* XXX XLS */
		return 1;

	return 0;
}

void
gpio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args * const oa = aux;
	struct gpio_softc * const gpio = device_private(self);
	int error;

	gpio->gpio_dev = self;

	if (oa->obio_intr == OBIOCF_INTR_DEFAULT)
		panic("\n%s: no intr assigned", device_xname(self));

	if (oa->obio_size == OBIOCF_SIZE_DEFAULT)
		oa->obio_size = 0x1000;

	gpio->gpio_memt = oa->obio_eb_bst;
	error = bus_space_map(oa->obio_eb_bst, oa->obio_addr, oa->obio_size,
	    0, &gpio->gpio_memh);

	if (error) {
		aprint_error(": failed to map register %#"
		    PRIxBUSSIZE "@%#" PRIxBUSADDR ": %d\n",
			oa->obio_size, oa->obio_addr, error);
		return;
	}

#ifdef NOTYET
	if (oa->obio_intr != OBIOCF_INTR_DEFAULT) {
		gpio->gpio_ih = intr_establish(oa->obio_intr, 
		    IPL_HIGH, IST_LEVEL, pic_handle_intr, &gpio->gpio_pic);
		KASSERT(gpio->gpio_ih != NULL);
		aprint_normal(", intr %d", oa->obio_intr);
	}
#endif
	aprint_normal("\n");
#if NGPIO > 0
	config_interrupts(self, gpio_defer);
#endif
}
