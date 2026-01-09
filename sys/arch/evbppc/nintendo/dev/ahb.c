/* $NetBSD: ahb.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ahb.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bitops.h>

#include <machine/intr.h>
#include <machine/wii.h>
#include <machine/wiiu.h>
#include <machine/pio.h>
#include <powerpc/pic/picvar.h>

#include "locators.h"
#include "mainbus.h"
#include "ahb.h"

#define WR4(reg, val)		out32(reg, val)
#define RD4(reg)		in32(reg)

static struct mainbus_attach_args ahb_maa;
static uint32_t pic_irqmask[2];

static int	ahb_match(device_t, cfdata_t, void *);
static void	ahb_attach(device_t, device_t, void *);

static int	ahb_search(device_t, cfdata_t, const int *, void *);
static int	ahb_print(void *, const char *);

static void	ahb_intr_init(int);

static void	hollywood_enable_irq(struct pic_ops *, int, int);
static void	hollywood_disable_irq(struct pic_ops *, int);
static int	hollywood_get_irq(struct pic_ops *, int);
static void	hollywood_ack_irq(struct pic_ops *, int);
static void	hollywood_establish_irq(struct pic_ops *, int, int, int);

static struct pic_ops hollywood_pic = {
	.pic_name = "hollywood",
	.pic_numintrs = 32,
	.pic_cookie = NULL,
	.pic_enable_irq = hollywood_enable_irq,
	.pic_reenable_irq = hollywood_enable_irq,
	.pic_disable_irq = hollywood_disable_irq,
	.pic_get_irq = hollywood_get_irq,
	.pic_ack_irq = hollywood_ack_irq,
	.pic_establish_irq = hollywood_establish_irq,
};

static void	latte_enable_irq(struct pic_ops *, int, int);
static void	latte_disable_irq(struct pic_ops *, int);
static int	latte_get_irq(struct pic_ops *, int);
static void	latte_ack_irq(struct pic_ops *, int);
static void	latte_establish_irq(struct pic_ops *, int, int, int);

static struct pic_ops latte_pic = {
	.pic_name = "latte",
	.pic_numintrs = 64,
	.pic_cookie = NULL,
	.pic_enable_irq = latte_enable_irq,
	.pic_reenable_irq = latte_enable_irq,
	.pic_disable_irq = latte_disable_irq,
	.pic_get_irq = latte_get_irq,
	.pic_ack_irq = latte_ack_irq,
	.pic_establish_irq = latte_establish_irq,
};

CFATTACH_DECL_NEW(ahb, 0, ahb_match, ahb_attach, NULL, NULL);

static int
ahb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return strcmp(maa->maa_name, "ahb") == 0;
}

static void
ahb_attach(device_t parent, device_t self, void *aux)
{
	uint32_t val;

	ahb_maa = *(struct mainbus_attach_args *)aux;

	aprint_naive("\n");
	if (wiiu_native) {
		val = RD4(LT_CHIPREVID);

		aprint_normal(": Latte 0x%02x\n",
		    (unsigned)(val & (LT_CHIPREVID_VERHI | LT_CHIPREVID_VERLO)));
	} else {
		val = RD4(HW_VERSION);

		aprint_normal(": Hollywood ES%u.%u\n",
		    (unsigned)__SHIFTOUT(val, HWVER_MASK) + 1,
		    (unsigned)__SHIFTOUT(val, HWREV_MASK));
	}

	ahb_intr_init(ahb_maa.maa_irq);

	config_search(self, NULL,
	    CFARGS(.search = ahb_search));
}

static int
ahb_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct ahb_attach_args aaa;

	aaa.aaa_bst = ahb_maa.maa_bst;
	aaa.aaa_dmat = ahb_maa.maa_dmat;
	if (cf->cf_loc[AHBCF_ADDR] != AHBCF_ADDR_DEFAULT) {
		aaa.aaa_addr = cf->cf_loc[AHBCF_ADDR];
	} else {
		aaa.aaa_addr = 0;
	}
	aaa.aaa_irq = cf->cf_loc[AHBCF_IRQ];

	if (config_probe(parent, cf, &aaa)) {
		config_attach(parent, cf, &aaa, ahb_print, CFARGS_NONE);
	}

	return 0;
}

static int
ahb_print(void *aux, const char *pnp)
{
	struct ahb_attach_args *aaa = aux;

	if (pnp != NULL) {
		return QUIET;
	}

	if (aaa->aaa_addr != 0) {
		aprint_normal(" addr 0x%08lx", aaa->aaa_addr);
	}
	if (aaa->aaa_irq != AHBCF_IRQ_DEFAULT) {
		aprint_normal(" irq %d", aaa->aaa_irq);
	}

	return UNCONF;
}

static void
hollywood_enable_irq(struct pic_ops *pic, int irq, int type)
{
	pic_irqmask[0] |= __BIT(irq);
	WR4(HW_PPCIRQMASK, pic_irqmask[0]);
}

static void
hollywood_disable_irq(struct pic_ops *pic, int irq)
{
	pic_irqmask[0] &= ~__BIT(irq);
	WR4(HW_PPCIRQMASK, pic_irqmask[0]);
}

static int
hollywood_get_irq(struct pic_ops *pic, int mode)
{
	uint32_t raw, pend;
	int irq;

	raw = RD4(HW_PPCIRQFLAGS);
	pend = raw & pic_irqmask[0];
	if (pend == 0) {
		return 255;
	}
	irq = ffs32(pend) - 1;

	return irq;
}

static void
hollywood_ack_irq(struct pic_ops *pic, int irq)
{
	WR4(HW_PPCIRQFLAGS, __BIT(irq));
}

static void
hollywood_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	uint32_t val;

	/* Mask and clear Starlet interrupt */
	val = RD4(HW_ARMIRQMASK);
	val &= ~__BIT(irq);
	WR4(HW_ARMIRQMASK, val);
	WR4(HW_ARMIRQFLAGS, __BIT(irq));
}

