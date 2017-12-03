/*	$NetBSD: imxgpio.c,v 1.2.12.3 2017/12/03 11:35:53 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: imxgpio.c,v 1.2.12.3 2017/12/03 11:35:53 jdolecek Exp $");

#define	_INTR_PRIVATE

#include "locators.h"
#include "gpio.h"
#include "opt_imxgpio.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <sys/bus.h>

#include <arm/imx/imx31reg.h>
#include <arm/imx/imx31var.h>
#include <arm/imx/imxgpioreg.h>
#include <arm/pic/picvar.h>

#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>

#if NGPIO > 0
/* GPIO access from userland */
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

#define	MAX_NGROUP	8

static void gpio_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static int gpio_pic_find_pending_irqs(struct pic_softc *);
static void gpio_pic_establish_irq(struct pic_softc *, struct intrsource *);

const struct pic_ops gpio_pic_ops = {
	.pic_unblock_irqs = gpio_pic_unblock_irqs,
	.pic_block_irqs = gpio_pic_block_irqs,
	.pic_find_pending_irqs = gpio_pic_find_pending_irqs,
	.pic_establish_irq = gpio_pic_establish_irq,
	.pic_source_name = NULL
};

struct gpio_softc {
	device_t gpio_dev;
	struct pic_softc gpio_pic;
#if defined(IMX_GPIO_INTR_SPLIT)
	struct intrsource *gpio_is_0_15;
	struct intrsource *gpio_is_16_31;
#else
	struct intrsource *gpio_is;
#endif
	bus_space_tag_t gpio_memt;
	bus_space_handle_t gpio_memh;
	uint32_t gpio_enable_mask;
	uint32_t gpio_edge_mask;
	uint32_t gpio_level_mask;
#if NGPIO > 0
	struct gpio_chipset_tag gpio_chipset;
	gpio_pin_t gpio_pins[32];
#endif
};

static struct {
	bus_space_tag_t	iot;
	struct {
		bus_space_handle_t ioh;
		struct gpio_softc *softc;
	} unit[MAX_NGROUP];
} gpio_handles;

extern struct cfdriver imxgpio_cd;

CFATTACH_DECL_NEW(imxgpio,
	sizeof(struct gpio_softc),
	imxgpio_match, imxgpio_attach,
	NULL, NULL);


#define	PIC_TO_SOFTC(pic) \
	((struct gpio_softc *)((char *)(pic) - \
		offsetof(struct gpio_softc, gpio_pic)))

#define	GPIO_READ(gpio, reg) \
	bus_space_read_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg))
#define	GPIO_WRITE(gpio, reg, val) \
	bus_space_write_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg), (val))

void
gpio_pic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	gpio->gpio_enable_mask |= irq_mask;

	GPIO_WRITE(gpio, GPIO_ISR, irq_mask);
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);
}

void
gpio_pic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	gpio->gpio_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);
}

int
gpio_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	uint32_t v;
	uint32_t pending;

	v = GPIO_READ(gpio, GPIO_ISR);
	pending = (v & gpio->gpio_enable_mask);
	if (pending == 0)
		return 0;

	/*
	 * Disable the pending interrupts.
	 */
	gpio->gpio_enable_mask &= ~pending;
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);

	/*
	 * If any of the sources are edge triggered, ack them now so
	 * we won't lose them.
	 */
	if (v & gpio->gpio_edge_mask)
		GPIO_WRITE(gpio, GPIO_ISR, v & gpio->gpio_edge_mask);

	/*
	 * Now find all the pending bits and mark them as pending.
	 */
	do {
		int irq;
		KASSERT(pending != 0);
		irq = 31 - __builtin_clz(pending);
		pending &= ~__BIT(irq);

		const struct intrsource *is = pic->pic_sources[irq];
		if (is->is_type == IST_EDGE_BOTH) {
			/*
			 * for both edge
			 */
			uint32_t icr_reg = GPIO_ICR1 + ((is->is_irq & 0x10) >> 2);
			v = GPIO_READ(gpio, icr_reg);
			uint32_t icr_shift = (is->is_irq & 0x0f) << 1;
			uint32_t mask = (3 << icr_shift);
			int gtype = __SHIFTOUT(v, mask);
			if (gtype == GPIO_ICR_EDGE_RISING)
				gtype = GPIO_ICR_EDGE_FALLING;
			else if (gtype == GPIO_ICR_EDGE_FALLING)
				gtype = GPIO_ICR_EDGE_RISING;
			v &= ~mask;
			v |= __SHIFTIN(gtype, mask);
			GPIO_WRITE(gpio, icr_reg, v);
		}
		pic_mark_pending(&gpio->gpio_pic, irq);
	} while (pending != 0);

	return 1;
}

