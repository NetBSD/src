/* $NetBSD: rk3399_iomux.c,v 1.1 2018/08/12 16:48:05 jmcneill Exp $ */

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

//#define RK3399_IOMUX_DEBUG

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rk3399_iomux.c,v 1.1 2018/08/12 16:48:05 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

/* PU/PD control */
#define	 GRF_GPIO_P_CTL(_idx)		(0x3 << (((_idx) & 7) * 2))
#define	 GRF_GPIO_P_WRITE_EN(_idx)	(0x3 << (((_idx) & 7) * 2 + 16))
/* Different bias value mapping for GRF and PMU registers */
#define	  GRF_GPIO_P_CTL_Z		0
#define	  GRF_GPIO_P_CTL_PULLDOWN	1
#define	  GRF_GPIO_P_CTL_Z_ALT		2
#define	  GRF_GPIO_P_CTL_PULLUP		3
#define	  PMU_GPIO_P_CTL_Z		0
#define	  PMU_GPIO_P_CTL_PULLUP		1
#define	  PMU_GPIO_P_CTL_PULLDOWN	2
#define	  PMU_GPIO_P_CTL_RESERVED	3

/* Drive strength control */
/* Different drive strength value mapping for GRF and PMU registers */
#define	  GRF_GPIO_E_CTL_2MA		0
#define	  GRF_GPIO_E_CTL_4MA		1
#define	  GRF_GPIO_E_CTL_8MA		2
#define	  GRF_GPIO_E_CTL_12MA		3
#define	  PMU_GPIO_E_CTL_5MA		0
#define	  PMU_GPIO_E_CTL_10MA		1
#define	  PMU_GPIO_E_CTL_15MA		2
#define	  PMU_GPIO_E_CTL_20MA		3

enum rk3399_drv_type {
	RK3399_DRV_TYPE_IO_DEFAULT,
	RK3399_DRV_TYPE_IO_1V8_3V0,
	RK3399_DRV_TYPE_IO_1V8,
	RK3399_DRV_TYPE_IO_1V8_3V0_AUTO,
	RK3399_DRV_TYPE_IO_3V3,
};

static int rk3399_drv_strength[5][9] = {
	[RK3399_DRV_TYPE_IO_DEFAULT] =		{ 2, 4, 8, 12, -1 },
	[RK3399_DRV_TYPE_IO_1V8_3V0] =		{ 3, 6, 9, 12, -1 },
	[RK3399_DRV_TYPE_IO_1V8] =		{ 5, 10, 15, 20, -1 },
	[RK3399_DRV_TYPE_IO_1V8_3V0_AUTO] =	{ 4, 6, 8, 10, 12, 14, 16, 18, -1 },
	[RK3399_DRV_TYPE_IO_3V3] =		{ 4, 7, 10, 13, 16, 19, 22, 26, -1 },
};

struct rk3399_iomux {
	enum rk3399_drv_type	drv_type;
};

struct rk3399_iomux_bank {
	struct rk3399_iomux	iomux[5];
	u_int			regs;
#define	RK_IOMUX_REGS_GRF	0
#define	RK_IOMUX_REGS_PMU	1
};

static const struct rk3399_iomux_bank rk3399_iomux_banks[] = {
	[0] = {
		.regs = RK_IOMUX_REGS_PMU,
		.iomux = {
			[0] = { .drv_type = RK3399_DRV_TYPE_IO_1V8 },
			[1] = { .drv_type = RK3399_DRV_TYPE_IO_1V8 },
			[2] = { .drv_type = RK3399_DRV_TYPE_IO_DEFAULT },
			[3] = { .drv_type = RK3399_DRV_TYPE_IO_DEFAULT },
		},
	},
	[1] = {
		.regs = RK_IOMUX_REGS_PMU,
		.iomux = {
			[0] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[1] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[2] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[3] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
		}
	},
	[2] = {
		.regs = RK_IOMUX_REGS_GRF,
		.iomux = {
			[0] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[1] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[2] = { .drv_type = RK3399_DRV_TYPE_IO_1V8 },
			[3] = { .drv_type = RK3399_DRV_TYPE_IO_1V8 },
		},
	},
	[3] = {
		.regs = RK_IOMUX_REGS_GRF,
		.iomux = {
			[0] = { .drv_type = RK3399_DRV_TYPE_IO_3V3 },
			[1] = { .drv_type = RK3399_DRV_TYPE_IO_3V3 },
			[2] = { .drv_type = RK3399_DRV_TYPE_IO_3V3 },
			[3] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
		},
	},
	[4] = {
		.regs = RK_IOMUX_REGS_GRF,
		.iomux = {
			[0] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[1] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0_AUTO },
			[2] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
			[3] = { .drv_type = RK3399_DRV_TYPE_IO_1V8_3V0 },
		},
	},
};

