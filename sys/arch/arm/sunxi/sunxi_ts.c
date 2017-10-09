/* $NetBSD: sunxi_ts.c,v 1.2 2017/10/09 14:28:01 jmcneill Exp $ */

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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: sunxi_ts.c,v 1.2 2017/10/09 14:28:01 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/tpcalibvar.h>

#define	TS_TP_SENSITIVITY_ADJUST_DEFAULT	15
#define	TS_FILTER_TYPE_DEFAULT			1

#define	TP_CTRL0		0x00
#define	 TP_CTRL0_ADC_FIRST_DLY		__BITS(31,24)
#define	 TP_CTRL0_ADC_FIRST_DLY_MODE	__BIT(23)
#define	 TP_CTRL0_ADC_CLK_SELECT	__BIT(22)
#define	 TP_CTRL0_ADC_CLK_DIVIDER	__BITS(21,20)
#define	 TP_CTRL0_FS_DIV		__BITS(19,16)
#define	 TP_CTRL0_T_ACQ			__BITS(15,0)
#define	TP_CTRL1		0x04
#define	 TP_CTRL1_STYLUS_UP_DEBOUNCE	__BITS(19,12)
#define	 TP_CTRL1_STYLUS_UP_DEBOUNCE_EN	__BIT(9)
#define	 TP_CTRL1_TOUCH_PAN_CALI_EN	__BIT(6)
#define	 TP_CTRL1_TP_DUAL_EN		__BIT(5)
#define	 TP_CTRL1_TP_MODE_EN		__BIT(4)
#define	 TP_CTRL1_TP_ADC_SELECT		__BIT(3)
#define	 TP_CTRL1_ADC_CHAN_SELECT	__BITS(2,0)
#define	TP_CTRL2		0x08
#define	 TP_CTRL2_SENSITIVE_ADJUST	__BITS(31,28)
#define	 TP_CTRL2_MODE_SELECT		__BITS(27,26)
#define	 TP_CTRL2_PRE_MEA_EN		__BIT(24)
#define	 TP_CTRL2_PRE_MEA_THRE_CNT	__BITS(23,0)
#define	TP_CTRL3		0x0c
#define	 TP_CTRL3_FILTER_EN		__BIT(2)
#define	 TP_CTRL3_FILTER_TYPE		__BITS(1,0)
#define	TP_INT			0x10
#define	 TP_INT_TEMP_IRQ_EN	__BIT(18)
#define	 TP_INT_OVERRUN_IRQ_EN	__BIT(17)
#define	 TP_INT_DATA_IRQ_EN	__BIT(16)
#define	 TP_INT_DATA_XY_CHANGE	__BIT(13)
#define	 TP_INT_FIFO_TRIG_LEVEL	__BITS(12,8)
#define	 TP_INT_DATA_DRQ_EN	__BIT(7)
#define	 TP_INT_FIFO_FLUSH	__BIT(4)
#define	 TP_INT_UP_IRQ_EN	__BIT(1)
#define	 TP_INT_DOWN_IRQ_EN	__BIT(0)
#define	TP_FIFOCS		0x14
#define	 TP_FIFOCS_TEMP_IRQ_PENDING __BIT(18)
#define	 TP_FIFOCS_OVERRUN_PENDING __BIT(17)
#define	 TP_FIFOCS_DATA_PENDING	__BIT(16)
#define	 TP_FIFOCS_RXA_CNT	__BITS(12,8)
#define	 TP_FIFOCS_IDLE_FLG	__BIT(2)
#define	 TP_FIFOCS_UP_PENDING	__BIT(1)
#define	 TP_FIFOCS_DOWN_PENDING	__BIT(0)
#define	TP_TPR			0x18
#define	 TP_TPR_TEMP_EN		__BIT(16)
#define	 TP_TPR_TEMP_PER	__BITS(15,0)
#define	TP_CDAT			0x1c
#define	 TP_CDAT_MASK		__BITS(11,0)
#define	TEMP_DATA		0x20
#define	 TEMP_DATA_MASK		__BITS(11,0)
#define	TP_DATA			0x24
#define	 TP_DATA_MASK		__BITS(11,0)
#define	TP_IO_CONFIG		0x28
#define	TP_PORT_DATA		0x2c
#define	 TP_PORT_DATA_MASK	__BITS(3,0)

#define	TEMP_C_TO_K		273150000

static int sunxi_ts_match(device_t, cfdata_t, void *);
static void sunxi_ts_attach(device_t, device_t, void *);

struct sunxi_ts_config {
	int64_t	temp_offset;
	int64_t	temp_step;
	uint32_t tp_mode_en_mask;
};

static const struct sunxi_ts_config sun4i_a10_ts_config = {
	.temp_offset = 257000000,
	.temp_step = 133000,
	.tp_mode_en_mask = TP_CTRL1_TP_MODE_EN,
};

