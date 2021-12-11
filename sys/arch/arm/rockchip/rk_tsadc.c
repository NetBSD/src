/*	$NetBSD: rk_tsadc.c,v 1.16 2021/12/11 19:24:21 mrg Exp $	*/

/*
 * Copyright (c) 2019 Matthew R. Green
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

__KERNEL_RCSID(0, "$NetBSD: rk_tsadc.c,v 1.16 2021/12/11 19:24:21 mrg Exp $");

/*
 * Driver for the TSADC temperature sensor monitor in RK3328 and RK3399.
 *
 * TODO:
 * - handle setting various temp values
 * - handle DT trips/temp value defaults
 * - interrupts aren't triggered (test by lowering warn/crit values), and
 *   once they work, make the interrupt do something
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <dev/sysmon/sysmonvar.h>

#ifdef RKTSADC_DEBUG
#define DPRINTF(fmt, ...) \
	printf("%s:%d: " fmt "\n", __func__, __LINE__, ## __VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

/* Register definitions */
#define TSADC_USER_CON                          0x00
#define  TSADC_USER_CON_ADC_STATUS              __BIT(12)
#define  TSADC_USER_CON_INTER_PD_SOC            __BITS(11,6)
#define  TSADC_USER_CON_START                   __BIT(5)
#define  TSADC_USER_CON_START_MODE              __BIT(4)
#define  TSADC_USER_CON_ADC_POWER_CTRL          __BIT(3)
#define  TSADC_USER_CON_ADC_INPUT_SRC_SEL       __BITS(2,0)
#define TSADC_AUTO_CON                          0x04
#define  TSADC_AUTO_CON_LAST_TSHUT_2CRU         __BIT(25)
#define  TSADC_AUTO_CON_LAST_TSHUT_2GPIO        __BIT(24)
#define  TSADC_AUTO_CON_SAMPLE_DLY_SEL          __BIT(17)
#define  TSADC_AUTO_CON_AUTO_STATUS             __BIT(16)
#define  TSADC_AUTO_CON_SRC1_LT_EN              __BIT(13)
#define  TSADC_AUTO_CON_SRC0_LT_EN              __BIT(12)
#define  TSADC_AUTO_CON_TSHUT_POLARITY          __BIT(8)
#define  TSADC_AUTO_CON_SRC1_EN                 __BIT(5)
#define  TSADC_AUTO_CON_SRC0_EN                 __BIT(4)
#define  TSADC_AUTO_CON_Q_SEL                   __BIT(1)
#define  TSADC_AUTO_CON_AUTO_EN                 __BIT(0)
#define TSADC_INT_EN                            0x08
#define  TSADC_INT_EN_EOC_INT_EN                __BIT(16)
#define  TSADC_INT_EN_LT_INTEN_SRC1             __BIT(13)
#define  TSADC_INT_EN_LT_INTEN_SRC0             __BIT(12)
#define  TSADC_INT_EN_TSHUT_2CRU_EN_SRC1        __BIT(9)
#define  TSADC_INT_EN_TSHUT_2CRU_EN_SRC0        __BIT(8)
#define  TSADC_INT_EN_TSHUT_2GPIO_EN_SRC1       __BIT(5)
#define  TSADC_INT_EN_TSHUT_2GPIO_EN_SRC0       __BIT(4)
#define  TSADC_INT_EN_HT_INTEN_SRC1             __BIT(1)
#define  TSADC_INT_EN_HT_INTEN_SRC0             __BIT(0)
#define TSADC_INT_PD                            0x0c
#define  TSADC_INT_PD_EOC_INT_PD_V3             __BIT(16)
#define  TSADC_INT_PD_LT_IRQ_SRC1               __BIT(13)
#define  TSADC_INT_PD_LT_IRQ_SRC0               __BIT(12)
#define  TSADC_INT_PD_EOC_INT_PD_V2             __BIT(8)
#define  TSADC_INT_PD_TSHUT_O_SRC1              __BIT(5)
#define  TSADC_INT_PD_TSHUT_O_SRC0              __BIT(4)
#define  TSADC_INT_PD_HT_IRQ_SRC1               __BIT(1)
#define  TSADC_INT_PD_HT_IRQ_SRC0               __BIT(0)
#define TSADC_DATA0                             0x20
#define  TSADC_DATA0_ADC_DATA                   __BITS(11,0)
#define TSADC_DATA1                             0x24
#define  TSADC_DATA1_ADC_DATA                   __BITS(11,0)
#define TSADC_COMP0_INT                         0x30
#define  TSADC_COMP0_INT_COMP_SRC0              __BITS(11,0)
#define TSADC_COMP1_INT                         0x34
#define  TSADC_COMP1_INT_COMP_SRC1              __BITS(11,0)
#define TSADC_COMP0_SHUT                        0x40
#define  TSADC_COMP0_SHUT_COMP_SRC0             __BITS(11,0)
#define TSADC_COMP1_SHUT                        0x44
#define  TSADC_COMP1_SHUT_COMP_SRC1             __BITS(11,0)
#define TSADC_HIGH_INT_DEBOUNCE                 0x60
#define  TSADC_HIGH_INT_DEBOUNCE_TEMP           __BITS(7,0)
#define TSADC_HIGH_TSHUT_DEBOUNCE               0x64
#define  TSADC_HIGH_TSHUT_DEBOUNCE_TEMP         __BITS(7,0)
#define TSADC_AUTO_PERIOD                       0x68
#define  TSADC_AUTO_PERIOD_TEMP                 __BITS(31,0)
#define TSADC_AUTO_PERIOD_HT                    0x6c
#define  TSADC_AUTO_PERIOD_HT_TEMP              __BITS(31,0)
#define TSADC_COMP0_LOW_INT                     0x80
#define  TSADC_COMP0_LOW_INT_COMP_SRC0          __BITS(11,0)
#define TSADC_COMP1_LOW_INT                     0x84
#define  TSADC_COMP1_LOW_INT_COMP_SRC1          __BITS(11,0)

