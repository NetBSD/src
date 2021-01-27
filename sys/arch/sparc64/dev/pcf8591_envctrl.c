/*	$NetBSD: pcf8591_envctrl.c,v 1.19 2021/01/27 02:20:03 thorpej Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: pcf8591_envctrl.c,v 1.19 2021/01/27 02:20:03 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/autoconf.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

#ifdef ECADC_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

/* Translation tables contain 254 entries */
#define XLATE_SIZE		256
#define XLATE_MAX		(XLATE_SIZE - 2)

#define PCF8591_CHANNELS	4

#define PCF8591_CTRL_CH0	0x00
#define PCF8591_CTRL_CH1	0x01
#define PCF8591_CTRL_CH2	0x02
#define PCF8591_CTRL_CH3	0x03
#define PCF8591_CTRL_AUTOINC	0x04
#define PCF8591_CTRL_OSCILLATOR	0x40

#define PCF8591_TEMP_SENS	0x00
#define PCF8591_SYS_FAN_CTRL	0x01

struct ecadc_channel {
	u_int		chan_num;
	u_int		chan_type;
	envsys_data_t	chan_sensor;
	u_char		*chan_xlate;
	int64_t		chan_factor;
	int64_t		chan_min;
	int64_t		chan_warn;
	int64_t		chan_crit;
	u_int8_t	chan_speed;
};

struct ecadc_softc {
	device_t		sc_dev;
	i2c_tag_t		sc_tag;
	i2c_addr_t		sc_addr;
	u_char			sc_ps_xlate[XLATE_SIZE];
	u_char			sc_cpu_xlate[XLATE_SIZE];
	u_char			sc_cpu_fan_spd[XLATE_SIZE];
	u_int			sc_nchan;
	struct ecadc_channel	sc_channels[PCF8591_CHANNELS];
	struct sysmon_envsys	*sc_sme;
	int			sc_hastimer;
	callout_t		sc_timer;
};

static int	ecadc_match(device_t, cfdata_t, void *);
static void	ecadc_attach(device_t, device_t, void *);
static int	ecadc_detach(device_t, int);
static void	ecadc_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	ecadc_get_limits(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, u_int32_t *);
static int	ecadc_set_fan_speed(struct ecadc_softc *, u_int8_t, u_int8_t);
static void	ecadc_timeout(void *);
static void	ecadc_fan_adjust(void *);

CFATTACH_DECL3_NEW(ecadc, sizeof(struct ecadc_softc),
	ecadc_match, ecadc_attach, ecadc_detach, NULL, NULL, NULL,
	DVF_DETACH_SHUTDOWN);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ecadc" },
	DEVICE_COMPAT_EOL
};

