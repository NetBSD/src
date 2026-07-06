/* $NetBSD $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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
 * Drivers for the PLL of the USB PHYs on the TI AM18XX family of SoCs.
 *
 * Locking Strategy: Any access to the hardware and sc->sc_state should be made
 * with sc->sc_lock held.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/clk/clk_backend.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

enum usbphyclk_state {
	SUSPENDED,
	USB11_ONLY, /* USB1.1 only */
	USB20_ONLY, /* USB2.0 only */
	BOTH        /* USB2.0 and USB1.1 */
};

struct am18xx_usbphyclk_clk {
	struct clk clk_base; /* must be first */
};

struct am18xx_usbphyclk_softc {
	struct clk_domain sc_clkdom;
	struct am18xx_usbphyclk_clk sc_usb11_clk;
	struct am18xx_usbphyclk_clk sc_usb20_clk;
	kmutex_t sc_lock;
	struct clk *sc_ref_clk;
	struct clk *sc_phy_logic_clk;
	struct syscon *sc_syscon;
	enum usbphyclk_state sc_state;
};

static int am18xx_usbphyclk_match(device_t, cfdata_t, void *);
static void am18xx_usbphyclk_attach(device_t, device_t, void *);
static struct clk *am18xx_usbphyclk_decode(device_t, int, const void *, size_t);
static struct clk *am18xx_usbphyclk_clk_get(void *, const char *);
static u_int am18xx_usbphyclk_clk_get_rate(void *, struct clk *);
static struct clk *am18xx_usbphyclk_clk_get_parent(void *, struct clk *);
static int am18xx_usbphyclk_clk_enable(void *, struct clk *);
static int am18xx_usbphyclk_clk_disable(void *, struct clk *);
static int am18xx_usbphyclk_state_transition(struct am18xx_usbphyclk_softc *,
    enum usbphyclk_state);
static int am18xx_usbphyclk_set_refreq(struct am18xx_usbphyclk_softc *);

CFATTACH_DECL_NEW(am18xxuphyclk, sizeof(struct am18xx_usbphyclk_softc),
    am18xx_usbphyclk_match, am18xx_usbphyclk_attach, NULL, NULL);

static const struct fdtbus_clock_controller_func am18xx_usbphyclk_fdt_funcs = {
	    .decode = am18xx_usbphyclk_decode,
};

static const struct clk_funcs am18xx_usbphyclk_clk_funcs = {
	.get = am18xx_usbphyclk_clk_get,
	.get_rate = am18xx_usbphyclk_clk_get_rate,
	.get_parent = am18xx_usbphyclk_clk_get_parent,
	.enable = am18xx_usbphyclk_clk_enable,
	.disable = am18xx_usbphyclk_clk_disable,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da830-usb-phy-clocks" },
	DEVICE_COMPAT_EOL
};

#define AM18XX_CFGCHIP2 0x8
#define AM18XX_CFGCHIP2_PHYCLKGD __BIT(17)
#define AM18XX_CFGCHIP2_PHYRESET __BIT(15)
#define AM18XX_CFGCHIP2_USB11CLKMUX __BIT(12)
#define AM18XX_CFGCHIP2_USB20CLKMUX __BIT(11)
#define AM18XX_CFGCHIP2_PHYPWRDN __BIT(10)
#define AM18XX_CFGCHIP2_PHYPLLON __BIT(6)
#define AM18XX_CFGCHIP2_PHYREFFREQ __BITS(3, 0)

/* This function should be called with sc->sc_lock held. */
static int
am18xx_usbphyclk_state_transition(struct am18xx_usbphyclk_softc *sc,
    enum usbphyclk_state state)
{
	int error;

	/* you need the usb 2.0 phy clocked to change the pll configuration */
	error = clk_enable(sc->sc_phy_logic_clk);
	if (error) {
		return error;
	}

	syscon_lock(sc->sc_syscon);

	/* first mux takes 0 as enabled, second takes 1 \_(ツ)_/ */
	uint32_t mask =
	    AM18XX_CFGCHIP2_USB11CLKMUX | AM18XX_CFGCHIP2_USB20CLKMUX;
	uint32_t update = AM18XX_CFGCHIP2_USB20CLKMUX;

	switch (state) {
	case SUSPENDED:
		mask |= AM18XX_CFGCHIP2_PHYRESET | AM18XX_CFGCHIP2_PHYPWRDN |
			AM18XX_CFGCHIP2_PHYPLLON;
		update |= AM18XX_CFGCHIP2_PHYRESET | AM18XX_CFGCHIP2_PHYPWRDN;
		break;
	case USB11_ONLY:
		mask |= AM18XX_CFGCHIP2_PHYRESET | AM18XX_CFGCHIP2_PHYPWRDN |
			AM18XX_CFGCHIP2_PHYPLLON;
		update |= AM18XX_CFGCHIP2_PHYPLLON;
		break;
	case USB20_ONLY:
		mask = AM18XX_CFGCHIP2_PHYRESET | AM18XX_CFGCHIP2_PHYPWRDN |
		       AM18XX_CFGCHIP2_PHYPLLON;
		update |= 0;
		break;
	case BOTH:
		mask = AM18XX_CFGCHIP2_PHYRESET | AM18XX_CFGCHIP2_PHYPWRDN |
		       AM18XX_CFGCHIP2_PHYPLLON;
		update |= AM18XX_CFGCHIP2_PHYPLLON;
		break;
	}