#define	RK3288_TSADC_AUTO_PERIOD_TIME		250 /* 250ms */
#define	RK3288_TSADC_AUTO_PERIOD_HT_TIME	50  /* 50ms */
#define RK3328_TSADC_AUTO_PERIOD_TIME           250 /* 250ms */
#define RK3399_TSADC_AUTO_PERIOD_TIME           1875 /* 2.5ms */
#define TSADC_HT_DEBOUNCE_COUNT                 4

/*
 * All this magic is taking from the Linux rockchip_thermal driver.
 *
 * VCM means "voltage common mode", but the documentation for RK3399
 * does not mention this and I don't know what any of this really
 * is for.
 */
#define RK3399_GRF_SARADC_TESTBIT               0xe644
#define  RK3399_GRF_SARADC_TESTBIT_ON           (0x10001 << 2)
#define RK3399_GRF_TSADC_TESTBIT_L              0xe648
#define  RK3399_GRF_TSADC_TESTBIT_VCM_EN_L      (0x10001 << 7)
#define RK3399_GRF_TSADC_TESTBIT_H              0xe64c
#define  RK3399_GRF_TSADC_TESTBIT_VCM_EN_H      (0x10001 << 7)
#define  RK3399_GRF_TSADC_TESTBIT_H_ON          (0x10001 << 2)

#define TEMP_uC_TO_uK             273150000

#define TSHUT_MODE_CPU    0
#define TSHUT_MODE_GPIO   1

#define TSHUT_LOW_ACTIVE  0
#define TSHUT_HIGH_ACTIVE 1

#define TSHUT_DEF_TEMP    95000

#define TSADC_DATA_MAX    0xfff

#define MAX_SENSORS       2

typedef struct rk_data_array {
	uint32_t data;  /* register value */
	int temp;       /* micro-degC */
} rk_data_array;

struct rk_tsadc_softc;
typedef struct rk_data {
	const char		*rd_name;
	const rk_data_array	*rd_array;
	size_t			 rd_size;
	void			(*rd_init)(struct rk_tsadc_softc *, int, int);
	bool			 rd_decr;  /* lower values -> higher temp */
	unsigned		 rd_min, rd_max;
	unsigned		 rd_auto_period;
	unsigned		 rd_auto_period_ht;
	unsigned		 rd_num_sensors;
	unsigned		 rd_version;
} rk_data;

/* Per-sensor data */
struct rk_tsadc_sensor {
	envsys_data_t	s_data;
	bool		s_attached;
	/* TSADC register offsets for this sensor */
	unsigned	s_data_reg;
	unsigned	s_comp_tshut;
	unsigned	s_comp_int;
	/* enable bit in AUTO_CON register */
	unsigned	s_comp_int_en;
	/* warn/crit values in micro Kelvin */
	int		s_warn;
	int		s_tshut;
};