static int
ecadc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
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
	sc->sc_nchan = 0;
	sc->sc_hastimer = 0;

	DPRINTF("\n");
	if ((len = OF_getprop(node, "thermisters", term,
	    sizeof(term))) < 0) {
		aprint_error(": couldn't find \"thermisters\" property\n");
		return;
	}

	if (OF_getprop(node, "cpu-temp-factors", &sc->sc_cpu_xlate[2],
	    XLATE_MAX) < 0) {
		aprint_error(": couldn't find \"cpu-temp-factors\" property\n");
		return;
	}
	sc->sc_cpu_xlate[0] = sc->sc_cpu_xlate[1] = sc->sc_cpu_xlate[2];

	/* Only the Sun Enterprise 450 has these. */
	OF_getprop(node, "ps-temp-factors", &sc->sc_ps_xlate[2], XLATE_MAX);
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
		sc->sc_channels[sc->sc_nchan].chan_type = PCF8591_TEMP_SENS;

		sensor = &sc->sc_channels[sc->sc_nchan].chan_sensor;
		sensor->units = ENVSYS_STEMP;
		sensor->flags |= ENVSYS_FMONLIMITS;
		sensor->state = ENVSYS_SINVALID;
		strlcpy(sensor->desc, desc, sizeof(sensor->desc));

		if (strncmp(desc, "CPU", 3) == 0) {
			sc->sc_channels[sc->sc_nchan].chan_xlate =
			    sc->sc_cpu_xlate;
			DPRINTF("%s: "
			    "added %s sensor (chan %d) with cpu_xlate\n",
			    device_xname(sc->sc_dev), desc, chan);
		} else if (strncmp(desc, "PS", 2) == 0) {
			sc->sc_channels[sc->sc_nchan].chan_xlate =
			    sc->sc_ps_xlate;
			DPRINTF("%s: "
			    "added %s sensor (chan %d) with ps_xlate\n",
			    device_xname(sc->sc_dev), desc, chan);
		} else {
			sc->sc_channels[sc->sc_nchan].chan_factor =
			    (1000000 * num) / den;
			DPRINTF("%s: "
			    "added %s sensor (chan %d) without xlate\n",
			    device_xname(sc->sc_dev), desc, chan);
		}
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

	/* Try a read now, so we can fail if this component isn't present */
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    NULL, 0, junk, sc->sc_nchan + 1, 0)) {
		aprint_normal(": read failed\n");
		iic_release_bus(sc->sc_tag, 0);
		return;
	}

	iic_release_bus(sc->sc_tag, 0);

	/*
	 * Fan speed changing information is missing from OFW
	 * The E250 CPU fan is connected to the sensor at addr 0x4a, channel 1
	 */
	if (ia->ia_addr == 0x4a && !strcmp(machine_model, "SUNW,Ultra-250") &&
	    OF_getprop(node, "cpu-fan-speeds", &sc->sc_cpu_fan_spd,
	    XLATE_MAX) > 0) {
		sc->sc_channels[sc->sc_nchan].chan_num = 1;
		sc->sc_channels[sc->sc_nchan].chan_type = PCF8591_SYS_FAN_CTRL;
		sensor = &sc->sc_channels[sc->sc_nchan].chan_sensor;
		sensor->units = ENVSYS_INTEGER;
		sensor->flags = ENVSYS_FMONNOTSUPP;
		sensor->state = ENVSYS_SINVALID;
		strlcpy(sensor->desc, "SYSFAN", sizeof(sensor->desc));
		sc->sc_channels[sc->sc_nchan].chan_xlate = sc->sc_cpu_fan_spd;
		DPRINTF("%s: "
		    "added CPUFAN sensor (chan %d) with cpu-fan xlate\n",
		    device_xname(sc->sc_dev),
		    sc->sc_channels[sc->sc_nchan].chan_num);

		/* Set the fan to medium speed */
		sc->sc_channels[sc->sc_nchan].chan_speed =
		    (sc->sc_cpu_fan_spd[0]+sc->sc_cpu_fan_spd[XLATE_MAX])/2;
		ecadc_set_fan_speed(sc, sc->sc_channels[sc->sc_nchan].chan_num,
		    sc->sc_channels[sc->sc_nchan].chan_speed);

		sc->sc_nchan++;
		sc->sc_hastimer = 1;
	}

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
		sc->sc_sme = NULL;
		return;
	}
	
	if (sc->sc_hastimer) {
		callout_init(&sc->sc_timer, CALLOUT_MPSAFE);
		callout_reset(&sc->sc_timer, hz*20, ecadc_timeout, sc);
	}

	aprint_naive(": Temp Sensors\n");
	aprint_normal(": %s Temp Sensors (%d channels)\n", ia->ia_name,
	    sc->sc_nchan);
}

static int
ecadc_detach(device_t self, int flags)
{
	struct ecadc_softc *sc = device_private(self);
	int c, i;

	if (sc->sc_hastimer) {
		callout_halt(&sc->sc_timer, NULL);
		callout_destroy(&sc->sc_timer);
	}

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	for (i = 0; i < sc->sc_nchan; i++) {
		struct ecadc_channel *chp = &sc->sc_channels[i];

		if (chp->chan_type == PCF8591_SYS_FAN_CTRL) {
			/* Loop in case the bus is busy */
			for (c = 0; c < 5; c++) {
				chp->chan_speed = sc->sc_cpu_fan_spd[0];
				if (!ecadc_set_fan_speed(sc, chp->chan_num,
				    chp->chan_speed))
					return 0;
				delay(10000);
			}
			printf("%s: cannot set fan speed (chan %d)\n",
			    device_xname(sc->sc_dev), chp->chan_num);
		}
	}

	return 0;
}

static void
ecadc_refresh(struct sysmon_envsys *sme, envsys_data_t *sensor)
{
	struct ecadc_softc *sc = sme->sme_cookie;
	u_int i;
	u_int8_t data[PCF8591_CHANNELS + 1];
	u_int8_t ctrl = PCF8591_CTRL_CH0 | PCF8591_CTRL_AUTOINC |
	    PCF8591_CTRL_OSCILLATOR;

	if (iic_acquire_bus(sc->sc_tag, 0))
		return;
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &ctrl, 1, NULL, 0, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		return;
	}
	/*
	 * Each data byte that we read is the result of the previous request,
	 * so read num_channels + 1 and update envsys values from chan + 1.
	 */
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    NULL, 0, data, PCF8591_CHANNELS + 1, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		return;
	}
	iic_release_bus(sc->sc_tag, 0);

	/* Temperature with/without translation or relative (ADC value) */
	for (i = 0; i < sc->sc_nchan; i++) {
		struct ecadc_channel *chp = &sc->sc_channels[i];

		if (chp->chan_type == PCF8591_TEMP_SENS) {

			/* Encode the raw value to use for the fan control */
			if (chp->chan_xlate) {
				int32_t temp;

				temp = 273150000 + 1000000 *
				    chp->chan_xlate[data[1 + chp->chan_num]];
				temp &= ~0xff;
				temp += data[1 + chp->chan_num];
				chp->chan_sensor.value_cur = temp;
				DPRINTF("%s: xlate %s sensor = %d"
				    " (0x%x > 0x%x)\n",
				    device_xname(sc->sc_dev),
				    chp->chan_sensor.desc, temp,
				    data[1 + chp->chan_num],
				    chp->chan_xlate[data[1 + chp->chan_num]]);
			} else {
				chp->chan_sensor.value_cur = 273150000 +
				    chp->chan_factor * data[1 + chp->chan_num];
				DPRINTF("%s: read %s sensor = %d (0x%x)\n",
				    device_xname(sc->sc_dev),
				    chp->chan_sensor.desc,
				    chp->chan_sensor.value_cur,
				    data[1 + chp->chan_num]);
			}
			chp->chan_sensor.flags |= ENVSYS_FMONLIMITS;
		}
		if (chp->chan_type == PCF8591_SYS_FAN_CTRL)
			chp->chan_sensor.value_cur = data[1 + chp->chan_num];

		chp->chan_sensor.state = ENVSYS_SVALID;
	}
}

