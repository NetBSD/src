/*	$NetBSD: omap2_gpio.c,v 1.16.10.2 2016/12/05 10:54:50 skrll Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: omap2_gpio.c,v 1.16.10.2 2016/12/05 10:54:50 skrll Exp $");

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

#include <arm/omap/am335x_prcm.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_gpio.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/sitara_cm.h>
#include <arm/omap/sitara_cmreg.h>
#include <arm/pic/picvar.h>

#if NGPIO > 0
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

#ifdef TI_AM335X
static const struct omap_module gpio_module[] = {
	{ 0, 0 },
	{ AM335X_PRCM_CM_PER, CM_PER_GPIO1_CLKCTRL },
	{ AM335X_PRCM_CM_PER, CM_PER_GPIO2_CLKCTRL },
	{ AM335X_PRCM_CM_PER, CM_PER_GPIO3_CLKCTRL },
};
#endif

static void gpio_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_block_irqs2(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_unblock_irqs2(struct pic_softc *, size_t, uint32_t);
static int gpio_pic_find_pending_irqs(struct pic_softc *);
static int gpio_pic_find_pending_irqs2(struct pic_softc *);
static void gpio_pic_establish_irq(struct pic_softc *, struct intrsource *);

const struct pic_ops gpio_pic_ops = {
	.pic_block_irqs = gpio_pic_block_irqs,
	.pic_unblock_irqs = gpio_pic_unblock_irqs,
	.pic_find_pending_irqs = gpio_pic_find_pending_irqs,
	.pic_establish_irq = gpio_pic_establish_irq,
};
const struct pic_ops gpio_pic_ops2 = {
	.pic_block_irqs = gpio_pic_block_irqs2,
	.pic_unblock_irqs = gpio_pic_unblock_irqs2,
	.pic_find_pending_irqs = gpio_pic_find_pending_irqs2,
	.pic_establish_irq = gpio_pic_establish_irq,
};

struct gpio_softc {
	device_t gpio_dev;
	struct pic_softc gpio_pic;
	struct intrsource *gpio_is;
	bus_space_tag_t gpio_memt;
	bus_space_handle_t gpio_memh;
	bus_space_handle_t gpio_memh2;
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
	gpio_pin_t gpio_pins[32];
#endif
};

#define	PIC_TO_SOFTC(pic) \
	((struct gpio_softc *)((char *)(pic) - \
		offsetof(struct gpio_softc, gpio_pic)))

#define	GPIO_READ(gpio, reg) \
	bus_space_read_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg))
#define	GPIO_WRITE(gpio, reg, val) \
	bus_space_write_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg), (val))
#define	GPIO_READ2(gpio, reg) \
	bus_space_read_4((gpio)->gpio_memt, (gpio)->gpio_memh2, (reg))
#define	GPIO_WRITE2(gpio, reg, val) \
	bus_space_write_4((gpio)->gpio_memt, (gpio)->gpio_memh2, (reg), (val))

void
gpio_pic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	gpio->gpio_enable_mask |= irq_mask;
	/*
	 * If this a level source, ack it now.  If it's still asserted
	 * it'll come back.
	 */
	GPIO_WRITE(gpio, GPIO_SETIRQENABLE1, gpio->gpio_enable_mask);
	if (irq_mask & gpio->gpio_level_mask)
		GPIO_WRITE(gpio, GPIO_IRQSTATUS1,
		    irq_mask & gpio->gpio_level_mask);
}

void
gpio_pic_unblock_irqs2(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	/*
	 * If this a level source, ack it now.  If it's still asserted
	 * it'll come back.
	 */
	GPIO_WRITE2(gpio, GPIO_IRQSTATUS_SET_0, irq_mask);
	if (irq_mask & gpio->gpio_level_mask)
		GPIO_WRITE2(gpio, GPIO_IRQSTATUS_0,
		    irq_mask & gpio->gpio_level_mask);
}

void
gpio_pic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	gpio->gpio_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpio, GPIO_CLEARIRQENABLE1, irq_mask);
	/*
	 * If any of the sources are edge triggered, ack them now so
	 * we won't lose them.
	 */
	if (irq_mask & gpio->gpio_edge_mask)
		GPIO_WRITE(gpio, GPIO_IRQSTATUS1,
		    irq_mask & gpio->gpio_edge_mask);
}

void
gpio_pic_block_irqs2(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	GPIO_WRITE2(gpio, GPIO_IRQSTATUS_CLR_0, irq_mask);
	/*
	 * If any of the sources are edge triggered, ack them now so
	 * we won't lose them.
	 */
	if (irq_mask & gpio->gpio_edge_mask)
		GPIO_WRITE2(gpio, GPIO_IRQSTATUS_0,
		    irq_mask & gpio->gpio_edge_mask);
}

int
gpio_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	uint32_t v;
	uint32_t pending;

	v = GPIO_READ(gpio, GPIO_IRQSTATUS1);
	pending = (v & gpio->gpio_enable_mask);
	if (pending == 0)
		return 0;

	/*
	 * Now find all the pending bits and mark them as pending.
	 */
	(void) pic_mark_pending_sources(&gpio->gpio_pic, 0, pending);

	return 1;
}

