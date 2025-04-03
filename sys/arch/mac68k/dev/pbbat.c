/* $NetBSD: pbbat.c,v 1.1 2025/04/03 01:54:46 nat Exp $ */

/*-
 * Copyright (c) 2025 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

/* Based on acpibat(4). */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum of By Noon Software, Inc.
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

/*
 * Copyright 2001 Bill Sommerfeld.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* AC Adaptor attachment and logic based on macppc/smartbat(4). */

/*-
 * Copyright (c) 2007 Michael Lorenz
 *               2008 Magnus Henoch
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
__KERNEL_RCSID(0, "$NetBSD: pbbat.c,v 1.1 2025/04/03 01:54:46 nat Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/types.h>
#include <sys/systm.h>

#include <dev/sysmon/sysmonvar.h>

#include <machine/param.h>
#include <machine/cpu.h>

#include <mac68k/dev/pm_direct.h>

static int	pbbatmatch(device_t, cfdata_t, void *);
static void	pbbatattach(device_t, device_t, void *);

static void	bat_get_pm_limits(device_t);
static uint16_t	bat_get_status(device_t);
static void	bat_init_envsys(device_t);
static void	bat_update_status(void *);

extern int	pm_pmgrop_pm1(PMData *);

struct pbatt_softc {
	device_t		 sc_dev;
	struct sysmon_envsys	*sc_ac_sme;
	envsys_data_t		 sc_ac_sensor[1];
	struct sysmon_pswitch	 sc_sm_acpower;
	int8_t			 sc_ac_state;
	struct sysmon_envsys	*sc_bat_sme;
	envsys_data_t		*sc_bat_sensor;
	struct timeval		 sc_last;
	kmutex_t		 sc_mutex;
	int32_t			 sc_dcapacity;
	int32_t			 sc_dvoltage;
	int32_t			 sc_disrate;
	int32_t			 sc_chargerate;
	int32_t			 sc_empty;
	int32_t			 sc_lcapacity;
	int32_t			 sc_wcapacity;
	int                      sc_present;
};

#define PBBAT_AC_PRESENT	 0

/* AC Adaptor states */
#define PBBAT_AC_UNKNOWN	-1
#define PBBAT_AC_DISCONNECTED	 0
#define PBBAT_AC_CONNECTED	 1

enum {
	PBBAT_PRESENT		 = 0,
	PBBAT_DVOLTAGE		 = 1,
	PBBAT_VOLTAGE		 = 2,
	PBBAT_DCAPACITY		 = 3,
	PBBAT_LFCCAPACITY	 = 4,
	PBBAT_CAPACITY		 = 5,
	PBBAT_CHARGERATE	 = 6,
	PBBAT_DISCHARGERATE	 = 7,
	PBBAT_CHARGING		 = 8,
	PBBAT_CHARGE_STATE	 = 9,
	PBBAT_COUNT		 = 10
};

/* Driver definition */
CFATTACH_DECL_NEW(pbbat, sizeof(struct pbatt_softc),
    pbbatmatch, pbbatattach, NULL, NULL);

/* Battery voltage definitions (mV) */
#define VOLTS_DESIGN	6000
#define WATTS_DESIGN	60000	/* mW */
#define VOLTS_CHARGING	6600
#define VOLTS_NOBATT	7700

#define VOLTS_MULTI	35	/* PM value multiplier. */
#define LIMIT_SCALE	(100 * 100 / (VOLTS_DESIGN / 1000))

#define PM_BATT_VOLTS	0x68	/* 0x69 is a duplicate. */
#define PM_BATT_LIMITS	0x6a

static int
pbbatmatch(device_t parent, cfdata_t cf, void *aux)
{
	switch (mac68k_machine.machineid) {
		case MACH_MACPB140:
		case MACH_MACPB145:
		case MACH_MACPB160:
		case MACH_MACPB165:
		case MACH_MACPB165C:
		case MACH_MACPB170:
		case MACH_MACPB180:
		case MACH_MACPB180C:
			return 1;
			break;
		default:
			return 0;
	}

	return 0;
}

