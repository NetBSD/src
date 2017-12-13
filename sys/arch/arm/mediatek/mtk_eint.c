/* $NetBSD: mtk_eint.c,v 1.1.2.1 2017/12/13 01:06:02 matt Exp $ */

/*-
 * Copyright (c) 2017 Hongkun Cao  <hongun.cao@mediatek.com>
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
__KERNEL_RCSID(0, "$NetBSD: mtk_eint.c,v 1.1.2.1 2017/12/13 01:06:02 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <arm/mediatek/mercury_reg.h>
#include <dev/fdt/fdtvar.h>

#include "mtk_eint.h"
/*
 * There can be only one eint, so we keep a local pointer
 */
static struct mtk_eint_softc *g_sc;

static int	mtk_eint_match(device_t, cfdata_t, void *);
static void	mtk_eint_attach(device_t, device_t, void *);

static void	mtk_eint_set_sen(u_int eint_num, u_int level);
static void	mtk_eint_set_pol(u_int eint_num, u_int high);
static void	mtk_eint_intr_ack(u_int eint_num);
static int	mtk_eint_intr(void *);

static const char * const compatible[] = {
	"mediatek,mercury-eint",
	NULL
};

CFATTACH_DECL_NEW(mtk_eint, sizeof(struct mtk_eint_softc),
    mtk_eint_match, mtk_eint_attach, NULL, NULL);


#define EINT_BARR(sc) bus_space_barrier((sc)->sc_iot, (sc)->sc_ioh, 0, \
		(sc)->sc_size, BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)

static __inline uint32_t
mtk_eint_readl(bus_size_t offset)
{
	EINT_BARR(g_sc);
	return bus_space_read_4((g_sc)->sc_iot, (g_sc)->sc_ioh, offset);
}

static __inline void
mtk_eint_writel( bus_size_t offset, uint32_t val)
{
	EINT_BARR(g_sc);
	bus_space_write_4((g_sc)->sc_iot, (g_sc)->sc_ioh, offset, (val));
}

static int
mtk_eint_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mtk_eint_attach(device_t parent, device_t self, void *aux)
{
	int i = 0;
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	g_sc = device_private(self);
	g_sc->sc_dev = self;
	g_sc->sc_iot = faa->faa_bst;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(g_sc->sc_iot, addr, size, 0, &g_sc->sc_ioh) != 0) {
		aprint_error_dev(g_sc->sc_dev, "unable to map device\n");
		return;
	}

	//enable eint to domain0
	for (i = 0; i < EINT_COUNT; i += 32)
		mtk_eint_writel((i / 32) * 4 + EINT_REG_DOM_EN, 0xffffffff);

	/* Install our ISR. */
	memset(g_sc->sc_handlers, 0, sizeof(g_sc->sc_handlers));

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	g_sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_BIO,
		FDT_INTR_MPSAFE, mtk_eint_intr, g_sc);
	if (g_sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
			intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}


void *
mtk_eint_intr_establish(u_int eint_num, int level, const char *name,
	int (*func)(void *), void *arg)
{
	struct eint_irq_handler *gh = NULL;


	KDASSERT(eint_num < EINT_COUNT);

	if (g_sc->sc_handlers[eint_num] != NULL) {
		printf("mtk eint: Illegal shared interrupt on eint [%d]\n", eint_num);
		return gh;
	}

	gh = malloc(sizeof(struct eint_irq_handler), M_DEVBUF, M_NOWAIT);
	if (gh == NULL)
		return gh;
	memset(gh, 0, sizeof(struct eint_irq_handler));
	/* 1,config gpio dir:input, mode:gpio */


	/* 2,install isr */
	gh->gh_func = func;
	gh->gh_arg = arg;
	gh->gh_eint = eint_num;
	g_sc->sc_handlers[eint_num] = gh;
	snprintf(gh->name, EINT_EVNAMESZ, "#%d %s", eint_num, name);
	evcnt_attach_dynamic(&gh->ev, EVCNT_TYPE_INTR, NULL,
			     "mtk eint", gh->name);

	/* 3,Set up the level control. */
	mtk_eint_set_type(eint_num, level);

	 /* 4,enable eint */
	 mtk_eint_intr_ack(eint_num);
	 mtk_eint_intr_unmask(eint_num);

	return gh;
}

void
mtk_eint_intr_disestablish(void *cookie)
{
	struct eint_irq_handler *gh = cookie;
	u_int eint_num;

	KDASSERT(cookie != NULL);

	eint_num = gh->gh_eint;
	evcnt_detach(&gh->ev);

	/* disable eint */
	mtk_eint_intr_mask(eint_num);
	g_sc->sc_handlers[eint_num] = NULL;


	free(gh, M_DEVBUF);
}

static void
mtk_eint_set_sen(u_int eint_num, u_int level)
{
	u_int reg = (eint_num / 32) * 4;
	u_int bit = __BIT32(eint_num & 0x1f);

	if (level)
		mtk_eint_writel(EINT_REG_SENS_SET + reg, bit);
	else
		mtk_eint_writel(EINT_REG_SENS_CLR + reg, bit);
}

