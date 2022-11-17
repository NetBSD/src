/*	$NetBSD: aht20.c,v 1.1 2022/11/17 19:20:06 brad Exp $	*/

/*
 * Copyright (c) 2022 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: aht20.c,v 1.1 2022/11/17 19:20:06 brad Exp $");

/*
  Driver for the Guangzhou Aosong AHT20 temperature and humidity sensor
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
#include <dev/i2c/aht20reg.h>
#include <dev/i2c/aht20var.h>


static uint8_t 	aht20_crc(uint8_t *, size_t);
static int 	aht20_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	aht20_match(device_t, cfdata_t, void *);
static void 	aht20_attach(device_t, device_t, void *);
static int 	aht20_detach(device_t, int);
static void 	aht20_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	aht20_verify_sysctl(SYSCTLFN_ARGS);

#define AHT20_DEBUG
#ifdef AHT20_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_aht20debug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(aht20temp, sizeof(struct aht20_sc),
    aht20_match, aht20_attach, aht20_detach, NULL);

static struct aht20_sensor aht20_sensors[] = {
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
	},
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
	}
};

/*
 * The delays are mentioned in the datasheet for the chip, except for
 * the get status command.
 */

static struct aht20_timing aht20_timings[] = {
	{
		.cmd = AHT20_INITIALIZE,
		.typicaldelay = 10000,
	},
	{
		.cmd = AHT20_TRIGGER_MEASUREMENT,
		.typicaldelay = 80000,
	},
	{
		.cmd = AHT20_GET_STATUS,
		.typicaldelay = 5000,
	},
	{
		.cmd = AHT20_SOFT_RESET,
		.typicaldelay = 20000,
	}
};

int
aht20_verify_sysctl(SYSCTLFN_ARGS)
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

static int
aht20_cmddelay(uint8_t cmd)
{
	int r = -1;

	for(int i = 0;i < __arraycount(aht20_timings);i++) {
		if (cmd == aht20_timings[i].cmd) {
			r = aht20_timings[i].typicaldelay;
			break;
		}
	}

	if (r == -1) {
		panic("Bad command look up in cmd delay: cmd: %d\n",cmd);
	}

	return r;
}

static int
aht20_cmd(i2c_tag_t tag, i2c_addr_t addr, uint8_t *cmd,
    uint8_t clen, uint8_t *buf, size_t blen, int readattempts)
{
	int error;
	int cmddelay;

	error = iic_exec(tag,I2C_OP_WRITE_WITH_STOP,addr,cmd,clen,NULL,0,0);

	/* Every command returns something except for the soft reset and
	   initialize which returns nothing.
	*/

	if (error == 0) {
		cmddelay = aht20_cmddelay(cmd[0]);
		delay(cmddelay);

		if (cmd[0] != AHT20_SOFT_RESET &&
		    cmd[0] != AHT20_INITIALIZE) {
			for (int aint = 0; aint < readattempts; aint++) {
				error = iic_exec(tag,I2C_OP_READ_WITH_STOP,addr,NULL,0,buf,blen,0);
				if (error == 0)
					break;
				delay(1000);
			}
		}
	}

	return error;
}

static int
aht20_cmdr(struct aht20_sc *sc, uint8_t *cmd, uint8_t clen, uint8_t *buf, size_t blen)
{
	KASSERT(clen > 0);

	return aht20_cmd(sc->sc_tag, sc->sc_addr, cmd, clen, buf, blen, sc->sc_readattempts);
}

static	uint8_t
aht20_crc(uint8_t * data, size_t size)
{
	uint8_t crc = 0xFF;

	for (size_t i = 0; i < size; i++) {
		crc ^= data[i];
		for (size_t j = 8; j > 0; j--) {
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc <<= 1;
		}
	}
	return crc;
}

static int
aht20_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = AHT20_GET_STATUS;
	uint8_t buf[6];
	int error;

	error = aht20_cmd(tag, addr, &reg, 1, buf, 1, 10);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
