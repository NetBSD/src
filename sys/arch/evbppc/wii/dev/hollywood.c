/* $NetBSD: hollywood.c,v 1.2.2.2 2024/02/03 11:47:04 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: hollywood.c,v 1.2.2.2 2024/02/03 11:47:04 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bitops.h>

#include <machine/intr.h>
#include <machine/wii.h>
#include <machine/pio.h>
#include <powerpc/pic/picvar.h>

#include "locators.h"
#include "mainbus.h"
#include "hollywood.h"

#define WR4(reg, val)		out32(reg, val)
#define RD4(reg)		in32(reg)

static struct mainbus_attach_args hollywood_maa;
static uint32_t pic_irqmask;

static int	hollywood_match(device_t, cfdata_t, void *);
static void	hollywood_attach(device_t, device_t, void *);

static int	hollywood_search(device_t, cfdata_t, const int *, void *);
static int	hollywood_print(void *, const char *);

static void	hollywood_intr_init(int);
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

CFATTACH_DECL_NEW(hollywood, 0,
	hollywood_match, hollywood_attach, NULL, NULL);

static int
hollywood_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return strcmp(maa->maa_name, "hollywood") == 0;
}

static void
hollywood_attach(device_t parent, device_t self, void *aux)
{
	uint32_t val;

	hollywood_maa = *(struct mainbus_attach_args *)aux;

	val = RD4(HW_VERSION);

	aprint_naive("\n");
	aprint_normal(": Hollywood ES%u.%u\n",
	    (unsigned)__SHIFTOUT(val, HWVER_MASK) + 1,
	    (unsigned)__SHIFTOUT(val, HWREV_MASK));

	hollywood_intr_init(hollywood_maa.maa_irq);

	config_search(self, NULL,
	    CFARGS(.search = hollywood_search));
}

static int
hollywood_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct hollywood_attach_args haa;

	haa.haa_bst = hollywood_maa.maa_bst;
	haa.haa_dmat = hollywood_maa.maa_dmat;
	if (cf->cf_loc[HOLLYWOODCF_ADDR] != HOLLYWOODCF_ADDR_DEFAULT) {
		haa.haa_addr = cf->cf_loc[HOLLYWOODCF_ADDR];
	} else {
		haa.haa_addr = 0;
	}
	haa.haa_irq = cf->cf_loc[HOLLYWOODCF_IRQ];

	if (config_probe(parent, cf, &haa)) {
		config_attach(parent, cf, &haa, hollywood_print,
		    CFARGS_NONE);
	}

	return 0;
}

static int
hollywood_print(void *aux, const char *pnp)
{
	struct hollywood_attach_args *haa = aux;

	if (pnp != NULL) {
		return QUIET;
	}

	if (haa->haa_addr != 0) {
		aprint_normal(" addr 0x%lx", haa->haa_addr);
	}
	if (haa->haa_irq != HOLLYWOODCF_IRQ_DEFAULT) {
		aprint_normal(" irq %d", haa->haa_irq);
	}

	return UNCONF;
}

static void
hollywood_enable_irq(struct pic_ops *pic, int irq, int type)
{
	pic_irqmask |= __BIT(irq);
	WR4(HW_PPCIRQMASK, pic_irqmask);
}

static void
hollywood_disable_irq(struct pic_ops *pic, int irq)
{
	pic_irqmask &= ~__BIT(irq);
	WR4(HW_PPCIRQMASK, pic_irqmask);
}

static int
hollywood_get_irq(struct pic_ops *pic, int mode)
{
	uint32_t raw, pend;
	int irq;

	raw = RD4(HW_PPCIRQFLAGS);
	pend = raw & pic_irqmask;
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
hollywood_intr_init(int irq)
{
	pic_irqmask = 0;

	/* Mask and clear all interrupts. */
	WR4(HW_PPCIRQMASK, 0);
	WR4(HW_PPCIRQFLAGS, ~0U);

	pic_add(&hollywood_pic);

	intr_establish_xname(irq, IST_LEVEL, IPL_SCHED, pic_handle_intr,
	    &hollywood_pic, "hollywood0");
}

void *
hollywood_intr_establish(int irq, int ipl, int (*func)(void *), void *arg,
    const char *name)
{
	KASSERT(hollywood_pic.pic_intrbase != 0);

	return intr_establish_xname(hollywood_pic.pic_intrbase + irq,
	    IST_LEVEL, ipl, func, arg, name);
}