int
gpio_pic_find_pending_irqs2(struct pic_softc *pic)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	uint32_t pending;

	pending = GPIO_READ2(gpio, GPIO_IRQSTATUS_0);
	if (pending == 0)
		return 0;

	/*
	 * Now find all the pending bits and mark them as pending.
	 */
	(void) pic_mark_pending_sources(&gpio->gpio_pic, 0, pending);

	return 1;
}

void
gpio_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(is->is_irq < 32);
	uint32_t irq_mask = __BIT(is->is_irq);
	uint32_t v;
#if 0
	unsigned int i;
	struct intrsource *maybe_is;
#endif

	/*
	 * Make sure the irq isn't enabled and not asserting.
	 */
#if defined(OMAP_4430) || defined(TI_AM335X)
	GPIO_WRITE2(gpio, GPIO_IRQSTATUS_SET_0, irq_mask);
	GPIO_WRITE2(gpio, GPIO_IRQSTATUS_0, irq_mask);
#else
	gpio->gpio_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpio, GPIO_IRQENABLE1, gpio->gpio_enable_mask);
	GPIO_WRITE(gpio, GPIO_IRQSTATUS1, irq_mask);
#endif

	/*
	 * Convert the type to a gpio type and figure out which bits in what
	 * register we have to tweak.
	 */
	gpio->gpio_edge_rising_mask &= ~irq_mask;
	gpio->gpio_edge_falling_mask &= ~irq_mask;
	gpio->gpio_level_hi_mask &= ~irq_mask;
	gpio->gpio_level_lo_mask &= ~irq_mask;
	switch (is->is_type) {
	case IST_LEVEL_LOW: gpio->gpio_level_lo_mask |= irq_mask; break;
	case IST_LEVEL_HIGH: gpio->gpio_level_hi_mask |= irq_mask; break;
	case IST_EDGE_FALLING: gpio->gpio_edge_falling_mask |= irq_mask; break;
	case IST_EDGE_RISING: gpio->gpio_edge_rising_mask |= irq_mask; break;
	case IST_EDGE_BOTH: gpio->gpio_edge_falling_mask |= irq_mask;
			    gpio->gpio_edge_rising_mask |= irq_mask; break;
	}
	gpio->gpio_edge_mask =
	    gpio->gpio_edge_rising_mask | gpio->gpio_edge_falling_mask;
	gpio->gpio_level_mask =
	    gpio->gpio_level_hi_mask|gpio->gpio_level_lo_mask;
	gpio->gpio_inuse_mask |= irq_mask;

	/*
	 * Set the interrupt type.
	 */
	GPIO_WRITE(gpio, GPIO_LEVELDETECT0, gpio->gpio_level_lo_mask);
	GPIO_WRITE(gpio, GPIO_LEVELDETECT1, gpio->gpio_level_hi_mask);
	GPIO_WRITE(gpio, GPIO_RISINGDETECT, gpio->gpio_edge_rising_mask);
	GPIO_WRITE(gpio, GPIO_FALLINGDETECT, gpio->gpio_edge_falling_mask);

	/*
	 * Mark it as input.
	 */
	v = GPIO_READ(gpio, GPIO_OE);
	v |= irq_mask;
	GPIO_WRITE(gpio, GPIO_OE, v);
#if 0
	for (i = 0, maybe_is = NULL; i < 32; i++) {
		if ((is = pic->pic_sources[i]) != NULL) {
			if (maybe_is == NULL || is->is_ipl > maybe_is->is_ipl)
				maybe_is = is;
		}
	}
	if (maybe_is != NULL) {
		is = gpio->gpio_is;
		KASSERT(is != NULL);
		is->is_ipl = maybe_is->is_ipl;
		(*is->is_pic->pic_ops->pic_establish_irq)(is->is_pic, is);
	}
#endif
}

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

	return (GPIO_READ(gpio, GPIO_DATAIN) >> pin) & 1;
}

static void
omap2gpio_pin_write(void *arg, int pin, int value)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;

	if (value)
		GPIO_WRITE(gpio, GPIO_SETDATAOUT, mask);
	else
		GPIO_WRITE(gpio, GPIO_CLEARDATAOUT, mask);
}