#define	GPIO_TYPEMAP \
	((GPIO_ICR_LEVEL_LOW << (2*IST_LEVEL_LOW)) | \
	 (GPIO_ICR_LEVEL_HIGH << (2*IST_LEVEL_HIGH)) | \
	 (GPIO_ICR_EDGE_RISING << (2*IST_EDGE_RISING)) | \
	 (GPIO_ICR_EDGE_FALLING << (2*IST_EDGE_FALLING)) | \
	 (GPIO_ICR_EDGE_RISING << (2*IST_EDGE_BOTH)))

void
gpio_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct gpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(is->is_irq < 32);
	uint32_t irq_mask = __BIT(is->is_irq);
	uint32_t v;
	unsigned int icr_shift, icr_reg;
	unsigned int gtype;

	/*
	 * Make sure the irq isn't enabled and not asserting.
	 */
	gpio->gpio_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpio, GPIO_ISR, irq_mask);
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);
	/*
	 * Convert the type to a gpio type and figure out which bits in what
	 * register we have to tweak.
	 */
	gtype = (GPIO_TYPEMAP >> (2 * is->is_type)) & 3;
	icr_shift = (is->is_irq & 0x0f) << 1;
	icr_reg = GPIO_ICR1 + ((is->is_irq & 0x10) >> 2);

	/*
	 * Set the interrupt type.
	 */
	v = GPIO_READ(gpio, icr_reg);
	v &= ~(3 << icr_shift);
	v |= gtype << icr_shift;
	GPIO_WRITE(gpio, icr_reg, v);

	/*
	 * Mark it as input.
	 */
	v = GPIO_READ(gpio, GPIO_DIR);
	v &= ~irq_mask;
	GPIO_WRITE(gpio, GPIO_DIR, v);

	/*
	 * Now record the type of interrupt.
	 */
	if (gtype == GPIO_ICR_EDGE_RISING || gtype == GPIO_ICR_EDGE_FALLING) {
		gpio->gpio_edge_mask |= irq_mask;
		gpio->gpio_level_mask &= ~irq_mask;
	} else {
		gpio->gpio_edge_mask &= ~irq_mask;
		gpio->gpio_level_mask |= irq_mask;
	}
}

#if NGPIO > 0

static int
imxgpio_pin_read(void *arg, int pin)
{
	struct gpio_softc * const gpio = arg;

	return (GPIO_READ(gpio, GPIO_DR) >> pin) & 1;
}

static void
imxgpio_pin_write(void *arg, int pin, int value)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	old = GPIO_READ(gpio, GPIO_DR);
	if (value)
		new = old | mask;
	else
		new = old & ~mask;

	if (old != new)
		GPIO_WRITE(gpio, GPIO_DR, new);
}

static void
imxgpio_pin_ctl(void *arg, int pin, int flags)
{
	struct gpio_softc * const gpio = arg;
	uint32_t mask = 1 << pin;
	uint32_t old, new;

	old = GPIO_READ(gpio, GPIO_DIR);
	new = old;
	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:	new &= ~mask; break;
	case GPIO_PIN_OUTPUT:	new |= mask; break;
	default:		return;
	}
	if (old != new)
		GPIO_WRITE(gpio, GPIO_DIR, new);
}

static void
gpio_defer(device_t self)
{
	struct gpio_softc * const gpio = device_private(self);
	struct gpio_chipset_tag * const gp = &gpio->gpio_chipset;
	struct gpiobus_attach_args gba;
	gpio_pin_t *pins;
	uint32_t mask, dir, value;
	int pin;

	gp->gp_cookie = gpio;
	gp->gp_pin_read = imxgpio_pin_read;
	gp->gp_pin_write = imxgpio_pin_write;
	gp->gp_pin_ctl = imxgpio_pin_ctl;

	gba.gba_gc = gp;
	gba.gba_pins = gpio->gpio_pins;
	gba.gba_npins = __arraycount(gpio->gpio_pins);

	dir = GPIO_READ(gpio, GPIO_DIR);
	value = GPIO_READ(gpio, GPIO_DR);
	for (pin = 0, mask = 1, pins = gpio->gpio_pins;
	     pin < 32; pin++, mask <<= 1, pins++) {
		pins->pin_num = pin;
		if ((gpio->gpio_edge_mask|gpio->gpio_level_mask) & mask)
			pins->pin_caps = GPIO_PIN_INPUT;
		else
			pins->pin_caps = GPIO_PIN_INPUT|GPIO_PIN_OUTPUT;
		pins->pin_flags =
		    (dir & mask) ? GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		pins->pin_state =
		    (value & mask) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
	}

	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}
