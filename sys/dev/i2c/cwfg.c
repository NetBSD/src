/* $NetBSD: cwfg.c,v 1.5 2021/11/07 17:14:38 jmcneill Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: cwfg.c,v 1.5 2021/11/07 17:14:38 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/fdt/fdtvar.h>

#define	VERSION_REG	0x00
#define	VCELL_HI_REG	0x02
#define	 VCELL_HI	__BITS(5,0)
#define	VCELL_LO_REG	0x03
#define	 VCELL_LO	__BITS(7,0)
#define	SOC_HI_REG	0x04
#define	SOC_LO_REG	0x05
#define	RTT_ALRT_HI_REG	0x06
#define	 RTT_ALRT	__BIT(7)
#define	 RTT_HI		__BITS(4,0)
#define	RTT_ALRT_LO_REG	0x07
#define	 RTT_LO		__BITS(7,0)
#define	CONFIG_REG	0x08
#define	 CONFIG_ATHD	__BITS(7,3)
#define	 CONFIG_UFG	__BIT(1)
#define	MODE_REG	0x0a
#define	 MODE_SLEEP	__BITS(7,6)
#define	  MODE_SLEEP_WAKE	0x0
#define	  MODE_SLEEP_SLEEP	0x3
#define	 MODE_QSTRT	__BITS(5,4)
#define	 MODE_POR	__BITS(3,0)
#define	BATINFO_REG(n)	(0x10 + (n))

#define	VCELL_STEP	312
#define	VCELL_DIV	1024
#define	BATINFO_SIZE	64
#define	RESET_COUNT	30
#define	RESET_DELAY	100000

enum cwfg_sensor {
	CWFG_SENSOR_VCELL,
	CWFG_SENSOR_SOC,
	CWFG_SENSOR_RTT,
	CWFG_NSENSORS
};

struct cwfg_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	uint8_t		sc_batinfo[BATINFO_SIZE];

	u_int		sc_alert_level;
	u_int		sc_monitor_interval;
	u_int		sc_design_capacity;

	struct sysmon_envsys *sc_sme;

	envsys_data_t	sc_sensor[CWFG_NSENSORS];
};

#define	CWFG_MONITOR_INTERVAL_DEFAULT	8
#define	CWFG_DESIGN_CAPACITY_DEFAULT	2000
#define	CWFG_ALERT_LEVEL_DEFAULT	0

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cellwise,cw2015" },
	{ .compat = "cellwise,cw201x" },	/* DTCOMPAT */
	DEVICE_COMPAT_EOL
};

static int
cwfg_lock(struct cwfg_softc *sc)
{
	return iic_acquire_bus(sc->sc_i2c, 0);
}

static void
cwfg_unlock(struct cwfg_softc *sc)
{
	iic_release_bus(sc->sc_i2c, 0);
}

static int
cwfg_read(struct cwfg_softc *sc, uint8_t reg, uint8_t *val)
{
	return iic_smbus_read_byte(sc->sc_i2c, sc->sc_addr, reg, val, 0);
}

static int
cwfg_write(struct cwfg_softc *sc, uint8_t reg, uint8_t val)
{
	return iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr, reg, val, 0);
}

static void
cwfg_sensor_refresh(struct sysmon_envsys *sme, envsys_data_t *e)
{
	struct cwfg_softc *sc = sme->sme_cookie;
	u_int vcell, rtt, tmp;
	uint8_t val;
	int error, n;

	e->state = ENVSYS_SINVALID;

	if ((error = cwfg_lock(sc)) != 0)
		return;

	switch (e->private) {
	case CWFG_SENSOR_VCELL:
		/* Take the average of three readings */
		vcell = 0;
		for (n = 0; n < 3; n++) {
			if ((error = cwfg_read(sc, VCELL_HI_REG, &val)) != 0)
				goto done;
			tmp = __SHIFTOUT(val, VCELL_HI) << 8;
			if ((error = cwfg_read(sc, VCELL_LO_REG, &val)) != 0)
				goto done;
			tmp |= __SHIFTOUT(val, VCELL_LO);
			vcell += tmp;
		}
		vcell /= 3;

		e->state = ENVSYS_SVALID;
		e->value_cur = ((vcell * VCELL_STEP) / VCELL_DIV) * 1000;
		break;

	case CWFG_SENSOR_SOC:
		if ((error = cwfg_read(sc, SOC_HI_REG, &val)) != 0)
			goto done;

		if (val != 0xff) {
			e->state = ENVSYS_SVALID;
			e->value_cur = val;			/* batt % */
		}
		break;

	case CWFG_SENSOR_RTT:
		if ((error = cwfg_read(sc, RTT_ALRT_HI_REG, &val)) != 0)
			goto done;
		rtt = __SHIFTOUT(val, RTT_HI) << 8;
		if ((error = cwfg_read(sc, RTT_ALRT_LO_REG, &val)) != 0)
			goto done;
		rtt |= __SHIFTOUT(val, RTT_LO);

		if (rtt != 0x1fff) {
			e->state = ENVSYS_SVALID;
			e->value_cur = rtt;			/* minutes */
		}
		break;
	}

done:
	cwfg_unlock(sc);
}

