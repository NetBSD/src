/*	$NetBSD: imx31_gpio.c,v 1.5 2010/11/15 18:18:39 bsh Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: imx31_gpio.c,v 1.5 2010/11/15 18:18:39 bsh Exp $");

#define _INTR_PRIVATE

#include "locators.h"
#include "gpio.h"
 
#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
 
#include <uvm/uvm_extern.h>
  
#include <machine/intr.h>
 
#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <machine/bus.h>

#include <arm/imx/imx31reg.h>
#include <arm/imx/imx31var.h>
#include <arm/imx/imxgpioreg.h>
#include <arm/pic/picvar.h>

#if NGPIO > 0
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

static void gpio_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static int gpio_pic_find_pending_irqs(struct pic_softc *);
static void gpio_pic_establish_irq(struct pic_softc *, struct intrsource *);

const struct pic_ops gpio_pic_ops = {
	.pic_block_irqs = gpio_pic_block_irqs,
	.pic_unblock_irqs = gpio_pic_unblock_irqs,
	.pic_find_pending_irqs = gpio_pic_find_pending_irqs,
	.pic_establish_irq = gpio_pic_establish_irq,
};

struct gpio_softc {
	struct device gpio_dev;
	struct pic_softc gpio_pic;
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
	/*
	 * If this a level source, ack it now.  If it's still asserted
	 * it'll come back.
	 */
	if (irq_mask & gpio->gpio_level_mask)
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
		pic_mark_pending(&gpio->gpio_pic, irq);
	} while (pending != 0);

	return 1;
}

#define GPIO_TYPEMAP \
	((GPIO_ICR_LEVEL_LOW << (2*IST_LEVEL_LOW)) | \
	 (GPIO_ICR_LEVEL_HIGH << (2*IST_LEVEL_HIGH)) | \
	 (GPIO_ICR_EDGE_RISING << (2*IST_EDGE_RISING)) | \
	 (GPIO_ICR_EDGE_FALLING << (2*IST_EDGE_FALLING)))

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
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);
	GPIO_WRITE(gpio, GPIO_ISR, irq_mask);

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

static int gpio_match(device_t, cfdata_t, void *);
static void gpio_attach(device_t, device_t, void *);

CFATTACH_DECL(imxgpio,
	sizeof(struct gpio_softc),
	gpio_match, gpio_attach,
	NULL, NULL);

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
	case GPIO_PIN_INPUT:	new |= mask; break;
	case GPIO_PIN_OUTPUT:	new &= ~mask; break;
	default:		return;
	}
	if (old != new)
		GPIO_WRITE(gpio, GPIO_DIR, new);
}

static void
gpio_defer(device_t self)
{
	struct gpio_softc * const gpio = (void *) self;
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

int
gpio_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct ahb_attach_args *ahba = aux;
	bus_space_handle_t memh;
	bus_size_t size;
	int error;

	if (ahba->ahba_addr != GPIO1_BASE
	    && ahba->ahba_addr != GPIO2_BASE
	    && ahba->ahba_addr != GPIO3_BASE)
		return 0;

	size = (ahba->ahba_size == AHBCF_SIZE_DEFAULT) ? GPIO_SIZE : ahba->ahba_size;

	error = bus_space_map(ahba->ahba_memt, ahba->ahba_addr, size, 0, &memh);
	if (error)
		return 0;

	bus_space_unmap(ahba->ahba_memt, memh, size);
	return 1;
}

void
gpio_attach(device_t parent, device_t self, void *aux)
{
	struct ahb_attach_args * const ahba = aux;
	struct gpio_softc * const gpio = (void *) self;
	int error;

	if (ahba->ahba_size == AHBCF_SIZE_DEFAULT)
		ahba->ahba_size = GPIO_SIZE;

	gpio->gpio_memt = ahba->ahba_memt;
	error = bus_space_map(ahba->ahba_memt, ahba->ahba_addr, ahba->ahba_size,
	    0, &gpio->gpio_memh);

	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    ahba->ahba_size, ahba->ahba_addr, error);
		return;
	}

	if (ahba->ahba_irqbase != AHBCF_IRQBASE_DEFAULT) {
		gpio->gpio_pic.pic_ops = &gpio_pic_ops;
		strlcpy(gpio->gpio_pic.pic_name, self->dv_xname,
		    sizeof(gpio->gpio_pic.pic_name));
		gpio->gpio_pic.pic_maxsources = 32;
		pic_add(&gpio->gpio_pic, ahba->ahba_irqbase);
		aprint_normal(": interrupts %d..%d",
		    ahba->ahba_irqbase, ahba->ahba_irqbase + 31);
	}
	aprint_normal("\n");
#if NGPIO > 0
	config_interrupts(self, gpio_defer);
#endif
}