	/* update bits */
	uint32_t val = syscon_read_4(sc->sc_syscon, AM18XX_CFGCHIP2);
	val &= ~mask;
	val |= mask & update;
	syscon_write_4(sc->sc_syscon, AM18XX_CFGCHIP2, val);

	if (state != SUSPENDED) {
		/* wait for the PLL to lock */
		while ((syscon_read_4(sc->sc_syscon, AM18XX_CFGCHIP2) &
			   AM18XX_CFGCHIP2_PHYCLKGD) == 0) {
			delay(1); /* wait for PLL to lock */
		}
	}

	sc->sc_state = state;
	syscon_unlock(sc->sc_syscon);

	/* safe since psc clocks are refcounted */
	error = clk_disable(sc->sc_phy_logic_clk);
	if (error) {
		return error;
	}

	return 0;
}

static int
am18xx_usbphyclk_clk_enable(void *priv, struct clk *clkp)
{
	struct am18xx_usbphyclk_softc *const sc = priv;
	int retval;

	mutex_enter(&sc->sc_lock);

	if (clkp == &sc->sc_usb11_clk.clk_base) {
		/* this is the usb 1.1 clk */
		switch (sc->sc_state) {
		case SUSPENDED:
			retval = am18xx_usbphyclk_state_transition(sc,
			    USB11_ONLY);
			break;
		case USB20_ONLY:
			retval = am18xx_usbphyclk_state_transition(sc,
			    BOTH);
			break;
		default:
			retval = 0;
		}
	} else if (clkp == &sc->sc_usb20_clk.clk_base) {
		/* this is the usb 2.0 clk */
		switch (sc->sc_state) {
		case SUSPENDED:
			retval = am18xx_usbphyclk_state_transition(sc,
			    USB20_ONLY);
			break;
		case USB11_ONLY:
			retval = am18xx_usbphyclk_state_transition(sc,
			    BOTH);
			break;
		default:
			retval = 0;
		}
	} else {
		retval = EINVAL;
	}

	mutex_exit(&sc->sc_lock);

	return retval;
}

static int
am18xx_usbphyclk_clk_disable(void *priv, struct clk *clkp)
{
	struct am18xx_usbphyclk_softc *const sc = priv;
	int retval;

	mutex_enter(&sc->sc_lock);

	if (clkp == &sc->sc_usb11_clk.clk_base) {
		/* this is the usb 1.1 clk */
		switch (sc->sc_state) {
		case BOTH:
			retval = am18xx_usbphyclk_state_transition(sc,
			    USB20_ONLY);
			break;
		case USB11_ONLY:
			retval = am18xx_usbphyclk_state_transition(sc,
			    SUSPENDED);
			break;
		default:
			retval = 0;
		}
	} else if (clkp == &sc->sc_usb20_clk.clk_base) {
		/* this is the usb 2.0 clk */
		switch (sc->sc_state) {
		case BOTH:
			retval = am18xx_usbphyclk_state_transition(sc,
			    USB11_ONLY);
			break;
		case USB20_ONLY:
			retval = am18xx_usbphyclk_state_transition(sc,
			    SUSPENDED);
			break;
		default:
			retval = 0;
		}
	} else {
		retval = EINVAL;
	}

	mutex_exit(&sc->sc_lock);

	return retval;
}

static struct clk *
am18xx_usbphyclk_clk_get(void *priv, const char *name)
{
	struct am18xx_usbphyclk_softc *const sc = priv;

	if (strcmp(sc->sc_usb11_clk.clk_base.name, name) == 0) {
		return &sc->sc_usb11_clk.clk_base;
	} else if (strcmp(sc->sc_usb20_clk.clk_base.name, name) == 0) {
		return &sc->sc_usb20_clk.clk_base;
	}

	return NULL;
}

static u_int
am18xx_usbphyclk_clk_get_rate(void *priv, struct clk *clkp)
{
	struct am18xx_usbphyclk_softc *const sc = priv;
	u_int retval = 0;

	mutex_enter(&sc->sc_lock);

	bool is_usb11 = clkp == &sc->sc_usb11_clk.clk_base;
	bool is_usb20 = clkp == &sc->sc_usb20_clk.clk_base;

	if (is_usb11 && (sc->sc_state == USB11_ONLY || sc->sc_state == BOTH)) {
		retval = 48000000;
	}
	if (is_usb20 && (sc->sc_state == USB20_ONLY || sc->sc_state == BOTH)) {
		retval = 48000000;
	}

	mutex_exit(&sc->sc_lock);

	return retval;
}