struct rk_tsadc_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	size_t			sc_size;
	uint32_t		sc_data_mask;
	void			*sc_ih;

	struct sysmon_envsys	*sc_sme;
	struct rk_tsadc_sensor	sc_sensors[MAX_SENSORS];

	struct clk		*sc_clock;
	struct clk		*sc_clockapb;
	struct fdtbus_reset	*sc_reset;
	struct syscon		*sc_syscon;

	const rk_data		*sc_rd;
};

static int rk_tsadc_match(device_t, cfdata_t, void *);
static void rk_tsadc_attach(device_t, device_t, void *);
static int rk_tsadc_detach(device_t, int);
static int rk_tsadc_init_clocks(struct rk_tsadc_softc *);
static void rk_tsadc_init_counts(struct rk_tsadc_softc *);
static void rk_tsadc_tshut_set(struct rk_tsadc_softc *s);
static void rk_tsadc_init_tshut(struct rk_tsadc_softc *, int, int);
static void rk_tsadc_init_common(struct rk_tsadc_softc *, int, int);
static void rk_tsadc_init_rk3399(struct rk_tsadc_softc *, int, int);
static void rk_tsadc_init_enable(struct rk_tsadc_softc *);
static void rk_tsadc_init(struct rk_tsadc_softc *, int, int);
static void rk_tsadc_refresh(struct sysmon_envsys *, envsys_data_t *);
static void rk_tsadc_get_limits(struct sysmon_envsys *, envsys_data_t *,
                                sysmon_envsys_lim_t *, uint32_t *);

static int rk_tsadc_intr(void *);
static int rk_tsadc_data_to_temp(struct rk_tsadc_softc *, uint32_t);
static uint32_t rk_tsadc_temp_to_data(struct rk_tsadc_softc *, int);

/* RK3328/RK3399 compatible sensors */
static const struct rk_tsadc_sensor rk_tsadc_sensors[] = {
	{
	  .s_data = { .desc = "CPU" },
	  .s_data_reg = TSADC_DATA0,
	  .s_comp_tshut = TSADC_COMP0_SHUT,
	  .s_comp_int = TSADC_COMP0_INT,
	  .s_comp_int_en = TSADC_AUTO_CON_SRC0_EN,
	  /*
	   * XXX DT has:
	   * cpu_alert1: cpu_alert1 {
	   *	temperature = <75000>;
	   *	hysteresis = <2000>;
	   * cpu_crit: cpu_crit {
	   *    temperature = <95000>;
	   *    hysteresis = <2000>;
	   * pull out of here?
	   * do something with hysteresis?  put in debounce?
	   * 
	   * Note that tshut may be overridden by the board specific DT.
	   */
	  .s_warn = 75000000,
	  .s_tshut = 95000000,
	}, {
	  .s_data = { .desc = "GPU" },
	  .s_data_reg = TSADC_DATA1,
	  .s_comp_tshut = TSADC_COMP1_SHUT,
	  .s_comp_int = TSADC_COMP1_INT,
	  .s_comp_int_en = TSADC_AUTO_CON_SRC1_EN,
	  .s_warn = 75000000,
	  .s_tshut = 95000000,
	},
};

/*
 * Table from RK3288 manual.
 */
static const rk_data_array rk3288_data_array[] = {
#define ENTRY(d,C)	{ .data = (d), .temp = (C) * 1000 * 1000, }
	ENTRY(TSADC_DATA_MAX, -40),
	ENTRY(3800, -40),
	ENTRY(3792, -35),
	ENTRY(3783, -30),
	ENTRY(3774, -25),
	ENTRY(3765, -20),
	ENTRY(3756, -15),
	ENTRY(3747, -10),
	ENTRY(3737, -5),
	ENTRY(3728, 0),
	ENTRY(3718, 5),
	ENTRY(3708, 10),
	ENTRY(3698, 15),
	ENTRY(3688, 20),
	ENTRY(3678, 25),
	ENTRY(3667, 30),
	ENTRY(3656, 35),
	ENTRY(3645, 40),
	ENTRY(3634, 45),
	ENTRY(3623, 50),
	ENTRY(3611, 55),
	ENTRY(3600, 60),
	ENTRY(3588, 65),
	ENTRY(3575, 70),
	ENTRY(3563, 75),
	ENTRY(3550, 80),
	ENTRY(3537, 85),
	ENTRY(3524, 90),
	ENTRY(3510, 95),
	ENTRY(3496, 100),
	ENTRY(3482, 105),
	ENTRY(3467, 110),
	ENTRY(3452, 115),
	ENTRY(3437, 120),
	ENTRY(3421, 125),
	ENTRY(0, 15),
#undef ENTRY
};

