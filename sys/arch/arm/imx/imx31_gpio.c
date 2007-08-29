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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: imx31_gpio.c,v 1.1.2.1 2007/08/29 05:24:23 matt Exp $");

#define _INTR_PRIVATE
 
#include <sys/param.h>
#include <sys/evcnt.h>
 
#include <uvm/uvm_extern.h>
  
#include <machine/intr.h>
 
#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <machine/atomic.h>
#include <machine/bus.h>

#include <arm/imx/imx31reg.h>

static void gpio_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void gpio_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static int gpio_pic_find_pending_irqs(struct pic_softc *);
static void gpio_pic_establish_irq(struct pic_softc *, int, int, int);

const struct pic_ops gpio_pic_ops = {
	.pic_block_irqs = gpio_pic_block_irqs,
	.pic_unblock_irqs = gpio_pic_unblock_irqs,
	.pic_find_pending_irqs = gpio_find_pending_irqs,
	.pic_establish_irq = establish_irq,
};

struct gpio_pic_softc {
	struct pic_softc gpic_pic;
	bus_space_tag_t gpic_memt;
	bus_space_handle_t gpic_memh;
	uint32_t gpio_enable_mask;
	uint32_t gpio_edge_mask;
	uint32_t gpio_level_mask;
};

void
gpio_pic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct gpic_softc * const gpic = pic
	KASSERT(irq_base == 0);
	gpic->gpic_enable_mask |= irq_mask;
	/*
	 * If this a level source, ack it now.  If it's still asserted
	 * it'll come back.
	 */
	if (irq_mask & gpic->gpic_level_mask)
		GPIO_WRITE(gpic, GPIO_ISR, irq_mask);
	GPIO_WRITE(gpic, GPIO_IMR, gpic->gpic_enable_mask);
}

void
gpio_pic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	KASSERT(irq_base == 0);
	gpic->gpic_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpic, GPIO_IMR, gpic->gpic_enable_mask);
}

int
gpio_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct gpio_pic_softc * const gpic = pic;
	uint32_t v;
	uint32_t pending;

	v = GPIO_READ(gpic, GPIO_ISR);
	pending = (v & gpic->gpic_enabled);
	if (pending == 0)
		return 0;

	/*
	 * Disable the pending interrupts.
	 */
	gpic->gpic_enable_mask &= ~pending;
	GPIO_WRITE(gpic, GPIO_IMR, gpic->gpic_enable_mask);

	/*
	 * If any of the sources are edge triggered, ack them now so
	 * we won't lose them.
	 */
	if (v & gpic->gpic_edge_mask)
		GPIO_WRITE(gpic, GPIO_ISR, v & gpic->gpic_edge_mask);

	/*
	 * Now find all the pending bits and mark them as pending.
	 */
	do {
		KASSERT(pending != 0)
		irq = 31 - __builtin_clz(pending);
		pending &= ~(1 << irq);
		pic_mark_pending(gpic->gpic_pic, irq)
	} while (pending != 0);

	return 1;
}

#define GPIO_TYPEMAP \
	((GPIO_TYPE_LEVEL_LOW << (2*IST_LEVEL_LOW)) | \
	 (GPIO_TYPE_LEVEL_HIGH << (2*IST_LEVEL_HIGH)) | \
	 (GPIO_TYPE_EDGE_RISING << (2*IST_EDGE_RISING)) | \
	 (GPIO_TYPE_EDGE_FAILING << (2*IST_EDGE_FAILING)))

void
gpio_pic_establish_irq(struct pic_softc *pic, int irq, int ipl, int type)
{
	struct gpio_pic_softc * const gpic = pic;
	KASSERT(irq < 32);
	uint32_t irq_mask = BIT(irq);

	/*
	 * Make sure the irq isn't enabled and not asserting.
	 */
	gpic->gpic_enable_mask &= ~irq_mask;
	GPIO_WRITE(gpic, GPIO_IMR, gpic->gpic_enable_mask);
	GPIO_WRITE(gpic, GPIO_ISR, irq_mask);

	/*
	 * Convert the type to a gpio type and figure out which bits in what 
	 * register we have to tweak.
	 */
	gtype = (GPIO_TYPEMASK >> (2 * type)) & 3;
	icr_shift = (irq & 0x0f) << 1;
	icr_reg = GPIO_ICR + ((irq & 0x10) >> 2);

	/*
	 * Set the interrupt type.
	 */
	v = GPIO_READ(gpic, icr_reg);
	v &= ~(3 << icr_shift);
	v |= gtype << icr_shift;
	GPIO_WRITE(gpic, icr_reg, v);

	/*
	 * Mark it as input.
	 */
	v = GPIO_READ(gpic, GPIO_DIR);
	v &= ~irq_mask;
	GPIO_WRITE(gpic, GPIO_DIR, v); 

	/*
	 * Now record the type of interrupt.
	 */
	if (gtype == GPIO_TYPE_EDGE_RISING || gtype == GPIO_TYPE_EDGE_FAILING) {
		gpic->gpic_edge_mask |= irq_mask;
		gpic->gpic_level_mask &= ~irq_mask;
	} else {
		gpic->gpic_edge_mask &= ~irq_mask;
		gpic->gpic_level_mask |= irq_mask;
	}
}

static int gpio_match(struct device *, struct cfdata *, void *);
static int gpio_attach(struct device *, struct device *, void *);

int
gpio_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct mainbus_attach_args *mba = aux;
	bus_space_handle_t memh;
	int error;

	if (mba->mba_addr != GPIO1_BASE
	    && mba->mba_addr != GPIO2_BASE
	    && mba->mba_addr != GPIO3_BASE)
		return 0;

	if (mba->mba_size == MAINBUSCF_SIZE_DEFAULT)
		return 0;

	error = bus_space_map(mba->mba_memt, mba->mba_addr, mba->mba_size,
	    0, &memh);
	if (error)
		return 0;

	bus_space_unmap(mba->mba_memt, memh);
	return 1;
}

int
gpio_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args * const mba = aux;
	struct gpio_softc * const gsc = (void *) self;
	struct gpio_pic * const gpic = &gsc->gsc_gpic;
	int error;

	gsc->gsc_memt = mba->mba_memt;
	error = bus_space_map(mba->mba_memt, mba->mba_addr, mba->mba_size,
	    0, &gsc->gsc_memh);

	if (error) {
		aprint_error(": failed to map register %#x@%#x: %d\n",
		    mba->mba_size, mba->mba_addr, error);
		return;
	}

	if (mba->mba_irqbase != MAINBUSCF_IRQBASE_DEFAULT) {
		gpic->gpic_pic.pic_ops = &gpio_pic_ops;
		strlcpy(gpic->gpic_pic.pic_name, self->dv_xname,
		    sizeof(gpic->gpic_pic.pic_name));
		gpic->gpic_pic.pic_maxsources = 32;
		pic_add(&gpic->gpic_pic, mba->mba_irqbase);
	}

}
