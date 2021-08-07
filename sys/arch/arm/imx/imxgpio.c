/*	$NetBSD: imxgpio.c,v 1.11 2021/08/07 16:18:44 thorpej Exp $ */

/*-
 * Copyright (c) 2007, 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: imxgpio.c,v 1.11 2021/08/07 16:18:44 thorpej Exp $");

#define	_INTR_PRIVATE

#include "locators.h"
#include "gpio.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>

#if NGPIO > 0
/* GPIO access from userland */
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

#define	MAX_NGROUP	8

static void imxgpio_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void imxgpio_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static int imxgpio_pic_find_pending_irqs(struct pic_softc *);
static void imxgpio_pic_establish_irq(struct pic_softc *, struct intrsource *);

const struct pic_ops imxgpio_pic_ops = {
	.pic_unblock_irqs = imxgpio_pic_unblock_irqs,
	.pic_block_irqs = imxgpio_pic_block_irqs,
	.pic_find_pending_irqs = imxgpio_pic_find_pending_irqs,
	.pic_establish_irq = imxgpio_pic_establish_irq,
	.pic_source_name = NULL
};

static struct imxgpio_softc *imxgpio_handles[MAX_NGROUP];

CFATTACH_DECL_NEW(imxgpio, sizeof(struct imxgpio_softc),
    imxgpio_match, imxgpio_attach, NULL, NULL);

#define	PIC_TO_SOFTC(pic)						      \
	((struct imxgpio_softc *)((char *)(pic) -			      \
	    offsetof(struct imxgpio_softc, gpio_pic)))

#define	GPIO_READ(gpio, reg)						      \
	bus_space_read_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg))
#define	GPIO_WRITE(gpio, reg, val)					      \
	bus_space_write_4((gpio)->gpio_memt, (gpio)->gpio_memh, (reg), (val))

void
imxgpio_pic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct imxgpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	gpio->gpio_enable_mask |= irq_mask;

	GPIO_WRITE(gpio, GPIO_ISR, irq_mask);
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);
}

void
imxgpio_pic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct imxgpio_softc * const gpio = PIC_TO_SOFTC(pic);
	KASSERT(irq_base == 0);

	gpio->gpio_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpio, GPIO_IMR, gpio->gpio_enable_mask);
}

int
imxgpio_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct imxgpio_softc * const gpio = PIC_TO_SOFTC(pic);
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
imxgpio_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct imxgpio_softc * const gpio = PIC_TO_SOFTC(pic);
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

int
imxgpio_pin_read(void *arg, int pin)
{
	struct imxgpio_softc * const gpio = arg;
	int val;

	val = __SHIFTOUT(GPIO_READ(gpio, GPIO_DR), __BIT(pin));

	return val;
}

void
imxgpio_pin_write(void *arg, int pin, int value)
{
	struct imxgpio_softc * const gpio = arg;
	uint32_t mask = __BIT(pin);
	uint32_t old;
	uint32_t new;

	mutex_enter(&gpio->gpio_lock);

	old = GPIO_READ(gpio, GPIO_DR);
	if (value)
		new = old | mask;
	else
		new = old & ~mask;

	if (old != new)
		GPIO_WRITE(gpio, GPIO_DR, new);

	mutex_exit(&gpio->gpio_lock);
}

void
imxgpio_pin_ctl(void *arg, int pin, int flags)
{
	struct imxgpio_softc * const gpio = arg;
	uint32_t mask = __BIT(pin);
	uint32_t old;
	uint32_t new;

	mutex_enter(&gpio->gpio_lock);

	old = GPIO_READ(gpio, GPIO_DIR);
	new = old;
	switch (flags & (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:
		new &= ~mask;
		break;
	case GPIO_PIN_OUTPUT:
		new |= mask;
		break;
	default:
		break;
	}

	if (old != new)
		GPIO_WRITE(gpio, GPIO_DIR, new);

	mutex_exit(&gpio->gpio_lock);
}

static void
imxgpio_attach_ports(struct imxgpio_softc *gpio)
{
	struct gpio_chipset_tag * const gp = &gpio->gpio_chipset;
	struct gpiobus_attach_args gba;
	uint32_t dir;
	u_int pin;

	gp->gp_cookie = gpio;
	gp->gp_pin_read = imxgpio_pin_read;
	gp->gp_pin_write = imxgpio_pin_write;
	gp->gp_pin_ctl = imxgpio_pin_ctl;

	dir = GPIO_READ(gpio, GPIO_DIR);
	for (pin = 0; pin < __arraycount(gpio->gpio_pins); pin++) {
		uint32_t mask = __BIT(pin);
		gpio_pin_t *pins = &gpio->gpio_pins[pin];
		pins->pin_num = pin;
		if ((gpio->gpio_edge_mask | gpio->gpio_level_mask) & mask)
			pins->pin_caps = GPIO_PIN_INPUT;
		else
			pins->pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		pins->pin_flags =
		    (dir & mask) ? GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		pins->pin_state = imxgpio_pin_read(gpio, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = gpio->gpio_pins;
	gba.gba_npins = __arraycount(gpio->gpio_pins);
	config_found(gpio->gpio_dev, &gba, gpiobus_print, CFARGS_NONE);
}
#endif /* NGPIO > 0 */

void
imxgpio_attach_common(device_t self)
{
	struct imxgpio_softc * const gpio = device_private(self);

	gpio->gpio_dev = self;

	if (gpio->gpio_irqbase == PIC_IRQBASE_ALLOC || gpio->gpio_irqbase > 0) {
		gpio->gpio_pic.pic_ops = &imxgpio_pic_ops;
		strlcpy(gpio->gpio_pic.pic_name, device_xname(self),
		    sizeof(gpio->gpio_pic.pic_name));
		gpio->gpio_pic.pic_maxsources = GPIO_NPINS;

		gpio->gpio_irqbase = pic_add(&gpio->gpio_pic, gpio->gpio_irqbase);

		aprint_normal_dev(gpio->gpio_dev, "interrupts %d..%d\n",
		    gpio->gpio_irqbase, gpio->gpio_irqbase + GPIO_NPINS - 1);
	}

	mutex_init(&gpio->gpio_lock, MUTEX_DEFAULT, IPL_VM);

	if (gpio->gpio_unit != -1) {
		KASSERT(gpio->gpio_unit < MAX_NGROUP);
		imxgpio_handles[gpio->gpio_unit] = gpio;
	}

#if NGPIO > 0
	imxgpio_attach_ports(gpio);
#endif
}

/* in-kernel GPIO access utility functions */
void
imxgpio_set_direction(u_int gpio, int dir)
{
	int index = gpio / GPIO_NPINS;
	int pin = gpio % GPIO_NPINS;

	KDASSERT(index < imxgpio_ngroups);
	KASSERT(imxgpio_handles[index] != NULL);

	imxgpio_pin_ctl(imxgpio_handles[index], pin, dir);
}

void
imxgpio_data_write(u_int gpio, u_int value)
{
	int index = gpio / GPIO_NPINS;
	int pin = gpio % GPIO_NPINS;

	KDASSERT(index < imxgpio_ngroups);
	KASSERT(imxgpio_handles[index] != NULL);

	imxgpio_pin_write(imxgpio_handles[index], pin, value);
}

bool
imxgpio_data_read(u_int gpio)
{
	int index = gpio / GPIO_NPINS;
	int pin = gpio % GPIO_NPINS;

	KDASSERT(index < imxgpio_ngroups);
	KASSERT(imxgpio_handles[index] != NULL);

	return imxgpio_pin_read(imxgpio_handles[index], pin) ? true : false;
}