#endif /* NGPIO > 0 */

void
imxgpio_attach_common(device_t self, bus_space_tag_t iot,
    bus_space_handle_t ioh, int index, int intr, int irqbase)
{
	struct gpio_softc * const gpio = device_private(self);

	KASSERT(index < MAX_NGROUP);

	gpio->gpio_dev = self;
	gpio->gpio_memt = iot;
	gpio->gpio_memh = ioh;

	if (irqbase > 0) {
		gpio->gpio_pic.pic_ops = &gpio_pic_ops;
		strlcpy(gpio->gpio_pic.pic_name, device_xname(self),
		    sizeof(gpio->gpio_pic.pic_name));
		gpio->gpio_pic.pic_maxsources = 32;

		pic_add(&gpio->gpio_pic, irqbase);

		aprint_normal(": interrupts %d..%d",
		    irqbase, irqbase + GPIO_NPINS - 1);

#if defined(IMX_GPIO_INTR_SPLIT)
		gpio->gpio_is_0_15 = intr_establish(intr,
		    IPL_NET, IST_LEVEL, pic_handle_intr, &gpio->gpio_pic);
		KASSERT( gpio->gpio_is_0_15 != NULL );
		gpio->gpio_is_16_31 = intr_establish(intr + 1,
		    IPL_NET, IST_LEVEL, pic_handle_intr, &gpio->gpio_pic);
		KASSERT( gpio->gpio_is_16_31 != NULL );
#else
		gpio->gpio_is = intr_establish(intr,
		    IPL_NET, IST_LEVEL, pic_handle_intr, &gpio->gpio_pic);
		KASSERT( gpio->gpio_is != NULL );
#endif
	}
	aprint_normal("\n");

	gpio_handles.iot = iot;
	gpio_handles.unit[index].softc = gpio;
	gpio_handles.unit[index].ioh = ioh;

#if NGPIO > 0
	config_interrupts(self, gpio_defer);
#endif
}

#define	GPIO_GROUP_READ(index,offset)	\
	bus_space_read_4(gpio_handles.iot, gpio_handles.unit[index].ioh, \
	    (offset))
#define	GPIO_GROUP_WRITE(index,offset,value)				\
	bus_space_write_4(gpio_handles.iot, gpio_handles.unit[index].ioh, \
	    (offset), (value))

void
gpio_set_direction(u_int gpio, u_int dir)
{
	int index = gpio / GPIO_NPINS;
	int bit = gpio % GPIO_NPINS;
	uint32_t reg;

	KDASSERT(index < imxgpio_ngroups);

	/* XXX lock */


	reg = GPIO_GROUP_READ(index, GPIO_DIR);
	if (dir == GPIO_DIR_OUT)
		reg |= __BIT(bit);
	else
		reg &= ~__BIT(bit);
	GPIO_GROUP_WRITE(index, GPIO_DIR, reg);
		      
	/* XXX unlock */
}


void
gpio_data_write(u_int gpio, u_int value)
{
	int index = gpio / GPIO_NPINS;
	int bit = gpio % GPIO_NPINS;
	uint32_t reg;

	KDASSERT(index < imxgpio_ngroups);

	/* XXX lock */
	reg = GPIO_GROUP_READ(index, GPIO_DR);
	if (value)
		reg |= __BIT(bit);
	else
		reg &= ~__BIT(bit);
	GPIO_GROUP_WRITE(index, GPIO_DR, reg);

	/* XXX unlock */
}

bool
gpio_data_read(u_int gpio)
{
	int index = gpio / GPIO_NPINS;
	int bit = gpio % GPIO_NPINS;
	uint32_t reg;

	KDASSERT(index < imxgpio_ngroups);

	reg = GPIO_GROUP_READ(index, GPIO_DR);

	return reg & __BIT(bit) ? true : false;
}
