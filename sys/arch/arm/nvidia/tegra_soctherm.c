/* $NetBSD: tegra_soctherm.c,v 1.6.2.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_soctherm.c,v 1.6.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_socthermreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

#define FUSE_TSENSOR_CALIB_CP_TS_BASE	__BITS(12,0)
#define FUSE_TSENSOR_CALIB_FT_TS_BASE	__BITS(25,13)

#define FUSE_TSENSOR8_CALIB_REG		0x180
#define FUSE_TSENSOR8_CALIB_CP_TS_BASE	__BITS(9,0)
#define FUSE_TSENSOR8_CALIB_FT_TS_BASE	__BITS(20,10)

#define FUSE_SPARE_REALIGNMENT_REG	0x1fc
#define FUSE_SPARE_REALIGNMENT_CP	__BITS(5,0)
#define FUSE_SPARE_REALIGNMENT_FT	__BITS(25,21)

static int	tegra_soctherm_match(device_t, cfdata_t, void *);
static void	tegra_soctherm_attach(device_t, device_t, void *);

struct tegra_soctherm_config {
	uint32_t init_pdiv;
	uint32_t init_hotspot_off;
	uint32_t nominal_calib_ft;
	uint32_t nominal_calib_cp;
	uint32_t tall;
	uint32_t tsample;
	uint32_t tiddq_en;
	uint32_t ten_count;
	uint32_t pdiv;
	uint32_t tsample_ate;
	uint32_t pdiv_ate;
};

static const struct tegra_soctherm_config tegra124_soctherm_config = {
	.init_pdiv = 0x8888,
	.init_hotspot_off = 0x60600,
	.nominal_calib_ft = 105,
	.nominal_calib_cp = 25,
	.tall = 16300,
	.tsample = 120,
	.tiddq_en = 1,
	.ten_count = 1,
	.pdiv = 8,
	.tsample_ate = 480,
	.pdiv_ate = 8
};

struct tegra_soctherm_sensor {
	envsys_data_t		s_data;
	u_int			s_base;
	u_int			s_fuse;
	int			s_fuse_corr_alpha;
	int			s_fuse_corr_beta;
	int16_t			s_therm_a;
	int16_t			s_therm_b;
};

static const struct tegra_soctherm_sensor tegra_soctherm_sensors[] = {
	{ .s_data = { .desc = "CPU0" }, .s_base = 0x0c0, .s_fuse = 0x098,
	  .s_fuse_corr_alpha = 1135400, .s_fuse_corr_beta = -6266900 },
	{ .s_data = { .desc = "CPU1" }, .s_base = 0x0e0, .s_fuse = 0x084,
	  .s_fuse_corr_alpha = 1122220, .s_fuse_corr_beta = -5700700 },
	{ .s_data = { .desc = "CPU2" }, .s_base = 0x100, .s_fuse = 0x088,
	  .s_fuse_corr_alpha = 1127000, .s_fuse_corr_beta = -6768200 },
	{ .s_data = { .desc = "CPU3" }, .s_base = 0x120, .s_fuse = 0x12c,
	  .s_fuse_corr_alpha = 1110900, .s_fuse_corr_beta = -6232000 },
	{ .s_data = { .desc = "MEM0" }, .s_base = 0x140, .s_fuse = 0x158,
	  .s_fuse_corr_alpha = 1122300, .s_fuse_corr_beta = -5936400 },
	{ .s_data = { .desc = "MEM1" }, .s_base = 0x160, .s_fuse = 0x15c,
	  .s_fuse_corr_alpha = 1145700, .s_fuse_corr_beta = -7124600 },
	{ .s_data = { .desc = "GPU" },  .s_base = 0x180, .s_fuse = 0x154,
	  .s_fuse_corr_alpha = 1120100, .s_fuse_corr_beta = -6000500 },
	{ .s_data = { .desc = "PLLX" }, .s_base = 0x1a0, .s_fuse = 0x160,
	  .s_fuse_corr_alpha = 1106500, .s_fuse_corr_beta = -6729300 },
};

struct tegra_soctherm_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct clk		*sc_clk_tsensor;
	struct clk		*sc_clk_soctherm;
	struct fdtbus_reset	*sc_rst_soctherm;

	struct sysmon_envsys	*sc_sme;
	struct tegra_soctherm_sensor *sc_sensors;
	const struct tegra_soctherm_config *sc_config;

	uint32_t		sc_base_cp;
	uint32_t		sc_base_ft;
	int32_t			sc_actual_temp_cp;
	int32_t			sc_actual_temp_ft;
};

static int	tegra_soctherm_init_clocks(struct tegra_soctherm_softc *);
static void	tegra_soctherm_init_sensors(device_t);
static void	tegra_soctherm_init_sensor(struct tegra_soctherm_softc *,
		    struct tegra_soctherm_sensor *);
static void	tegra_soctherm_refresh(struct sysmon_envsys *, envsys_data_t *);
static int	tegra_soctherm_decodeint(uint32_t, uint32_t);
static int64_t	tegra_soctherm_divide(int64_t, int64_t);

CFATTACH_DECL_NEW(tegra_soctherm, sizeof(struct tegra_soctherm_softc),
	tegra_soctherm_match, tegra_soctherm_attach, NULL, NULL);

#define SOCTHERM_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define SOCTHERM_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define SOCTHERM_SET_CLEAR(sc, reg, set, clr)	\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

#define SENSOR_READ(sc, s, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (s)->s_base + (reg))
#define SENSOR_WRITE(sc, s, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (s)->s_base + (reg), (val))
#define SENSOR_SET_CLEAR(sc, s, reg, set, clr)	\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (s)->s_base + (reg), (set), (clr))

static const struct of_compat_data compat_data[] = {
	{ "nvidia,tegra124-soctherm",	(uintptr_t)&tegra124_soctherm_config },
	{ NULL }
};

static int
tegra_soctherm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
tegra_soctherm_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_soctherm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_clk_tsensor = fdtbus_clock_get(phandle, "tsensor");
	if (sc->sc_clk_tsensor == NULL) {
		aprint_error(": couldn't get clock tsensor\n");
		return;
	}
	sc->sc_clk_soctherm = fdtbus_clock_get(phandle, "soctherm");
	if (sc->sc_clk_soctherm == NULL) {
		aprint_error(": couldn't get clock soctherm\n");
		return;
	}
	sc->sc_rst_soctherm = fdtbus_reset_get(phandle, "soctherm");
	if (sc->sc_rst_soctherm == NULL) {
		aprint_error(": couldn't get reset soctherm\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SOC_THERM\n");

	sc->sc_config = (void *)of_search_compatible(phandle, compat_data)->data;
	if (sc->sc_config == NULL) {
		aprint_error_dev(self, "unsupported SoC\n");
		return;
	}

	if (tegra_soctherm_init_clocks(sc) != 0)
		return;

	config_defer(self, tegra_soctherm_init_sensors);
}

static int
tegra_soctherm_init_clocks(struct tegra_soctherm_softc *sc)
{
	int error;

	fdtbus_reset_assert(sc->sc_rst_soctherm);

	error = clk_set_rate(sc->sc_clk_soctherm, 51000000);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't set soctherm rate: %d\n", error);
		return error;
	}

	error = clk_set_rate(sc->sc_clk_tsensor, 400000);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't set tsensor rate: %d\n", error);
		return error;
	}

	error = clk_enable(sc->sc_clk_tsensor);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable tsensor: %d\n",
		    error);
		return error;
	}

	error = clk_enable(sc->sc_clk_soctherm);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable soctherm: %d\n",
		    error);
		return error;
	}

	fdtbus_reset_deassert(sc->sc_rst_soctherm);

	return 0;
}

static void
tegra_soctherm_init_sensors(device_t dev)
{
	struct tegra_soctherm_softc * const sc = device_private(dev);
	const struct tegra_soctherm_config *config = sc->sc_config;
	const u_int nsensors = __arraycount(tegra_soctherm_sensors);
	const size_t len = sizeof(*sc->sc_sensors) * nsensors;
	uint32_t val;
	u_int n;

	val = tegra_fuse_read(FUSE_TSENSOR8_CALIB_REG);
	sc->sc_base_cp = __SHIFTOUT(val, FUSE_TSENSOR8_CALIB_CP_TS_BASE);
	sc->sc_base_ft = __SHIFTOUT(val, FUSE_TSENSOR8_CALIB_FT_TS_BASE);
	val = tegra_fuse_read(FUSE_SPARE_REALIGNMENT_REG);
	const int calib_cp = tegra_soctherm_decodeint(val,
	    FUSE_SPARE_REALIGNMENT_CP);
	const int calib_ft = tegra_soctherm_decodeint(val,
	    FUSE_SPARE_REALIGNMENT_FT);
	sc->sc_actual_temp_cp = 2 * config->nominal_calib_cp + calib_cp;
	sc->sc_actual_temp_ft = 2 * config->nominal_calib_ft + calib_ft;

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = tegra_soctherm_refresh;

	sc->sc_sensors = kmem_zalloc(len, KM_SLEEP);
	for (n = 0; n < nsensors; n++) {
		sc->sc_sensors[n] = tegra_soctherm_sensors[n];
		tegra_soctherm_init_sensor(sc, &sc->sc_sensors[n]);
	}

	SOCTHERM_WRITE(sc, SOC_THERM_TSENSOR_PDIV_REG, config->init_pdiv);
	SOCTHERM_WRITE(sc, SOC_THERM_TSENSOR_HOTSPOT_OFF_REG,
	    config->init_hotspot_off);

	sysmon_envsys_register(sc->sc_sme);
}

static void
tegra_soctherm_init_sensor(struct tegra_soctherm_softc *sc,
    struct tegra_soctherm_sensor *s)
{
	const struct tegra_soctherm_config *config = sc->sc_config;
	int64_t temp_a, temp_b, tmp;
	uint32_t val;

	val = tegra_fuse_read(s->s_fuse);
	const int calib_cp = tegra_soctherm_decodeint(val,
	    FUSE_TSENSOR_CALIB_CP_TS_BASE);
	const int calib_ft = tegra_soctherm_decodeint(val,
	    FUSE_TSENSOR_CALIB_FT_TS_BASE);
	const int actual_cp = sc->sc_base_cp * 64 + calib_cp;
	const int actual_ft = sc->sc_base_ft * 32 + calib_ft;

	const int64_t d_sensor = actual_ft - actual_cp;
	const int64_t d_temp = sc->sc_actual_temp_ft - sc->sc_actual_temp_cp;
	const int mult = config->pdiv * config->tsample_ate;
	const int div = config->tsample * config->pdiv_ate;

	temp_a = tegra_soctherm_divide(d_temp * 0x2000 * mult,
	    d_sensor * div);
	tmp = (int64_t)actual_ft * sc->sc_actual_temp_cp -
	      (int64_t)actual_cp * sc->sc_actual_temp_ft;
	temp_b = tegra_soctherm_divide(tmp, d_sensor);
	temp_a = tegra_soctherm_divide(
	    temp_a * s->s_fuse_corr_alpha, 1000000);
	temp_b = (uint16_t)tegra_soctherm_divide(
	    temp_b * s->s_fuse_corr_alpha + s->s_fuse_corr_beta, 1000000);

	s->s_therm_a = (int16_t)temp_a;
	s->s_therm_b = (int16_t)temp_b;

	SENSOR_SET_CLEAR(sc, s, SOC_THERM_TSENSOR_CONFIG0_OFFSET,
	    SOC_THERM_TSENSOR_CONFIG0_STATUS_CLR |
	    SOC_THERM_TSENSOR_CONFIG0_STOP, 0);
	SENSOR_WRITE(sc, s, SOC_THERM_TSENSOR_CONFIG0_OFFSET,
	    __SHIFTIN(config->tall, SOC_THERM_TSENSOR_CONFIG0_TALL) |
	    SOC_THERM_TSENSOR_CONFIG0_STOP);

	SENSOR_WRITE(sc, s, SOC_THERM_TSENSOR_CONFIG1_OFFSET,
	    __SHIFTIN(config->tsample - 1, SOC_THERM_TSENSOR_CONFIG1_TSAMPLE) |
	    __SHIFTIN(config->tiddq_en, SOC_THERM_TSENSOR_CONFIG1_TIDDQ_EN) |
	    __SHIFTIN(config->ten_count, SOC_THERM_TSENSOR_CONFIG1_TEN_COUNT) |
	    SOC_THERM_TSENSOR_CONFIG1_TEMP_ENABLE);

	SENSOR_WRITE(sc, s, SOC_THERM_TSENSOR_CONFIG2_OFFSET,
	    __SHIFTIN((uint16_t)s->s_therm_a,
		      SOC_THERM_TSENSOR_CONFIG2_THERM_A) |
	    __SHIFTIN((uint16_t)s->s_therm_b,
		      SOC_THERM_TSENSOR_CONFIG2_THERM_B));

	SENSOR_SET_CLEAR(sc, s, SOC_THERM_TSENSOR_CONFIG0_OFFSET,
	    0, SOC_THERM_TSENSOR_CONFIG0_STOP);

	s->s_data.units = ENVSYS_STEMP;
	s->s_data.state = ENVSYS_SINVALID;
	sysmon_envsys_sensor_attach(sc->sc_sme, &s->s_data);
}

static void
tegra_soctherm_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct tegra_soctherm_softc * const sc = sme->sme_cookie;
	struct tegra_soctherm_sensor *s = (struct tegra_soctherm_sensor *)edata;
	uint32_t status;

	status = SENSOR_READ(sc, s, SOC_THERM_TSENSOR_STATUS1_OFFSET);
	if (status & SOC_THERM_TSENSOR_STATUS1_TEMP_VALID) {
		const u_int temp = __SHIFTOUT(status,
		    SOC_THERM_TSENSOR_STATUS1_TEMP);
		int64_t val = ((temp >> 8) & 0xff) * 1000000;
		if (temp & 0x80)
			val += 500000;
		if (temp & 0x02)
			val = -val;
		edata->value_cur = val + 273150000;
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}
}

static int
tegra_soctherm_decodeint(uint32_t val, uint32_t bitmask)
{
	const uint32_t v = __SHIFTOUT(val, bitmask);
	const int bits = popcount32(bitmask);
	int ret = v << (32 - bits);
	return ret >> (32 - bits);
}

static int64_t
tegra_soctherm_divide(int64_t num, int64_t denom)
{
	int64_t ret = ((num << 16) * 2 + 1) / (2 * denom);
	return ret >> 16;
}
