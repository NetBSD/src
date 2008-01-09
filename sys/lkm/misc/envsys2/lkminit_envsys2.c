/*	$NetBSD: lkminit_envsys2.c,v 1.3.10.2 2008/01/09 01:56:55 matt Exp $	*/

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is a virtual driver for sysmon_envsys(9). That shows in how
 * the framework works and to verify correct operation of the framework.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lkminit_envsys2.c,v 1.3.10.2 2008/01/09 01:56:55 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lkm.h>
#include <sys/errno.h>
#include <dev/sysmon/sysmonvar.h>

/*
 * The list of sensors.
 */
enum sensors {
	SENSOR_CPUTEMP	= 0,
	SENSOR_CPUFAN,
	SENSOR_VCORE,
	SENSOR_EXTERNAL_VOLTAGE,
	SENSOR_VCORE_RESISTANCE,
	SENSOR_CURRENT_POWER,
	SENSOR_CURRENT_POTENCY,
	SENSOR_BATTERY0_CAPACITY,
	SENSOR_BATTERY0_CAPSTATE,
	SENSOR_BATTERY0_CHARGESTATE,
	SENSOR_BATTERY1_CAPACITY,
	SENSOR_BATTERY1_CAPSTATE,
	SENSOR_BATTERY1_CHARGESTATE,
	SENSOR_POWER_LED,
	SENSOR_TECHNOLOGY,
	SENSOR_MASTER_DISK,
	SENSOR_DUP_CPUTEMP,
	SENSOR_DUP_TECHNOLOGY,
	SENSOR_EMPTY_DESC,
	SENSOR_INVALID_UNITS,
	SENSOR_MAXSENSORS		/* must be last */
};

struct envsys2_softc {
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[SENSOR_MAXSENSORS];
};

/* 
 * Required stuff for the LKM part.
 */
int 		envsys2_lkmentry(struct lkm_table *, int, int);
static int 	envsys2_mod_handle(struct lkm_table *, int);
MOD_MISC("envsys2");

/* 
 * Required stuff by sysmon_envsys(9).
 */
static void 	envsys2_refresh(struct sysmon_envsys *, envsys_data_t *);
static void 	envsys2_initsensors(struct envsys2_softc *);
static struct 	envsys2_softc e2_sc;

/*
 * This function assigns sensor description and units type for
 * all sensors. If we don't assign a valid state (ENVSYS_SVALID),
 * its state will be unknown and the value won't be visible on
 * envstat(8).
 */