static void
mtk_eint_set_pol(u_int eint_num, u_int high)
{
	u_int reg = (eint_num / 32) * 4;
	u_int bit = __BIT32(eint_num & 0x1f);

	if(high)
		mtk_eint_writel(EINT_REG_POL_SET + reg, bit);
	else
		mtk_eint_writel(EINT_REG_POL_CLR + reg, bit);
}

void
mtk_eint_set_type(u_int eint_num, u_int level)
{
	switch (level) {
		case IST_EDGE_FALLING:
			mtk_eint_set_pol(eint_num, 0);
			mtk_eint_set_sen(eint_num, 0);
			break;
		case IST_LEVEL_LOW:
			mtk_eint_set_pol(eint_num, 0);
			mtk_eint_set_sen(eint_num, 1);
			break;
		case IST_EDGE_RISING:
			mtk_eint_set_pol(eint_num, 1);
			mtk_eint_set_sen(eint_num, 0);
			break;
		case IST_LEVEL_HIGH:
			mtk_eint_set_pol(eint_num, 1);
			mtk_eint_set_sen(eint_num, 1);
			break;
		case IST_EDGE_BOTH:
			panic("mtk eint: Not support level %d.", level);
			break;
		default:
			panic("mtk eint: Unknown level %d.", level);
			/* NOTREACHED */
	}
}

void
mtk_eint_intr_mask(u_int eint_num)
{
	u_int reg = (eint_num / 32) * 4;
	u_int bit = __BIT32(eint_num & 0x1f);

	mtk_eint_writel(EINT_REG_MASK_SET + reg, bit);
}

void
mtk_eint_intr_unmask(u_int eint_num)
{
	u_int reg = (eint_num / 32) * 4;
	u_int bit = __BIT32(eint_num & 0x1f);

	mtk_eint_writel(EINT_REG_MASK_CLR + reg, bit);
}

static void
mtk_eint_reset_debounce(u_int eint_num)
{
	unsigned int rst, ctrl_offset;

	ctrl_offset = (eint_num / 4) * 4;
	rst = EINT_DBNC_RST_BIT << ((eint_num % 4) * 8);
	mtk_eint_writel(ctrl_offset + EINT_REG_DBC_SET, rst);
}

int
mtk_eint_set_debounce(u_int eint_num, u_int us)
{
	u_int mask, sta, rst;
	u_int reg = (eint_num / 32) * 4;
	u_int bit = __BIT32(eint_num & 0x1f);
	u_int eint_offset, set_offset, clr_bit, clr_offset, i, dbnc;

	if (eint_num > EINT_SUPPORT_DEBOUNCE)
		return -ENOTSUP;

	eint_offset = (eint_num % 4) * 8;
	set_offset = (eint_num / 4) * 4;
	clr_offset = (eint_num / 4) * 4;
	dbnc = ARRAY_SIZE(debounce_time);
	for (i = 0; i < ARRAY_SIZE(debounce_time); i++) {
		if (us <= debounce_time[i]) {
			dbnc = i;
			break;
		}
	}

	mask = mtk_eint_readl(EINT_REG_MASK + reg) & bit;
	if (!mask){
		mask = 1;
		mtk_eint_intr_mask(eint_num);
	} else {
		mask = 0;
	}

	clr_bit = 0xff << eint_offset;
	mtk_eint_writel(clr_bit, clr_offset + EINT_REG_DBC_CLR);

	sta = ((dbnc << EINT_DBNC_SET_DBNC_BITS) | EINT_DBNC_SET_EN) << eint_offset;
	rst = EINT_DBNC_RST_BIT << eint_offset;
	mtk_eint_writel(EINT_REG_DBC_SET + set_offset, rst | sta);

	delay(200);
	if (mask == 1) {
		mtk_eint_intr_unmask(eint_num);
	}
	return 0;
}

static void
mtk_eint_intr_ack(u_int eint_num)
{
	u_int reg = (eint_num / 32) * 4;
	u_int bit = __BIT32(eint_num & 0x1f);

	mtk_eint_writel(EINT_REG_ACK + reg, bit);
}

void
mtk_eint_intr_wakeup(u_int eint_num, int enable)
{

}

static int
mtk_eint_intr(void *arg)
{
	struct eint_irq_handler *gh;
	u_int status, eint_num;
	int offset, index, handled = 0;
	bus_size_t reg = 0;

	for (eint_num = 0; eint_num < EINT_COUNT; eint_num += 32, reg += 4) {
		status = mtk_eint_readl(EINT_REG_STAT + reg);
		while (status) {
			offset = ffs32(status)- 1;
			index = eint_num + offset;
			status &= ~__BIT32(offset);

			mtk_eint_intr_mask(index);
			mtk_eint_intr_ack(index);
			if ((gh = g_sc->sc_handlers[index]) == NULL) {
				printf("%s: unhandled eint %d interrupt.\n",
				    device_xname(g_sc->sc_dev), index);
				continue;
			}
			gh->ev.ev_count++;
			handled |= (gh->gh_func)(gh->gh_arg);

			if (gh->dbc_enable) {
				mtk_eint_reset_debounce(eint_num);
			}
			mtk_eint_intr_unmask(index);
		}
	}

	return (handled);
}

