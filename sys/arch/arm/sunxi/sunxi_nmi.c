/* $NetBSD: sunxi_nmi.c,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_nmi.c,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $");

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

/* ctrl_reg */
#define	NMI_CTRL_IRQ_LOW_LEVEL	0
#define	NMI_CTRL_IRQ_LOW_EDGE	1
#define	NMI_CTRL_IRQ_HIGH_LEVEL	2
#define	NMI_CTRL_IRQ_HIGH_EDGE	3
#define	NMI_CTRL_IRQ_TYPE	__BITS(1,0)

/* pend_reg */
#define	NMI_PEND_IRQ_ACK	__BIT(0)

/* enable_reg */
#define	NMI_ENABLE_IRQEN	__BIT(0)

struct sunxi_nmi_config {
	const char *	name;
	bus_size_t	ctrl_reg;
	bus_size_t	pend_reg;
	bus_size_t	enable_reg;
};

static const struct sunxi_nmi_config sun7i_a20_sc_nmi_config = {
	.name = "NMI",
	.ctrl_reg = 0x00,
	.pend_reg = 0x04,
	.enable_reg = 0x08,
};

static const struct sunxi_nmi_config sun6i_a31_r_intc_config = {
	.name = "R_INTC",
	.ctrl_reg = 0x0c,
	.pend_reg = 0x10,
	.enable_reg = 0x40,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun7i-a20-sc-nmi",	(uintptr_t)&sun7i_a20_sc_nmi_config },
	{ "allwinner,sun6i-a31-r-intc",	(uintptr_t)&sun6i_a31_r_intc_config },
	{ NULL }
};

struct sunxi_nmi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;

	const struct sunxi_nmi_config *sc_config;

	int (*sc_func)(void *);
	void *sc_arg;
};

#define NMI_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define NMI_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
sunxi_nmi_irq_ack(struct sunxi_nmi_softc *sc)
{
	uint32_t val;

	val = NMI_READ(sc, sc->sc_config->pend_reg);
	val |= NMI_PEND_IRQ_ACK;
	NMI_WRITE(sc, sc->sc_config->pend_reg, val);
}

static void
sunxi_nmi_irq_enable(struct sunxi_nmi_softc *sc, bool on)
{
	uint32_t val;

	val = NMI_READ(sc, sc->sc_config->enable_reg);
	if (on)
		val |= NMI_ENABLE_IRQEN;
	else
		val &= ~NMI_ENABLE_IRQEN;
	NMI_WRITE(sc, sc->sc_config->enable_reg, val);
}

static void
sunxi_nmi_irq_set_type(struct sunxi_nmi_softc *sc, u_int irq_type)
{
	uint32_t val;

	val = NMI_READ(sc, sc->sc_config->ctrl_reg);
	val &= ~NMI_CTRL_IRQ_TYPE;
	val |= __SHIFTIN(irq_type, NMI_CTRL_IRQ_TYPE);
	NMI_WRITE(sc, sc->sc_config->ctrl_reg, val);
}

static int
sunxi_nmi_intr(void *priv)
{
	struct sunxi_nmi_softc * const sc = priv;
	int rv = 0;

	if (sc->sc_func)
		rv = sc->sc_func(sc->sc_arg);

	sunxi_nmi_irq_ack(sc);

	return rv;
}

static void *
sunxi_nmi_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);
	u_int irq_type;

	/* 1st cell is the interrupt number */
	const u_int irq = be32toh(specifier[0]);
	/* 2nd cell is polarity */
	const u_int pol = be32toh(specifier[1]);

	if (sc->sc_func != NULL) {
#ifdef DIAGNOSTIC
		device_printf(dev, "%s in use\n", sc->sc_config->name);
#endif
		return NULL;
	}

	if (irq != 0) {
#ifdef DIAGNOSTIC
		device_printf(dev, "IRQ %u is invalid\n", irq);
#endif
		return NULL;
	}

	switch (pol & 0x7) {
	case 1:	/* IRQ_TYPE_EDGE_RISING */
		irq_type = NMI_CTRL_IRQ_HIGH_EDGE;
		break;
	case 2:	/* IRQ_TYPE_EDGE_FALLING */
		irq_type = NMI_CTRL_IRQ_LOW_EDGE;
		break;
	case 3:	/* IRQ_TYPE_LEVEL_HIGH */
		irq_type = NMI_CTRL_IRQ_HIGH_LEVEL;
		break;
	case 4:	/* IRQ_TYPE_LEVEL_LOW */
		irq_type = NMI_CTRL_IRQ_LOW_LEVEL;
		break;
	default:
		irq_type = NMI_CTRL_IRQ_LOW_LEVEL;
		break;
	}

	sc->sc_func = func;
	sc->sc_arg = arg;

	sunxi_nmi_irq_set_type(sc, irq_type);
	sunxi_nmi_irq_enable(sc, true);

	return fdtbus_intr_establish(sc->sc_phandle, 0, ipl, flags,
	    sunxi_nmi_intr, sc);
}

static void
sunxi_nmi_fdt_disestablish(device_t dev, void *ih)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);

	sunxi_nmi_irq_enable(sc, false);

	fdtbus_intr_disestablish(sc->sc_phandle, ih);

	sc->sc_func = NULL;
	sc->sc_arg = NULL;
}

static bool
sunxi_nmi_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);

	snprintf(buf, buflen, "%s", sc->sc_config->name);

	return true;
}

static const struct fdtbus_interrupt_controller_func sunxi_nmi_fdt_funcs = {
	.establish = sunxi_nmi_fdt_establish,
	.disestablish = sunxi_nmi_fdt_disestablish,
	.intrstr = sunxi_nmi_fdt_intrstr,
};

static int
sunxi_nmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_nmi_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_nmi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_config = (void *)of_search_compatible(phandle, compat_data)->data;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_config->name);

	sunxi_nmi_irq_enable(sc, false);
	sunxi_nmi_irq_ack(sc);

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &sunxi_nmi_fdt_funcs);
	if (error) {
		aprint_error_dev(self, "couldn't register with fdtbus: %d\n",
		    error);
		return;
	}
}

CFATTACH_DECL_NEW(sunxi_nmi, sizeof(struct sunxi_nmi_softc),
	sunxi_nmi_match, sunxi_nmi_attach, NULL, NULL);