static void
pbbatattach(device_t parent, device_t self, void *aux)
{
	struct pbatt_softc *sc = device_private(self);

	aprint_naive(": PowerBook Battery\n");
	aprint_normal(": PowerBook Battery\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_bat_sensor = kmem_zalloc(PBBAT_COUNT *
	    sizeof(*sc->sc_bat_sensor), KM_SLEEP);

	memset(&sc->sc_sm_acpower, 0, sizeof(struct sysmon_pswitch));
	sc->sc_ac_state = PBBAT_AC_UNKNOWN;
	sc->sc_sm_acpower.smpsw_name = "AC Power";
	sc->sc_sm_acpower.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_acpower) != 0)
		printf("%s: unable to register AC power status with sysmon\n",
		    device_xname(sc->sc_dev));

	config_interrupts(self, bat_init_envsys);
}

static void
bat_get_pm_limits(device_t self)
{
	int s;
	int rval;
	PMData pmdata;
	struct pbatt_softc *sc = device_private(self);

	s = splhigh();

	pmdata.command = PM_BATT_LIMITS;
	pmdata.num_data = 0;
	pmdata.data[0] = pmdata.data[1] = 0;
	pmdata.s_buf = pmdata.data;
	pmdata.r_buf = pmdata.data;
	rval = pm_pmgrop_pm1(&pmdata);
	if (rval != 0) {
#ifdef ADB_DEBUG
		if (adb_debug)
			printf("pm: PM is not ready. error code=%08x\n", rval);
#endif
		splx(s);
		return;
	}

	splx(s);

	sc->sc_empty = (pmdata.data[1] & 0xff) * VOLTS_MULTI;
	sc->sc_lcapacity = (pmdata.data[0] & 0xff) * VOLTS_MULTI;
	sc->sc_wcapacity = sc->sc_lcapacity * 12 / 10;

	return;
}

static uint16_t
bat_get_voltage(void)
{
	int s;
	int rval;
	PMData pmdata;

	s = splhigh();

	pmdata.command = PM_BATT_VOLTS;
	pmdata.num_data = 0;
	pmdata.data[0] = pmdata.data[1] = 0;
	pmdata.s_buf = pmdata.data;
	pmdata.r_buf = pmdata.data;
	rval = pm_pmgrop_pm1(&pmdata);
	if (rval != 0) {
#ifdef ADB_DEBUG
		if (adb_debug)
			printf("pm: PM is not ready. error code=%08x\n", rval);
#endif
		splx(s);
		return 0;
	}

	splx(s);

	return (pmdata.data[1] & 0xff) * VOLTS_MULTI;
}

static void
bat_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	device_t self = sme->sme_cookie;
	struct pbatt_softc *sc = device_private(self);
	struct timeval tv, tmp;

	tmp.tv_sec = 10;
	tmp.tv_usec = 0;

	microuptime(&tv);
	timersub(&tv, &tmp, &tv);
	if (timercmp(&tv, &sc->sc_last, <) != 0)
		return;

	bat_update_status(self);
}

static void
bat_refresh_ac(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct pbatt_softc *sc = sme->sme_cookie;
	int which = edata->sensor;

	mutex_enter(&sc->sc_mutex);
	switch (which) {
		case PBBAT_AC_PRESENT:
			edata->value_cur =
			    (sc->sc_ac_state == PBBAT_AC_CONNECTED ? 1 : 0);
			edata->state = ENVSYS_SVALID;
			break;
		default:
			edata->value_cur = 0;
			edata->state = ENVSYS_SINVALID;
	}
	mutex_exit(&sc->sc_mutex);
}