aht20_sysctl_init(struct aht20_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_aht20log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("aht20 controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef AHT20_DEBUG
	if ((error = sysctl_createv(&sc->sc_aht20log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), aht20_verify_sysctl, 0,
	    &sc->sc_aht20debug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

	if ((error = sysctl_createv(&sc->sc_aht20log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readattempts",
	    SYSCTL_DESCR("The number of times to attempt to read the values"),
	    aht20_verify_sysctl, 0, &sc->sc_readattempts, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_aht20log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ignorecrc",
	    SYSCTL_DESCR("Ignore the CRC byte"), NULL, 0, &sc->sc_ignorecrc,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	return 0;
}

static int
aht20_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	/* indirect config - check for configured address */
	if (ia->ia_addr != AHT20_TYPICAL_ADDR)
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

	error = aht20_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
aht20_attach(device_t parent, device_t self, void *aux)
{
	struct aht20_sc *sc;
	struct i2c_attach_args *ia;
	uint8_t cmd[1];
	int error, i;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_aht20debug = 0;
	sc->sc_readattempts = 10;
	sc->sc_ignorecrc = false;
	sc->sc_sme = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_numsensors = __arraycount(aht20_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}
	if ((error = aht20_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	cmd[0] = AHT20_SOFT_RESET;
	error = aht20_cmdr(sc, cmd, 1, NULL, 0);
	if (error != 0)
		aprint_error_dev(self, "Reset failed: %d\n", error);

	iic_release_bus(sc->sc_tag, 0);

	if (error != 0) {
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	for (i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc, aht20_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = aht20_sensors[i].type;
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
	sc->sc_sme->sme_refresh = aht20_refresh;

	DPRINTF(sc, 2, ("aht20_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
			"unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	aprint_normal_dev(self, "Guangzhou Aosong AHT20\n");

	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static void
aht20_refresh(struct sysmon_envsys * sme, envsys_data_t * edata)
{
	struct aht20_sc *sc;
	sc = sme->sme_cookie;
	int error;
	uint8_t cmd[3];
	uint8_t rawdata[7];
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

	  %RH = (Srh / 2^20) * 100%

	  T in Celsius = ((St / 2^20) * 200) - 50

	  It follows then:

	  T in Kelvin = ((St / 2^20) * 200) + 223.15

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

	cmd[0] = AHT20_TRIGGER_MEASUREMENT;
	cmd[1] = AHT20_TRIGGER_PARAM1;
	cmd[2] = AHT20_TRIGGER_PARAM2;
	error = aht20_cmdr(sc, cmd, 3, rawdata, 7);

	if (error == 0) {
		if (rawdata[0] & AHT20_STATUS_BUSY_MASK) {
			aprint_error_dev(sc->sc_dev,
			    "Chip is busy.  Status register: %02x\n",
			    rawdata[0]);
			error = EINVAL;
		}

		if (error == 0 &&
		    rawdata[0] & AHT20_STATUS_CAL_MASK) {

			uint8_t testcrc;

			testcrc = aht20_crc(&rawdata[0],6);

			DPRINTF(sc, 2, ("%s: Raw data: STATUS: %02x - RH: %02x %02x - %02x - TEMP: %02x %02x - CRC: %02x -- %02x\n",
			    device_xname(sc->sc_dev), rawdata[0], rawdata[1], rawdata[2],
			    rawdata[3], rawdata[4], rawdata[5], rawdata[6], testcrc));

			/* This chip splits the %rh and temp raw files ove the 3 byte returned.  Since
			   there is no choice but to get both, split them both apart every time */

			uint64_t rawhum;
			uint64_t rawtemp;

			rawhum = (rawdata[1] << 12) | (rawdata[2] << 4) | ((rawdata[3] & 0xf0) >> 4);
			rawtemp = ((rawdata[3] & 0x0f) << 16) | (rawdata[4] << 8) | rawdata[5];

			DPRINTF(sc, 2, ("%s: Raw broken data: RH: %04jx (%jd) - TEMP: %04jx (%jd)\n",
			    device_xname(sc->sc_dev), rawhum, rawhum, rawtemp, rawtemp));

			/* Fake out the CRC check if being asked to ignore CRC */
			if (sc->sc_ignorecrc) {
				testcrc = rawdata[6];
			}

			if (rawdata[6] == testcrc) {
				uint64_t q = 0;

				switch (edata->sensor) {
				case AHT20_TEMP_SENSOR:
					q = (((rawtemp * 1000000000) / 10485760) * 2) + 223150000;

					break;
				case AHT20_HUMIDITY_SENSOR:
					q = (rawhum * 1000000000) / 10485760;

					break;
				default:
					error = EINVAL;
					break;
				}

				DPRINTF(sc, 2, ("%s: Computed sensor: %#jx (%jd)\n",
				    device_xname(sc->sc_dev), (uintmax_t)q, (uintmax_t)q));

				/* The results will fit in 32 bits, so nothing will be lost */
				edata->value_cur = (uint32_t) q;
				edata->state = ENVSYS_SVALID;
			} else {
				error = EINVAL;
			}
		} else {
			if (error == 0) {
				aprint_error_dev(sc->sc_dev,"Calibration needs to be run on the chip.\n");

				cmd[0] = AHT20_INITIALIZE;
				cmd[1] = AHT20_INITIALIZE_PARAM1;
				cmd[2] = AHT20_INITIALIZE_PARAM2;
				error = aht20_cmdr(sc, cmd, 3, NULL, 0);

				if (error) {
					DPRINTF(sc, 2, ("%s: Calibration failed to run: %d\n",
					    device_xname(sc->sc_dev), error));
				}
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
aht20_detach(device_t self, int flags)
{
	struct aht20_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);

	/* Remove the sensors */
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	mutex_exit(&sc->sc_mutex);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_aht20log);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, aht20temp, "iic,sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
aht20temp_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_aht20temp,
		    cfattach_ioconf_aht20temp, cfdata_ioconf_aht20temp);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_aht20temp,
		      cfattach_ioconf_aht20temp, cfdata_ioconf_aht20temp);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
