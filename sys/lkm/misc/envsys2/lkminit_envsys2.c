/*	$NetBSD: lkminit_envsys2.c,v 1.1 2007/07/20 14:21:00 xtraeme Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
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
 *      This product includes software developed by Juan Romero Pardines
 *      for the NetBSD Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

/*
 * This is a virtual driver for sysmon_envsys(9), that shows in how
 * the framework works and to verify correct operation of the framework.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lkminit_envsys2.c,v 1.1 2007/07/20 14:21:00 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lkm.h>
#include <sys/errno.h>
#include <dev/sysmon/sysmonvar.h>

#define MAXSENSORS	15

struct envsys2_softc {
	struct sysmon_envsys sc_sysmon;
	envsys_data_t sc_sensor[MAXSENSORS];
};

/* Required stuff for LKM */
int envsys2_lkmentry(struct lkm_table *, int, int);
static int envsys2_mod_handle(struct lkm_table *, int);
MOD_MISC("envsys2");

/* Required stuff by envsys(4) */
static int envsys2_gtredata(struct sysmon_envsys *, envsys_data_t *);
static void envsys2_initsensors(struct envsys2_softc *);
static void envsys2_refresh_sensor(struct sysmon_envsys *, envsys_data_t *);

static struct envsys2_softc e2_sc;

static int
envsys2_gtredata(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	printf("%s: edata->sensor=%d\n", __func__, edata->sensor);
	/* 
	 * refresh data in the sensor X via edata->sensor, which is
	 * the index used to find the sensor.
	 */
	envsys2_refresh_sensor(sme, edata);
	/* we must always return 0 to signal envsys2 that is ok. */
	return 0;
}

/*
 * This function assigns sensor index, state, description and unit
 * type. All sensors should be initialized with a valid state
 * (ENVSYS_SVALID), otherwise some members won't be created on its
 * dictionary.
 */
