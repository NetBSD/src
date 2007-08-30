/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
__KERNEL_RCSID(0, "$NetBSD: imx31_icu.c,v 1.1.2.2 2007/08/30 07:08:15 matt Exp $");

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

#include <arm/imx/imx31_intrreg.h>

static void avic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void avic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void avic_establish_irq(struct pic_softc *, int, int, int);
static void avic_source_name(struct pic_softc *, int, char *, size_t);

const struct pic_ops avic_pic_ops = {
	.pic_unblock_irqs = avic_unblock_irqs,
	.pic_block_irqs = avic_block_irqs,
	.pic_establish_irq = avic_establish_irq,
	.pic_source_name = avic_source_name
};

struct avic_softc {
	struct pic_softc avic_pic;
	bus_space_tag_t avic_memt;
	bus_space_handle_t avic_memh;
} pic_avic = {
	.avic_pic = {
		.pic_ops = &avic_pic_ops,
		.pic_maxsources = 64,
		.pic_name = "avic",
	},
};

#define	INTC_READ(avic, reg) \
	bus_space_read_4((avic)->avic_memt, (avic)->avic_memh, (reg))
#define	INTC_WRITE(avic, reg, val) \
	bus_space_write_4((avic)->avic_memt, (avic)->avic_memh, (reg), (val))
#define	HW_TO_SW_IPL(ipl)	((ipl) + 1)
#define	SW_TO_HW_IPL(ipl)	((ipl) - 1)

void
avic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct avic_softc * const avic = (void *) pic;
#if 0
	if (irq_base == 0)
		INTC_WRITE(avic, IMX31_INTENABLEL, irq_mask);
	else
		INTC_WRITE(avic, IMX31_INTENABLEH, irq_mask);
#else
	uint32_t irq;
	while ((irq = ffs(irq_mask)) != 0) {
		irq--;
		irq_base += irq;
		irq_mask >>= irq;
		INTC_WRITE(avic, IMX31_INTENNUM, irq_base);
	}
#endif
}

void
avic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct avic_softc * const avic = (void *) pic;
#if 0
	if (irq_base == 0)
		INTC_WRITE(avic, IMX31_INTDISABLEL, irq_mask);
	else
		INTC_WRITE(avic, IMX31_INTDISABLEH, irq_mask);
#else
	uint32_t irq;
	while ((irq = ffs(irq_mask)) != 0) {
		irq--;
		irq_base += irq;
		irq_mask >>= irq;
		INTC_WRITE(avic, IMX31_INTDISNUM, irq_base);
	}
#endif
}

void
avic_establish_irq(struct pic_softc *pic, int irq, int ipl, int type)
{
	struct avic_softc * const avic = (void *) pic;
	bus_addr_t priority_reg;
	int priority_shift;
	uint32_t v;

	KASSERT(irq < 64);
	KASSERT(ipl < 16);

	priority_reg = IMX31_NIPRIORITY0 - (irq >> 3);
	priority_shift = (irq & 7) * 4; 
	v = INTC_READ(avic, priority_reg);
	v &= ~(0x0f << priority_shift);
	v |= SW_TO_HW_IPL(ipl) << priority_shift;
	INTC_WRITE(avic, priority_reg, v);

	KASSERT(type == IST_LEVEL);
}

static const char * const avic_intr_source_names[] = AVIC_INTR_SOURCE_NAMES;

void
avic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{
	strlcpy(buf, avic_intr_source_names[irq], len);
}

void
imx31_irq_handler(void *frame)
{
	struct avic_softc * const avic = &pic_avic;
	struct pic_softc * const pic = &avic->avic_pic;
	int32_t saved_nimask;
	int32_t irq;
	int ipl, newipl, oldipl;

	saved_nimask = INTC_READ(avic, IMX31_NIMASK);
	for (;;) {
		irq = INTC_READ(avic, IMX31_NIVECSR);
		if (irq < 0)
			break;
		ipl = (int16_t) irq;
		KASSERT(ipl >= 0);
		irq >>= 16;
		KASSERT(irq < 64);
		KASSERT(pic->pic_sources[irq] != NULL);

		/*
		 * If this interrupt is not above the current spl,
		 * mark it as pending and try again.
		 */
		newipl = HW_TO_SW_IPL(ipl);
		if (newipl <= curcpu()->ci_cpl) {
			pic_mark_pending(pic, irq);
			continue;
		}

		/*
		 * Before enabling interrupts, mask out lower priority
		 * interrupts and raise SPL to its equivalent.
		 */

		INTC_WRITE(avic, IMX31_NIMASK, ipl);
		oldipl = _splraise(newipl);
		cpsie(I32_bit);

		pic_dispatch(pic->pic_sources[irq], frame);

		/*
		 * Disable interrupts again.  Drop SPL.  Restore saved
		 * HW interrupt level.
		 */
		cpsid(I32_bit);
		splx(oldipl);
		INTC_WRITE(avic, IMX31_NIMASK, saved_nimask);
	}
}

void
imx31_icu_init(void)
{
	pic_add(&pic_avic.avic_pic, 0);
	softintr_init();
}
