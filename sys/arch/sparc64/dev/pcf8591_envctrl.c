/*	$NetBSD: pcf8591_envctrl.c,v 1.6.38.1 2018/06/25 07:25:45 pgoyette Exp $	*/
/*	$OpenBSD: pcf8591_envctrl.c,v 1.6 2007/10/25 21:17:20 kettenis Exp $ */

/*
 * Copyright (c) 2006 Damien Miller <djm@openbsd.org>
 * Copyright (c) 2007 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcf8591_envctrl.c,v 1.6.38.1 2018/06/25 07:25:45 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

#define PCF8591_CHANNELS	4

#define PCF8591_CTRL_CH0	0x00
#define PCF8591_CTRL_CH1	0x01
#define PCF8591_CTRL_CH2	0x02
#define PCF8591_CTRL_CH3	0x03
#define PCF8591_CTRL_AUTOINC	0x04
#define PCF8591_CTRL_OSCILLATOR	0x40

struct ecadc_channel {
	u_int		chan_num;
	envsys_data_t	chan_sensor;
	u_char		*chan_xlate;
	int64_t		chan_factor;
	int64_t		chan_min;
	int64_t		chan_warn;
	int64_t		chan_crit;
};

struct ecadc_softc {
	device_t		sc_dev;
	i2c_tag_t		sc_tag;
	i2c_addr_t		sc_addr;
	u_char			sc_ps_xlate[256];
	u_char			sc_cpu_xlate[256];
	u_int			sc_nchan;
	struct ecadc_channel	sc_channels[PCF8591_CHANNELS];
	struct sysmon_envsys	*sc_sme;
};

static int	ecadc_match(device_t, cfdata_t, void *);
static void	ecadc_attach(device_t, device_t, void *);
static void	ecadc_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	ecadc_get_limits(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);

CFATTACH_DECL_NEW(ecadc, sizeof(struct ecadc_softc),
	ecadc_match, ecadc_attach, NULL, NULL);

static const char * ecadc_compats[] = {
	"ecadc",
	NULL
};

static const struct device_compatible_entry ecadc_compat_data[] = {
	DEVICE_COMPAT_ENTRY(ecadc_compats),
	DEVICE_COMPAT_TERMINATOR
};

static int
ecadc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, ecadc_compat_data, &match_result))
		return match_result;

	/* This driver is direct-config only. */

	return 0;
}

