/* $NetBSD: meson_thermal.c,v 1.6 2021/01/27 03:10:18 thorpej Exp $ */

/*
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_thermal.c,v 1.6 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/sysmon/sysmonvar.h>

#define TS_CFG_REG1	0x01
#define  TS_CFG_REG1_ANA_EN_VCM		__BIT(10)
#define  TS_CFG_REG1_ANA_EN_VBG		__BIT(9)
#define  TS_CFG_REG1_FILTER_EN		__BIT(5)
#define  TS_CFG_REG1_DEM_EN		__BIT(3)
#define  TS_CFG_REG1_ANA_CH_SEL		__BITS(2,0)
#define TS_CFG_REG2	0x02
#define TS_CFG_REG3	0x03
#define TS_CFG_REG4	0x04
#define TS_CFG_REG5	0x05
#define TS_CFG_REG6	0x06
#define TS_CFG_REG7	0x07
#define TS_STAT0_REG	0x10
#define  TS_STAT0_FILTER_OUT		__BITS(15,0)
#define TS_STAT1_REG	0x11
#define TS_STAT2_REG	0x12
#define TS_STAT3_REG	0x13
#define TS_STAT4_REG	0x14
#define TS_STAT5_REG	0x15
#define TS_STAT6_REG	0x16
#define TS_STAT7_REG	0x17
#define TS_STAT8_REG	0x18
#define TS_STAT9_REG	0x19

#define THERMAL_READ_REG(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg) * 4)
#define THERMAL_WRITE_REG(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg) * 4, (val))

#define AOSECURE_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh_ao, (reg))
#define AOSECURE_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh_ao, (reg), (val))


struct meson_thermal_config {
	const char *name;
	bus_size_t aosec_reg;
};

static struct meson_thermal_config thermal_cpu_conf = {
	.name = "CPU",
	.aosec_reg = 0x128
};

static struct meson_thermal_config thermal_ddr_conf = {
	.name = "DDR",
	.aosec_reg = 0xf0
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,g12a-cpu-thermal", .data = &thermal_cpu_conf },
	{ .compat = "amlogic,g12a-ddr-thermal", .data = &thermal_ddr_conf },
	DEVICE_COMPAT_EOL
};

struct meson_thermal_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_bsh_ao;
	const struct meson_thermal_config *sc_conf;
	int sc_phandle;
	int sc_ao_calib;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor_temp;
};


static void
meson_thermal_init(struct meson_thermal_softc *sc)
{
	uint32_t val;

	val = THERMAL_READ_REG(sc, TS_CFG_REG1);
	val |= TS_CFG_REG1_ANA_EN_VCM;
	val |= TS_CFG_REG1_ANA_EN_VBG;
	val |= TS_CFG_REG1_FILTER_EN;
	val |= TS_CFG_REG1_DEM_EN;
	val &= ~TS_CFG_REG1_ANA_CH_SEL;
	val |= __SHIFTIN(3, TS_CFG_REG1_ANA_CH_SEL);
	THERMAL_WRITE_REG(sc, TS_CFG_REG1, val);

	/* read calibration value in ao-secure */
#define TS_AO_CALIB_VERSION_MASK	__BITS(31,24)
#define TS_AO_CALIB_SIGN_MASK		__BIT(15)
#define TS_AO_CALIB_TEMP_MASK		__BITS(14,0)
	val = AOSECURE_READ(sc, sc->sc_conf->aosec_reg);
	if ((val & TS_AO_CALIB_VERSION_MASK) != 0) {
		sc->sc_ao_calib = (val & TS_AO_CALIB_TEMP_MASK);
		if ((val & TS_AO_CALIB_SIGN_MASK) != 0)
			sc->sc_ao_calib *= -1;
	} else {
		sc->sc_ao_calib = 0;
	}
}

static int
meson_get_temperature(struct meson_thermal_softc *sc)
{
	int val, temp;
	int64_t factor, uptat;

	val = THERMAL_READ_REG(sc, TS_STAT0_REG) & TS_STAT0_FILTER_OUT;

#define CALIB_A		9411
#define CALIB_B		3159
#define CALIB_m_1024	4342	/* 4.24 */
#define CALIB_n_1024	3318	/* 3.24 */

	factor = (val * CALIB_n_1024) / 1024;
	uptat = (val * CALIB_m_1024) / 1024;

	uptat = (uptat * (1 << 16)) / ((1 << 16) + factor);
	temp = ((uptat + sc->sc_ao_calib) * CALIB_A);
	temp = (temp - (CALIB_B * (1 << 16))) * 100000LL / (1 << 16);

	return temp;	/* microcelsius */
}

static void
meson_thermal_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct meson_thermal_softc *sc = sme->sme_cookie;

	edata->value_cur = meson_get_temperature(sc) + 273150000;
	edata->state = ENVSYS_SVALID;
}

static int
meson_thermal_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_thermal_attach(device_t parent, device_t self, void *aux)
{
	struct meson_thermal_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size, aosize;
	int phandle, phandle_aosec;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle = faa->faa_phandle;
	sc->sc_conf = of_compatible_lookup(phandle, compat_data)->data;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		goto attach_failure0;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		goto attach_failure0;
	}

	phandle_aosec = fdtbus_get_phandle(phandle, "amlogic,ao-secure");
	if (fdtbus_get_reg(phandle_aosec, 0, &addr, &aosize) != 0) {
		aprint_error(": couldn't get registers\n");
		goto attach_failure1;
	}
	if (bus_space_map(sc->sc_bst, addr, aosize, 0, &sc->sc_bsh_ao) != 0) {
		aprint_error(": couldn't map registers\n");
		goto attach_failure1;
	}

	if (fdtbus_clock_enable_index(phandle, 0, true) != 0) {
		aprint_error(": couldn't enable clock\n");
		goto attach_failure2;
	}

	meson_thermal_init(sc);

	aprint_naive("\n");
	aprint_normal(": %s TEMP Sensor\n", sc->sc_conf->name);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_flags = 0;
	sc->sc_sme->sme_events_timeout = 1;
	sc->sc_sme->sme_refresh = meson_thermal_refresh;
	sc->sc_sensor_temp.units = ENVSYS_STEMP;
	sc->sc_sensor_temp.state = ENVSYS_SINVALID;
	snprintf(sc->sc_sensor_temp.desc, ENVSYS_DESCLEN,
	    "%s", sc->sc_conf->name);

	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor_temp);
	sysmon_envsys_register(sc->sc_sme);

	meson_thermal_refresh(sc->sc_sme, &sc->sc_sensor_temp);
	return;

 attach_failure2:
	bus_space_unmap(sc->sc_bst, sc->sc_bsh_ao, aosize);
 attach_failure1:
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, size);
 attach_failure0:
	return;
}

CFATTACH_DECL_NEW(meson_thermal, sizeof(struct meson_thermal_softc),
    meson_thermal_match, meson_thermal_attach, NULL, NULL);
