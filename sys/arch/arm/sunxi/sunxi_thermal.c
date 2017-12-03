/* $NetBSD: sunxi_thermal.c,v 1.3.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

/*-
 * Copyright (c) 2016-2017 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Allwinner thermal sensor controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_thermal.c,v 1.3.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_sid.h>

#define	THS_CTRL0		0x00
#define	THS_CTRL1		0x04
#define	 ADC_CALI_EN		(1 << 17)
#define	THS_CTRL2		0x40
#define	 SENSOR_ACQ1_SHIFT	16
#define	 SENSOR2_EN		(1 << 2)
#define	 SENSOR1_EN		(1 << 1)
#define	 SENSOR0_EN		(1 << 0)
#define	THS_INTC		0x44
#define	THS_INTS		0x48
#define	 THS2_DATA_IRQ_STS	(1 << 10)
#define	 THS1_DATA_IRQ_STS	(1 << 9)
#define	 THS0_DATA_IRQ_STS	(1 << 8)
#define	 SHUT_INT2_STS		(1 << 6)
#define	 SHUT_INT1_STS		(1 << 5)
#define	 SHUT_INT0_STS		(1 << 4)
#define	 ALARM_INT2_STS		(1 << 2)
#define	 ALARM_INT1_STS		(1 << 1)
#define	 ALARM_INT0_STS		(1 << 0)
#define	THS_ALARM0_CTRL		0x50
#define	 ALARM_T_HOT_MASK	0xfff
#define	 ALARM_T_HOT_SHIFT	16
#define	 ALARM_T_HYST_MASK	0xfff
#define	 ALARM_T_HYST_SHIFT	0
#define	THS_SHUTDOWN0_CTRL	0x60
#define	 SHUT_T_HOT_MASK	0xfff
#define	 SHUT_T_HOT_SHIFT	16
#define	THS_FILTER		0x70
#define	THS_CALIB0		0x74
#define	THS_CALIB1		0x78
#define	THS_DATA0		0x80
#define	THS_DATA1		0x84
#define	THS_DATA2		0x88
#define	 DATA_MASK		0xfff

#define	A83T_ADC_ACQUIRE_TIME	0x17
#define	A83T_FILTER		0x4
#define	A83T_INTC		0x1000
#define	A83T_TEMP_BASE		2719000
#define	A83T_TEMP_MUL		1000
#define	A83T_TEMP_DIV		14186
#define	A83T_CLK_RATE		24000000

#define	A64_ADC_ACQUIRE_TIME	0x190
#define	A64_FILTER		0x6
#define	A64_INTC		0x18000
#define	A64_TEMP_BASE		2170000
#define	A64_TEMP_MUL		1000
#define	A64_TEMP_DIV		8560
#define	A64_CLK_RATE		4000000

#define	H3_ADC_ACQUIRE_TIME	0x3f
#define	H3_FILTER		0x6
#define	H3_INTC			0x191000
#define	H3_TEMP_BASE		217
#define	H3_TEMP_MUL		1000
#define	H3_TEMP_DIV		8253
#define	H3_TEMP_MINUS		1794000
#define	H3_CLK_RATE		4000000
#define	H3_INIT_ALARM		90	/* degC */
#define	H3_INIT_SHUT		105	/* degC */

#define	TEMP_C_TO_K		273150000
#define	SENSOR_ENABLE_ALL	(SENSOR0_EN|SENSOR1_EN|SENSOR2_EN)
#define	SHUT_INT_ALL		(SHUT_INT0_STS|SHUT_INT1_STS|SHUT_INT2_STS)
#define	ALARM_INT_ALL		(ALARM_INT0_STS)

#define	MAX_SENSORS	3

#if notyet
#define	THROTTLE_ENABLE_DEFAULT	1

/* Enable thermal throttling */
static int sunxi_thermal_throttle_enable = THROTTLE_ENABLE_DEFAULT;
#endif

struct sunxi_thermal_sensor {
	const char		*name;
	const char		*desc;
	int			init_alarm;
	int			init_shut;
};

struct sunxi_thermal_config {
	struct sunxi_thermal_sensor	sensors[MAX_SENSORS];
	int				nsensors;
	uint64_t			clk_rate;
	uint32_t			adc_acquire_time;
	int				adc_cali_en;
	uint32_t			filter;
	uint32_t			intc;
	int				(*to_temp)(uint32_t);
	uint32_t			(*to_reg)(int);
	int				temp_base;
	int				temp_mul;
	int				temp_div;
	int				calib0, calib1;
	uint32_t			calib0_mask, calib1_mask;
};