/*
 * Table from RK3328 manual.  Note that the manual lists valid numbers as
 * 4096 - number.  This also means it is increasing not decreasing for
 * higher temps, and the min and max are also offset from 4096.
 */
#define RK3328_DATA_OFFSET (4096)
static const rk_data_array rk3328_data_array[] = {
#define ENTRY(d,C) \
	{ .data = RK3328_DATA_OFFSET - (d), .temp = (C) * 1000 * 1000, }
	ENTRY(TSADC_DATA_MAX,    -40),
	ENTRY(3800, -40),
	ENTRY(3792, -35),
	ENTRY(3783, -30),
	ENTRY(3774, -25),
	ENTRY(3765, -20),
	ENTRY(3756, -15),
	ENTRY(3747, -10),
	ENTRY(3737,  -5),
	ENTRY(3728,   0),
	ENTRY(3718,   5),
	ENTRY(3708,  10),
	ENTRY(3698,  15),
	ENTRY(3688,  20),
	ENTRY(3678,  25),
	ENTRY(3667,  30),
	ENTRY(3656,  35),
	ENTRY(3645,  40),
	ENTRY(3634,  45),
	ENTRY(3623,  50),
	ENTRY(3611,  55),
	ENTRY(3600,  60),
	ENTRY(3588,  65),
	ENTRY(3575,  70),
	ENTRY(3563,  75),
	ENTRY(3550,  80),
	ENTRY(3537,  85),
	ENTRY(3524,  90),
	ENTRY(3510,  95),
	ENTRY(3496, 100),
	ENTRY(3482, 105),
	ENTRY(3467, 110),
	ENTRY(3452, 115),
	ENTRY(3437, 120),
	ENTRY(3421, 125),
	ENTRY(0,    125),
#undef ENTRY
};

/* Table from RK3399 manual */
static const rk_data_array rk3399_data_array[] = {
#define ENTRY(d,C)	{ .data = (d), .temp = (C) * 1000 * 1000, }
	ENTRY(0,   -40),
	ENTRY(402, -40),
	ENTRY(410, -35),
	ENTRY(419, -30),
	ENTRY(427, -25),
	ENTRY(436, -20),
	ENTRY(444, -15),
	ENTRY(453, -10),
	ENTRY(461,  -5),
	ENTRY(470,   0),
	ENTRY(478,   5),
	ENTRY(487,  10),
	ENTRY(496,  15),
	ENTRY(504,  20),
	ENTRY(513,  25),
	ENTRY(521,  30),
	ENTRY(530,  35),
	ENTRY(538,  40),
	ENTRY(547,  45),
	ENTRY(555,  50),
	ENTRY(564,  55),
	ENTRY(573,  60),
	ENTRY(581,  65),
	ENTRY(590,  70),
	ENTRY(599,  75),
	ENTRY(607,  80),
	ENTRY(616,  85),
	ENTRY(624,  90),
	ENTRY(633,  95),
	ENTRY(642, 100),
	ENTRY(650, 105),
	ENTRY(659, 110),
	ENTRY(668, 115),
	ENTRY(677, 120),
	ENTRY(685, 125),
	ENTRY(TSADC_DATA_MAX, 125),
#undef ENTRY
};

static const rk_data rk3288_data_table = {
	.rd_name = "RK3288",
	.rd_array = rk3288_data_array,
	.rd_size = __arraycount(rk3288_data_array),
	.rd_init = rk_tsadc_init_common,
	.rd_decr = true,
	.rd_max = 3800,
	.rd_min = 3421,
	.rd_auto_period = RK3288_TSADC_AUTO_PERIOD_TIME,
	.rd_auto_period_ht = RK3288_TSADC_AUTO_PERIOD_HT_TIME,
	.rd_num_sensors = 2,
	.rd_version = 2,
};

static const rk_data rk3328_data_table = {
	.rd_name = "RK3328",
	.rd_array = rk3328_data_array,
	.rd_size = __arraycount(rk3328_data_array),
	.rd_init = rk_tsadc_init_common,
	.rd_decr = false,
	.rd_max = RK3328_DATA_OFFSET - 3420,
	.rd_min = RK3328_DATA_OFFSET - 3801,
	.rd_auto_period = RK3328_TSADC_AUTO_PERIOD_TIME,
	.rd_auto_period_ht = RK3328_TSADC_AUTO_PERIOD_TIME,
	.rd_num_sensors = 1,
	.rd_version = 3,
};

