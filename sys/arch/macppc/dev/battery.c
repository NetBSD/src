/*	$NetBSD: battery.c,v 1.1.4.6 2008/01/21 09:37:26 yamt Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: battery.c,v 1.1.4.6 2008/01/21 09:37:26 yamt Exp $");

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
#include <machine/pio.h>
#include "opt_battery.h"

#ifdef BATTERY_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define BTYPE_COMET	1
#define BTYPE_HOOPER	2

#define BAT_CPU_TEMPERATURE	0
#define BAT_AC_PRESENT		1
#define BAT_PRESENT		2
#define BAT_VOLTAGE		3
#define BAT_CURRENT		4
#define BAT_MAX_CHARGE		5
#define BAT_CHARGE		6
#define BAT_CHARGING		7
#define BAT_FULL		8
#define BAT_TEMPERATURE		9
#define BAT_NSENSORS		10  /* number of sensors */

struct battery_softc {
	struct device sc_dev;
	struct pmu_ops *sc_pmu_ops;
	int sc_type;
	
	/* envsys stuff */
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[BAT_NSENSORS];
	struct sysmon_pswitch sc_sm_acpower;

	/* battery status */
	int sc_flags;
	int sc_oflags;
	int sc_voltage;
	int sc_charge;
	int sc_current;
	int sc_time;
	int sc_cpu_temp;
	int sc_bat_temp;
	int sc_vmax_charged;
	int sc_vmax_charging;
	uint32_t sc_timestamp;
};

static void battery_attach(struct device *, struct device *, void *);
static int battery_match(struct device *, struct cfdata *, void *);
static int battery_update(struct battery_softc *, int);
static void battery_setup_envsys(struct battery_softc *);
static void battery_refresh(struct sysmon_envsys *, envsys_data_t *);
static void battery_poll(void *);

CFATTACH_DECL(battery, sizeof(struct battery_softc),
    battery_match, battery_attach, NULL, NULL);

static int
battery_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct battery_attach_args *baa = aux;

	if (baa->baa_type == BATTERY_TYPE_LEGACY)
		return 1;

	return 0;
}

static void
battery_attach(struct device *parent, struct device *self, void *aux)
{
	struct battery_attach_args *baa = aux;
	struct battery_softc *sc = (struct battery_softc *)self;
	uint32_t reg;

	sc->sc_pmu_ops = baa->baa_pmu_ops;
	printf(": legacy battery ");

	reg = in32rb(0xf3000034);
	DPRINTF("reg: %08x\n", reg);
	if (reg & 0x20000000) {
		sc->sc_type = BTYPE_HOOPER;
		sc->sc_vmax_charged = 330;
		sc->sc_vmax_charging = 365;
		printf("[hooper]\n");
	} else {
		sc->sc_type = BTYPE_COMET;
		sc->sc_vmax_charged = 189;
		sc->sc_vmax_charging = 213;
		printf("[comet]\n");
	}
	battery_update(sc, 1);
	/* trigger a status update */
	sc->sc_oflags = ~sc->sc_flags;

	battery_setup_envsys(sc);
	sc->sc_pmu_ops->register_callback(sc->sc_pmu_ops->cookie, battery_poll,
	    sc);

	memset(&sc->sc_sm_acpower, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_acpower.smpsw_name = "AC Power";
	sc->sc_sm_acpower.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_acpower) != 0)
		printf("%s: unable to register AC power status with sysmon\n",
		    sc->sc_dev.dv_xname);

}