static void
bat_get_info(device_t dv)
{
	struct pbatt_softc *sc = device_private(dv);
	int capunit;

	capunit = ENVSYS_SWATTHOUR;

	sc->sc_bat_sensor[PBBAT_DCAPACITY].units = capunit;
	sc->sc_bat_sensor[PBBAT_CHARGERATE].units = capunit;
	sc->sc_bat_sensor[PBBAT_DISCHARGERATE].units = capunit;
	sc->sc_bat_sensor[PBBAT_LFCCAPACITY].units = capunit;
	sc->sc_bat_sensor[PBBAT_CAPACITY].units = capunit;

	/* Design capacity. */

	/*
	 * This is a guesstimate - repacked battery runs at 10 Watts/h for an
	 * 1 hour.
         */

	sc->sc_bat_sensor[PBBAT_DCAPACITY].value_cur = WATTS_DESIGN * 1000;
	sc->sc_bat_sensor[PBBAT_DCAPACITY].state = ENVSYS_SVALID;

	/* Design voltage. */
	sc->sc_bat_sensor[PBBAT_DVOLTAGE].value_cur = VOLTS_DESIGN * 1000;
	sc->sc_bat_sensor[PBBAT_DVOLTAGE].state = ENVSYS_SVALID;

	sc->sc_bat_sensor[PBBAT_LFCCAPACITY].state = ENVSYS_SINVALID;

	bat_get_pm_limits(dv);

	sc->sc_bat_sensor[PBBAT_CAPACITY].value_max = 100 * 1000 * 1000;
}

static void
bat_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	device_t self = sme->sme_cookie;
	struct pbatt_softc *sc = device_private(self);

	if (edata->sensor != PBBAT_CAPACITY)
		return;

	limits->sel_critmin = sc->sc_lcapacity * LIMIT_SCALE;
	limits->sel_warnmin = sc->sc_wcapacity * LIMIT_SCALE;

	*props |= PROP_BATTCAP | PROP_BATTWARN | PROP_DRIVER_LIMITS;
}

static void
bat_update_status(void *arg)
{
	device_t dv = arg;
	struct pbatt_softc *sc = device_private(dv);
	int i;
	uint16_t val;

	mutex_enter(&sc->sc_mutex);

	val = bat_get_status(dv);
	if (val != 0) {
		if (sc->sc_present == 0)
			bat_get_info(dv);
	} else {
		i = PBBAT_DVOLTAGE;
		while (i < PBBAT_COUNT) {
			sc->sc_bat_sensor[i].state = ENVSYS_SINVALID;
			i++;
		}
	}

	sc->sc_present = (val >= VOLTS_NOBATT ? 0 : 1);

	microuptime(&sc->sc_last);

	mutex_exit(&sc->sc_mutex);
}