static const rk_data rk3399_data_table = {
	.rd_name = "RK3399",
	.rd_array = rk3399_data_array,
	.rd_size = __arraycount(rk3399_data_array),
	.rd_init = rk_tsadc_init_rk3399,
	.rd_decr = false,
	.rd_max = 686,
	.rd_min = 401,
	.rd_auto_period = RK3399_TSADC_AUTO_PERIOD_TIME,
	.rd_auto_period_ht = RK3399_TSADC_AUTO_PERIOD_TIME,
	.rd_num_sensors = 2,
	.rd_version = 3,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3288-tsadc",	.data = &rk3288_data_table },
	{ .compat = "rockchip,rk3328-tsadc",	.data = &rk3328_data_table },
	{ .compat = "rockchip,rk3399-tsadc",	.data = &rk3399_data_table },
	DEVICE_COMPAT_EOL
};

#define	TSADC_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	TSADC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL3_NEW(rk_tsadc, sizeof(struct rk_tsadc_softc),
	rk_tsadc_match, rk_tsadc_attach, rk_tsadc_detach, NULL, NULL, NULL,
	DVF_DETACH_SHUTDOWN);

/* init/teardown support */
static int
rk_tsadc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_tsadc_attach(device_t parent, device_t self, void *aux)
{
	struct rk_tsadc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	int mode, polarity, tshut_temp;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_sme = sysmon_envsys_create();

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = rk_tsadc_refresh;
	sc->sc_sme->sme_get_limits = rk_tsadc_get_limits;
	sc->sc_data_mask = TSADC_DATA_MAX;

	pmf_device_register(self, NULL, NULL);

	sc->sc_rd = of_compatible_lookup(faa->faa_phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": %s Temperature Sensor ADC\n", sc->sc_rd->rd_name);

	/* Default to tshut via gpio and tshut low is active */
	if (of_getprop_uint32(phandle, "rockchip,hw-tshut-mode",
			      &mode) != 0) {
		aprint_error(": could not get TSHUT mode, default to GPIO");
		mode = TSHUT_MODE_GPIO;
	}
	if (mode != TSHUT_MODE_CPU && mode != TSHUT_MODE_GPIO) {
		aprint_error(": TSHUT mode should be 0 or 1\n");
		goto fail;
	}

	if (of_getprop_uint32(phandle, "rockchip,hw-tshut-polarity",
			      &polarity) != 0) {
		aprint_error(": could not get TSHUT polarity, default to low");
		polarity = TSHUT_LOW_ACTIVE;
	}
	if (of_getprop_uint32(phandle,
			      "rockchip,hw-tshut-temp", &tshut_temp) != 0) {
		aprint_error(": could not get TSHUT temperature, default to %u",
			     TSHUT_DEF_TEMP);
		tshut_temp = TSHUT_DEF_TEMP;
	}
	tshut_temp *= 1000;	/* convert fdt mK -> uK */

	memcpy(sc->sc_sensors, rk_tsadc_sensors, sizeof(sc->sc_sensors));
	for (unsigned n = 0; n < sc->sc_rd->rd_num_sensors; n++) {
		struct rk_tsadc_sensor *rks = &sc->sc_sensors[n];

		rks->s_data.flags = ENVSYS_FMONLIMITS;
		rks->s_data.units = ENVSYS_STEMP;
		rks->s_data.state = ENVSYS_SINVALID;

		if (sysmon_envsys_sensor_attach(sc->sc_sme, &rks->s_data))
			goto fail;
		rks->s_attached = true;
		rks->s_tshut = tshut_temp;
#if 0
		// testing
		rks->s_tshut = 68000000;
		rks->s_warn = 61000000;
#endif
	}

	sc->sc_syscon = fdtbus_syscon_acquire(phandle, "rockchip,grf");
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get grf syscon\n");
		goto fail;
	}
	if (fdtbus_get_reg(phandle, 0, &addr, &sc->sc_size) != 0) {
		aprint_error(": couldn't get registers\n");
		sc->sc_size = 0;
		goto fail;
	}
	if (bus_space_map(sc->sc_bst, addr, sc->sc_size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		sc->sc_size = 0;
		goto fail;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		goto fail;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    rk_tsadc_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (rk_tsadc_init_clocks(sc)) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}

	/*
	 * Manual says to setup auto period (both), high temp (interrupt),
	 * high temp (shutdown), enable high temp resets (TSHUT to GPIO
	 * or reset chip), set the debounce times, and, finally, enable the
	 * controller iself.
	 */
	rk_tsadc_init(sc, mode, polarity);

	return;

fail:
	rk_tsadc_detach(self, 0);
}

