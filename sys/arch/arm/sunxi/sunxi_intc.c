/* $NetBSD: sunxi_intc.c,v 1.3 2017/10/24 15:07:09 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_intc.c,v 1.3 2017/10/24 15:07:09 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cpu.h>
#include <arm/pic/picvar.h>
#include <arm/fdt/arm_fdtvar.h>

#define	INTC_MAX_SOURCES	96
#define	INTC_MAX_GROUPS		3

#define	INTC_VECTOR_REG		0x00
#define	INTC_BASE_ADDR_REG	0x04
#define	INTC_PROTECT_REG	0x08
#define	 INTC_PROTECT_EN	__BIT(0)
#define	INTC_NMII_CTRL_REG	0x0c
#define	INTC_IRQ_PEND_REG(n)	(0x10 + ((n) * 4))
#define	INTC_FIQ_PEND_REG(n)	(0x20 + ((n) * 4))
#define	INTC_SEL_REG(n)		(0x30 + ((n) * 4))
#define	INTC_EN_REG(n)		(0x40 + ((n) * 4))
#define	INTC_MASK_REG(n)	(0x50 + ((n) * 4))
#define	INTC_RESP_REG(n)	(0x60 + ((n) * 4))
#define	INTC_FORCE_REG(n)	(0x70 + ((n) * 4))
#define	INTC_SRC_PRIO_REG(n)	(0x80 + ((n) * 4))

static const char * const compatible[] = {
	"allwinner,sun4i-a10-ic",
	NULL
};

struct sunxi_intc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;

	uint32_t sc_enabled_irqs[INTC_MAX_GROUPS];

	struct pic_softc sc_pic;
};

static struct sunxi_intc_softc *intc_softc;

#define	PICTOSOFTC(pic)	\
	((void *)((uintptr_t)(pic) - offsetof(struct sunxi_intc_softc, sc_pic)))

#define INTC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define INTC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
sunxi_intc_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct sunxi_intc_softc * const sc = PICTOSOFTC(pic);
	const u_int group = irqbase / 32;

	KASSERT((mask & sc->sc_enabled_irqs[group]) == 0);
	sc->sc_enabled_irqs[group] |= mask;
	INTC_WRITE(sc, INTC_EN_REG(group), sc->sc_enabled_irqs[group]);
	INTC_WRITE(sc, INTC_MASK_REG(group), ~sc->sc_enabled_irqs[group]);
}

static void
sunxi_intc_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t mask)
{
	struct sunxi_intc_softc * const sc = PICTOSOFTC(pic);
	const u_int group = irqbase / 32;

	sc->sc_enabled_irqs[group] &= ~mask;
	INTC_WRITE(sc, INTC_EN_REG(group), sc->sc_enabled_irqs[group]);
	INTC_WRITE(sc, INTC_MASK_REG(group), ~sc->sc_enabled_irqs[group]);
}

static void
sunxi_intc_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	KASSERT(is->is_irq < INTC_MAX_SOURCES);
	KASSERT(is->is_type == IST_LEVEL);
}

static void
sunxi_intc_set_priority(struct pic_softc *pic, int ipl)
{
}

static const struct pic_ops sunxi_intc_picops = {
	.pic_unblock_irqs = sunxi_intc_unblock_irqs,
	.pic_block_irqs = sunxi_intc_block_irqs,
	.pic_establish_irq = sunxi_intc_establish_irq,
	.pic_set_priority = sunxi_intc_set_priority,
};

static void *
sunxi_intc_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	/* 1st cell is the interrupt number */
	const u_int irq = be32toh(specifier[0]);

	if (irq >= INTC_MAX_SOURCES) {
#ifdef DIAGNOSTIC
		device_printf(dev, "IRQ %u is invalid\n", irq);
#endif
		return NULL;
	}

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	return intr_establish(irq, ipl, IST_LEVEL | mpsafe, func, arg);
}

static void
sunxi_intc_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
sunxi_intc_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	/* 1st cell is the interrupt number */
	if (!specifier)
		return false;
	const u_int irq = be32toh(specifier[0]);

	snprintf(buf, buflen, "INTC irq %d", irq);

	return true;
}

static const struct fdtbus_interrupt_controller_func sunxi_intc_fdt_funcs = {
	.establish = sunxi_intc_fdt_establish,
	.disestablish = sunxi_intc_fdt_disestablish,
	.intrstr = sunxi_intc_fdt_intrstr,
};

static int
sunxi_intc_find_pending_irqs(struct sunxi_intc_softc *sc, u_int group)
{
	uint32_t pend;

	pend = INTC_READ(sc, INTC_IRQ_PEND_REG(group));
	pend &= sc->sc_enabled_irqs[group];

	if (pend == 0)
		return 0;

	INTC_WRITE(sc, INTC_IRQ_PEND_REG(group), pend);

	return pic_mark_pending_sources(&sc->sc_pic, group * 32, pend);
}

static void
sunxi_intc_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct sunxi_intc_softc * const sc = intc_softc;
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	ci->ci_data.cpu_nintr++;

	if (sc->sc_enabled_irqs[0])
		ipl_mask |= sunxi_intc_find_pending_irqs(sc, 0);
	if (sc->sc_enabled_irqs[1])
		ipl_mask |= sunxi_intc_find_pending_irqs(sc, 1);
	if (sc->sc_enabled_irqs[2])
		ipl_mask |= sunxi_intc_find_pending_irqs(sc, 2);

	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

static int
sunxi_intc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_intc_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_intc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error, i;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Interrupt Controller\n");

	/* Disable IRQs */
	for (i = 0; i < INTC_MAX_GROUPS; i++) {
		INTC_WRITE(sc, INTC_EN_REG(i), 0);
		INTC_WRITE(sc, INTC_MASK_REG(i), ~0U);
		INTC_WRITE(sc, INTC_IRQ_PEND_REG(i),
		    INTC_READ(sc, INTC_IRQ_PEND_REG(i)));
	}
	/* Disable user mode access to intc registers */
	INTC_WRITE(sc, INTC_PROTECT_REG, INTC_PROTECT_EN);

	sc->sc_pic.pic_ops = &sunxi_intc_picops;
	sc->sc_pic.pic_maxsources = INTC_MAX_SOURCES;
	snprintf(sc->sc_pic.pic_name, sizeof(sc->sc_pic.pic_name), "intc");
	pic_add(&sc->sc_pic, 0);

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &sunxi_intc_fdt_funcs);
	if (error) {
		aprint_error_dev(self, "couldn't register with fdtbus: %d\n",
		    error);
		return;
	}

	KASSERT(intc_softc == NULL);
	intc_softc = sc;
	arm_fdt_irq_set_handler(sunxi_intc_irq_handler);
}

CFATTACH_DECL_NEW(sunxi_intc, sizeof(struct sunxi_intc_softc),
	sunxi_intc_match, sunxi_intc_attach, NULL, NULL);