static void
ecadc_attach(device_t parent, device_t self, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct ecadc_softc *sc = device_private(self);
	u_char term[256];
	u_char *cp, *desc;
	int64_t minv, warnv, crit, num, den;
	u_int8_t junk[PCF8591_CHANNELS + 1];
	envsys_data_t *sensor;
	int len, error, addr, chan, node = (int)ia->ia_cookie;
	u_int i;

	sc->sc_dev = self;
	if ((len = OF_getprop(node, "thermisters", term,
	    sizeof(term))) < 0) {
		aprint_error(": couldn't find \"thermisters\" property\n");
		return;
	}

	if (OF_getprop(node, "cpu-temp-factors", &sc->sc_cpu_xlate[2],
	    sizeof(sc->sc_cpu_xlate) - 2) < 0) {
		aprint_error(": couldn't find \"cpu-temp-factors\" property\n");
		return;
	}
	sc->sc_cpu_xlate[0] = sc->sc_cpu_xlate[1] = sc->sc_cpu_xlate[2];

	/* Only the Sun Enterprise 450 has these. */
	OF_getprop(node, "ps-temp-factors", &sc->sc_ps_xlate[2],
	    sizeof(sc->sc_ps_xlate) - 2);
	sc->sc_ps_xlate[0] = sc->sc_ps_xlate[1] = sc->sc_ps_xlate[2];

	cp = term;
	while (cp < term + len) {
		addr = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		chan = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		minv = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		warnv = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		crit = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		num = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		den = cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3]; cp += 4;
		desc = cp;
		while (cp < term + len && *cp++);

		if (addr != (ia->ia_addr << 1))
			continue;

		if (num == 0 || den == 0)
			num = den = 1;

		sc->sc_channels[sc->sc_nchan].chan_num = chan;

		sensor = &sc->sc_channels[sc->sc_nchan].chan_sensor;
		sensor->units = ENVSYS_STEMP;
		sensor->flags |= ENVSYS_FMONLIMITS;
		sensor->state = ENVSYS_SINVALID;
		strlcpy(sensor->desc, desc, sizeof(sensor->desc));

		if (strncmp(desc, "CPU", 3) == 0)
			sc->sc_channels[sc->sc_nchan].chan_xlate =
			    sc->sc_cpu_xlate;
		else if (strncmp(desc, "PS", 2) == 0)
			sc->sc_channels[sc->sc_nchan].chan_xlate =
			    sc->sc_ps_xlate;
		else
			sc->sc_channels[sc->sc_nchan].chan_factor =
			    (1000000 * num) / den;
		sc->sc_channels[sc->sc_nchan].chan_min =
		    273150000 + 1000000 * minv;
		sc->sc_channels[sc->sc_nchan].chan_warn =
		    273150000 + 1000000 * warnv;
		sc->sc_channels[sc->sc_nchan].chan_crit =
		    273150000 + 1000000 * crit;
		sc->sc_nchan++;
	}

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	iic_acquire_bus(sc->sc_tag, 0);

	/* Try a read now, so we can fail if it doesn't work */
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    NULL, 0, junk, sc->sc_nchan + 1, 0)) {
		aprint_error(": read failed\n");
		iic_release_bus(sc->sc_tag, 0);
		return;
	}

	iic_release_bus(sc->sc_tag, 0);

	/* Hook us into the sysmon_envsys subsystem */
	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = ecadc_refresh;
	sc->sc_sme->sme_get_limits = ecadc_get_limits;

	/* Initialize sensor data. */
	for (i = 0; i < sc->sc_nchan; i++)
		sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_channels[i].chan_sensor);

	error = sysmon_envsys_register(sc->sc_sme);
	if (error) {
		aprint_error_dev(self, "error %d registering with sysmon\n",
			error);
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	aprint_naive(": Temp Sensors\n");
	aprint_normal(": %s Temp Sensors\n", ia->ia_name);
}

static void
ecadc_refresh(struct sysmon_envsys *sme, envsys_data_t *sensor)
{
	struct ecadc_softc *sc = sme->sme_cookie;
	u_int i;
	u_int8_t data[PCF8591_CHANNELS + 1];
	u_int8_t ctrl = PCF8591_CTRL_CH0 | PCF8591_CTRL_AUTOINC |
	    PCF8591_CTRL_OSCILLATOR;

	iic_acquire_bus(sc->sc_tag, 0);
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &ctrl, 1, NULL, 0, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		return;
	}
	/* NB: first byte out is stale, so read num_channels + 1 */
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    NULL, 0, data, PCF8591_CHANNELS + 1, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		return;
	}
	iic_release_bus(sc->sc_tag, 0);

	/* We only support temperature channels. */
	for (i = 0; i < sc->sc_nchan; i++) {
		struct ecadc_channel *chp = &sc->sc_channels[i];

		if (chp->chan_xlate)
			chp->chan_sensor.value_cur = 273150000 + 1000000 *
			    chp->chan_xlate[data[1 + chp->chan_num]];
		else
			chp->chan_sensor.value_cur = 273150000 +
			    chp->chan_factor * data[1 + chp->chan_num];

		chp->chan_sensor.state = ENVSYS_SVALID;
		chp->chan_sensor.flags &= ~ENVSYS_FNEED_REFRESH;
		chp->chan_sensor.flags |= ENVSYS_FMONLIMITS;
	}
}

static void
ecadc_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct ecadc_softc *sc = sme->sme_cookie;
	int i;

	for (i = 0; i < sc->sc_nchan; i++) {
		if (edata != &sc->sc_channels[i].chan_sensor)
			continue;
		*props |= PROP_WARNMIN|PROP_WARNMAX|PROP_CRITMAX;
		limits->sel_warnmin = sc->sc_channels[i].chan_min;
		limits->sel_warnmax = sc->sc_channels[i].chan_warn;
		limits->sel_critmax = sc->sc_channels[i].chan_crit;
		return;
	}
}