static int
rk_tsadc_detach(device_t self, int flags)
{
	struct rk_tsadc_softc *sc = device_private(self);

	pmf_device_deregister(self);

	for (unsigned n = 0; n < sc->sc_rd->rd_num_sensors; n++) {
		struct rk_tsadc_sensor *rks = &sc->sc_sensors[n];

		if (rks->s_attached) {
			sysmon_envsys_sensor_detach(sc->sc_sme, &rks->s_data);
			rks->s_attached = false;
		}
	}

	sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_clockapb)
		clk_disable(sc->sc_clockapb);
	if (sc->sc_clock)
		clk_disable(sc->sc_clock);

	if (sc->sc_ih)
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);

	if (sc->sc_size)
		bus_space_unmap(sc->sc_bst, sc->sc_bsh, sc->sc_size);

	sysmon_envsys_destroy(sc->sc_sme);

	return 0;
}

static int
rk_tsadc_init_clocks(struct rk_tsadc_softc *sc)
{
	int error;

	fdtbus_clock_assign(sc->sc_phandle);

	sc->sc_reset = fdtbus_reset_get(sc->sc_phandle, "tsadc-apb");
	sc->sc_clock = fdtbus_clock_get(sc->sc_phandle, "tsadc");
	sc->sc_clockapb = fdtbus_clock_get(sc->sc_phandle, "apb_pclk");
	if (sc->sc_reset == NULL ||
	    sc->sc_clock == NULL ||
	    sc->sc_clockapb == NULL)
		return EINVAL;

	fdtbus_reset_assert(sc->sc_reset);

	error = clk_enable(sc->sc_clock);
	if (error) {
		fdtbus_reset_deassert(sc->sc_reset);
		return error;
	}

	error = clk_enable(sc->sc_clockapb);

	DELAY(20);
	fdtbus_reset_deassert(sc->sc_reset);

	return error;
}

static void
rk_tsadc_init_counts(struct rk_tsadc_softc *sc)
{

	TSADC_WRITE(sc, TSADC_AUTO_PERIOD, sc->sc_rd->rd_auto_period);
	TSADC_WRITE(sc, TSADC_AUTO_PERIOD_HT, sc->sc_rd->rd_auto_period_ht);
	TSADC_WRITE(sc, TSADC_HIGH_INT_DEBOUNCE, TSADC_HT_DEBOUNCE_COUNT);
	TSADC_WRITE(sc, TSADC_HIGH_TSHUT_DEBOUNCE, TSADC_HT_DEBOUNCE_COUNT);
}

/* Configure the hardware with the tshut setup. */
static void
rk_tsadc_tshut_set(struct rk_tsadc_softc *sc)
{
	uint32_t val = TSADC_READ(sc, TSADC_AUTO_CON);

	for (unsigned n = 0; n < sc->sc_rd->rd_num_sensors; n++) {
		struct rk_tsadc_sensor *rks = &sc->sc_sensors[n];
		uint32_t data, warndata;

		if (!rks->s_attached)
			continue;

		data = rk_tsadc_temp_to_data(sc, rks->s_tshut);
		warndata = rk_tsadc_temp_to_data(sc, rks->s_warn);

		DPRINTF("(%s:%s): tshut/data %d/%u warn/data %d/%u",
			sc->sc_sme->sme_name, rks->s_data.desc,
			rks->s_tshut, data,
			rks->s_warn, warndata);

		if (data == sc->sc_data_mask) {
			aprint_error_dev(sc->sc_dev,
			    "Failed converting critical temp %u.%06u to code",
			    rks->s_tshut / 1000000, rks->s_tshut % 1000000);
			continue;
		}
		if (warndata == sc->sc_data_mask) {
			aprint_error_dev(sc->sc_dev,
			    "Failed converting warn temp %u.%06u to code",
			    rks->s_warn / 1000000, rks->s_warn % 1000000);
			continue;
		}

		TSADC_WRITE(sc, rks->s_comp_tshut, data);
		TSADC_WRITE(sc, rks->s_comp_int, warndata);

		val |= rks->s_comp_int_en;
	}
	TSADC_WRITE(sc, TSADC_AUTO_CON, val);
}