static int
a83t_to_temp(uint32_t val)
{
	return ((A83T_TEMP_BASE - (val * A83T_TEMP_MUL)) / A83T_TEMP_DIV);
}

static const struct sunxi_thermal_config a83t_config = {
	.nsensors = 3,
	.sensors = {
		[0] = {
			.name = "cluster0",
			.desc = "CPU cluster 0 temperature",
		},
		[1] = {
			.name = "cluster1",
			.desc = "CPU cluster 1 temperature",
		},
		[2] = {
			.name = "gpu",
			.desc = "GPU temperature",
		},
	},
	.clk_rate = A83T_CLK_RATE,
	.adc_acquire_time = A83T_ADC_ACQUIRE_TIME,
	.adc_cali_en = 1,
	.filter = A83T_FILTER,
	.intc = A83T_INTC,
	.to_temp = a83t_to_temp,
	.calib0_mask = 0xffffffff,
	.calib1_mask = 0xffffffff,
};

static int
a64_to_temp(uint32_t val)
{
	return ((A64_TEMP_BASE - (val * A64_TEMP_MUL)) / A64_TEMP_DIV);
}

static const struct sunxi_thermal_config a64_config = {
	.nsensors = 3,
	.sensors = {
		[0] = {
			.name = "cpu",
			.desc = "CPU temperature",
		},
		[1] = {
			.name = "gpu1",
			.desc = "GPU temperature 1",
		},
		[2] = {
			.name = "gpu2",
			.desc = "GPU temperature 2",
		},
	},
	.clk_rate = A64_CLK_RATE,
	.adc_acquire_time = A64_ADC_ACQUIRE_TIME,
	.filter = A64_FILTER,
	.intc = A64_INTC,
	.to_temp = a64_to_temp,
};

static int
h3_to_temp(uint32_t val)
{
	return (H3_TEMP_BASE - ((val * H3_TEMP_MUL) / H3_TEMP_DIV));
}

static uint32_t
h3_to_reg(int val)
{
	return ((H3_TEMP_MINUS - (val * H3_TEMP_DIV)) / H3_TEMP_MUL);
}

static const struct sunxi_thermal_config h3_config = {
	.nsensors = 1,
	.sensors = {
		[0] = {
			.name = "cpu",
			.desc = "CPU temperature",
			.init_alarm = H3_INIT_ALARM,
			.init_shut = H3_INIT_SHUT,
		},
	},
	.clk_rate = H3_CLK_RATE,
	.adc_acquire_time = H3_ADC_ACQUIRE_TIME,
	.filter = H3_FILTER,
	.intc = H3_INTC,
	.to_temp = h3_to_temp,
	.to_reg = h3_to_reg,
	.calib0_mask = 0xfff,
};

static struct of_compat_data compat_data[] = {
	{ "allwinner,sun8i-a83t-ts",	(uintptr_t)&a83t_config },
	{ "allwinner,sun8i-h3-ts",	(uintptr_t)&h3_config },
	{ "allwinner,sun50i-a64-ts",	(uintptr_t)&a64_config },
	{ NULL,				(uintptr_t)NULL }
};

struct sunxi_thermal_softc {
	device_t			dev;
	int				phandle;
	bus_space_tag_t			bst;
	bus_space_handle_t		bsh;
	struct sunxi_thermal_config	*conf;

	kmutex_t			lock;
	callout_t			tick;

	struct sysmon_envsys		*sme;
	envsys_data_t			data[MAX_SENSORS];
};

#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->bst, (sc)->bsh, (reg))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->bst, (sc)->bsh, (reg), (val))