static int
battery_update(struct battery_softc *sc, int out)
{
	int len, vmax, pcharge, vb;
	uint8_t buf[16];

	if (sc->sc_timestamp == time_second)
		return 0;
	sc->sc_timestamp = time_second;

	len = sc->sc_pmu_ops->do_command(sc->sc_pmu_ops->cookie,
	    PMU_BATTERY_STATE, 0, NULL, 16, buf);
	if (len != 9)
		return -1;	

	sc->sc_flags = buf[1];

	if (out) {
		if (buf[1] & PMU_PWR_AC_PRESENT)
			printf(" AC");
		if (buf[1] & PMU_PWR_BATT_CHARGING)
			printf(" charging");
		if (buf[1] & PMU_PWR_BATT_PRESENT)
			printf(" present");
		if (buf[1] & PMU_PWR_BATT_FULL)
			printf(" full");
		printf("\n");
	}

	sc->sc_cpu_temp = buf[4];

	if ((sc->sc_flags & PMU_PWR_BATT_PRESENT) == 0) {
		sc->sc_voltage = 0;
		sc->sc_current = 0;
		sc->sc_bat_temp = 0;
		sc->sc_charge = 0;
		sc->sc_time = 0;
		return 0;
	}

	vmax = sc->sc_vmax_charged;
	vb = (buf[2] << 8) | buf[3];
	sc->sc_voltage = (vb * 265 + 72665) / 10;
	sc->sc_current = buf[6];
	if ((sc->sc_flags & PMU_PWR_AC_PRESENT) == 0) { 
	    	if (sc->sc_current > 200)
			vb += ((sc->sc_current - 200) * 15) / 100;
	} else {
		vmax = sc->sc_vmax_charging;
	}
	sc->sc_charge = (100 * vb) / vmax;
	if (sc->sc_flags & PMU_PWR_PCHARGE_RESET) {
		pcharge = (buf[7] << 8) | buf[8];
		if (pcharge > 6500)
			pcharge = 6500;
		pcharge = 100 - pcharge * 100 / 6500;
		if (pcharge < sc->sc_charge)
			sc->sc_charge = pcharge;
	}
	if (sc->sc_current > 0) {
		sc->sc_time = (sc->sc_charge * 16440) / sc->sc_current;
	} else 
		sc->sc_time = 0;
	
	sc->sc_bat_temp = buf[5];

	if (out) {
		printf("voltage: %d.%03d\n", sc->sc_voltage / 1000,
		    sc->sc_voltage % 1000);
		printf("charge:  %d%%\n", sc->sc_charge);
		if (sc->sc_time > 0)
			printf("time:    %d:%02d\n", sc->sc_time / 60, 
			    sc->sc_time % 60);
	}

	return 0;
}

#define INITDATA(index, unit, string)					\
	sc->sc_sensor[index].units = unit;     				\
	snprintf(sc->sc_sensor[index].desc,				\
	    sizeof(sc->sc_sensor[index].desc), "%s", string);

static void
battery_setup_envsys(struct battery_softc *sc)
{
	int i;

	sc->sc_sme = sysmon_envsys_create();

	INITDATA(BAT_CPU_TEMPERATURE, ENVSYS_STEMP, "CPU temperature");
	INITDATA(BAT_AC_PRESENT, ENVSYS_INDICATOR, "AC present");
	INITDATA(BAT_PRESENT, ENVSYS_INDICATOR, "Battery present");
	INITDATA(BAT_VOLTAGE, ENVSYS_SVOLTS_DC, "Battery voltage");
	INITDATA(BAT_CHARGE, ENVSYS_INTEGER, "Battery charge");
	INITDATA(BAT_MAX_CHARGE, ENVSYS_INTEGER, "Battery design cap");
	INITDATA(BAT_CURRENT, ENVSYS_SAMPS, "Battery current");
	INITDATA(BAT_TEMPERATURE, ENVSYS_STEMP, "Battery temperature");
	INITDATA(BAT_CHARGING, ENVSYS_INDICATOR, "Battery charging");
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
	sc->sc_sme->sme_refresh = battery_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
battery_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct battery_softc *sc = sme->sme_cookie;
	int which = edata->sensor;

	battery_update(sc, 0);

	switch (which) {
	case BAT_CPU_TEMPERATURE:
		edata->value_cur = sc->sc_cpu_temp * 1000000 + 273150000;
		break;
	case BAT_AC_PRESENT:
		edata->value_cur = (sc->sc_flags & PMU_PWR_AC_PRESENT);
		break;
	case BAT_VOLTAGE:
		edata->value_cur = sc->sc_voltage * 1000;
		break;
	case BAT_CURRENT:
		edata->value_cur = sc->sc_current * 1000;
		break;
	case BAT_CHARGE:
		edata->value_cur = sc->sc_charge;
		break;
	case BAT_MAX_CHARGE:
		edata->value_cur = 100;
		break;
	case BAT_TEMPERATURE:
		edata->value_cur = sc->sc_bat_temp * 1000000 + 273150000;
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
}

static void
battery_poll(void *cookie)
{
	struct battery_softc *sc = cookie;


	battery_update(sc, 0);
	if ((sc->sc_flags & PMU_PWR_AC_PRESENT) == sc->sc_oflags)
		return;

	sc->sc_oflags = sc->sc_flags & PMU_PWR_AC_PRESENT;

	sysmon_pswitch_event(&sc->sc_sm_acpower, 
	    sc->sc_oflags ? PSWITCH_EVENT_PRESSED :
	    PSWITCH_EVENT_RELEASED);
}