static void
rk_tsadc_init_tshut(struct rk_tsadc_softc *sc, int mode, int polarity)
{
	uint32_t val;

	/* Handle TSHUT temp setting. */
	rk_tsadc_tshut_set(sc);

	/* Handle TSHUT mode setting. */
	val = TSADC_READ(sc, TSADC_INT_EN);
	if (mode == TSHUT_MODE_CPU) {
		val |= TSADC_INT_EN_TSHUT_2CRU_EN_SRC1 |
		       TSADC_INT_EN_TSHUT_2CRU_EN_SRC0;
		val &= ~(TSADC_INT_EN_TSHUT_2GPIO_EN_SRC1 |
			 TSADC_INT_EN_TSHUT_2GPIO_EN_SRC0);
	} else {
		KASSERT(mode == TSHUT_MODE_GPIO);
		val &= ~(TSADC_INT_EN_TSHUT_2CRU_EN_SRC1 |
			 TSADC_INT_EN_TSHUT_2CRU_EN_SRC0);
		val |= TSADC_INT_EN_TSHUT_2GPIO_EN_SRC1 |
		       TSADC_INT_EN_TSHUT_2GPIO_EN_SRC0;
	}
	TSADC_WRITE(sc, TSADC_INT_EN, val);

	/* Handle TSHUT polarity setting. */
	val = TSADC_READ(sc, TSADC_AUTO_CON);
	if (polarity == TSHUT_HIGH_ACTIVE)
		val |= TSADC_AUTO_CON_TSHUT_POLARITY;
	else
		val &= ~TSADC_AUTO_CON_TSHUT_POLARITY;
	TSADC_WRITE(sc, TSADC_AUTO_CON, val);
}

static void
rk_tsadc_init_common(struct rk_tsadc_softc *sc, int mode, int polarity)
{

	rk_tsadc_init_tshut(sc, mode, polarity);
	rk_tsadc_init_counts(sc);
}

static void
rk_tsadc_init_rk3399(struct rk_tsadc_softc *sc, int mode, int polarity)
{

	syscon_lock(sc->sc_syscon);
	syscon_write_4(sc->sc_syscon, RK3399_GRF_TSADC_TESTBIT_L,
				      RK3399_GRF_TSADC_TESTBIT_VCM_EN_L);
	syscon_write_4(sc->sc_syscon, RK3399_GRF_TSADC_TESTBIT_H,
				      RK3399_GRF_TSADC_TESTBIT_VCM_EN_H);

	DELAY(20);
	syscon_write_4(sc->sc_syscon, RK3399_GRF_SARADC_TESTBIT,
				      RK3399_GRF_SARADC_TESTBIT_ON);
	syscon_write_4(sc->sc_syscon, RK3399_GRF_TSADC_TESTBIT_H,
				      RK3399_GRF_TSADC_TESTBIT_H_ON);
	DELAY(100);
	syscon_unlock(sc->sc_syscon);

	rk_tsadc_init_common(sc, mode, polarity);
}

static void
rk_tsadc_init_enable(struct rk_tsadc_softc *sc)
{
	uint32_t val;

	val = TSADC_READ(sc, TSADC_AUTO_CON);
	val |= TSADC_AUTO_CON_AUTO_STATUS | 
	       TSADC_AUTO_CON_SRC1_LT_EN | TSADC_AUTO_CON_SRC0_LT_EN;
	TSADC_WRITE(sc, TSADC_AUTO_CON, val);

	/* Finally, register & enable the controller */
	sysmon_envsys_register(sc->sc_sme);

	val = TSADC_READ(sc, TSADC_AUTO_CON);
	val |= TSADC_AUTO_CON_AUTO_EN;
	if (sc->sc_rd->rd_version >= 3) {
		val |= TSADC_AUTO_CON_Q_SEL;
	}
	TSADC_WRITE(sc, TSADC_AUTO_CON, val);
}

static void
rk_tsadc_init(struct rk_tsadc_softc *sc, int mode, int polarity)
{

	(*sc->sc_rd->rd_init)(sc, mode, polarity);
	rk_tsadc_init_enable(sc);
}

/* run time support */

/* given edata, find the matching rk sensor structure */
static struct rk_tsadc_sensor *
rk_tsadc_edata_to_sensor(struct rk_tsadc_softc * const sc, envsys_data_t *edata)
{

	for (unsigned n = 0; n < sc->sc_rd->rd_num_sensors; n++) {
		struct rk_tsadc_sensor *rks = &sc->sc_sensors[n];

		if (&rks->s_data == edata)
			return rks;
	}
	return NULL;
}

static void
rk_tsadc_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct rk_tsadc_softc * const sc = sme->sme_cookie;
	struct rk_tsadc_sensor *rks = rk_tsadc_edata_to_sensor(sc, edata);
	unsigned data;
	int temp;

	if (rks == NULL)
		return;

	data = TSADC_READ(sc, rks->s_data_reg) & sc->sc_data_mask;
	temp = rk_tsadc_data_to_temp(sc, data);

	DPRINTF("(%s:%s): temp/data %d/%u",
		sc->sc_sme->sme_name, rks->s_data.desc,
		temp, data);

	if (temp == sc->sc_data_mask) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = temp + TEMP_uC_TO_uK;
		edata->state = ENVSYS_SVALID;
	}
}

