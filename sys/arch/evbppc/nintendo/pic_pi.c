/* $NetBSD: pic_pi.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $ */

/*-
 * Copyright (c) 2024-2025 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Processor interface interrupt controller. Top level controller for all
 * EXT interrupts.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_pi.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/bitops.h>
#include <sys/cpu.h>
#include <powerpc/include/spr.h>
#include <powerpc/include/oea/spr.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
#include <machine/wii.h>
#include <machine/wiiu.h>

#include "pic_pi.h"

struct pic_state {
	uint32_t irqmask;
	uint32_t actmask;
	uint32_t intsr;
	uint32_t intmr;
} __aligned(32);

static struct pic_state pic_s[CPU_MAXNUM];

#define WR4(reg, val)	out32(reg, val)
#define RD4(reg)	in32(reg)

#ifdef MULTIPROCESSOR
extern struct ipi_ops ipiops;

#define IRQ_IS_IPI(_irq)	\
    ((_irq) >= WIIU_PI_IRQ_MB_CPU(0) && (_irq) <= WIIU_PI_IRQ_MB_CPU(2))
#endif

static u_int
pi_irq_affinity(int irq)
{
#ifdef MULTIPROCESSOR
	if (wiiu_native && IRQ_IS_IPI(irq)) {
		return irq - WIIU_PI_IRQ_MB_CPU(0);
	}
#endif
	return 0;
}

static void
pi_enable_irq(struct pic_ops *pic, int irq, int type)
{
	const u_int cpu_num = pi_irq_affinity(irq);

	pic_s[cpu_num].irqmask |= __BIT(irq);
	WR4(pic_s[cpu_num].intmr, pic_s[cpu_num].irqmask & ~pic_s[cpu_num].actmask);
}

static void
pi_disable_irq(struct pic_ops *pic, int irq)
{
	const u_int cpu_num = pi_irq_affinity(irq);

	pic_s[cpu_num].irqmask &= ~__BIT(irq);
	WR4(pic_s[cpu_num].intmr, pic_s[cpu_num].irqmask & ~pic_s[cpu_num].actmask);
}

#ifdef MULTIPROCESSOR
static void
pi_ipi_ack(const u_int cpu_num, register_t spr)
{
	do {
		mtspr(SPR_SCR, spr & ~SPR_SCR_IPI_PEND(cpu_num));
		spr = mfspr(SPR_SCR);
	} while ((spr & SPR_SCR_IPI_PEND(cpu_num)) != 0);
}
#endif

static int
pi_get_irq(struct pic_ops *pic, int mode)
{
	const u_int cpu_num = cpu_number();
	uint32_t raw, pend;
	int irq;

#ifdef MULTIPROCESSOR
	if (wiiu_native) {
		register_t spr = mfspr(SPR_SCR);

		if ((spr & SPR_SCR_IPI_PEND(cpu_num)) != 0) {
			pi_ipi_ack(cpu_num, spr);
			return WIIU_PI_IRQ_MB_CPU(cpu_num);
		}
	}
#endif

	raw = RD4(pic_s[cpu_num].intsr);
	pend = raw & pic_s[cpu_num].irqmask;
	if (pend == 0) {
		return 255;
	}
	irq = ffs32(pend) - 1;

	pic_s[cpu_num].actmask |= __BIT(irq);
	WR4(pic_s[cpu_num].intmr, pic_s[cpu_num].irqmask & ~pic_s[cpu_num].actmask);

	return irq;
}

static void
pi_ack_irq(struct pic_ops *pic, int irq)
{
	const u_int cpu_num = cpu_number();

	pic_s[cpu_num].actmask &= ~__BIT(irq);
	WR4(pic_s[cpu_num].intmr, pic_s[cpu_num].irqmask & ~pic_s[cpu_num].actmask);
	WR4(pic_s[cpu_num].intsr, __BIT(irq));
}

static struct pic_ops pic = {
	.pic_name = "pi",
	.pic_numintrs = 32,
	.pic_cookie = NULL,
	.pic_enable_irq = pi_enable_irq,
	.pic_reenable_irq = pi_enable_irq,
	.pic_disable_irq = pi_disable_irq,
	.pic_get_irq = pi_get_irq,
	.pic_ack_irq = pi_ack_irq,
	.pic_establish_irq = dummy_pic_establish_intr,
};

void
pi_init_intr(void)
{
	u_int cpu_num;

	for (cpu_num = 0; cpu_num < uimin(3, CPU_MAXNUM); cpu_num++) {
		pic_s[cpu_num].irqmask = 0;
		pic_s[cpu_num].actmask = 0;
		if (wiiu_native) {
			pic_s[cpu_num].intmr = WIIU_PI_INTMSK(cpu_num);
			pic_s[cpu_num].intsr = WIIU_PI_INTSR(cpu_num);
		} else {
			pic_s[cpu_num].intmr = PI_INTMR;
			pic_s[cpu_num].intsr = PI_INTSR;
		}

		/* Mask and clear all interrupts. */
		WR4(pic_s[cpu_num].intmr, 0);
		WR4(pic_s[cpu_num].intsr, ~0U);
	}

	pic_add(&pic);
}
