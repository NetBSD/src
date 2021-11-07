/* $NetBSD: sunxi_nmi.c,v 1.12 2021/11/07 17:13:38 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_nmi.c,v 1.12 2021/11/07 17:13:38 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/lwp.h>

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

static const struct sunxi_nmi_config sun9i_a80_nmi_config = {
	.name = "NMI",
	.ctrl_reg = 0x00,
	.pend_reg = 0x04,
	.enable_reg = 0x08,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun7i-a20-sc-nmi",
	  .data = &sun7i_a20_sc_nmi_config },
	{ .compat = "allwinner,sun6i-a31-r-intc",
	  .data = &sun6i_a31_r_intc_config },
	{ .compat = "allwinner,sun9i-a80-nmi",
	  .data = &sun9i_a80_nmi_config },

	DEVICE_COMPAT_EOL
};

struct sunxi_nmi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;

	u_int sc_intr_nmi;
	u_int sc_intr_cells;

	kmutex_t sc_intr_lock;

	const struct sunxi_nmi_config *sc_config;

	struct intrsource sc_is;
	void	*sc_ih;
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
	int (*func)(void *);
	int rv = 0;

	func = atomic_load_acquire(&sc->sc_is.is_func);
	if (func)
		rv = func(sc->sc_is.is_arg);

	/*
	 * We don't serialize access to this register because we're the
	 * only thing fiddling wth it.
	 */
	sunxi_nmi_irq_ack(sc);

	return rv;
}

static void *
sunxi_nmi_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);
	u_int irq_type, irq, pol;
	int ist;

	if (sc->sc_intr_cells == 2) {
		/* 1st cell is the interrupt number */
		irq = be32toh(specifier[0]);
		/* 2nd cell is polarity */
		pol = be32toh(specifier[1]);
	} else {
		/* 1st cell is the GIC interrupt type and must be GIC_SPI */
		if (be32toh(specifier[0]) != 0) {
#ifdef DIAGNOSTIC
			device_printf(dev, "GIC intr type %u is invalid\n",
			    be32toh(specifier[0]));
#endif
			return NULL;
		}
		/* 2nd cell is the interrupt number */
		irq = be32toh(specifier[1]);
		/* 3rd cell is polarity */
		pol = be32toh(specifier[2]);
	}

	if (sc->sc_intr_cells == 3 && irq != sc->sc_intr_nmi) {
		/*
		 * Driver is requesting a wakeup irq, which we don't
		 * support today. Just pass it through to the parent
		 * interrupt controller.
		 */
		const int ihandle = fdtbus_intr_parent(sc->sc_phandle);
		if (ihandle == -1) {
#ifdef DIAGNOSTIC
			device_printf(dev, "couldn't find interrupt parent\n");
#endif
			return NULL;
		}
		return fdtbus_intr_establish_raw(ihandle, specifier, ipl,
		    flags, func, arg, xname);
	}

	if (sc->sc_intr_cells == 2 && irq != 0) {
#ifdef DIAGNOSTIC
		device_printf(dev, "IRQ %u is invalid\n", irq);
#endif
		return NULL;
	}

	switch (pol & 0xf) {
	case 1:	/* IRQ_TYPE_EDGE_RISING */
		irq_type = NMI_CTRL_IRQ_HIGH_EDGE;
		ist = IST_EDGE;
		break;
	case 2:	/* IRQ_TYPE_EDGE_FALLING */
		irq_type = NMI_CTRL_IRQ_LOW_EDGE;
		ist = IST_EDGE;
		break;
	case 4:	/* IRQ_TYPE_LEVEL_HIGH */
		irq_type = NMI_CTRL_IRQ_HIGH_LEVEL;
		ist = IST_LEVEL;
		break;
	case 8:	/* IRQ_TYPE_LEVEL_LOW */
		irq_type = NMI_CTRL_IRQ_LOW_LEVEL;
		ist = IST_LEVEL;
		break;
	default:
		irq_type = NMI_CTRL_IRQ_LOW_LEVEL;
		ist = IST_LEVEL;
		break;
	}

	mutex_enter(&sc->sc_intr_lock);

	if (atomic_load_relaxed(&sc->sc_is.is_func) != NULL) {
		mutex_exit(&sc->sc_intr_lock);
#ifdef DIAGNOSTIC
		device_printf(dev, "%s in use\n", sc->sc_config->name);
#endif
		return NULL;
	}

	sc->sc_is.is_arg = arg;
	atomic_store_release(&sc->sc_is.is_func, func);

	sc->sc_is.is_type = ist;
	sc->sc_is.is_ipl = ipl;
	sc->sc_is.is_mpsafe = (flags & FDT_INTR_MPSAFE) ? true : false;

	mutex_exit(&sc->sc_intr_lock);

	sc->sc_ih = fdtbus_intr_establish_xname(sc->sc_phandle, 0, ipl, flags,
	    sunxi_nmi_intr, sc, device_xname(dev));

	mutex_enter(&sc->sc_intr_lock);
	sunxi_nmi_irq_set_type(sc, irq_type);
	sunxi_nmi_irq_enable(sc, true);
	mutex_exit(&sc->sc_intr_lock);

	return &sc->sc_is;
}