static void
envsys2_initsensors(struct envsys2_softc *sc)
{
	/*
	 * Initialize the sensors with units and description.
	 * And optionally with monitoring flags.
	 */
#define INITSENSOR(idx, unit, string)					\
	do {								\
		sc->sc_sensor[idx].units = unit;			\
		(void)strlcpy(sc->sc_sensor[idx].desc, string,		\
		    sizeof(sc->sc_sensor->desc));			\
	} while (/* CONSTCOND */ 0)

	/*
	 * We want to monitor for critical state in the CPU Temp sensor.
	 */
	INITSENSOR(SENSOR_CPUTEMP, ENVSYS_STEMP, "CPU Temp");
	sc->sc_sensor[SENSOR_CPUTEMP].monitor = true;
	sc->sc_sensor[SENSOR_CPUTEMP].flags = ENVSYS_FMONCRITICAL;

	/*
	 * We want to monitor for a critical under state in the CPU Fan
	 * sensor, so that we know if the fan is not working or stopped.
	 */
	INITSENSOR(SENSOR_CPUFAN, ENVSYS_SFANRPM, "CPU Fan");
	sc->sc_sensor[SENSOR_CPUFAN].monitor = true;
	sc->sc_sensor[SENSOR_CPUFAN].flags = ENVSYS_FMONCRITUNDER;

	/*
	 * We want to monitor for a critical over state in the VCore
	 * sensor, so that we know if there's overvoltage on it.
	 */
	INITSENSOR(SENSOR_VCORE, ENVSYS_SVOLTS_DC, "VCore");
	sc->sc_sensor[SENSOR_VCORE].monitor = true;
	sc->sc_sensor[SENSOR_VCORE].flags = ENVSYS_FMONCRITOVER;

	INITSENSOR(SENSOR_EXTERNAL_VOLTAGE, ENVSYS_SVOLTS_AC,
		   "External Voltage");
	INITSENSOR(SENSOR_VCORE_RESISTANCE, ENVSYS_SOHMS, "VCore Resistance");
	INITSENSOR(SENSOR_CURRENT_POWER, ENVSYS_SWATTS, "Current power");
	INITSENSOR(SENSOR_CURRENT_POTENCY, ENVSYS_SAMPS, "Current potency");
	INITSENSOR(SENSOR_BATTERY0_CAPACITY, ENVSYS_SWATTHOUR,
		   "Battery0 capacity");

	/*
	 * We want to monitor for state changes in battery capacity
	 * sensors, so that its state has been changed, we will be notified.
	 */
	INITSENSOR(SENSOR_BATTERY0_CAPSTATE, ENVSYS_BATTERY_CAPACITY,
		   "Battery0 state");
	sc->sc_sensor[SENSOR_BATTERY0_CAPSTATE].monitor = true;
	sc->sc_sensor[SENSOR_BATTERY0_CAPSTATE].flags = ENVSYS_FMONSTCHANGED;

	INITSENSOR(SENSOR_BATTERY0_CHARGESTATE, ENVSYS_BATTERY_CHARGE,
		   "Battery0 charging");

	/*
	 * Same than Battery0, but this uses Ah.
	 */
	INITSENSOR(SENSOR_BATTERY1_CAPACITY, ENVSYS_SAMPHOUR,
		   "Battery1 capacity");

	INITSENSOR(SENSOR_BATTERY1_CAPSTATE, ENVSYS_BATTERY_CAPACITY,
		   "Battery1 state");
	sc->sc_sensor[SENSOR_BATTERY1_CAPSTATE].monitor = true;
	sc->sc_sensor[SENSOR_BATTERY1_CAPSTATE].flags = ENVSYS_FMONSTCHANGED;

	INITSENSOR(SENSOR_BATTERY1_CHARGESTATE, ENVSYS_BATTERY_CHARGE,
		   "Battery1 charging");

	INITSENSOR(SENSOR_POWER_LED, ENVSYS_INDICATOR, "Power Led");

	/*
	 * We don't want to be able to set a critical limit in userland,
	 * so we must enable the monitoring flag in the sensor.
	 */
	INITSENSOR(SENSOR_TECHNOLOGY, ENVSYS_INTEGER, "Technology");
	sc->sc_sensor[SENSOR_TECHNOLOGY].flags = ENVSYS_FMONNOTSUPP;

	/* 
	 * We want to monitor the state in the drive sensor, so
	 * that we know if it's not online or in normal operation.
	 */
	INITSENSOR(SENSOR_MASTER_DISK, ENVSYS_DRIVE, "Master disk");
	sc->sc_sensor[SENSOR_MASTER_DISK].monitor = true;
	sc->sc_sensor[SENSOR_MASTER_DISK].flags = ENVSYS_FMONSTCHANGED;

	/*
	 * Let's add two sensors with duplicate descriptions
	 * (they won't be attached).
	 *
	 */
	INITSENSOR(SENSOR_DUP_CPUTEMP, ENVSYS_STEMP, "CPU Temp");
	INITSENSOR(SENSOR_DUP_TECHNOLOGY, ENVSYS_INTEGER, "Technology");
	/*
	 * Another one with empty description (it won't be attached). 
 	 */
	INITSENSOR(SENSOR_EMPTY_DESC, ENVSYS_STEMP, "");
	/*
	 * And finally another sensor with invalid units type
	 * (it won't be attached).
	 */
	INITSENSOR(SENSOR_INVALID_UNITS, -1, "Invalid units");
}

/*
 * This is the function that envsys2 uses to retrieve new data from
 * the sensors registered in our driver.
 *
 * Note that we must use always the index specified on edata->sensor,
 * otherwise we might be refreshing the sensor more times than it
 * should.
 */