static void
latte_enable_irq(struct pic_ops *pic, int irq, int type)
{
	unsigned reg_en = irq < 32 ? LT_PPCnINT1EN(0) : LT_PPCnINT2EN(0);

	pic_irqmask[irq / 32] |= __BIT(irq % 32);
	WR4(reg_en, pic_irqmask[irq / 32]);
}

static void
latte_disable_irq(struct pic_ops *pic, int irq)
{
	unsigned reg_en = irq < 32 ? LT_PPCnINT1EN(0) : LT_PPCnINT2EN(0);

	pic_irqmask[irq / 32] &= ~__BIT(irq % 32);
	WR4(reg_en, pic_irqmask[irq / 32]);
}

static int
latte_get_irq(struct pic_ops *pic, int mode)
{
	uint32_t raw, pend;
	int irq;

	raw = RD4(LT_PPCnINT1STS(0));
	pend = raw & pic_irqmask[0];
	if (pend == 0) {
		raw = RD4(LT_PPCnINT2STS(0));
		pend = raw & pic_irqmask[1];
		if (pend == 0) {
			return 255;
		}
		irq = 32 + ffs32(pend) - 1;
	} else {
		irq = ffs32(pend) - 1;
	}

	return irq;
}

static void
latte_ack_irq(struct pic_ops *pic, int irq)
{
	unsigned reg_sts = irq < 32 ? LT_PPCnINT1STS(0) : LT_PPCnINT2STS(0);

	WR4(reg_sts, __BIT(irq % 32));
}

static void
latte_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	unsigned reg_en = irq < 32 ? LT_IOPIRQINT1EN : LT_IOPIRQINT2EN;
	unsigned reg_sts = irq < 32 ? LT_IOPINT1STS : LT_IOPINT2STS;
	uint32_t val;

	/* Mask and clear IOP interrupt */
	val = RD4(reg_en);
	val &= ~__BIT(irq % 32);
	WR4(reg_en, val);
	WR4(reg_sts, __BIT(irq % 32));
}


static void
ahb_intr_init(int irq)
{
	struct pic_ops *pic_impl;
	memset(pic_irqmask, 0, sizeof(pic_irqmask));

	/* Mask and clear all interrupts. */
	if (wiiu_native) {
		WR4(LT_PPCnINT1EN(0), 0);
		WR4(LT_PPCnINT2EN(0), 0);
		WR4(LT_PPCnINT1STS(0), ~0U);
		WR4(LT_PPCnINT2STS(0), ~0U);

		pic_impl = &latte_pic;
	} else {
		WR4(HW_PPCIRQMASK, 0);
		WR4(HW_PPCIRQFLAGS, ~0U);

		pic_impl = &hollywood_pic;
	}

	pic_add(pic_impl);
	intr_establish_xname(irq, IST_LEVEL, IPL_SCHED, pic_handle_intr,
	    pic_impl, pic_impl->pic_name);
}

void *
ahb_intr_establish(int irq, int ipl, int (*func)(void *), void *arg,
    const char *name)
{
	if (wiiu_native) {
		KASSERT(latte_pic.pic_intrbase != 0);

		return intr_establish_xname(latte_pic.pic_intrbase + irq,
		    IST_LEVEL, ipl, func, arg, name);
	} else {
		KASSERT(hollywood_pic.pic_intrbase != 0);

		return intr_establish_xname(hollywood_pic.pic_intrbase + irq,
		    IST_LEVEL, ipl, func, arg, name);
	}
}

void
ahb_claim_device(device_t dev, uint32_t mask)
{
	if (!wiiu_native) {
		/*
		 * Restrict IOP access to a device, giving exclusive access
		 * to PPC.
		 */
		WR4(HW_AHBPROT, RD4(HW_AHBPROT) & ~mask);
	}
}