static int
sunxi_thermal_init(struct sunxi_thermal_softc *sc)
{
	uint32_t calib[2];
	int error;

	if (sc->conf->calib0_mask != 0 || sc->conf->calib1_mask != 0) {
		/* Read calibration settings from SRAM */
		error = sunxi_sid_read_tscalib(calib);
		if (error != 0)
			return error;

		calib[0] &= sc->conf->calib0_mask;
		calib[1] &= sc->conf->calib1_mask;

		/* Write calibration settings to thermal controller */
		if (calib[0] != 0)
			WR4(sc, THS_CALIB0, calib[0]);
		if (calib[1] != 0)
			WR4(sc, THS_CALIB1, calib[1]);
	}

	/* Configure ADC acquire time (CLK_IN/(N+1)) and enable sensors */
	WR4(sc, THS_CTRL1, ADC_CALI_EN);
	WR4(sc, THS_CTRL0, sc->conf->adc_acquire_time);
	WR4(sc, THS_CTRL2, sc->conf->adc_acquire_time << SENSOR_ACQ1_SHIFT);

	/* Enable average filter */
	WR4(sc, THS_FILTER, sc->conf->filter);

	/* Enable interrupts */
	WR4(sc, THS_INTS, RD4(sc, THS_INTS));
	WR4(sc, THS_INTC, sc->conf->intc | SHUT_INT_ALL | ALARM_INT_ALL);

	/* Enable sensors */
	WR4(sc, THS_CTRL2, RD4(sc, THS_CTRL2) | SENSOR_ENABLE_ALL);

	return 0;
}

static int
sunxi_thermal_gettemp(struct sunxi_thermal_softc *sc, int sensor)
{
	uint32_t val;

	val = RD4(sc, THS_DATA0 + (sensor * 4));

	return sc->conf->to_temp(val);
}

static int
sunxi_thermal_getshut(struct sunxi_thermal_softc *sc, int sensor)
{
	uint32_t val;

	val = RD4(sc, THS_SHUTDOWN0_CTRL + (sensor * 4));
	val = (val >> SHUT_T_HOT_SHIFT) & SHUT_T_HOT_MASK;

	return sc->conf->to_temp(val);
}

static void
sunxi_thermal_setshut(struct sunxi_thermal_softc *sc, int sensor, int temp)
{
	uint32_t val;

	val = RD4(sc, THS_SHUTDOWN0_CTRL + (sensor * 4));
	val &= ~(SHUT_T_HOT_MASK << SHUT_T_HOT_SHIFT);
	val |= (sc->conf->to_reg(temp) << SHUT_T_HOT_SHIFT);
	WR4(sc, THS_SHUTDOWN0_CTRL + (sensor * 4), val);
}

static int
sunxi_thermal_gethyst(struct sunxi_thermal_softc *sc, int sensor)
{
	uint32_t val;

	val = RD4(sc, THS_ALARM0_CTRL + (sensor * 4));
	val = (val >> ALARM_T_HYST_SHIFT) & ALARM_T_HYST_MASK;

	return sc->conf->to_temp(val);
}

static int
sunxi_thermal_getalarm(struct sunxi_thermal_softc *sc, int sensor)
{
	uint32_t val;

	val = RD4(sc, THS_ALARM0_CTRL + (sensor * 4));
	val = (val >> ALARM_T_HOT_SHIFT) & ALARM_T_HOT_MASK;

	return sc->conf->to_temp(val);
}

static void
sunxi_thermal_setalarm(struct sunxi_thermal_softc *sc, int sensor, int temp)
{
	uint32_t val;

	val = RD4(sc, THS_ALARM0_CTRL + (sensor * 4));
	val &= ~(ALARM_T_HOT_MASK << ALARM_T_HOT_SHIFT);
	val |= (sc->conf->to_reg(temp) << ALARM_T_HOT_SHIFT);
	WR4(sc, THS_ALARM0_CTRL + (sensor * 4), val);
}

static void
sunxi_thermal_task_shut(void *arg)
{
	struct sunxi_thermal_softc * const sc = arg;

	device_printf(sc->dev,
	    "WARNING - current temperature exceeds safe limits\n");

	cpu_reboot(RB_POWERDOWN, NULL);
}

static void
sunxi_thermal_task_alarm(void *arg)
{
	struct sunxi_thermal_softc * const sc = arg;

	const int alarm_val = sunxi_thermal_getalarm(sc, 0);
	const int temp_val = sunxi_thermal_gettemp(sc, 0);

	if (temp_val < alarm_val)
		pmf_event_inject(NULL, PMFE_THROTTLE_DISABLE);
	else
		callout_schedule(&sc->tick, hz);
}

static void
sunxi_thermal_tick(void *arg)
{
	struct sunxi_thermal_softc * const sc = arg;

	sysmon_task_queue_sched(0, sunxi_thermal_task_alarm, sc);
}