static const struct sunxi_ts_config sun5i_a13_ts_config = {
	.temp_offset = 144700000,
	.temp_step = 100000,
	.tp_mode_en_mask = TP_CTRL1_TP_MODE_EN,
};

static const struct sunxi_ts_config sun6i_a31_ts_config = {
	.temp_offset = 271000000,
	.temp_step = 167000,
	.tp_mode_en_mask = TP_CTRL1_TP_DUAL_EN,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-ts",	(uintptr_t)&sun4i_a10_ts_config },
	{ "allwinner,sun5i-a13-ts",	(uintptr_t)&sun5i_a13_ts_config },
	{ "allwinner,sun6i-a31-ts",	(uintptr_t)&sun6i_a31_ts_config },
	{ NULL }
};

static struct wsmouse_calibcoords sunxi_ts_default_calib = {
	.minx = 0,
	.miny = 0,
	.maxx = __SHIFTOUT_MASK(TP_DATA_MASK),
	.maxy = __SHIFTOUT_MASK(TP_DATA_MASK),
	.samplelen = WSMOUSE_CALIBCOORDS_RESET,
};

struct sunxi_ts_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	const struct sunxi_ts_config *sc_conf;

	bool			sc_ts_attached;
	bool			sc_ts_inverted_x;
	bool			sc_ts_inverted_y;

	struct tpcalib_softc	sc_tpcalib;
	device_t		sc_wsmousedev;
	bool			sc_ignoredata;

	u_int			sc_tp_x;
	u_int			sc_tp_y;
	u_int			sc_tp_btns;

	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_temp_data;
};

CFATTACH_DECL_NEW(sunxi_ts, sizeof(struct sunxi_ts_softc),
	sunxi_ts_match, sunxi_ts_attach, NULL, NULL);

#define	TS_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	TS_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sunxi_ts_enable(void *v)
{
	struct sunxi_ts_softc * const sc = v;
	uint32_t val;

	/* reset state */
	sc->sc_ignoredata = true;
	sc->sc_tp_x = 0;
	sc->sc_tp_y = 0;
	sc->sc_tp_btns = 0;

	/* Enable touchpanel IRQs */
	val = TS_READ(sc, TP_INT);
	val |= TP_INT_DATA_IRQ_EN | TP_INT_FIFO_FLUSH | TP_INT_UP_IRQ_EN;
	val &= ~TP_INT_FIFO_TRIG_LEVEL;
	val |= __SHIFTIN(1, TP_INT_FIFO_TRIG_LEVEL);
	TS_WRITE(sc, TP_INT, val);

	return 0;
}

static void
sunxi_ts_disable(void *v)
{
	struct sunxi_ts_softc * const sc = v;
	uint32_t val;

	/* Disable touchpanel IRQs */
	val = TS_READ(sc, TP_INT);
	val &= ~(TP_INT_DATA_IRQ_EN | TP_INT_FIFO_FLUSH | TP_INT_UP_IRQ_EN);
	TS_WRITE(sc, TP_INT, val);
}

static int
sunxi_ts_ioctl(void *v, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct sunxi_ts_softc * const sc = v;
	struct wsmouse_id *id;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(int *)data = WSMOUSE_TYPE_TPANEL;
		return 0;

	case WSMOUSEIO_GETID:
		id = data;
		if (id->type != WSMOUSE_ID_TYPE_UIDSTR)
			return EINVAL;
		snprintf(id->data, WSMOUSE_ID_MAXLEN,
		    "Allwinner TS SN000000");
		id->length = strlen(id->data);
		return 0;

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
		return tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
	}

	return EPASSTHROUGH;
}

static const struct wsmouse_accessops sunxi_ts_accessops = {
	.enable = sunxi_ts_enable,
	.disable = sunxi_ts_disable,
	.ioctl = sunxi_ts_ioctl,
};