static void
envsys2_initsensors(struct envsys2_softc *sc)
{
#define COPYDESCR(a, b)					\
	do {						\
		(void)strlcpy((a), (b), sizeof((a)));	\
	} while (/* CONSTCOND */ 0)

	int i;

	/*
	 * Initialize the sensors with the index and a valid state,
	 * and optionally with a monitoring flag.
	 */
	for (i = 0; i < MAXSENSORS; i++) {
		sc->sc_sensor[i].sensor = i;
		sc->sc_sensor[i].state = ENVSYS_SVALID;
	}

	/* In a LKM, the first sensor must be dummy. */
	sc->sc_sensor[0].units = 0;
	COPYDESCR(sc->sc_sensor[0].desc, "Unused");

	/*
	 * Assign units and description, note that description must be
	 * unique in a device, and sensors with a duplicate description
	 * will be simply ignored.
	 */
	sc->sc_sensor[1].units = ENVSYS_STEMP;
	COPYDESCR(sc->sc_sensor[1].desc, "CPU Temp");
	/*
	 * We want to monitor for critical state in the CPU Temp sensor.
	 */
	sc->sc_sensor[1].monitor = true;
	sc->sc_sensor[1].flags = ENVSYS_FMONCRITICAL;

	sc->sc_sensor[2].units = ENVSYS_SFANRPM;
	COPYDESCR(sc->sc_sensor[2].desc, "CPU Fan");
	/*
	 * We want to monitor for a critical under state in the CPU Fan
	 * sensor, so that we know if the fan is not working or stopped.
	 */
	sc->sc_sensor[2].monitor = true;
	sc->sc_sensor[2].flags = ENVSYS_FMONCRITUNDER;

	sc->sc_sensor[3].units = ENVSYS_SVOLTS_DC;
	COPYDESCR(sc->sc_sensor[3].desc, "VCore");
	/*
	 * We want to monitor for a critical over state in the VCore
	 * sensor, so that we know if there's overvolt on it.
	 */
	sc->sc_sensor[3].monitor = true;
	sc->sc_sensor[3].flags = ENVSYS_FMONCRITOVER;

	sc->sc_sensor[4].units = ENVSYS_SVOLTS_AC;
	COPYDESCR(sc->sc_sensor[4].desc, "External Voltage");

	sc->sc_sensor[5].units = ENVSYS_SOHMS;
	COPYDESCR(sc->sc_sensor[5].desc, "VCore Resistance");

	sc->sc_sensor[6].units = ENVSYS_SWATTS;
	COPYDESCR(sc->sc_sensor[6].desc, "Current power");

	sc->sc_sensor[7].units = ENVSYS_SAMPS;
	COPYDESCR(sc->sc_sensor[7].desc, "Current potency");

	sc->sc_sensor[8].units = ENVSYS_SWATTHOUR;
	COPYDESCR(sc->sc_sensor[8].desc, "Battery0 capacity");

	sc->sc_sensor[9].units = ENVSYS_SAMPHOUR;
	COPYDESCR(sc->sc_sensor[9].desc, "Battery1 capacity");

	sc->sc_sensor[10].units = ENVSYS_INDICATOR;
	COPYDESCR(sc->sc_sensor[10].desc, "Power Led");

	sc->sc_sensor[11].units = ENVSYS_INTEGER;
	COPYDESCR(sc->sc_sensor[11].desc, "Technology");
	/*
	 * We don't want to be able to set a critical limit in userland,
	 * so we must disable the monitoring flag in the sensor.
	 */
	sc->sc_sensor[11].flags = ENVSYS_FMONNOTSUPP;

	sc->sc_sensor[12].units = ENVSYS_DRIVE;
	COPYDESCR(sc->sc_sensor[12].desc, "Master disk");
	/* 
	 * We want to monitor the state in the drive sensor, so
	 * that we know if it's not online or in normal operation.
	 */
	sc->sc_sensor[12].monitor = true;
	sc->sc_sensor[12].flags = ENVSYS_FMONDRVSTATE;

	/*
	 * Let's add two sensors with duplicate descriptions.
	 */
	sc->sc_sensor[13].units = ENVSYS_SWATTS;
	COPYDESCR(sc->sc_sensor[13].desc, "CPU Temp");

	sc->sc_sensor[14].units = ENVSYS_INTEGER;
	COPYDESCR(sc->sc_sensor[14].desc, "Technology");

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
envsys2_refresh_sensor(struct sysmon_envsys *sme, envsys_data_t *edata)
{

	/* 
	 * Hack for the LKM, the first sensor has the number of
	 * edata structures allocated in kernel, so invalidate the sensor
	 * for now.
	 */
	if (edata->sensor > MAXSENSORS) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	switch (edata->sensor) {
	case 1:
		/* CPU Temp, use 38C */
		edata->value_cur = 38 * 1000000 + 273150000;
		break;
	case 2:
		/* CPU Fan, use 2560 RPM */
		edata->value_cur = 2560;
		break;
	case 3:
		/* Vcore in DC, use 1.232V */
		edata->value_cur = 1232 * 1000;
		break;
	case 4:
		/* External Voltage in AC, use 223.2V */
		edata->value_cur = 2232 * 100000;
		break;
	case 5:
		/* VCore resistance, use 12 Ohms */
		edata->value_cur = 1200 * 10000;
		break;
	case 6:
		/* Current power, use 30W */
		edata->value_cur = 30000000;
		break;
	case 7:
		/* Current potency, use 0.500A */
		edata->value_cur = 500 * 1000;
		break;
	case 8:
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
		/* enable value_max */
		edata->flags |= ENVSYS_FVALID_MAX;
		/* enable percentage display */
		edata->flags |= ENVSYS_FPERCENT;
		break;
	case 9:
		/* Battery1 in Ah */
		edata->value_cur = 1890000;
		edata->value_max= 4000000;
		edata->flags |= ENVSYS_FVALID_MAX;
		edata->flags |= ENVSYS_FPERCENT;
		break;
	case 10:
		/*
		 * sensor with indicator units, only need to be marked
		 * as valid and use a value of 1 or 0.
		 */
		edata->value_cur = 1;
		break;
	case 11:
		/* Technology, no comments */
		edata->value_cur = 2007;
		break;
	case 12:
		/* Master disk, use the common DRIVE_ONLINE. */
		edata->value_cur = ENVSYS_DRIVE_ONLINE;
		break;

	}

	/* 
	 * If we changed the state previously in a sensor and the value
	 * returned is acceptable, its state must be updated.
	 */
	edata->state = ENVSYS_SVALID;
}

static int
envsys2_mod_handle(struct lkm_table *lkmtp, int cmd)
{
	struct envsys2_softc *sc = &e2_sc;
	int err = 0;

	switch (cmd) {
	case LKM_E_LOAD:
		/*
		 * Don't load twice! (lkmexists() is exported by kern_lkm.c)
		 */
		if (lkmexists(lkmtp))
			return EEXIST;

		/* we must firstly create and initialize the sensors */
		envsys2_initsensors(sc);

		/*
		 * Now register our driver with sysmon_envsys(9).
		 */
		sc->sc_sysmon.sme_name = "envsys2";
		sc->sc_sysmon.sme_sensor_data = sc->sc_sensor;
		sc->sc_sysmon.sme_cookie = sc;
		sc->sc_sysmon.sme_gtredata = envsys2_gtredata;
		sc->sc_sysmon.sme_nsensors = MAXSENSORS;

		if (sysmon_envsys_register(&sc->sc_sysmon)) {
			printf("%s: unable to register with sysmon\n",
			    __func__);
			return ENODEV;
		}

		break;

	case LKM_E_UNLOAD:
		/*
		 * Unregister our driver with the sysmon_envsys(9)
		 * framework. This will remove all events assigned
		 * to the device before. 
		 */
		sysmon_envsys_unregister(&sc->sc_sysmon);
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
