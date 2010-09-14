/*	$NetBSD: smartbat.c,v 1.4 2010/09/14 02:45:25 macallan Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: smartbat.c,v 1.4 2010/09/14 02:45:25 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <macppc/dev/pmuvar.h>
#include <macppc/dev/batteryvar.h>
#include <machine/bus.h>
#include "opt_battery.h"

#ifdef SMARTBAT_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define BAT_AC_PRESENT		0
#define BAT_PRESENT		1
#define BAT_VOLTAGE		2
#define BAT_CURRENT		3
#define BAT_MAX_CHARGE		4
#define BAT_CHARGE		5
#define BAT_CHARGING		6
#define BAT_FULL		7
#define BAT_NSENSORS		8  /* number of sensors */

struct smartbat_softc {
	struct device sc_dev;
	struct pmu_ops *sc_pmu_ops;
	int sc_num;
	
	/* envsys stuff */
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[BAT_NSENSORS];
	struct sysmon_pswitch sc_sm_acpower;

	/* battery status */
	int sc_flags;
	int sc_oflags;
	int sc_voltage;
	int sc_charge;
	int sc_max_charge;
	int sc_draw;
	int sc_time;
	uint32_t sc_timestamp;
};

static void smartbat_attach(struct device *, struct device *, void *);
static int smartbat_match(struct device *, struct cfdata *, void *);
static void smartbat_setup_envsys(struct smartbat_softc *);
static void smartbat_refresh(struct sysmon_envsys *, envsys_data_t *);
static void smartbat_poll(void *);
static int smartbat_update(struct smartbat_softc *, int);

CFATTACH_DECL(smartbat, sizeof(struct smartbat_softc),
    smartbat_match, smartbat_attach, NULL, NULL);

static int
smartbat_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct battery_attach_args *baa = aux;

	if (baa->baa_type == BATTERY_TYPE_SMART)
		return 1;

	return 0;
}

static void
smartbat_attach(struct device *parent, struct device *self, void *aux)
{
	struct battery_attach_args *baa = aux;
	struct smartbat_softc *sc = (struct smartbat_softc *)self;

	sc->sc_pmu_ops = baa->baa_pmu_ops;
	sc->sc_num = baa->baa_num;

	printf(" addr %d: smart battery\n", sc->sc_num);

	smartbat_update(sc, 1);
	/* trigger a status update */
	sc->sc_oflags = ~sc->sc_flags;

	smartbat_setup_envsys(sc);
	sc->sc_pmu_ops->register_callback(sc->sc_pmu_ops->cookie, smartbat_poll,
	    sc);

	memset(&sc->sc_sm_acpower, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_acpower.smpsw_name = "AC Power";
	sc->sc_sm_acpower.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_acpower) != 0)
		printf("%s: unable to register AC power status with sysmon\n",
		    sc->sc_dev.dv_xname);
}

#define INITDATA(index, unit, string)					\
	sc->sc_sensor[index].units = unit;     				\
	snprintf(sc->sc_sensor[index].desc,				\
	    sizeof(sc->sc_sensor[index].desc), "%s", string);