static struct clk *
am18xx_usbphyclk_clk_get_parent(void *priv, struct clk *clkp)
{
	struct am18xx_usbphyclk_softc *const sc = priv;

	return sc->sc_ref_clk;
}

static struct clk *
am18xx_usbphyclk_decode(device_t dev, int cc_phandle, const void *data,
    size_t len)
{
	struct am18xx_usbphyclk_softc *const sc = device_private(dev);
	const u_int *cells = data;

	if (len != 4) {
		return NULL;
	}

	const u_int index = be32toh(cells[0]);

	if (index == 0) {
		return &sc->sc_usb20_clk.clk_base;
	} else if (index == 1) {
		return &sc->sc_usb11_clk.clk_base;
	}

	return NULL;
}

int
am18xx_usbphyclk_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static int
am18xx_usbphyclk_set_refreq(struct am18xx_usbphyclk_softc *sc)
{
	u_int ref_clk_rate = clk_get_rate(sc->sc_ref_clk);

	uint32_t rate_val;

	/* The USB PLL must know the frequency of its input clock. It only
	 * supports a few predefined clock rates, see the table in the technical
	 * reference manual. */
	switch (ref_clk_rate) {
	case 12000000: /* 12 MHz */
		rate_val = 1;
		break;
	case 13000000: /* 13 MHz */
		rate_val = 6;
		break;
	case 19200000: /* 19.2 MHz */
		rate_val = 4;
		break;
	case 20000000: /* 20 MHz */
		rate_val = 8;
		break;
	case 24000000: /* 24 MHz */
		rate_val = 2;
		break;
	case 26000000: /* 26 MHz */
		rate_val = 7;
		break;
	case 38400000: /* 38.4 MHz */
		rate_val = 5;
		break;
	case 40000000: /* 40 MHz */
		rate_val = 9;
		break;
	case 48000000: /* 48 MHz */
		rate_val = 3;
		break;
	default:
		return EINVAL; /* reference clock rate not supported */
	}

	mutex_enter(&sc->sc_lock);
	syscon_lock(sc->sc_syscon);
	uint32_t val = syscon_read_4(sc->sc_syscon, AM18XX_CFGCHIP2);
	val &= ~AM18XX_CFGCHIP2_PHYREFFREQ;
	val |= rate_val;
	syscon_write_4(sc->sc_syscon, AM18XX_CFGCHIP2, val);
	syscon_unlock(sc->sc_syscon);
	mutex_exit(&sc->sc_lock);

	return 0;
}

void
am18xx_usbphyclk_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_usbphyclk_softc *const sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	/* cfgchip syscon */
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	/* get reference clock */
	sc->sc_ref_clk = fdtbus_clock_get(phandle, "auxclk");
	if (sc->sc_ref_clk == NULL) {
		aprint_error(": failed to get auxclk\n");
		return;
	}
	if (clk_enable(sc->sc_ref_clk)) {
		aprint_error(": failed to enable reference clk\n");
		return;
	}

	/* get clock controlling the pll transitioning logic */
	sc->sc_phy_logic_clk = fdtbus_clock_get(phandle, "fck");
	if (sc->sc_phy_logic_clk == NULL) {
		aprint_error(": failed to get fck\n");
		return;
	}

	if (am18xx_usbphyclk_set_refreq(sc)) {
		aprint_error(": failed to set reference clock freq "
			     "(unsupported freq?)\n");
		return;
	}

	/* prepare a clock domain */
	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &am18xx_usbphyclk_clk_funcs;
	sc->sc_clkdom.priv = sc;

	/* fill the clock structures */
	sc->sc_usb20_clk.clk_base.name = "usb0_clk48";
	sc->sc_usb20_clk.clk_base.flags = 0;
	sc->sc_usb20_clk.clk_base.domain = &sc->sc_clkdom;
	sc->sc_usb11_clk.clk_base.name = "usb1_clk48";
	sc->sc_usb11_clk.clk_base.flags = 0;
	sc->sc_usb11_clk.clk_base.domain = &sc->sc_clkdom;

	/* bring the phy into a known state. We don't need to get the lock yet
	 * because the clock controller API hasn't been exposed yet. */
	am18xx_usbphyclk_state_transition(sc, SUSPENDED);

	/* create sysctl nodes for the clocks */
	clk_attach(&sc->sc_usb11_clk.clk_base);
	clk_attach(&sc->sc_usb20_clk.clk_base);

	/* register the clock */
	fdtbus_register_clock_controller(self, phandle,
	    &am18xx_usbphyclk_fdt_funcs);

	aprint_normal("\n");
}