static void
sunxi_nmi_fdt_mask(device_t dev, void *ih __unused)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_is.is_mask_count++ == 0) {
		sunxi_nmi_irq_enable(sc, false);
	}
	mutex_exit(&sc->sc_intr_lock);
}

static void
sunxi_nmi_fdt_unmask(device_t dev, void *ih __unused)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_is.is_mask_count-- == 1) {
		sunxi_nmi_irq_enable(sc, true);
	}
	mutex_exit(&sc->sc_intr_lock);
}

static void
sunxi_nmi_fdt_disestablish(device_t dev, void *ih)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);
	struct intrsource * const is = ih;

	KASSERT(is == &sc->sc_is);

	mutex_enter(&sc->sc_intr_lock);
	sunxi_nmi_irq_enable(sc, false);
	is->is_mask_count = 0;
	mutex_exit(&sc->sc_intr_lock);

	fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
	sc->sc_ih = NULL;

	mutex_enter(&sc->sc_intr_lock);
	is->is_arg = NULL;
	is->is_func = NULL;
	mutex_exit(&sc->sc_intr_lock);
}

static bool
sunxi_nmi_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct sunxi_nmi_softc * const sc = device_private(dev);

	if (sc->sc_intr_cells == 3) {
		const u_int irq = be32toh(specifier[1]);
		if (irq != sc->sc_intr_nmi) {
			const int ihandle = fdtbus_intr_parent(sc->sc_phandle);
			if (ihandle == -1) {
				return false;
			}
			return fdtbus_intr_str_raw(ihandle, specifier, buf,
			    buflen);
		}
	}

	snprintf(buf, buflen, "%s", sc->sc_config->name);

	return true;
}

static const struct fdtbus_interrupt_controller_func sunxi_nmi_fdt_funcs = {
	.establish = sunxi_nmi_fdt_establish,
	.disestablish = sunxi_nmi_fdt_disestablish,
	.intrstr = sunxi_nmi_fdt_intrstr,
	.mask = sunxi_nmi_fdt_mask,
	.unmask = sunxi_nmi_fdt_unmask,
};

static int
sunxi_nmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_nmi_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_nmi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const u_int *interrupts;
	bus_addr_t addr;
	bus_size_t size;
	int error, len;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_config = of_compatible_lookup(phandle, compat_data)->data;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	of_getprop_uint32(phandle, "#interrupt-cells", &sc->sc_intr_cells);
	interrupts = fdtbus_get_prop(phandle, "interrupts", &len);
	if (interrupts == NULL || len != 12 ||
	    be32toh(interrupts[0]) != 0 /* GIC_SPI */ ||
	    be32toh(interrupts[2]) != 4 /* IRQ_TYPE_LEVEL_HIGH */) {
		aprint_error(": couldn't find GIC SPI for NMI\n");
		return;
	}
	sc->sc_intr_nmi = be32toh(interrupts[1]);

	aprint_naive("\n");
	aprint_normal(": %s, NMI IRQ %u\n", sc->sc_config->name, sc->sc_intr_nmi);

	mutex_init(&sc->sc_intr_lock, MUTEX_SPIN, IPL_HIGH);

	/*
	 * Normally it's assumed that an intrsource can be passed to
	 * interrupt_distribute().  We're providing our own that's
	 * independent of our parent PIC, but because we will leave
	 * the intrsource::is_pic field NULL, the right thing
	 * (i.e. nothing) will happen in interrupt_distribute().
	 */
	snprintf(sc->sc_is.is_source, sizeof(sc->sc_is.is_source),
		 "%s", sc->sc_config->name);

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