static void
omap2gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	old = GPIO_READ(gpio, GPIO_OE);
	new = old;
	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:	new |= mask; break;
	case GPIO_PIN_OUTPUT:	new &= ~mask; break;
	default:		return;
	}
	if (old != new)
		GPIO_WRITE(gpio, GPIO_OE, new);
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

	dir = GPIO_READ(gpio, GPIO_OE);
	valueout = GPIO_READ(gpio, GPIO_DATAOUT);
	valuein = GPIO_READ(gpio, GPIO_DATAIN);
	for (pin = 0, mask = 1, pins = gpio->gpio_pins;
	     pin < 32; pin++, mask <<= 1, pins++) {
		pins->pin_num = pin;
		if (gpio->gpio_inuse_mask & mask)
			pins->pin_caps = GPIO_PIN_INPUT;
		else
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

#ifdef OMAP_2420
	if (oa->obio_addr == GPIO1_BASE_2420
	    || oa->obio_addr == GPIO2_BASE_2420
	    || oa->obio_addr == GPIO3_BASE_2420
	    || oa->obio_addr == GPIO4_BASE_2420)
		return 1;
#endif

#ifdef OMAP_2430
	if (oa->obio_addr == GPIO1_BASE_2430
	    || oa->obio_addr == GPIO2_BASE_2430
	    || oa->obio_addr == GPIO3_BASE_2430
	    || oa->obio_addr == GPIO4_BASE_2430
	    || oa->obio_addr == GPIO5_BASE_2430)
		return 1;
#endif

#ifdef OMAP_3430
	if (oa->obio_addr == GPIO1_BASE_3430
	    || oa->obio_addr == GPIO2_BASE_3430
	    || oa->obio_addr == GPIO3_BASE_3430
	    || oa->obio_addr == GPIO4_BASE_3430
	    || oa->obio_addr == GPIO5_BASE_3430
	    || oa->obio_addr == GPIO6_BASE_3430)
		return 1;
#endif

#ifdef OMAP_3530
	if (oa->obio_addr == GPIO1_BASE_3530
	    || oa->obio_addr == GPIO2_BASE_3530
	    || oa->obio_addr == GPIO3_BASE_3530
	    || oa->obio_addr == GPIO4_BASE_3530
	    || oa->obio_addr == GPIO5_BASE_3530
	    || oa->obio_addr == GPIO6_BASE_3530)
		return 1;
#endif

#ifdef OMAP_4430
	if (oa->obio_addr == GPIO1_BASE_4430
	    || oa->obio_addr == GPIO2_BASE_4430
	    || oa->obio_addr == GPIO3_BASE_4430
	    || oa->obio_addr == GPIO4_BASE_4430
	    || oa->obio_addr == GPIO5_BASE_4430
	    || oa->obio_addr == GPIO6_BASE_4430)
		return 1;
#endif

#ifdef OMAP_5430
	if (oa->obio_addr == GPIO1_BASE_5430
	    || oa->obio_addr == GPIO2_BASE_5430
	    || oa->obio_addr == GPIO3_BASE_5430
	    || oa->obio_addr == GPIO4_BASE_5430
	    || oa->obio_addr == GPIO5_BASE_5430
	    || oa->obio_addr == GPIO6_BASE_5430
	    || oa->obio_addr == GPIO7_BASE_5430
	    || oa->obio_addr == GPIO8_BASE_5430)
		return 1;
#endif

#ifdef TI_AM335X
	if (oa->obio_addr == GPIO0_BASE_TI_AM335X
	    || oa->obio_addr == GPIO1_BASE_TI_AM335X
	    || oa->obio_addr == GPIO2_BASE_TI_AM335X
	    || oa->obio_addr == GPIO3_BASE_TI_AM335X)
		return 1;
#endif

#ifdef TI_DM37XX
	if (oa->obio_addr == GPIO1_BASE_TI_DM37XX
	    || oa->obio_addr == GPIO2_BASE_TI_DM37XX
	    || oa->obio_addr == GPIO3_BASE_TI_DM37XX
	    || oa->obio_addr == GPIO4_BASE_TI_DM37XX
	    || oa->obio_addr == GPIO5_BASE_TI_DM37XX
	    || oa->obio_addr == GPIO6_BASE_TI_DM37XX)
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
#if defined(OMAP_4430) || defined(TI_AM335X)
	error = bus_space_map(oa->obio_iot, oa->obio_addr, oa->obio_size,
	    0, &gpio->gpio_memh2);
	if (error == 0)
		error = bus_space_subregion(gpio->gpio_memt, gpio->gpio_memh2,
		    GPIO_SIZE2, oa->obio_size - GPIO_SIZE2, &gpio->gpio_memh);
#else
	error = bus_space_map(oa->obio_iot, oa->obio_addr, oa->obio_size,
	    0, &gpio->gpio_memh);
#endif
	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    oa->obio_size, oa->obio_addr, error);
		return;
	}

	if (oa->obio_intrbase != OBIOCF_INTRBASE_DEFAULT) {
#if defined(OMAP_4430) || defined(TI_AM335X)
		gpio->gpio_pic.pic_ops = &gpio_pic_ops2;
#else
		gpio->gpio_pic.pic_ops = &gpio_pic_ops;
#endif
		strlcpy(gpio->gpio_pic.pic_name, device_xname(self),
		    sizeof(gpio->gpio_pic.pic_name));
		gpio->gpio_pic.pic_maxsources = 32;
		pic_add(&gpio->gpio_pic, oa->obio_intrbase);
		aprint_normal(": interrupts %d..%d",
		    oa->obio_intrbase, oa->obio_intrbase + 31);
		gpio->gpio_is = intr_establish(oa->obio_intr,
		    IPL_HIGH, IST_LEVEL, pic_handle_intr, &gpio->gpio_pic);
		KASSERT(gpio->gpio_is != NULL);
		aprint_normal(", intr %d", oa->obio_intr);
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
#if 0
	config_interrupts(self, gpio_attach1);
#else
	gpio_attach1(self);
#endif
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