static int
sunxi_thermal_intr(void *arg)
{
	struct sunxi_thermal_softc * const sc = arg;
	uint32_t ints;

	mutex_enter(&sc->lock);

	ints = RD4(sc, THS_INTS);
	WR4(sc, THS_INTS, ints);

	if ((ints & SHUT_INT_ALL) != 0)
		sysmon_task_queue_sched(0, sunxi_thermal_task_shut, sc);

	if ((ints & ALARM_INT_ALL) != 0) {
		pmf_event_inject(NULL, PMFE_THROTTLE_ENABLE);
		sysmon_task_queue_sched(0, sunxi_thermal_task_alarm, sc);
	}

	mutex_exit(&sc->lock);

	return 1;
}

static void
sunxi_thermal_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct sunxi_thermal_softc * const sc = sme->sme_cookie;

	const int64_t temp = sunxi_thermal_gettemp(sc, edata->private);

	edata->value_cur = temp * 1000000 + TEMP_C_TO_K;
	edata->state = ENVSYS_SVALID;
}

static int
sunxi_thermal_init_clocks(struct sunxi_thermal_softc *sc)
{
	struct fdtbus_reset *rst;
	struct clk *clk;
	int error;

	clk = fdtbus_clock_get(sc->phandle, "ahb");
	if (clk == NULL)
		return ENXIO;
	error = clk_enable(clk);
	if (error != 0)
		return error;

	clk = fdtbus_clock_get(sc->phandle, "ths");
	if (clk == NULL)
		return ENXIO;
	error = clk_set_rate(clk, sc->conf->clk_rate);
	if (error != 0)
		return error;
	error = clk_enable(clk);
	if (error != 0)
		return error;

	rst = fdtbus_reset_get_index(sc->phandle, 0);
	if (rst == NULL)
		return ENXIO;
	error = fdtbus_reset_deassert(rst);
	if (error != 0)
		return error;

	return 0;
}

static int
sunxi_thermal_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_thermal_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_thermal_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	void *ih;
	int i;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->dev = self;
	sc->phandle = phandle;
	sc->bst = faa->faa_bst;
	sc->conf = (void *)of_search_compatible(phandle, compat_data)->data;
	if (bus_space_map(sc->bst, addr, size, 0, &sc->bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->lock, MUTEX_DEFAULT, IPL_VM);
	callout_init(&sc->tick, CALLOUT_MPSAFE);
	callout_setfunc(&sc->tick, sunxi_thermal_tick, sc);

	if (sunxi_thermal_init_clocks(sc) != 0) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Thermal sensor controller\n");

	ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    sunxi_thermal_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	for (i = 0; i < sc->conf->nsensors; i++) {
		if (sc->conf->sensors[i].init_alarm > 0)
			sunxi_thermal_setalarm(sc, i,
			    sc->conf->sensors[i].init_alarm);
		if (sc->conf->sensors[i].init_shut > 0)
			sunxi_thermal_setshut(sc, i,
			    sc->conf->sensors[i].init_shut);
	}

	if (sunxi_thermal_init(sc) != 0) {
		aprint_error_dev(self, "failed to initialize sensors\n");
		return;
	}

	sc->sme = sysmon_envsys_create();
	sc->sme->sme_name = device_xname(self);
	sc->sme->sme_cookie = sc;
	sc->sme->sme_refresh = sunxi_thermal_refresh;
	for (i = 0; i < sc->conf->nsensors; i++) {
		sc->data[i].private = i;
		sc->data[i].units = ENVSYS_STEMP;
		sc->data[i].state = ENVSYS_SINVALID;
		strlcpy(sc->data[1].desc, sc->conf->sensors[i].desc,
		    sizeof(sc->data[1].desc));
		sysmon_envsys_sensor_attach(sc->sme, &sc->data[i]);
	}
	sysmon_envsys_register(sc->sme);

	for (i = 0; i < sc->conf->nsensors; i++) {
		device_printf(self,
		    "%s: alarm %dC hyst %dC shut %dC\n",
		    sc->conf->sensors[i].name,
		    sunxi_thermal_getalarm(sc, i),
		    sunxi_thermal_gethyst(sc, i),
		    sunxi_thermal_getshut(sc, i));
	}
}

CFATTACH_DECL_NEW(sunxi_thermal, sizeof(struct sunxi_thermal_softc),
    sunxi_thermal_match, sunxi_thermal_attach, NULL, NULL);
