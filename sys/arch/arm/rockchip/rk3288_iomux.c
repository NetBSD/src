/* $NetBSD: rk3288_iomux.c,v 1.2 2021/11/12 22:53:20 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rk3288_iomux.c,v 1.2 2021/11/12 22:53:20 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	GPIO_P_CTL_Z		0
#define	GPIO_P_CTL_PULLUP	1
#define	GPIO_P_CTL_PULLDOWN	2
#define	GPIO_P_CTL_MASK		0x3U

#define	GPIO_E_CTL_2MA		0
#define	GPIO_E_CTL_4MA		1
#define	GPIO_E_CTL_8MA		2
#define	GPIO_E_CTL_12MA		3
#define	GPIO_E_CTL_MASK		0x3U

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3288-pinctrl" },
	DEVICE_COMPAT_EOL
};

struct rk3288_iomux_softc {
	device_t sc_dev;
	struct syscon *sc_grf;
	struct syscon *sc_pmu;
};

struct rk3288_iomux_reg {
	struct syscon	*syscon;
	int		mux_reg;
	uint32_t	mux_bit;
	int		pull_reg;
	uint32_t	pull_bit;
	int		drv_reg;
	uint32_t	drv_bit;
	uint32_t	flags;
#define	IOMUX_NOROUTE	__BIT(0)
#define	IOMUX_4BIT	__BIT(1)
};

#define	LOCK(reg)		\
	syscon_lock((reg)->syscon)
#define	UNLOCK(reg)		\
	syscon_unlock((reg)->syscon)
#define	RD4(reg, off)		\
	syscon_read_4((reg)->syscon, (off))
#define	WR4(reg, off, val)	\
	syscon_write_4((reg)->syscon, (off), (val))
#define	ISPMU(sc, reg)		\
	((reg)->syscon == (sc)->sc_pmu)

static int	rk3288_iomux_match(device_t, cfdata_t, void *);
static void	rk3288_iomux_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rk3288_iomux, sizeof(struct rk3288_iomux_softc),
	rk3288_iomux_match, rk3288_iomux_attach, NULL, NULL);

#define	NBANKS		9
#define	NPINSPERBANK	32

static uint32_t rk3288_iomux_flags[NBANKS][NPINSPERBANK / 8] = {
	[0] = { 0, 0, 0, IOMUX_NOROUTE },
	[1] = { IOMUX_NOROUTE, IOMUX_NOROUTE, IOMUX_NOROUTE, 0 },
	[2] = { 0, 0, 0, IOMUX_NOROUTE },
	[3] = { 0, 0, 0, IOMUX_4BIT },
	[4] = { IOMUX_4BIT, IOMUX_4BIT, 0, 0 },
	[5] = { IOMUX_NOROUTE, 0, 0, IOMUX_NOROUTE },
	[6] = { 0, 0, 0, IOMUX_NOROUTE },
	[7] = { 0, 0, IOMUX_4BIT, IOMUX_NOROUTE },
	[8] = { 0, 0, IOMUX_NOROUTE, IOMUX_NOROUTE }
};

static int rk3288_iomux_offset[NBANKS][NPINSPERBANK / 8] = {
	/* PMU offsets */
	[0] = { 0x84, 0x88, 0x8c, -1 },
	/* GRF offsets */
	[1] = { -1, -1, -1, 0x0c },
	[2] = { 0x10, 0x14, 0x18, -1 },
	[3] = { 0x20, 0x24, 0x28, 0x2c },
	[4] = { 0x34, 0x3c, 0x44, 0x48 },
	[5] = { -1, 0x50, 0x54, -1 },
	[6] = { 0x5c, 0x60, 0x64, -1 },
	[7] = { 0x6c, 0x70, 0x74, -1 },
	[8] = { 0x80, 0x84, -1, -1 },
};

static bool
rk3288_iomux_get_reg(struct rk3288_iomux_softc *sc, u_int bank, u_int idx,
    struct rk3288_iomux_reg *reg)
{
	if (bank >= NBANKS || idx >= NPINSPERBANK) {
		return false;
	}

	if (bank == 0) {
		reg->syscon = sc->sc_pmu;
		reg->pull_reg = 0x64 + (idx / 8) * 4;
		reg->pull_bit = idx % 8 * 2;
		reg->drv_reg = 0x70 + (idx / 8) * 4;
		reg->drv_bit = idx % 8 * 2;
	} else {
		reg->syscon = sc->sc_grf;
		reg->pull_reg = 0x130 + (bank * 0x10) + (idx / 8) * 4;
		reg->pull_bit = idx % 8 * 2;
		reg->drv_reg = 0x1b0 + (bank * 0x10) + (idx / 8) * 4;
		reg->drv_bit = idx % 8 * 2;
	}
	reg->flags = rk3288_iomux_flags[bank][idx / 8];
	reg->mux_reg = rk3288_iomux_offset[bank][idx / 8];
	if (reg->mux_reg != -1) {
		if ((reg->flags & IOMUX_4BIT) == 0) {
			reg->mux_bit = idx % 8 * 2;
		} else {
			reg->mux_bit = idx % 4 * 4;
			if ((idx % 8) >= 4) {
				reg->mux_reg += 0x4;
			}
		}
	}

	return true;
}