static void
rk_tsadc_get_limits(struct sysmon_envsys *sme,
		    envsys_data_t *edata,
		    sysmon_envsys_lim_t *lim,
		    uint32_t *props)
{
	struct rk_tsadc_softc *sc = sme->sme_cookie;
	struct rk_tsadc_sensor *rks = rk_tsadc_edata_to_sensor(sc, edata);

	if (rks == NULL)
		return;

	lim->sel_critmax = rks->s_tshut + TEMP_uC_TO_uK;
	lim->sel_warnmax = rks->s_warn + TEMP_uC_TO_uK;

	*props = PROP_CRITMAX | PROP_WARNMAX;
}

/* XXX do something with interrupts that don't happen yet.  */
static int
rk_tsadc_intr(void *arg)
{
	struct rk_tsadc_softc * const sc = arg;
	uint32_t val;

	/* XXX */
	DPRINTF("(%s): interrupted", sc->sc_sme->sme_name);
	for (unsigned n = 0; n < __arraycount(rk_tsadc_sensors); n++) {
		struct rk_tsadc_sensor *rks = &sc->sc_sensors[n];

		rk_tsadc_refresh(sc->sc_sme, (envsys_data_t *)rks);
	}

	/* ack interrupt */
	val = TSADC_READ(sc, TSADC_INT_PD);
	if (sc->sc_rd->rd_version >= 3) {
		TSADC_WRITE(sc, TSADC_INT_PD,
		    val & ~TSADC_INT_PD_EOC_INT_PD_V3);
	} else {
		TSADC_WRITE(sc, TSADC_INT_PD,
		    val & ~TSADC_INT_PD_EOC_INT_PD_V2);
	}

	return 1;
}

/*
 * Convert TDASC data codes to temp and reverse.  The manual only has codes
 * and temperature values in 5 degC intervals, but says that interpolation
 * can be done to achieve better resolution between these values, and that
 * the spacing is linear.
 */
static int
rk_tsadc_data_to_temp(struct rk_tsadc_softc *sc, uint32_t data)
{
	unsigned i;
	const rk_data *rd = sc->sc_rd;

	if (data > rd->rd_max || data < rd->rd_min) {
		DPRINTF("data out of range (%u > %u || %u < %u)",
			data, rd->rd_max, data, rd->rd_min);
		return sc->sc_data_mask;
	}
	for (i = 1; i < rd->rd_size; i++) {
		if (rd->rd_array[i].data >= data) {
			int temprange, offset;
			uint32_t datarange, datadiff;
			unsigned first, secnd;

			if (rd->rd_array[i].data == data)
				return rd->rd_array[i].temp;

			/* must interpolate */
			if (rd->rd_decr) {
				first = i;
				secnd = i+1;
			} else {
				first = i;
				secnd = i-1;
			}

			temprange = rd->rd_array[first].temp -
				    rd->rd_array[secnd].temp;
			datarange = rd->rd_array[first].data -
				    rd->rd_array[secnd].data;
			datadiff = data - rd->rd_array[secnd].data;

			offset = (temprange * datadiff) / datarange;
			return rd->rd_array[secnd].temp + offset;
		}
	}
	panic("didn't find range");
}

static uint32_t
rk_tsadc_temp_to_data(struct rk_tsadc_softc *sc, int temp)
{
	unsigned i;
	const rk_data *rd = sc->sc_rd;

	for (i = 1; i < rd->rd_size; i++) {
		if (rd->rd_array[i].temp >= temp) {
			int temprange, tempdiff;
			uint32_t datarange, offset;
			unsigned first, secnd;

			if (rd->rd_array[i].temp == temp)
				return rd->rd_array[i].data;

			/* must interpolate */
			if (rd->rd_decr) {
				first = i;
				secnd = i+1;
			} else {
				first = i;
				secnd = i-1;
			}

			datarange = rd->rd_array[first].data -
				    rd->rd_array[secnd].data;
			temprange = rd->rd_array[first].temp -
				    rd->rd_array[secnd].temp;
			tempdiff = temp - rd->rd_array[secnd].temp;

			offset = (datarange * tempdiff) / temprange;
			return rd->rd_array[secnd].data + offset;
		}
	}

	return sc->sc_data_mask;
}