#define	RK3399_IOMUX_BANK_IS_PMU(_bank)	(rk3399_iomux_banks[(_bank)].regs == RK_IOMUX_REGS_PMU)

struct rk3399_iomux_conf {
	const struct rk3399_iomux_bank *banks;
	u_int nbanks;
};

static const struct rk3399_iomux_conf rk3399_iomux_conf = {
	.banks = rk3399_iomux_banks,
	.nbanks = __arraycount(rk3399_iomux_banks),
};

static const struct of_compat_data compat_data[] = {
	{ "rockchip,rk3399-pinctrl",	(uintptr_t)&rk3399_iomux_conf },
	{ NULL }
};

struct rk3399_iomux_softc {
	device_t sc_dev;
	struct syscon *sc_syscon[2];

	const struct rk3399_iomux_conf *sc_conf;
};

#define	LOCK(syscon) 		\
	syscon_lock(syscon)
#define	UNLOCK(syscon)		\
	syscon_unlock(syscon)
#define	RD4(syscon, reg) 	\
	syscon_read_4(syscon, (reg))
#define	WR4(syscon, reg, val) 	\
	syscon_write_4(syscon, (reg), (val))

static int	rk3399_iomux_match(device_t, cfdata_t, void *);
static void	rk3399_iomux_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rk3399_iomux, sizeof(struct rk3399_iomux_softc),
	rk3399_iomux_match, rk3399_iomux_attach, NULL, NULL);

static void
rk3399_iomux_set_bias(struct rk3399_iomux_softc *sc, u_int bank, u_int idx, int flags)
{
	const struct rk3399_iomux_bank *banks = sc->sc_conf->banks;
	bus_size_t reg;
	u_int bias;

	KASSERT(bank < sc->sc_conf->nbanks);

	struct syscon * const syscon = sc->sc_syscon[banks[bank].regs];
	if (RK3399_IOMUX_BANK_IS_PMU(bank)) {
		reg = 0x00040 + (0x10 * bank);
	} else {
		reg = 0x0e040 + (0x10 * (bank - 2));
	}
	reg += 0x4 * (idx / 8);

	if (flags == GPIO_PIN_PULLUP) {
		bias = RK3399_IOMUX_BANK_IS_PMU(bank) ? PMU_GPIO_P_CTL_PULLUP : GRF_GPIO_P_CTL_PULLUP;
	} else if (flags == GPIO_PIN_PULLDOWN) {
		bias = RK3399_IOMUX_BANK_IS_PMU(bank) ? PMU_GPIO_P_CTL_PULLDOWN : GRF_GPIO_P_CTL_PULLDOWN;
	} else {
		bias = RK3399_IOMUX_BANK_IS_PMU(bank) ? PMU_GPIO_P_CTL_Z : GRF_GPIO_P_CTL_Z;
	}

	const uint32_t bias_val = __SHIFTIN(bias, GRF_GPIO_P_CTL(idx));
	const uint32_t bias_mask = GRF_GPIO_P_WRITE_EN(idx);

#ifdef RK3399_IOMUX_DEBUG
	printf("%s: bank %d idx %d flags %#x: %08x -> ", __func__, bank, idx, flags, RD4(syscon, reg));
#endif
	WR4(syscon, reg, bias_val | bias_mask);
#ifdef RK3399_IOMUX_DEBUG
	printf("%08x (reg %#lx)\n", RD4(syscon, reg), reg);
#endif
}

