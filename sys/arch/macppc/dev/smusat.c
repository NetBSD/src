/*-
 * Copyright (c) 2013 Phileas Fogg
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <machine/autoconf.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <macppc/dev/smuiicvar.h>

#include "opt_smusat.h"

extern int smu_get_datablock(int, uint8_t *, size_t);

enum {
	SMUSAT_SENSOR_TEMP,
	SMUSAT_SENSOR_CURRENT,
	SMUSAT_SENSOR_VOLTAGE,
	SMUSAT_SENSOR_POWER,
};

struct smusat_softc;

struct smusat_sensor {
	struct smusat_softc *sc;

	char location[32];
	int type;
	int reg;
	int zone;
	int shift;
	int offset;
	int scale;
	int current_value;
};

#define SMUSAT_MAX_SENSORS	16
#define SMUSAT_MAX_SME_SENSORS	SMUSAT_MAX_SENSORS

struct smusat_softc {
	device_t sc_dev;
	int sc_node;
	i2c_addr_t sc_addr;
	uint8_t sc_cache[16];
	time_t sc_last_update;
	struct i2c_controller *sc_i2c;
	struct sysctlnode *sc_sysctl_me;

	int sc_num_sensors;
	struct smusat_sensor sc_sensors[SMUSAT_MAX_SENSORS];

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sme_sensors[SMUSAT_MAX_SME_SENSORS];
};

#ifdef SMUSAT_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

static int smusat_match(device_t, struct cfdata *, void *);
static void smusat_attach(device_t, device_t, void *);
static void smusat_setup_sme(struct smusat_softc *);
static void smusat_sme_refresh(struct sysmon_envsys *, envsys_data_t *);
static int smusat_sensors_update(struct smusat_softc *);
static int smusat_sensor_read(struct smusat_sensor *, int *);
static int smusat_sysctl_sensor_value(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(smusat, sizeof(struct smusat_softc),
    smusat_match, smusat_attach, NULL, NULL);

static const char * smusat_compats[] = {
	"sat",	
	"smu-sat",
	NULL
};

static int
smusat_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name == NULL) {
		/* no ID registers on this chip */
		if (ia->ia_addr == 0x58)
			return 1;
		return 0;
	} else {
		return iic_compat_match(ia, smusat_compats);
	}
}