static void
smartbat_setup_envsys(struct smartbat_softc *sc)
{
	int i;

	sc->sc_sme = sysmon_envsys_create();

	INITDATA(BAT_AC_PRESENT, ENVSYS_INDICATOR, "AC present");
	INITDATA(BAT_PRESENT, ENVSYS_INDICATOR, "Battery present");
	INITDATA(BAT_VOLTAGE, ENVSYS_SVOLTS_DC, "Battery voltage");
	INITDATA(BAT_CURRENT, ENVSYS_SAMPS, "Battery current");
	INITDATA(BAT_MAX_CHARGE, ENVSYS_SAMPHOUR, "Battery design cap");
	INITDATA(BAT_CHARGE, ENVSYS_SAMPHOUR, "Battery charge");
	INITDATA(BAT_CHARGING, ENVSYS_BATTERY_CHARGE, "Battery charging");
	INITDATA(BAT_FULL, ENVSYS_INDICATOR, "Battery full");
#undef INITDATA

	for (i = 0; i < BAT_NSENSORS; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}

	sc->sc_sme->sme_name = sc->sc_dev.dv_xname;
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = smartbat_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
smartbat_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct smartbat_softc *sc = sme->sme_cookie;
	int which = edata->sensor, present;

	smartbat_update(sc, 0);
	present = (sc->sc_flags & PMU_PWR_BATT_PRESENT) != 0;

	if (present) {
		switch (which) {
		case BAT_AC_PRESENT:
			edata->value_cur = (sc->sc_flags & PMU_PWR_AC_PRESENT);
			break;
		case BAT_PRESENT:
			edata->value_cur = present;
			break;
		case BAT_VOLTAGE:
			edata->value_cur = sc->sc_voltage * 1000;
			break;
		case BAT_CURRENT:
			edata->value_cur = sc->sc_draw * 1000;
			break;
		case BAT_MAX_CHARGE:
			edata->value_cur = sc->sc_max_charge * 1000;
			break;
		case BAT_CHARGE:
			edata->value_cur = sc->sc_charge * 1000;
			break;
		case BAT_CHARGING:
			if ((sc->sc_flags & PMU_PWR_BATT_CHARGING) &&
			    (sc->sc_flags & PMU_PWR_AC_PRESENT))
				edata->value_cur = 1;
			else
				edata->value_cur = 0;

			break;
		case BAT_FULL:
			edata->value_cur = (sc->sc_flags & PMU_PWR_BATT_FULL);
			break;
		}
		edata->state = ENVSYS_SVALID;
	} else {
		/* battery isn't there */
		switch (which) {
		case BAT_AC_PRESENT:
			edata->value_cur = (sc->sc_flags & PMU_PWR_AC_PRESENT);
			edata->state = ENVSYS_SVALID;
			break;
		case BAT_PRESENT:
			edata->value_cur = present;
			edata->state = ENVSYS_SVALID;
			break;
		default:
			edata->state = ENVSYS_SINVALID;
			edata->value_cur = 0;
		}
	}
}

/*
 * Thanks to Paul Mackerras and Fabio Riccardi's Linux implementation
 * for a clear description of the PMU results.
 */
static int
smartbat_update(struct smartbat_softc *sc, int out)
{
	int len;
	uint8_t buf[16];
	uint8_t battery_number;

	if (sc->sc_timestamp == time_second)
		return 0;
	sc->sc_timestamp = time_second;

	/* sc_num starts from 0, but we need to start from 1 */
	battery_number = sc->sc_num + 1;
	len = sc->sc_pmu_ops->do_command(sc->sc_pmu_ops->cookie,
					 PMU_SMART_BATTERY_STATE,
					 1, &battery_number,
					 16, buf);

	if (len < 0) {
		DPRINTF("%s: couldn't get battery data\n", sc->sc_dev.dv_xname);
		/* XXX: the return value is never checked */
		return -1;
	}

	/* Now, buf[0] is the command number, which we already know.
	   That's why all indexes are off by one compared to
	   pm_battery_info_smart in pm_direct.c.
	*/
	sc->sc_flags = buf[2];

        /* XXX: are these all valid for smart batteries? */
	if (out) {
		printf(" flags: %x", buf[2]);
		if (buf[2] & PMU_PWR_AC_PRESENT)
			printf(" AC");
		if (buf[2] & PMU_PWR_BATT_CHARGING)
			printf(" charging");
		if (buf[2] & PMU_PWR_BATT_PRESENT)
			printf(" present");
		if (buf[2] & PMU_PWR_BATT_FULL)
			printf(" full");
		printf("\n");
	}

	switch(buf[1]) {
	case 3:
	case 4:
		sc->sc_charge = buf[3];
		sc->sc_max_charge = buf[4];
		sc->sc_draw = *((signed char *)&buf[5]);
		sc->sc_voltage = buf[6];
		break;
	case 5:
		sc->sc_charge = ((buf[3] << 8) | (buf[4]));
		sc->sc_max_charge = ((buf[5] << 8) | (buf[6]));
		sc->sc_draw = *((signed short *)&buf[7]);
		sc->sc_voltage = ((buf[9] << 8) | (buf[8]));
		break;
	default:
		/* XXX - Error condition */
		DPRINTF("%s: why is buf[1] %x?\n", sc->sc_dev.dv_xname, buf[1]);
		sc->sc_charge = 0;
		sc->sc_max_charge = 0;
		sc->sc_draw = 0;
		sc->sc_voltage = 0;
		break;
	}

	return 1;
}

static void
smartbat_poll(void *cookie)
{
	struct smartbat_softc *sc = cookie;

	smartbat_update(sc, 0);
	if ((sc->sc_flags & PMU_PWR_AC_PRESENT) == sc->sc_oflags)
		return;

	sc->sc_oflags = sc->sc_flags & PMU_PWR_AC_PRESENT;

	sysmon_pswitch_event(&sc->sc_sm_acpower, 
	    sc->sc_oflags ? PSWITCH_EVENT_PRESSED :
	    PSWITCH_EVENT_RELEASED);
}