static int
rk3399_iomux_map_drive_strength(struct rk3399_iomux_softc *sc, enum rk3399_drv_type drv_type, u_int val)
{
	for (int n = 0; rk3399_drv_strength[drv_type][n] != -1; n++)
		if (rk3399_drv_strength[drv_type][n] == val)
			return n;
	return -1;
}

static int 
rk3399_iomux_set_drive_strength(struct rk3399_iomux_softc *sc, u_int bank, u_int idx, u_int val)
{
	const struct rk3399_iomux_bank *banks = sc->sc_conf->banks;
	uint32_t drv_mask, drv_val;
	bus_size_t reg;

	KASSERT(bank < sc->sc_conf->nbanks);

	if (idx >= 32)
		return EINVAL;

	const int drv = rk3399_iomux_map_drive_strength(sc, banks[bank].iomux[idx / 8].drv_type, val);
	if (drv == -1)
		return EINVAL;

	struct syscon * const syscon = sc->sc_syscon[banks[bank].regs];
	switch (bank) {
	case 0:
	case 1:
		reg = 0x00040 + (0x10 * bank) + 0x4 * (idx / 4);
		drv_mask = 0x3 << ((idx & 7) * 2);
		break;
	case 2:
		reg = 0x0e100 + 0x4 * (idx / 4);
		drv_mask = 0x3 << ((idx & 7) * 2);
		break;
	case 3:
		switch (idx / 8) {
		case 0:
		case 1:
		case 2:
			reg = 0x0e110 + 0x8 * (idx / 4);
			drv_mask = 0x7 << ((idx & 7) * 3);
			break;
		case 3:
			reg = 0x0e128;
			drv_mask = 0x3 << ((idx & 7) * 2);
			break;
		default:
			return EINVAL;
		}
		break;
	case 4:
		switch (idx / 8) {
		case 0:
			reg = 0x0e12c;
			drv_mask = 0x3 << ((idx & 7) * 2);
			break;
		case 1:
			reg = 0x0e130;
			drv_mask = 0x7 << ((idx & 7) * 3);
			break;
		case 2:
			reg = 0x0e138;
			drv_mask = 0x3 << ((idx & 7) * 2);
			break;
		case 3:
			reg = 0x0e13c;
			drv_mask = 0x3 << ((idx & 7) * 2);
			break;
		default:
			return EINVAL;
		}	
		break;
	default:
		return EINVAL;
	}
	drv_val = __SHIFTIN(val, drv_mask);

	while (drv_mask) {
		const uint32_t write_val = drv_val & 0xffff;
		const uint32_t write_mask = (drv_mask & 0xffff) << 16;
		if (write_mask) {
#ifdef RK3399_IOMUX_DEBUG
			printf("%s: bank %d idx %d val %d: %08x -> ", __func__, bank, idx, val, RD4(syscon, reg));
#endif
			WR4(syscon, reg, write_val | write_mask);
#ifdef RK3399_IOMUX_DEBUG
			printf("%08x (reg %#lx)\n", RD4(syscon, reg), reg);
#endif
		}
		reg += 0x4;
		drv_val >>= 16;
		drv_mask >>= 16;
	}

	return 0;
}

static void
rk3399_iomux_set_mux(struct rk3399_iomux_softc *sc, u_int bank, u_int idx, u_int mux)
{
	const struct rk3399_iomux_bank *banks = sc->sc_conf->banks;
	bus_size_t reg;
	uint32_t mask;

	KASSERT(bank < sc->sc_conf->nbanks);

	struct syscon * const syscon = sc->sc_syscon[banks[bank].regs];
	if (RK3399_IOMUX_BANK_IS_PMU(bank)) {
		reg = 0x00000 + (0x10 * bank);
	} else {
		reg = 0x0e000 + (0x10 * (bank - 2));
	}
	reg += 0x4 * (idx / 4);
	mask = 3 << ((idx & 7) * 2);

#ifdef RK3399_IOMUX_DEBUG
	printf("%s: bank %d idx %d mux %#x: %08x -> ", __func__, bank, idx, mux, RD4(syscon, reg));
#endif
	WR4(syscon, reg, (mask << 16) | __SHIFTIN(mux, mask));
#ifdef RK3399_IOMUX_DEBUG
	printf("%08x (reg %#lx)\n", RD4(syscon, reg), reg);
#endif
}

