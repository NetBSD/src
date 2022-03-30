/*	$NetBSD: sht4x.c,v 1.3 2022/03/30 00:06:50 pgoyette Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: sht4x.c,v 1.3 2022/03/30 00:06:50 pgoyette Exp $");

/*
  Driver for the Sensirion SHT40/SHT41/SHT45
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/sht4xreg.h>
#include <dev/i2c/sht4xvar.h>


static uint8_t 	sht4x_crc(uint8_t *, size_t);
static int 	sht4x_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	sht4x_match(device_t, cfdata_t, void *);
static void 	sht4x_attach(device_t, device_t, void *);
static int 	sht4x_detach(device_t, int);
static void 	sht4x_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	sht4x_verify_sysctl(SYSCTLFN_ARGS);
static int 	sht4x_verify_sysctl_resolution(SYSCTLFN_ARGS);
static int 	sht4x_verify_sysctl_heateron(SYSCTLFN_ARGS);
static int 	sht4x_verify_sysctl_heatervalue(SYSCTLFN_ARGS);
static int 	sht4x_verify_sysctl_heaterpulse(SYSCTLFN_ARGS);

#define SHT4X_DEBUG
#ifdef SHT4X_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_sht4xdebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(sht4xtemp, sizeof(struct sht4x_sc),
    sht4x_match, sht4x_attach, sht4x_detach, NULL);

static struct sht4x_sensor sht4x_sensors[] = {
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
	},
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
	}
};

/* The typical delays are documented in the datasheet for the chip.
   There is no need to be very accurate with these, just rough estimates
   will work fine.
*/

static struct sht4x_timing sht4x_timings[] = {
	{
		.cmd = SHT4X_READ_SERIAL,
		.typicaldelay = 5000,
	},
	{
		.cmd = SHT4X_SOFT_RESET,
		.typicaldelay = 1000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION,
		.typicaldelay = 8000,
	},
	{
		.cmd = SHT4X_MEASURE_MEDIUM_PRECISION,
		.typicaldelay = 4000,
	},
	{
		.cmd = SHT4X_MEASURE_LOW_PRECISION,
		.typicaldelay = 2000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_HIGH_HEAT_1_S,
		.typicaldelay = 1000000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_MEDIUM_HEAT_1_S,
		.typicaldelay = 1000000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_LOW_HEAT_1_S,
		.typicaldelay = 1000000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_HIGH_HEAT_TENTH_S,
		.typicaldelay = 100000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_MEDIUM_HEAT_TENTH_S,
		.typicaldelay = 100000,
	},
	{
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_LOW_HEAT_TENTH_S,
		.typicaldelay = 100000,
	}
};

/* Used when the heater is not on to find the command to use for the
 * measurement.
 */

static struct sht4x_resolution sht4x_resolutions[] = {
	{
		.text = "high",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION,
	},
	{
		.text = "medium",
		.cmd = SHT4X_MEASURE_MEDIUM_PRECISION,
	},
	{
		.text = "low",
		.cmd = SHT4X_MEASURE_LOW_PRECISION,
	}
};

static const char sht4x_resolution_names[] =
    "high, medium, low";

static struct sht4x_heaterpulse sht4x_heaterpulses[] = {
	{
		.length = "short",
	},
	{
		.length = "long",
	}
};

/* This is consulted when the heater is on for which command is to be
   used for the measurement.
*/

static struct sht4x_heateron_command sht4x_heateron_commands[] = {
	{
		.heatervalue = 1,
		.pulselength = "short",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_LOW_HEAT_TENTH_S,
	},
	{
		.heatervalue = 2,
		.pulselength = "short",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_MEDIUM_HEAT_TENTH_S,
	},
	{
		.heatervalue = 3,
		.pulselength = "short",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_HIGH_HEAT_TENTH_S,
	},
	{
		.heatervalue = 1,
		.pulselength = "long",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_LOW_HEAT_1_S,
	},
	{
		.heatervalue = 2,
		.pulselength = "long",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_MEDIUM_HEAT_1_S,
	},
	{
		.heatervalue = 3,
		.pulselength = "long",
		.cmd = SHT4X_MEASURE_HIGH_PRECISION_HIGH_HEAT_1_S,
	}
};