static void
ecadc_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		sysmon_envsys_lim_t *limits, u_int32_t *props)
{
	struct ecadc_softc *sc = sme->sme_cookie;
	int i;

	for (i = 0; i < sc->sc_nchan; i++) {
		if (edata != &sc->sc_channels[i].chan_sensor)
			continue;
		if (sc->sc_channels[i].chan_type == PCF8591_TEMP_SENS) {
			*props |= PROP_WARNMIN|PROP_WARNMAX|PROP_CRITMAX;
			limits->sel_warnmin = sc->sc_channels[i].chan_min;
			limits->sel_warnmax = sc->sc_channels[i].chan_warn;
			limits->sel_critmax = sc->sc_channels[i].chan_crit;
		}
		return;
	}
}

static void
ecadc_timeout(void *v)
{
	struct ecadc_softc *sc = v;

	sysmon_task_queue_sched(0, ecadc_fan_adjust, sc);
	callout_reset(&sc->sc_timer, hz*60, ecadc_timeout, sc);
}

static bool
is_cpu_temp(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	return strncmp(edata->desc, "CPU", 3) == 0;
}

static bool
is_high_temp(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_INDICATOR)
		return false;
	return strcmp(edata->desc, "high_temp") == 0;
}

static bool
is_fan_fail(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_INDICATOR)
		return false;
	return strcmp(edata->desc, "fan_fail") == 0;
}

static int
ecadc_set_fan_speed(struct ecadc_softc *sc, u_int8_t chan, u_int8_t val)
{
	u_int8_t ctrl = PCF8591_CTRL_AUTOINC | PCF8591_CTRL_OSCILLATOR;
	int ret;

	ctrl |= chan;
	ret = iic_acquire_bus(sc->sc_tag, 0);
	if (ret) {
		printf("%s: error acquiring i2c bus (ch %d)\n",
		    device_xname(sc->sc_dev), chan);
		return ret;
	}
	ret = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &ctrl, 1, &val, 1, 0);
	if (ret)
		printf("%s: error changing fan speed (ch %d)\n",
		    device_xname(sc->sc_dev), chan);
	else
		DPRINTF("%s changed fan speed (ch %d) to 0x%x\n",
		    device_xname(sc->sc_dev), chan, val);
	iic_release_bus(sc->sc_tag, 0);
	return ret;
}

static void
ecadc_fan_adjust(void *v)
{
	struct ecadc_softc *sc = v;
	struct ecadc_channel *chp;
	int i;
	u_int8_t temp, speed;
	u_int32_t htemp, ffail;

	for (i = 0; i < sc->sc_nchan; i++) {
		chp = &sc->sc_channels[i];
		if (chp->chan_type != PCF8591_SYS_FAN_CTRL)
			continue;

		/* Check for high temperature or fan failure */
		htemp = sysmon_envsys_get_max_value(is_high_temp, true);
		ffail = sysmon_envsys_get_max_value(is_fan_fail, true);
		if (htemp) {
			printf("%s: High temperature detected\n",
			    device_xname(sc->sc_dev));
			/* Set fans to maximum speed */
			speed = sc->sc_cpu_fan_spd[0];
		} else if (ffail) {
			printf("%s: Fan failure detected\n",
			    device_xname(sc->sc_dev));
			/* Set fans to maximum speed */
			speed = sc->sc_cpu_fan_spd[0];
		} else {
			/* Extract the raw value from the max CPU temp */
			temp = sysmon_envsys_get_max_value(is_cpu_temp, true)
			    & 0xff;
			if (!temp) {
				printf("%s: skipping temp adjustment"
				    " - no sensor values\n",
				    device_xname(sc->sc_dev));
				return;
			}
			if (temp > XLATE_MAX)
				temp = XLATE_MAX;
			speed = chp->chan_xlate[temp];
		}
		if (speed != chp->chan_speed) {
			if (!ecadc_set_fan_speed(sc, chp->chan_num,
			    speed))
				chp->chan_speed = speed;
		}
	}
}