static void
envsys2_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	switch (edata->sensor) {
	case SENSOR_CPUTEMP:
		/* CPU Temp, use 38C */
		edata->value_cur = 38 * 1000000 + 273150000;
		break;
	case SENSOR_CPUFAN:
		/* CPU Fan, use 2560 RPM */
		edata->value_cur = 2560;
		break;
	case SENSOR_VCORE:
		/* Vcore in DC, use 1.232V */
		edata->value_cur = 1232 * 1000;
		break;
	case SENSOR_EXTERNAL_VOLTAGE:
		/* External Voltage in AC, use 223.2V */
		edata->value_cur = 2232 * 100000;
		break;
	case SENSOR_VCORE_RESISTANCE:
		/* VCore resistance, use 12 Ohms */
		edata->value_cur = 1200 * 10000;
		break;
	case SENSOR_CURRENT_POWER:
		/* Current power, use 30W */
		edata->value_cur = 30000000;
		break;
	case SENSOR_CURRENT_POTENCY:
		/* Current potency, use 0.500A */
		edata->value_cur = 500 * 1000;
		break;
	case SENSOR_BATTERY0_CAPACITY:
		/* 
		 * Battery0 in Wh.
		 *
		 * Battery sensors (Ah/Wh) need at least data in
		 * value_cur and value_max to compute the percentage.
		 *
		 * Also the value_max member must be enabled, otherwise
		 * it won't be valid.
		 */
		edata->value_cur = 2420000;
		edata->value_max = 3893000;
		/* enable value_max and percentage display */
		edata->flags |= (ENVSYS_FVALID_MAX | ENVSYS_FPERCENT);
		break;
	case SENSOR_BATTERY0_CAPSTATE:
		edata->value_cur = ENVSYS_BATTERY_CAPACITY_NORMAL;
		break;
	case SENSOR_BATTERY0_CHARGESTATE:
		edata->value_cur = 1;
		break;
	case SENSOR_BATTERY1_CAPACITY:
		/* Battery1 in Ah */
		edata->value_cur = 1890000;
		edata->value_max= 4000000;
		edata->flags |= (ENVSYS_FVALID_MAX | ENVSYS_FPERCENT);
		break;
	case SENSOR_BATTERY1_CAPSTATE:
		edata->value_cur = ENVSYS_BATTERY_CAPACITY_CRITICAL;
		break;
	case SENSOR_BATTERY1_CHARGESTATE:
		edata->value_cur = 0;
		break;
	case SENSOR_POWER_LED:
		/*
		 * sensor with indicator units, only need to be marked
		 * as valid and use a value of 1 or 0.
		 */
		edata->value_cur = 1;
		break;
	case SENSOR_TECHNOLOGY:
		/* Technology, no comments */
		edata->value_cur = 2007;
		break;
	case SENSOR_MASTER_DISK:
		/* Master disk, use the common DRIVE_ONLINE. */
		edata->value_cur = ENVSYS_DRIVE_ONLINE;
		break;
	default:
		edata->state = ENVSYS_SINVALID;
		break;
	}

	/*
	 * Data provided was right, so mark the sensor as valid otherwise
	 * its state will be unknown.
	 */
	edata->state = ENVSYS_SVALID;
}

static int
envsys2_mod_handle(struct lkm_table *lkmtp, int cmd)
{
	struct envsys2_softc *sc = &e2_sc;
	int i, err = 0;

	switch (cmd) {
	case LKM_E_LOAD:
		/*
		 * Don't load twice! (lkmexists() is exported by kern_lkm.c)
		 */
		if (lkmexists(lkmtp))
			return EEXIST;

		/*
		 * Allocate and initialize the sysmon_envsys object. 
		 */
		sc->sc_sme = sysmon_envsys_create();

		/*
		 * Initialize the sensors with required values: units and desc.
		 */
		envsys2_initsensors(sc);

		/*
		 * Attach the sensors and ignore the ones that weren't
		 * accepted: two sensors with duplicate description, another
		 * one with empty description and the last one with invalid
		 * units type.
		 */
		for (i = 0; i < SENSOR_MAXSENSORS; i++) {
			if (sysmon_envsys_sensor_attach(sc->sc_sme,
							&sc->sc_sensor[i])) {
				printf("envsys2: failed to add sensor%d\n", i);
				continue;
			}
		}

		/*
		 * Now register our driver to sysmon_envsys(9).
		 */
		sc->sc_sme->sme_name = "envsys2";
		sc->sc_sme->sme_cookie = sc;
		sc->sc_sme->sme_refresh = envsys2_refresh;
		sc->sc_sme->sme_events_timeout = 60; /* 60 seconds */

		err = sysmon_envsys_register(sc->sc_sme);
		if (err) {
			printf("envsys2: unable to register with sysmon "
			    "(%d)\n", err);
			/*
			 * Was there an error? free resources used by
			 * sysmon_envsys_create().
			 */
			sysmon_envsys_destroy(sc->sc_sme);
		}
		break;

	case LKM_E_UNLOAD:
		/*
		 * Unregister our driver with the sysmon_envsys(9)
		 * framework. This will remove all events currently
		 * assigned, sensors attached and the object will be
		 * freed via sysmon_envsys_destroy() at last.
		 */
		sysmon_envsys_unregister(sc->sc_sme);
		break;

	default:	/* we only understand load/unload */
		err = EINVAL;
		break;
	}

	return err;
}

int
envsys2_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, envsys2_mod_handle,
	    envsys2_mod_handle, lkm_nofunc)
}