static const char sht4x_heaterpulse_names[] =
    "short, long";

int
sht4x_verify_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

/* None of the heater and resolutions sysctls change anything on the chip in
   real time.  The values set are used to send different commands depending on
   how they are set up.

   What this implies is that the chip could be reset and the driver would not care.

*/

int
sht4x_verify_sysctl_resolution(SYSCTLFN_ARGS)
{
	char buf[SHT4X_RES_NAME];
	struct sht4x_sc *sc;
	struct sysctlnode node;
	int error = 0;
	size_t i;

	node = *rnode;
	sc = node.sysctl_data;
	(void) memcpy(buf, sc->sc_resolution, SHT4X_RES_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(sht4x_resolutions); i++) {
		if (strncmp(node.sysctl_data, sht4x_resolutions[i].text,
		    SHT4X_RES_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(sht4x_resolutions))
		return EINVAL;
	(void) memcpy(sc->sc_resolution, node.sysctl_data, SHT4X_RES_NAME);

	return error;
}

int
sht4x_verify_sysctl_heateron(SYSCTLFN_ARGS)
{
	int 		error;
	bool 		t;
	struct sht4x_sc *sc;
	struct sysctlnode node;

	node = *rnode;
	sc = node.sysctl_data;
	t = sc->sc_heateron;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	sc->sc_heateron = t;

	return error;
}

int
sht4x_verify_sysctl_heatervalue(SYSCTLFN_ARGS)
{
	int 		error = 0, t;
	struct sht4x_sc *sc;
	struct sysctlnode node;

	node = *rnode;
	sc = node.sysctl_data;
	t = sc->sc_heaterval;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (t < 1 || t > 3)
		return (EINVAL);

	sc->sc_heaterval = t;

	return error;
}

int
sht4x_verify_sysctl_heaterpulse(SYSCTLFN_ARGS)
{
	char buf[SHT4X_PULSE_NAME];
	struct sht4x_sc *sc;
	struct sysctlnode node;
	int error = 0;
	size_t i;

	node = *rnode;
	sc = node.sysctl_data;
	(void) memcpy(buf, sc->sc_heaterpulse, SHT4X_PULSE_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(sht4x_heaterpulses); i++) {
		if (strncmp(node.sysctl_data, sht4x_heaterpulses[i].length,
		    SHT4X_RES_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(sht4x_heaterpulses))
		return EINVAL;
	(void) memcpy(sc->sc_heaterpulse, node.sysctl_data, SHT4X_PULSE_NAME);

	return error;
}

static int
sht4x_cmddelay(uint8_t cmd)
{
	int r = -1;

	for(int i = 0;i < __arraycount(sht4x_timings);i++) {
		if (cmd == sht4x_timings[i].cmd) {
			r = sht4x_timings[i].typicaldelay;
			break;
		}
	}

	if (r == -1) {
		panic("Bad command look up in cmd delay: cmd: %d\n",cmd);
	}

	return r;
}

static int
sht4x_cmd(i2c_tag_t tag, i2c_addr_t addr, uint8_t *cmd,
    uint8_t clen, uint8_t *buf, size_t blen, int readattempts)
{
	int error;
	int cmddelay;

	error = iic_exec(tag,I2C_OP_WRITE_WITH_STOP,addr,cmd,clen,NULL,0,0);

	/* Every command returns something except for the soft reset
	   which returns nothing.  This chip is also nice in that pretty
	   much every command that returns something does it in the same way.
	*/
	if (error == 0 && cmd[0] != SHT4X_SOFT_RESET) {
		cmddelay = sht4x_cmddelay(cmd[0]);
		delay(cmddelay);

		for (int aint = 0; aint < readattempts; aint++) {
			error = iic_exec(tag,I2C_OP_READ_WITH_STOP,addr,NULL,0,buf,blen,0);
			if (error == 0)
				break;
			delay(1000);
		}
	}

	return error;
}

static int
sht4x_cmdr(struct sht4x_sc *sc, uint8_t cmd, uint8_t *buf, size_t blen)
{
	return sht4x_cmd(sc->sc_tag, sc->sc_addr, &cmd, 1, buf, blen, sc->sc_readattempts);
}

static	uint8_t
sht4x_crc(uint8_t * data, size_t size)
{
	uint8_t crc = 0xFF;

	for (size_t i = 0; i < size; i++) {
		crc ^= data[i];
		for (size_t j = 8; j > 0; j--) {
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x131;
			else
				crc <<= 1;
		}
	}
	return crc;
}

static int
sht4x_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = SHT4X_READ_SERIAL;
	uint8_t buf[6];
	int error;

	error = sht4x_cmd(tag, addr, &reg, 1, buf, 6, 10);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
sht4x_sysctl_init(struct sht4x_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("sht4x controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef SHT4X_DEBUG
	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), sht4x_verify_sysctl, 0,
	    &sc->sc_sht4xdebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readattempts",
	    SYSCTL_DESCR("The number of times to attempt to read the values"),
	    sht4x_verify_sysctl, 0, &sc->sc_readattempts, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "resolutions",
	    SYSCTL_DESCR("Valid resolutions"), 0, 0,
	    __UNCONST(sht4x_resolution_names),
	    sizeof(sht4x_resolution_names) + 1,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "resolution",
	    SYSCTL_DESCR("Resolution of RH and Temp"),
	    sht4x_verify_sysctl_resolution, 0, (void *) sc,
	    SHT4X_RES_NAME, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ignorecrc",
	    SYSCTL_DESCR("Ignore the CRC byte"), NULL, 0, &sc->sc_ignorecrc,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "heateron",
	    SYSCTL_DESCR("Heater on"), sht4x_verify_sysctl_heateron, 0,
	    (void *)sc, 0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "heaterstrength",
	    SYSCTL_DESCR("Heater strength 1 to 3"),
	    sht4x_verify_sysctl_heatervalue, 0, (void *)sc, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "heaterpulses",
	    SYSCTL_DESCR("Valid heater pulse lengths"), 0, 0,
	    __UNCONST(sht4x_heaterpulse_names),
	    sizeof(sht4x_heaterpulse_names) + 1,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht4xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "heaterpulse",
	    SYSCTL_DESCR("Heater pulse length"),
	    sht4x_verify_sysctl_heaterpulse, 0, (void *) sc,
	    SHT4X_RES_NAME, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;
	return 0;
}

static int
sht4x_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	/* indirect config - check for configured address */
	if (ia->ia_addr != SHT4X_TYPICAL_ADDR)
		return 0;

	/*
	 * Check to see if something is really at this i2c address. This will
	 * keep phantom devices from appearing
	 */
	if (iic_acquire_bus(ia->ia_tag, 0) != 0) {
		if (matchdebug)
			printf("in match acquire bus failed\n");
		return 0;
	}

	error = sht4x_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
sht4x_attach(device_t parent, device_t self, void *aux)
{
	struct sht4x_sc *sc;
	struct i2c_attach_args *ia;
	int error, i;
	int ecount = 0;
	uint8_t buf[6];
	uint8_t sncrcpt1, sncrcpt2;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_sht4xdebug = 0;
	strlcpy(sc->sc_resolution,"high",SHT4X_RES_NAME);
	sc->sc_readattempts = 10;
	sc->sc_ignorecrc = false;
	sc->sc_heateron = false;
	sc->sc_heaterval = 1;
	strlcpy(sc->sc_heaterpulse,"short",SHT4X_PULSE_NAME);
	sc->sc_sme = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_numsensors = __arraycount(sht4x_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}
	if ((error = sht4x_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	error = sht4x_cmdr(sc, SHT4X_SOFT_RESET, NULL, 0);
	if (error != 0)
		aprint_error_dev(self, "Reset failed: %d\n", error);

	delay(1000); /* 1 ms max */

	error = sht4x_cmdr(sc, SHT4X_READ_SERIAL, buf, 6);
	if (error) {
		aprint_error_dev(self, "Failed to read serial number: %d\n",
		    error);
		ecount++;
	}

	sncrcpt1 = sht4x_crc(&buf[0],2);
	sncrcpt2 = sht4x_crc(&buf[3],2);

	DPRINTF(sc, 2, ("%s: read serial number values: %02x%02x - %02x, %02x%02x - %02x ; %02x %02x\n",
	    device_xname(sc->sc_dev), buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], sncrcpt1, sncrcpt2));

	iic_release_bus(sc->sc_tag, 0);
	if (error != 0) {
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	for (i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc, sht4x_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = sht4x_sensors[i].type;
		sc->sc_sensors[i].state = ENVSYS_SINVALID;

		DPRINTF(sc, 2, ("%s: registering sensor %d (%s)\n", __func__, i,
		    sc->sc_sensors[i].desc));

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[i]);
		if (error) {
			aprint_error_dev(self,
			    "Unable to attach sensor %d: %d\n", i, error);
			goto out;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = sht4x_refresh;

	DPRINTF(sc, 2, ("sht4x_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
			"unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	/* There is no documented way to ask the chip what version it is.  This
	   is likely fine as the only apparent difference is in how precise the
	   measurements will be.  The actual conversation with the chip is
	   identical no matter which one you are talking to.
	*/

	aprint_normal_dev(self, "Sensirion SHT40/SHT41/SHT45, "
	    "Serial number: %02x%02x%02x%02x%s",
	    buf[0], buf[1], buf[3], buf[4],
	    (sncrcpt1 == buf[2] && sncrcpt2 == buf[5]) ? "\n" : " (bad crc)\n");
	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

/* If you use the heater on this chip, there is no documented choice but to use
   the highest precision.  If the heater is not in use one may select different
   precisions or repeatability for the measurement.

   Further, if the heater is used, it will only be active during the measurement.
   The use of the heater will add delay to the measurement as chip will not
   return anything until the heater pulse time is over.
*/

static uint8_t
sht4x_compute_measure_command(char *resolution, bool heateron,
    int heatervalue, char *heaterpulse)
{
	int i;
	uint8_t r;

	if (heateron == false) {
		for (i = 0; i < __arraycount(sht4x_resolutions); i++) {
			if (strncmp(resolution, sht4x_resolutions[i].text,
			    SHT4X_RES_NAME) == 0) {
				r = sht4x_resolutions[i].cmd;
				break;
			}
		}

		if (i == __arraycount(sht4x_resolutions))
			panic("Heater off could not find command for resolution: %s\n",resolution);
	} else {
		for (i = 0; i < __arraycount(sht4x_heateron_commands); i++) {
			if (heatervalue == sht4x_heateron_commands[i].heatervalue &&
			    strncmp(heaterpulse, sht4x_heateron_commands[i].pulselength,
			    SHT4X_PULSE_NAME) == 0) {
				r = sht4x_heateron_commands[i].cmd;
				break;
			}
		}

		if (i == __arraycount(sht4x_heateron_commands))
			panic("Heater on could not find command for heatervalue, heaterpulse: %d %s\n",
			    heatervalue,heaterpulse);
	}

	return r;
}

static void
sht4x_refresh(struct sysmon_envsys * sme, envsys_data_t * edata)
{
	struct sht4x_sc *sc;
	sc = sme->sme_cookie;
	int error;
	uint8_t rawdata[6];
	uint8_t measurement_command;
	edata->state = ENVSYS_SINVALID;

	mutex_enter(&sc->sc_mutex);
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire i2c bus: %x\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	/*
	  The documented conversion calculations for the raw values are as follows:

	  %RH = (-6 + 125 * rawvalue / 65535)

	  T in Celsius = (-45 + 175 * rawvalue / 65535)

	  It follows then:

	  T in Kelvin = (228.15 + 175 * rawvalue / 65535)

	  given the relationship between Celsius and Kelvin.

	  What follows reorders the calculation a bit and scales it up to avoid
	  the use of any floating point.  All that would really have to happen
	  is a scale up to 10^6 for the sysenv framework, which wants
	  temperature in micro-kelvin and percent relative humidity scaled up
	  10^6, but since this conversion uses 64 bits due to intermediate
	  values that are bigger than 32 bits the conversion first scales up to
	  10^9 and the scales back down by 10^3 at the end.  This preserves some
	  precision in the conversion that would otherwise be lost.
	 */

	measurement_command = sht4x_compute_measure_command(sc->sc_resolution,
	    sc->sc_heateron, sc->sc_heaterval, sc->sc_heaterpulse);
	DPRINTF(sc, 2, ("%s: Measurement command: %02x\n",
	    device_xname(sc->sc_dev), measurement_command));

	/* This chip is pretty nice in that all commands are the same length and
	   return the same result.  What is not so nice is that you can not ask
	   for temperature and humidity independently.

	   The result will be 16 bits of raw temperature and a CRC byte followed
	   by 16 bits of humidity followed by a CRC byte.
	*/

	error = sht4x_cmdr(sc,measurement_command,rawdata,6);

	if (error == 0) {
		DPRINTF(sc, 2, ("%s: Raw data: %02x%02x %02x - %02x%02x %02x\n",
		    device_xname(sc->sc_dev), rawdata[0], rawdata[1], rawdata[2],
		    rawdata[3], rawdata[4], rawdata[5]));


		uint8_t *svalptr;
		uint64_t svalue;
		int64_t v1;
		uint64_t v2;
		uint64_t d1 = 65535;
		uint64_t mul1;
		uint64_t mul2;
		uint64_t div1 = 10000;
		uint64_t q;

		switch (edata->sensor) {
		case SHT4X_TEMP_SENSOR:
			svalptr = &rawdata[0];
			v1 = 22815; /* this is scaled up already from 228.15 */
			v2 = 175;
			mul1 = 10000000000;
			mul2 = 100000000;
			break;
		case SHT4X_HUMIDITY_SENSOR:
			svalptr = &rawdata[3];
			v1 = -6;
			v2 = 125;
			mul1 = 10000000000;
			mul2 = 10000000000;
			break;
		default:
			error = EINVAL;
			break;
		}

		if (error == 0) {
			uint8_t testcrc;

			/* Fake out the CRC check if being asked to ignore CRC */
			if (sc->sc_ignorecrc) {
				testcrc = *(svalptr + 2);
			} else {
				testcrc = sht4x_crc(svalptr,2);
			}

			if (*(svalptr + 2) == testcrc) {
				svalue = *svalptr << 8 | *(svalptr + 1);
				DPRINTF(sc, 2, ("%s: Raw sensor 16 bit: %#jx\n",
				    device_xname(sc->sc_dev), (uintmax_t)svalue));

				/* Scale up */
				svalue = svalue * mul1;
				v1 = v1 * mul2;
				/* Perform the conversion */
				q = ((v2 * (svalue / d1)) + v1) / div1;

				DPRINTF(sc, 2, ("%s: Computed sensor: %#jx\n",
				    device_xname(sc->sc_dev), (uintmax_t)q));
				/* The results will fit in 32 bits, so nothing will be lost */
				edata->value_cur = (uint32_t) q;
				edata->state = ENVSYS_SVALID;
			} else {
				error = EINVAL;
			}
		}
	}

	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to get new status in refresh %d\n",
		    device_xname(sc->sc_dev), error));
	}

	iic_release_bus(sc->sc_tag, 0);
out:
	mutex_exit(&sc->sc_mutex);
}

static int
sht4x_detach(device_t self, int flags)
{
	struct sht4x_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);

	/* Remove the sensors */
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	mutex_exit(&sc->sc_mutex);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_sht4xlog);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, sht4xtemp, "iic,sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
sht4xtemp_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_sht4xtemp,
		    cfattach_ioconf_sht4xtemp, cfdata_ioconf_sht4xtemp);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_sht4xtemp,
		      cfattach_ioconf_sht4xtemp, cfdata_ioconf_sht4xtemp);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
