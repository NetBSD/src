/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel.
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
 * IST-AG P14 calibrated Hygro-/Temperature sensor module
 * Devices: HYT-271, HYT-221 and HYT-939 
 *
 * see:
 * http://www.ist-ag.com/eh/ist-ag/resource.nsf/imgref/Download_AHHYTM_E2.1.pdf/
 *      $FILE/AHHYTM_E2.1.pdf
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hytp14.c,v 1.2.2.2 2014/08/10 06:54:51 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/hytp14reg.h>
#include <dev/i2c/hytp14var.h>

static int hytp14_match(device_t, cfdata_t, void *);
static void hytp14_attach(device_t, device_t, void *);
static int hytp14_detach(device_t, int);
static int hytp14_refresh_sensor(struct hytp14_sc *sc);
static void hytp14_refresh(struct sysmon_envsys *, envsys_data_t *);
static void hytp14_refresh_humidity(struct hytp14_sc *, envsys_data_t *);
static void hytp14_refresh_temp(struct hytp14_sc *, envsys_data_t *);

/* #define HYT_DEBUG 3 */
#ifdef HYT_DEBUG
volatile int hythygtemp_debug = HYT_DEBUG;

#define DPRINTF(_L_, _X_) do {			\
	  if ((_L_) <= hythygtemp_debug) {	\
	    printf _X_;				\
	  }                                     \
        } while (0)
#else
#define DPRINTF(_L_, _X_)
#endif

CFATTACH_DECL_NEW(hythygtemp, sizeof(struct hytp14_sc),
    hytp14_match, hytp14_attach, hytp14_detach, NULL);

static struct hytp14_sensor hytp14_sensors[] = {
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
		.refresh = hytp14_refresh_humidity
	},
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
		.refresh = hytp14_refresh_temp
	}
};

static int
hytp14_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name) {
		/* direct config - check name */
		if (strcmp(ia->ia_name, "hythygtemp") == 0)
			return 1;
	} else {
		/* indirect config - check for configured address */
		if ((ia->ia_addr > 0) && (ia->ia_addr <= 0x7F))
			return 1;
	}
	return 0;
}

static void
hytp14_attach(device_t parent, device_t self, void *aux)
{
	struct hytp14_sc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	int i;

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_refresh = 0;
	sc->sc_valid = ENVSYS_SINVALID;
	sc->sc_numsensors = __arraycount(hytp14_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create sysmon structure\n");
		return;
	}
	
	for (i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc,
			hytp14_sensors[i].desc,
			sizeof sc->sc_sensors[i].desc);
		
		sc->sc_sensors[i].units = hytp14_sensors[i].type;
		sc->sc_sensors[i].state = ENVSYS_SINVALID;
		
		DPRINTF(2, ("hytp14_attach: registering sensor %d (%s)\n", i,
			    sc->sc_sensors[i].desc));
		
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensors[i])) {
			aprint_error_dev(sc->sc_dev,
			    "unable to attach sensor\n");
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = hytp14_refresh;

	DPRINTF(2, ("hytp14_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	aprint_normal(": HYT-221/271/939 humidity and temperature sensor\n");
}

static int hytp14_detach(device_t self, int flags)
{
	struct hytp14_sc *sc = device_private(self);

	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	
	return 0;
}

static int
hytp14_refresh_sensor(struct hytp14_sc *sc)
{
	int error = 0;
	uint8_t buf[I2C_EXEC_MAX_BUFLEN];

	/* no more than once per second */
	if (hardclock_ticks - sc->sc_refresh < hz)
		return sc->sc_valid;
	
	DPRINTF(2, ("hytp14_refresh_sensor(%s)\n", device_xname(sc->sc_dev)));

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) == 0) {
		DPRINTF(3, ("hytp14_refresh_sensor(%s): bus locked\n", device_xname(sc->sc_dev)));

		/* send MR command */
                /* avoid quick read/write by providing a result buffer */
		error = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
				 sc->sc_addr, NULL, 0, buf, sizeof buf, 0);
                if (error == 0) {
			DPRINTF(3, ("hytp14_refresh_sensor(%s): MR sent\n",
				    device_xname(sc->sc_dev)));

			/* send DF command - read data from sensor */
			error = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
					 sc->sc_addr, NULL, 0, sc->sc_data,
					      sizeof sc->sc_data, 0);
                        if (error != 0) {
				DPRINTF(2, ("%s: %s: failed read from 0x%02x - error %d\n",
					    device_xname(sc->sc_dev),
					    __func__, sc->sc_addr, error));
			} else {
				DPRINTF(2, ("hytp14_refresh_sensor(%s): DF success : 0x%02x%02x%02x%02x\n",
					    device_xname(sc->sc_dev),
					    sc->sc_data[0],
					    sc->sc_data[1],
					    sc->sc_data[2],
					    sc->sc_data[3]));
			}
		} else {
			DPRINTF(2, ("%s: %s: failed read from 0x%02x - error %d\n",
				    device_xname(sc->sc_dev), __func__,
				    sc->sc_addr, error));
		}

		iic_release_bus(sc->sc_tag, 0);	
		DPRINTF(3, ("hytp14_refresh_sensor(%s): bus released\n", device_xname(sc->sc_dev)));
	} else {
		DPRINTF(2, ("%s: %s: failed read from 0x%02x - error %d\n",
			    device_xname(sc->sc_dev), __func__, sc->sc_addr, error));
	}
			
	sc->sc_refresh = hardclock_ticks;
	
	/* skip data if sensor is in command mode */
	if (error == 0 && (sc->sc_data[0] & HYTP14_RESP_CMDMODE) == 0) {
		sc->sc_valid = ENVSYS_SVALID;
	} else {
		sc->sc_valid = ENVSYS_SINVALID;
	}
	
	return sc->sc_valid;
}


static void
hytp14_refresh_humidity(struct hytp14_sc *sc, envsys_data_t *edata)
{
	uint16_t hyg;
	int status;
	
	status = hytp14_refresh_sensor(sc);
	
	if (status == ENVSYS_SVALID) {
		hyg = (sc->sc_data[0] << 8) | sc->sc_data[1];
		
		edata->value_cur = (1000000000 / HYTP14_HYG_SCALE) * (int32_t)HYTP14_HYG_RAWVAL(hyg);
		edata->value_cur /= 10;
	}

	edata->state = status;
}

static void
hytp14_refresh_temp(struct hytp14_sc *sc, envsys_data_t *edata)
{
	uint16_t temp;
	int status;
	
	status = hytp14_refresh_sensor(sc);
	
	if (status == ENVSYS_SVALID) {
		temp = HYTP14_TEMP_RAWVAL((sc->sc_data[2] << 8) | sc->sc_data[3]);

		edata->value_cur = (HYTP14_TEMP_FACTOR * 1000000) / HYTP14_TEMP_SCALE;
		edata->value_cur *= (int32_t)temp;
		edata->value_cur += HYTP14_TEMP_OFFSET * 1000000 + 273150000;
	}

	edata->state = status;
}

static void
hytp14_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct hytp14_sc *sc = sme->sme_cookie;
	
	hytp14_sensors[edata->sensor].refresh(sc, edata);
}


MODULE(MODULE_CLASS_DRIVER, hythygtemp, "iic");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
hythygtemp_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_hythygtemp,
		    cfattach_ioconf_hythygtemp, cfdata_ioconf_hythygtemp);
#endif
		return error;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_hythygtemp,
		    cfattach_ioconf_hythygtemp, cfdata_ioconf_hythygtemp);
#endif
		return error;

	default:
		return ENOTTY;
	}
}