static void
smusat_attach(device_t parent, device_t self, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct smusat_softc *sc = device_private(self);
	struct smusat_sensor *sensor;
	struct sysctlnode *sysctl_sensors, *sysctl_sensor, *sysctl_node;
	char type[32], sysctl_sensor_name[32];
	int node, i, j;

	sc->sc_dev = self;
	sc->sc_node = ia->ia_cookie;
	sc->sc_addr = ia->ia_addr;
	sc->sc_i2c = ia->ia_tag;

	sysctl_createv(NULL, 0, NULL, (void *) &sc->sc_sysctl_me,
	    CTLFLAG_READWRITE,
	   CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	for (node = OF_child(sc->sc_node);
	    (node != 0) && (sc->sc_num_sensors < SMUSAT_MAX_SENSORS);
	    node = OF_peer(node)) {
		sensor = &sc->sc_sensors[sc->sc_num_sensors];
		sensor->sc = sc;

		memset(sensor->location, 0, sizeof(sensor->location));
		OF_getprop(node, "location", sensor->location,
		    sizeof(sensor->location));

		if (OF_getprop(node, "reg", &sensor->reg,
		        sizeof(sensor->reg)) <= 0)
			continue;

		if ((sensor->reg < 0x30) || (sensor->reg > 0x37))
			continue;
		sensor->reg -= 0x30; 

		if (OF_getprop(node, "zone", &sensor->zone,
		        sizeof(sensor->zone)) <= 0)
			continue;

		memset(type, 0, sizeof(type));
		OF_getprop(node, "device_type", type, sizeof(type));

		if (strcmp(type, "temp-sensor") == 0) {
			sensor->type = SMUSAT_SENSOR_TEMP;
			sensor->shift = 10;
		} else if (strcmp(type, "current-sensor") == 0) {
			sensor->type = SMUSAT_SENSOR_CURRENT;
			sensor->shift = 8;
		} else if (strcmp(type, "voltage-sensor") == 0) {
			sensor->type = SMUSAT_SENSOR_VOLTAGE;
			sensor->shift = 4;
		} else if (strcmp(type, "power-sensor") == 0) {
			sensor->type = SMUSAT_SENSOR_POWER;
			sensor->shift = 0;
		}

		DPRINTF("sensor: location %s reg %x zone %d type %s\n",
		    sensor->location, sensor->reg, sensor->zone, type);

		sc->sc_num_sensors++;
	}

	/* Create sysctl nodes for each sensor */

	sysctl_createv(NULL, 0, NULL, (void *) &sysctl_sensors,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_NODE, "sensors", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP,
	    sc->sc_sysctl_me->sysctl_num,
	    CTL_CREATE, CTL_EOL);

	for (i = 0; i < sc->sc_num_sensors; i++) {
		sensor = &sc->sc_sensors[i];

		for (j = 0; j < strlen(sensor->location); j++) {
			sysctl_sensor_name[j] = tolower(sensor->location[j]);
			if (sysctl_sensor_name[j] == ' ')
				sysctl_sensor_name[j] = '_';
		}
		sysctl_sensor_name[j] = '\0';

		sysctl_createv(NULL, 0, NULL, (void *) &sysctl_sensor,
		    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
		    CTLTYPE_NODE, sysctl_sensor_name, "sensor information",
		    NULL, 0, NULL, 0,
		    CTL_MACHDEP,
		    sc->sc_sysctl_me->sysctl_num,
		    sysctl_sensors->sysctl_num,
		    CTL_CREATE, CTL_EOL);

		sysctl_createv(NULL, 0, NULL, (void *) &sysctl_node,
		    CTLFLAG_READONLY | CTLFLAG_OWNDESC,
		    CTLTYPE_INT, "zone", "sensor zone",
		    NULL, 0, &sensor->zone, 0,
		    CTL_MACHDEP,
		    sc->sc_sysctl_me->sysctl_num,
		    sysctl_sensors->sysctl_num,
		    sysctl_sensor->sysctl_num,
		    CTL_CREATE, CTL_EOL);

		sysctl_createv(NULL, 0, NULL, (void *) &sysctl_node,
		    CTLFLAG_READONLY | CTLFLAG_OWNDESC,
		    CTLTYPE_INT, "value", "sensor current value",
		    smusat_sysctl_sensor_value, 0, (void *) sensor, 0,
		    CTL_MACHDEP,
		    sc->sc_sysctl_me->sysctl_num,
		    sysctl_sensors->sysctl_num,
		    sysctl_sensor->sysctl_num,
		    CTL_CREATE, CTL_EOL);
	}

	smusat_setup_sme(sc);

	printf("\n");
}

static void
smusat_setup_sme(struct smusat_softc *sc)
{
	struct smusat_sensor *sensor;
	envsys_data_t *sme_sensor;
	int i;

	sc->sc_sme = sysmon_envsys_create();

	for (i = 0; i < sc->sc_num_sensors; i++) {
		sme_sensor = &sc->sc_sme_sensors[i];
		sensor = &sc->sc_sensors[i];

		switch (sensor->type) {
		case SMUSAT_SENSOR_TEMP:
			sme_sensor->units = ENVSYS_STEMP;
		break;
		case SMUSAT_SENSOR_CURRENT:
			sme_sensor->units = ENVSYS_SAMPS;
		break;
		case SMUSAT_SENSOR_VOLTAGE:
			sme_sensor->units = ENVSYS_SVOLTS_DC;
		break;
		case SMUSAT_SENSOR_POWER:
			sme_sensor->units = ENVSYS_SWATTS;
		break;
		default:
			sme_sensor->units = ENVSYS_INTEGER;
		}

		sme_sensor->state = ENVSYS_SINVALID;
		snprintf(sme_sensor->desc, sizeof(sme_sensor->desc),
		    "%s", sensor->location);

		if (sysmon_envsys_sensor_attach(sc->sc_sme, sme_sensor)) {
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = smusat_sme_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
smusat_sme_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct smusat_softc *sc = sme->sme_cookie;
	struct smusat_sensor *sensor;
	int which = edata->sensor;
	int ret;

	edata->state = ENVSYS_SINVALID;

	if (which < sc->sc_num_sensors) {
		sensor = &sc->sc_sensors[which];

		ret = smusat_sensor_read(sensor, NULL);
		if (ret == 0) {
			switch (sensor->type) {
			case SMUSAT_SENSOR_TEMP:
				edata->value_cur = sensor->current_value *
				    1000000 + 273150000;
			break;
			case SMUSAT_SENSOR_CURRENT:
				edata->value_cur = sensor->current_value * 1000000;
			break;
			case SMUSAT_SENSOR_VOLTAGE:
				edata->value_cur = sensor->current_value * 1000000;
			break;
			case SMUSAT_SENSOR_POWER:
				edata->value_cur = sensor->current_value * 1000000;
			break;
			default:
				edata->value_cur = sensor->current_value;
			}

			edata->state = ENVSYS_SVALID;
		}
	}
}

static int
smusat_sensors_update(struct smusat_softc *sc)
{
	u_char reg = 0x3f;
	int ret;

	iic_acquire_bus(sc->sc_i2c, 0);
	ret = iic_exec(sc->sc_i2c, I2C_OP_READ, sc->sc_addr, &reg, 1, sc->sc_cache, 16, 0);
	iic_release_bus(sc->sc_i2c, 0);

	if (ret != 0)
		return (ret);

	sc->sc_last_update = time_uptime;

	return 0;
}

static int
smusat_sensor_read(struct smusat_sensor *sensor, int *value)
{
	struct smusat_softc *sc = sensor->sc;
	int ret, reg;

	if (time_uptime - sc->sc_last_update > 1) {
		ret = smusat_sensors_update(sc);
		if (ret != 0)
			return ret;
	}

	reg = sensor->reg << 1;
	sensor->current_value = (sc->sc_cache[reg] << 8) + sc->sc_cache[reg + 1];
	sensor->current_value <<= sensor->shift;
	/* Discard the .16 */ 
	sensor->current_value >>= 16;

	if (value != NULL)
		*value = sensor->current_value;

	return 0;
}

static int
smusat_sysctl_sensor_value(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct smusat_sensor *sensor = node.sysctl_data;
	int value = 0;
	int ret;

	node.sysctl_data = &value;

	ret = smusat_sensor_read(sensor, &value);
	if (ret != 0)
		return (ret);

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

SYSCTL_SETUP(smusat_sysctl_setup, "SMU-SAT sysctl subtree setup")
{
	sysctl_createv(NULL, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
}