static void
rk3288_iomux_set_bias(struct rk3288_iomux_softc *sc, struct rk3288_iomux_reg *reg,
    int bias)
{
	uint32_t val;
	u_int p;

	switch (bias) {
	case 0:
		p = GPIO_P_CTL_Z;
		break;
	case GPIO_PIN_PULLUP:
		p = GPIO_P_CTL_PULLUP;
		break;
	case GPIO_PIN_PULLDOWN:
		p = GPIO_P_CTL_PULLDOWN;
		break;
	default:
		return;
	}

	if (ISPMU(sc, reg)) {
		val = RD4(reg, reg->pull_reg);
		val &= ~(GPIO_P_CTL_MASK << reg->pull_bit);
	} else {
		val = GPIO_P_CTL_MASK << (reg->pull_bit + 16);
	}
	val |= p << reg->pull_bit;
	WR4(reg, reg->pull_reg, val);
}

static void
rk3288_iomux_set_drive_strength(struct rk3288_iomux_softc *sc,
    struct rk3288_iomux_reg *reg, int drv)
{
	uint32_t val;
	u_int e;

	switch (drv) {
	case 2:
		e = GPIO_E_CTL_2MA;
		break;
	case 4:
		e = GPIO_E_CTL_4MA;
		break;
	case 8:
		e = GPIO_E_CTL_8MA;
		break;
	case 12:
		e = GPIO_E_CTL_12MA;
		break;
	default:
		return;
	}

	if (ISPMU(sc, reg)) {
		val = RD4(reg, reg->drv_reg);
		val &= ~(GPIO_E_CTL_MASK << reg->drv_bit);
	} else {
		val = GPIO_E_CTL_MASK << (reg->drv_bit + 16);
	}
	val = e << reg->drv_bit;
	WR4(reg, reg->drv_reg, val);
}

static void
rk3288_iomux_set_mux(struct rk3288_iomux_softc *sc,
    struct rk3288_iomux_reg *reg, u_int mux)
{
	uint32_t val;

	KASSERT(reg->mux_reg != -1);

	const uint32_t mask = (reg->flags & IOMUX_4BIT) ? 0xf : 0x3;
	if (ISPMU(sc, reg)) {
		val = RD4(reg, reg->mux_reg);
		val &= ~(mask << reg->mux_bit);
	} else {
		val = mask << (reg->mux_bit + 16);
	}
	val |= mux << reg->mux_bit;
	WR4(reg, reg->mux_reg, val);
}

static void
rk3288_iomux_config(struct rk3288_iomux_softc *sc, struct rk3288_iomux_reg *reg,
    u_int mux, const int phandle)
{
	int bias, drv;

	bias = fdtbus_pinctrl_parse_bias(phandle, NULL);
	drv = fdtbus_pinctrl_parse_drive_strength(phandle);

#ifdef RK3288_IOMUX_DEBUG
	printf("  -> %s mux %#x/%d, pull %#x/%d, drv %#x/%d, flags %#x\n",
	    reg->syscon == sc->sc_pmu ? "pmu" : "grf",
	    reg->mux_reg, reg->mux_bit,
	    reg->pull_reg, reg->pull_bit,
	    reg->drv_reg, reg->drv_bit,
	    reg->flags);
	printf("     bias %d drv %d mux %u\n", bias, drv, mux);
#endif

	LOCK(reg);

	if (bias != -1) {
		rk3288_iomux_set_bias(sc, reg, bias);
	}
	if (drv != -1) {
		rk3288_iomux_set_drive_strength(sc, reg, drv);
	}
	if ((reg->flags & IOMUX_NOROUTE) == 0) {
		rk3288_iomux_set_mux(sc, reg, mux);
	}

	UNLOCK(reg);
}

static int
rk3288_iomux_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct rk3288_iomux_softc * const sc = device_private(dev);
	int pins_len = 0;

	if (len != 4) {
		return -1;
	}

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));
	const u_int *pins = fdtbus_get_prop(phandle, "rockchip,pins", &pins_len);

	while (pins_len >= 16) {
		const u_int bank = be32toh(pins[0]);
		const u_int idx = be32toh(pins[1]);
		const u_int mux = be32toh(pins[2]);
		const int cfg = fdtbus_get_phandle_from_native(be32toh(pins[3]));
		struct rk3288_iomux_reg regdef = {};

		if (rk3288_iomux_get_reg(sc, bank, idx, &regdef)) {
#ifdef RK3288_IOMUX_DEBUG
			printf(" -> gpio%u P%c%u (%u)\n", bank, 'A' + (idx / 8), idx % 8, idx);
#endif
			rk3288_iomux_config(sc, &regdef, mux, cfg);
		} else {
			aprint_error_dev(dev, "unsupported iomux bank %u idx %u\n",
			    bank, mux);
		}

		pins_len -= 16;
		pins += 4;
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func rk3288_iomux_pinctrl_funcs = {
	.set_config = rk3288_iomux_pinctrl_set_config,
};

static int
rk3288_iomux_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk3288_iomux_attach(device_t parent, device_t self, void *aux)
{
	struct rk3288_iomux_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_grf = fdtbus_syscon_acquire(phandle, "rockchip,grf");
	if (sc->sc_grf == NULL) {
		aprint_error(": couldn't acquire grf syscon\n");
		return;
	}
	sc->sc_pmu = fdtbus_syscon_acquire(phandle, "rockchip,pmu");
	if (sc->sc_pmu == NULL) {
		aprint_error(": couldn't acquire pmu syscon\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": RK3288 IOMUX control\n");

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		for (int sub = OF_child(child); sub; sub = OF_peer(sub)) {
			if (!of_hasprop(sub, "rockchip,pins"))
				continue;
			fdtbus_register_pinctrl_config(self, sub, &rk3288_iomux_pinctrl_funcs);
		}
	}

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_phandle = child;
		cfaa.faa_name = fdtbus_get_string(child, "name");
		cfaa.faa_quiet = false;

		config_found(self, &cfaa, NULL, CFARGS_NONE);
	}
}