static int
rk3399_iomux_config(struct rk3399_iomux_softc *sc, const int phandle, u_int bank, u_int idx, u_int mux)
{
	u_int drv;

	if (of_hasprop(phandle, "bias-disable"))
		rk3399_iomux_set_bias(sc, bank, idx, 0);
	else if (of_hasprop(phandle, "bias-pull-up"))
		rk3399_iomux_set_bias(sc, bank, idx, GPIO_PIN_PULLUP);
	else if (of_hasprop(phandle, "bias-pull-down"))
		rk3399_iomux_set_bias(sc, bank, idx, GPIO_PIN_PULLDOWN);

	if (of_getprop_uint32(phandle, "drive-strength", &drv) == 0) {
		if (rk3399_iomux_set_drive_strength(sc, bank, idx, drv) != 0)
			return EINVAL;
	}

#if notyet
	if (of_hasprop(phandle, "input-enable"))
		rk3399_iomux_set_direction(sc, bank, idx, GPIO_PIN_INPUT, -1);
	else if (of_hasprop(phandle, "output-high"))
		rk3399_iomux_set_direction(sc, bank, idx, GPIO_PIN_OUTPUT, GPIO_PIN_HIGH);
	else if (of_hasprop(phandle, "output-low"))
		rk3399_iomux_set_direction(sc, bank, idx, GPIO_PIN_OUTPUT, GPIO_PIN_LOW);
#endif

	rk3399_iomux_set_mux(sc, bank, idx, mux);

	return 0;
}

static int
rk3399_iomux_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct rk3399_iomux_softc * const sc = device_private(dev);
	const struct rk3399_iomux_bank *banks = sc->sc_conf->banks;
	int pins_len;

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));
	const u_int *pins = fdtbus_get_prop(phandle, "rockchip,pins", &pins_len);

	while (pins_len >= 16) {
		const u_int bank = be32toh(pins[0]);
		const u_int idx = be32toh(pins[1]);
		const u_int mux = be32toh(pins[2]);
		const int cfg = fdtbus_get_phandle_from_native(be32toh(pins[3]));

		struct syscon * const syscon = sc->sc_syscon[banks[bank].regs];
		LOCK(syscon);
		rk3399_iomux_config(sc, cfg, bank, idx, mux);
		UNLOCK(syscon);

		pins_len -= 16;
		pins += 4;
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func rk3399_iomux_pinctrl_funcs = {
	.set_config = rk3399_iomux_pinctrl_set_config,
};

static int
rk3399_iomux_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
rk3399_iomux_attach(device_t parent, device_t self, void *aux)
{
	struct rk3399_iomux_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int child, sub;

	sc->sc_dev = self;
	sc->sc_syscon[RK_IOMUX_REGS_GRF] = fdtbus_syscon_acquire(phandle, "rockchip,grf");
	if (sc->sc_syscon[RK_IOMUX_REGS_GRF] == NULL) {
		aprint_error(": couldn't acquire grf syscon\n");
		return;
	}
	sc->sc_syscon[RK_IOMUX_REGS_PMU] = fdtbus_syscon_acquire(phandle, "rockchip,pmu");
	if (sc->sc_syscon[RK_IOMUX_REGS_PMU] == NULL) {
		aprint_error(": couldn't acquire pmu syscon\n");
		return;
	}
	sc->sc_conf = (void *)of_search_compatible(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": RK3399 IOMUX control\n");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		for (sub = OF_child(child); sub; sub = OF_peer(sub)) {
			if (!of_hasprop(sub, "rockchip,pins"))
				continue;
			fdtbus_register_pinctrl_config(self, sub, &rk3399_iomux_pinctrl_funcs);
		}
	}

	fdtbus_pinctrl_configure();

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_phandle = child;
		cfaa.faa_name = fdtbus_get_string(child, "name");
		cfaa.faa_quiet = false;

		config_found(self, &cfaa, NULL);
	}
}