static void
cwfg_attach_battery(struct cwfg_softc *sc)
{
	envsys_data_t *e;

	/* Cell voltage */
	e = &sc->sc_sensor[CWFG_SENSOR_VCELL];
	e->private = CWFG_SENSOR_VCELL;
	e->units = ENVSYS_SVOLTS_DC;
	e->state = ENVSYS_SINVALID;
	strlcpy(e->desc, "battery voltage", sizeof(e->desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, e);

	/* State of charge */
	e = &sc->sc_sensor[CWFG_SENSOR_SOC];
	e->private = CWFG_SENSOR_SOC;
	e->units = ENVSYS_INTEGER;
	e->state = ENVSYS_SINVALID;
	e->flags = ENVSYS_FPERCENT;
	strlcpy(e->desc, "battery percent", sizeof(e->desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, e);

	/* Remaining run time */
	e = &sc->sc_sensor[CWFG_SENSOR_RTT];
	e->private = CWFG_SENSOR_RTT;
	e->units = ENVSYS_INTEGER;
	e->state = ENVSYS_SINVALID;
	strlcpy(e->desc, "battery remaining minutes", sizeof(e->desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, e);
}

static void
cwfg_attach_sensors(struct cwfg_softc *sc)
{
	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = cwfg_sensor_refresh;
	sc->sc_sme->sme_events_timeout = sc->sc_monitor_interval;
	sc->sc_sme->sme_class = SME_CLASS_BATTERY;
	sc->sc_sme->sme_flags = SME_INIT_REFRESH;

	cwfg_attach_battery(sc);

	sysmon_envsys_register(sc->sc_sme);
}

static int
cwfg_set_config(struct cwfg_softc *sc)
{
	u_int alert_level;
	bool need_update;
	uint8_t config, mode, val;
	int error, n;

	/* Read current config */
	if ((error = cwfg_read(sc, CONFIG_REG, &config)) != 0)
		return error;

	/* Update alert level, if necessary */
	alert_level = __SHIFTOUT(config, CONFIG_ATHD);
	if (alert_level != sc->sc_alert_level) {
		config &= ~CONFIG_ATHD;
		config |= __SHIFTIN(sc->sc_alert_level, CONFIG_ATHD);
		if ((error = cwfg_write(sc, CONFIG_REG, config)) != 0)
			return error;
	}

	/* Re-read current config */
	if ((error = cwfg_read(sc, CONFIG_REG, &config)) != 0)
		return error;

	/*
	 * We need to upload a battery profile if either the UFG flag
	 * is unset, or the current battery profile differs from the
	 * one in the DT.
	 */
	need_update = (config & CONFIG_UFG) == 0;
	if (need_update == false) {
		for (n = 0; n < BATINFO_SIZE; n++) {
			if ((error = cwfg_read(sc, BATINFO_REG(n), &val)) != 0)
				return error;
			if (sc->sc_batinfo[n] != val) {
				need_update = true;
				break;
			}
		}
	}
	if (need_update == false)
		return 0;

	aprint_verbose_dev(sc->sc_dev, "updating battery profile\n");

	/* Update battery profile */
	for (n = 0; n < BATINFO_SIZE; n++) {
		val = sc->sc_batinfo[n];
		if ((error = cwfg_write(sc, BATINFO_REG(n), val)) != 0)
			return error;
	}

	/* Set UFG flag to switch to new profile */
	if ((error = cwfg_read(sc, CONFIG_REG, &config)) != 0)
		return error;
	config |= CONFIG_UFG;
	if ((error = cwfg_write(sc, CONFIG_REG, config)) != 0)
		return error;

	/* Restart the IC with new profile */
	if ((error = cwfg_read(sc, MODE_REG, &mode)) != 0)
		return error;
	mode |= MODE_POR;
	if ((error = cwfg_write(sc, MODE_REG, mode)) != 0)
		return error;
	delay(20000);
	mode &= ~MODE_POR;
	if ((error = cwfg_write(sc, MODE_REG, mode)) != 0)
		return error;

	return error;
}

static int
cwfg_init(struct cwfg_softc *sc)
{
	uint8_t mode, soc;
	int error, retry;

	cwfg_lock(sc);

	/* If the device is in sleep mode, wake it up */
	if ((error = cwfg_read(sc, MODE_REG, &mode)) != 0)
		goto done;
	if (__SHIFTOUT(mode, MODE_SLEEP) == MODE_SLEEP_SLEEP) {
		mode &= ~MODE_SLEEP;
		mode |= __SHIFTIN(MODE_SLEEP_WAKE, MODE_SLEEP);
		if ((error = cwfg_write(sc, MODE_REG, mode)) != 0)
			goto done;
	}

	/* Load battery profile */
	if ((error = cwfg_set_config(sc)) != 0)
		goto done;

	/* Wait for chip to become ready */
	for (retry = RESET_COUNT; retry > 0; retry--) {
		if ((error = cwfg_read(sc, SOC_HI_REG, &soc)) != 0)
			goto done;
		if (soc != 0xff)
			break;
		delay(RESET_DELAY);
	}
	if (retry == 0) {
		aprint_error_dev(sc->sc_dev,
		    "WARNING: timeout waiting for chip ready\n");
	}

done:
	cwfg_unlock(sc);

	return error;
}

static int
cwfg_parse_resources(struct cwfg_softc *sc)
{
	const u_int *batinfo;
	u_int val;
	int len = 0, n;

	batinfo = fdtbus_get_prop(sc->sc_phandle,
	    "cellwise,battery-profile", &len);
	if (batinfo == NULL) {
		/* DTCOMPAT */
		batinfo = fdtbus_get_prop(sc->sc_phandle,
		    "cellwise,bat-config-info", &len);
	}
	switch (len) {
	case BATINFO_SIZE:
		memcpy(sc->sc_batinfo, batinfo, BATINFO_SIZE);
		break;
	case BATINFO_SIZE * 4:
		for (n = 0; n < BATINFO_SIZE; n++)
			sc->sc_batinfo[n] = be32toh(batinfo[n]);
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "missing or invalid battery info\n");
		return EINVAL;
	}

	if (of_getprop_uint32(sc->sc_phandle,
	    "cellwise,monitor-interval-ms", &val) == 0) {
		sc->sc_monitor_interval = howmany(val, 1000);
	} else if (of_getprop_uint32(sc->sc_phandle,
	    "cellwise,monitor-interval", &val) == 0) {
		/* DTCOMPAT */
		sc->sc_monitor_interval = val;
	} else {
		sc->sc_monitor_interval = CWFG_MONITOR_INTERVAL_DEFAULT;
	}

	const int bphandle = fdtbus_get_phandle(sc->sc_phandle, "monitored-battery");
	if (bphandle != -1 && of_getprop_uint32(bphandle,
	    "charge-full-design-microamp-hours", &val) == 0) {
		sc->sc_design_capacity = howmany(val, 1000);
	} else if (of_getprop_uint32(sc->sc_phandle,
	    "cellwise,design-capacity", &val) == 0) {
		/* DTCOMPAT */
		sc->sc_design_capacity = val;
	} else {
		sc->sc_design_capacity = CWFG_DESIGN_CAPACITY_DEFAULT;
	}

	if (of_getprop_uint32(sc->sc_phandle,
	    "cellwise,alert-level", &sc->sc_alert_level) != 0) {
		sc->sc_alert_level = CWFG_ALERT_LEVEL_DEFAULT;
	}

	return 0;
}

static int
cwfg_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	/* This device is direct-config only. */

	return 0;
}

static void
cwfg_attach(device_t parent, device_t self, void *aux)
{
	struct cwfg_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t ver;
	int error;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	cwfg_lock(sc);
	error = cwfg_read(sc, VERSION_REG, &ver);
	cwfg_unlock(sc);

	if (error != 0) {
		aprint_error(": device not responding, error = %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": CellWise CW2015 Fuel Gauge IC (ver. 0x%02x)\n", ver);

	if (cwfg_parse_resources(sc) != 0) {
		aprint_error_dev(self, "failed to parse resources\n");
		return;
	}

	if (cwfg_init(sc) != 0) {
		aprint_error_dev(self, "failed to initialize device\n");
		return;
	}

	cwfg_attach_sensors(sc);
}

CFATTACH_DECL_NEW(cwfg, sizeof(struct cwfg_softc),
    cwfg_match, cwfg_attach, NULL, NULL);