static int
sunxi_ts_intr(void *priv)
{
	struct sunxi_ts_softc * const sc = priv;
	uint32_t fifocs, x, y;
	int s;

	fifocs = TS_READ(sc, TP_FIFOCS);

	if (fifocs & TP_FIFOCS_TEMP_IRQ_PENDING) {
		sc->sc_temp_data.value_cur = (TS_READ(sc, TEMP_DATA) *
		    sc->sc_conf->temp_step - sc->sc_conf->temp_offset) +
		    TEMP_C_TO_K;
		sc->sc_temp_data.state = ENVSYS_SVALID;
	}

	if (fifocs & TP_FIFOCS_DATA_PENDING) {
		x = TS_READ(sc, TP_DATA);
		y = TS_READ(sc, TP_DATA);
		if (sc->sc_ignoredata) {
			/* Discard the first report */
			sc->sc_ignoredata = false;
		} else {
			if (sc->sc_ts_inverted_x)
				x = __SHIFTOUT_MASK(TP_DATA_MASK) - x;
			if (sc->sc_ts_inverted_y)
				y = __SHIFTOUT_MASK(TP_DATA_MASK) - y;
			tpcalib_trans(&sc->sc_tpcalib, x, y,
			    &sc->sc_tp_x, &sc->sc_tp_y);
			sc->sc_tp_btns |= 1;

			s = spltty();
			wsmouse_input(sc->sc_wsmousedev,
			    sc->sc_tp_btns, sc->sc_tp_x, sc->sc_tp_y, 0, 0,
			    WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
			splx(s);
		}
	}

	if (fifocs & TP_FIFOCS_UP_PENDING) {
		sc->sc_ignoredata = true;
		sc->sc_tp_btns &= ~1;
		s = spltty();
		wsmouse_input(sc->sc_wsmousedev,
		    sc->sc_tp_btns, sc->sc_tp_x, sc->sc_tp_y, 0, 0,
		    WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
		splx(s);
	}

	TS_WRITE(sc, TP_FIFOCS, fifocs);

	return 1;
}

static void
sunxi_ts_init(struct sunxi_ts_softc *sc)
{
	u_int tp_sensitivity_adjust = TS_TP_SENSITIVITY_ADJUST_DEFAULT;
	u_int filter_type = TS_FILTER_TYPE_DEFAULT;

	of_getprop_uint32(sc->sc_phandle, "allwinner,tp-sensitive-adjust",
	    &tp_sensitivity_adjust);
	of_getprop_uint32(sc->sc_phandle, "allwinner,filter-type",
	    &filter_type);

	TS_WRITE(sc, TP_CTRL0,
	    __SHIFTIN(0, TP_CTRL0_ADC_CLK_SELECT) |
	    __SHIFTIN(2, TP_CTRL0_ADC_CLK_DIVIDER) |
	    __SHIFTIN(7, TP_CTRL0_FS_DIV) |
	    __SHIFTIN(63, TP_CTRL0_T_ACQ));
	TS_WRITE(sc, TP_CTRL2,
	    __SHIFTIN(0, TP_CTRL2_MODE_SELECT) |
	    __SHIFTIN(tp_sensitivity_adjust, TP_CTRL2_SENSITIVE_ADJUST));
	TS_WRITE(sc, TP_CTRL3,
	    TP_CTRL3_FILTER_EN |
	    __SHIFTIN(filter_type, TP_CTRL3_FILTER_TYPE));
	TS_WRITE(sc, TP_CTRL1,
	    sc->sc_conf->tp_mode_en_mask |
	    TP_CTRL1_STYLUS_UP_DEBOUNCE_EN |
	    __SHIFTIN(5, TP_CTRL1_STYLUS_UP_DEBOUNCE));

	/* Enable temperature sensor */
	TS_WRITE(sc, TP_TPR,
	    TP_TPR_TEMP_EN | __SHIFTIN(1953, TP_TPR_TEMP_PER));

	/* Enable temperature sensor IRQ */
	TS_WRITE(sc, TP_INT, TP_INT_TEMP_IRQ_EN);

	/* Clear pending IRQs */
	TS_WRITE(sc, TP_FIFOCS, TS_READ(sc, TP_FIFOCS));
}

static int
sunxi_ts_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_ts_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ts_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct wsmousedev_attach_args a;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_conf = (void *)of_search_compatible(phandle, compat_data)->data;

	sc->sc_ts_attached = of_getprop_bool(phandle, "allwinner,ts-attached");
	sc->sc_ts_inverted_x = of_getprop_bool(phandle,
	    "touchscreen-inverted-x");
	sc->sc_ts_inverted_y = of_getprop_bool(phandle,
	    "touchscreen-inverted-y");

	aprint_naive("\n");
	aprint_normal(": Touch Screen Controller\n");

	sunxi_ts_init(sc);

	ih = fdtbus_intr_establish(phandle, 0, IPL_VM, 0, sunxi_ts_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_flags = SME_DISABLE_REFRESH;

	sc->sc_temp_data.units = ENVSYS_STEMP;
	sc->sc_temp_data.state = ENVSYS_SINVALID;
	snprintf(sc->sc_temp_data.desc, sizeof(sc->sc_temp_data.desc),
	    "temperature");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_temp_data);

	sysmon_envsys_register(sc->sc_sme);

	if (sc->sc_ts_attached) {
		tpcalib_init(&sc->sc_tpcalib);
		tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		    &sunxi_ts_default_calib, 0, 0);

		memset(&a, 0, sizeof(a));
		a.accessops = &sunxi_ts_accessops;
		a.accesscookie = sc;
		sc->sc_wsmousedev = config_found_ia(self, "wsmousedev",
		    &a, wsmousedevprint);
	}
}