static void
bat_init_envsys(device_t dv)
{
	struct pbatt_softc *sc = device_private(dv);
	int i;

#define INITDATA(index, unit, string)					\
	sc->sc_ac_sensor[index].units = unit;     			\
	sc->sc_ac_sensor[index].state = ENVSYS_SINVALID;		\
	snprintf(sc->sc_ac_sensor[index].desc,				\
	    sizeof(sc->sc_ac_sensor[index].desc), "%s", string);

		INITDATA(PBBAT_AC_PRESENT, ENVSYS_INDICATOR, "connected");
#undef INITDATA

	sc->sc_ac_sme = sysmon_envsys_create();

	if (sysmon_envsys_sensor_attach(sc->sc_ac_sme, &sc->sc_ac_sensor[0])) {
		sysmon_envsys_destroy(sc->sc_ac_sme);
		return;
	}

	sc->sc_ac_sme->sme_name = "AC Adaptor";
	sc->sc_ac_sme->sme_cookie = sc;
	sc->sc_ac_sme->sme_refresh = bat_refresh_ac;
	sc->sc_ac_sme->sme_class = SME_CLASS_ACADAPTER;

	if (sysmon_envsys_register(sc->sc_ac_sme)) {
		aprint_error("%s: unable to register AC with sysmon\n",
		    device_xname(sc->sc_dev));
		sysmon_envsys_destroy(sc->sc_ac_sme);
	}

#define INITDATA(index, unit, string)					\
	do {								\
		sc->sc_bat_sensor[index].state = ENVSYS_SVALID;		\
		sc->sc_bat_sensor[index].units = unit;			\
		(void)strlcpy(sc->sc_bat_sensor[index].desc, string,	\
		    sizeof(sc->sc_bat_sensor[index].desc));			\
	} while (/* CONSTCOND */ 0)

	INITDATA(PBBAT_PRESENT, ENVSYS_INDICATOR, "present");
	INITDATA(PBBAT_DCAPACITY, ENVSYS_SWATTHOUR, "design cap");
	INITDATA(PBBAT_LFCCAPACITY, ENVSYS_SWATTHOUR, "last full cap");
	INITDATA(PBBAT_DVOLTAGE, ENVSYS_SVOLTS_DC, "design voltage");
	INITDATA(PBBAT_VOLTAGE, ENVSYS_SVOLTS_DC, "voltage");
	INITDATA(PBBAT_CAPACITY, ENVSYS_SWATTHOUR, "charge");
	INITDATA(PBBAT_CHARGERATE, ENVSYS_SWATTS, "charge rate");
	INITDATA(PBBAT_DISCHARGERATE, ENVSYS_SWATTS, "discharge rate");
	INITDATA(PBBAT_CHARGING, ENVSYS_BATTERY_CHARGE, "charging");
	INITDATA(PBBAT_CHARGE_STATE, ENVSYS_BATTERY_CAPACITY, "charge state");

#undef INITDATA

	sc->sc_bat_sensor[PBBAT_CHARGE_STATE].value_cur =
		ENVSYS_BATTERY_CAPACITY_NORMAL;

	sc->sc_bat_sensor[PBBAT_CAPACITY].flags |=
	    ENVSYS_FPERCENT | ENVSYS_FVALID_MAX | ENVSYS_FMONLIMITS;

	sc->sc_bat_sensor[PBBAT_CHARGE_STATE].flags |= ENVSYS_FMONSTCHANGED;

	/* Disable userland monitoring on these sensors. */
	sc->sc_bat_sensor[PBBAT_VOLTAGE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_bat_sensor[PBBAT_CHARGERATE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_bat_sensor[PBBAT_DISCHARGERATE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_bat_sensor[PBBAT_DCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_bat_sensor[PBBAT_LFCCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_bat_sensor[PBBAT_DVOLTAGE].flags = ENVSYS_FMONNOTSUPP;

	sc->sc_bat_sensor[PBBAT_CHARGERATE].flags |= ENVSYS_FHAS_ENTROPY;
	sc->sc_bat_sensor[PBBAT_DISCHARGERATE].flags |= ENVSYS_FHAS_ENTROPY;

	sc->sc_bat_sme = sysmon_envsys_create();

	for (i = 0; i < PBBAT_COUNT; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_bat_sme,
			&sc->sc_bat_sensor[i]))
			goto fail;
	}

	sc->sc_bat_sme->sme_name = device_xname(dv);
	sc->sc_bat_sme->sme_cookie = dv;
	sc->sc_bat_sme->sme_refresh = bat_refresh;
	sc->sc_bat_sme->sme_class = SME_CLASS_BATTERY;
	sc->sc_bat_sme->sme_flags = SME_POLL_ONLY;
	sc->sc_bat_sme->sme_get_limits = bat_get_limits;

	if (sysmon_envsys_register(sc->sc_bat_sme))
		goto fail;

	bat_get_pm_limits(dv);
	bat_update_status(dv);

	return;
fail:
	aprint_error("failed to initialize sysmon\n");

	sysmon_envsys_destroy(sc->sc_bat_sme);
	kmem_free(sc->sc_bat_sensor, PBBAT_COUNT * sizeof(*sc->sc_bat_sensor));

	sc->sc_bat_sme = NULL;
	sc->sc_bat_sensor = NULL;
}

static uint16_t
bat_get_status(device_t dv)
{
	struct pbatt_softc *sc = device_private(dv);
	uint16_t val;

	val = bat_get_voltage();

	sc->sc_bat_sensor[PBBAT_PRESENT].state = ENVSYS_SVALID;
	sc->sc_bat_sensor[PBBAT_PRESENT].value_cur = 1;

	if (val > VOLTS_NOBATT) {
		sc->sc_bat_sensor[PBBAT_PRESENT].value_cur = 0;
		sc->sc_bat_sensor[PBBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_bat_sensor[PBBAT_CHARGING].value_cur = 0;
		sc->sc_bat_sensor[PBBAT_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_bat_sensor[PBBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
		if (sc->sc_ac_state != PBBAT_AC_CONNECTED) {
			sysmon_pswitch_event(&sc->sc_sm_acpower,
			    PSWITCH_EVENT_PRESSED);
		}
		sc->sc_ac_state = PBBAT_AC_CONNECTED;
	} else if (val > VOLTS_CHARGING) {
		sc->sc_bat_sensor[PBBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_bat_sensor[PBBAT_CHARGING].value_cur = 1;
		sc->sc_bat_sensor[PBBAT_CHARGERATE].state = ENVSYS_SVALID;
		sc->sc_bat_sensor[PBBAT_CHARGERATE].value_cur =
		    sc->sc_chargerate;
		sc->sc_bat_sensor[PBBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
		if (sc->sc_ac_state != PBBAT_AC_CONNECTED) {
			sysmon_pswitch_event(&sc->sc_sm_acpower,
			    PSWITCH_EVENT_PRESSED);
		}
		sc->sc_ac_state = PBBAT_AC_CONNECTED;
	} else {
		sc->sc_bat_sensor[PBBAT_CHARGING].value_cur = 0;
		sc->sc_bat_sensor[PBBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_bat_sensor[PBBAT_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_bat_sensor[PBBAT_DISCHARGERATE].state = ENVSYS_SVALID;
		sc->sc_bat_sensor[PBBAT_DISCHARGERATE].value_cur =
		    sc->sc_disrate;
		if (sc->sc_ac_state != PBBAT_AC_DISCONNECTED) {
			sysmon_pswitch_event(&sc->sc_sm_acpower,
			    PSWITCH_EVENT_RELEASED);
		}
		sc->sc_ac_state = PBBAT_AC_DISCONNECTED;
	}

	/* Remaining capacity. */
	sc->sc_chargerate = sc->sc_bat_sensor[PBBAT_CAPACITY].value_cur;
	sc->sc_disrate = sc->sc_bat_sensor[PBBAT_CAPACITY].value_cur;

	sc->sc_bat_sensor[PBBAT_CAPACITY].value_cur =
	    (val - sc->sc_empty) * 10 * LIMIT_SCALE;

	sc->sc_chargerate =
	    (sc->sc_bat_sensor[PBBAT_CAPACITY].value_cur - sc->sc_chargerate) * 10;
	sc->sc_disrate =
	    (sc->sc_disrate - sc->sc_bat_sensor[PBBAT_CAPACITY].value_cur) * 10;

	/* Battery voltage. */
	sc->sc_bat_sensor[PBBAT_VOLTAGE].value_cur = val * 1000;
	sc->sc_bat_sensor[PBBAT_VOLTAGE].state =
	    (val >= VOLTS_NOBATT ? ENVSYS_SINVALID : ENVSYS_SVALID);

	if (val < sc->sc_lcapacity) {
		sc->sc_bat_sensor[PBBAT_CAPACITY].state = ENVSYS_SCRITUNDER;
		sc->sc_bat_sensor[PBBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_CRITICAL;
	} else if (val < sc->sc_wcapacity) {
		sc->sc_bat_sensor[PBBAT_CAPACITY].state = ENVSYS_SWARNUNDER;
		sc->sc_bat_sensor[PBBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_WARNING;
	} else {
		sc->sc_bat_sensor[PBBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_NORMAL;
	}

	sc->sc_bat_sensor[PBBAT_CHARGE_STATE].state = ENVSYS_SVALID;

	return val;
}
