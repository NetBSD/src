/*	$NetBSD: sgp40.c,v 1.4 2022/05/22 11:27:35 andvar Exp $	*/

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
 * ACTION OF CONTRACT, NEGL`IGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sgp40.c,v 1.4 2022/05/22 11:27:35 andvar Exp $");

/*
  Driver for the Sensirion SGP40 MOx gas sensor for air quality
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/kthread.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/sgp40reg.h>
#include <dev/i2c/sgp40var.h>

#include <dev/i2c/sensirion_arch_config.h>
#include <dev/i2c/sensirion_voc_algorithm.h>

static uint8_t 	sgp40_crc(uint8_t *, size_t);
static int      sgp40_cmdr(struct sgp40_sc *, uint16_t, uint8_t *, uint8_t,
    uint8_t *, size_t);
static int 	sgp40_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	sgp40_match(device_t, cfdata_t, void *);
static void 	sgp40_attach(device_t, device_t, void *);
static int 	sgp40_detach(device_t, int);
static void 	sgp40_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	sgp40_verify_sysctl(SYSCTLFN_ARGS);
static int 	sgp40_verify_temp_sysctl(SYSCTLFN_ARGS);
static int 	sgp40_verify_rh_sysctl(SYSCTLFN_ARGS);
static void     sgp40_thread(void *);
static void     sgp40_stop_thread(void *);
static void     sgp40_take_measurement(void *, VocAlgorithmParams *);

#define SGP40_DEBUG
#ifdef SGP40_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_sgp40debug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(sgp40mox, sizeof(struct sgp40_sc),
    sgp40_match, sgp40_attach, sgp40_detach, NULL);

static struct sgp40_sensor sgp40_sensors[] = {
	{
		.desc = "VOC index",
		.type = ENVSYS_INTEGER,
	}
};

static struct sgp40_timing sgp40_timings[] = {
	{
		.cmd = SGP40_MEASURE_RAW,
		.typicaldelay = 25000,
	},
	{
		.cmd = SGP40_MEASURE_TEST,
		.typicaldelay = 240000,
	},
	{
		.cmd = SGP40_HEATER_OFF,
		.typicaldelay = 100,
	},
	{
		.cmd = SGP40_GET_SERIAL_NUMBER,
		.typicaldelay = 100,
	},
	{
		.cmd = SGP40_GET_FEATURESET,
		.typicaldelay = 1000,
	}
};

void
sgp40_thread(void *aux)
{
	struct sgp40_sc *sc = aux;
	int rv;
	VocAlgorithmParams voc_algorithm_params;

	mutex_enter(&sc->sc_threadmutex);

	VocAlgorithm_init(&voc_algorithm_params);

	while (!sc->sc_stopping) {
		rv = cv_timedwait(&sc->sc_condvar, &sc->sc_threadmutex,
		    mstohz(1000));
		if (rv == EWOULDBLOCK && !sc->sc_stopping) {
			sgp40_take_measurement(sc,&voc_algorithm_params);
		}
	}
	mutex_exit(&sc->sc_threadmutex);
	kthread_exit(0);
}

static void
sgp40_stop_thread(void *aux)
{
	struct sgp40_sc *sc;
	sc = aux;
	int error;

	mutex_enter(&sc->sc_threadmutex);
	sc->sc_stopping = true;
	cv_signal(&sc->sc_condvar);
	mutex_exit(&sc->sc_threadmutex);

	/* wait for the thread to exit */
	kthread_join(sc->sc_thread);

	mutex_enter(&sc->sc_mutex);
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire iic bus for heater off "
		    "in stop thread: %d\n", device_xname(sc->sc_dev), error));
		goto out;
	}
	error = sgp40_cmdr(sc, SGP40_HEATER_OFF, NULL, 0, NULL, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Error turning heater off: %d\n",
		    device_xname(sc->sc_dev), error));
	}
out:
	iic_release_bus(sc->sc_tag, 0);
	mutex_exit(&sc->sc_mutex);
}

static int
sgp40_compute_temp_comp(int unconverted)
{
	/*
	 * The published algorithm for this conversion is:
	 * (temp_in_celcius + 45) * 65535 / 175
	 *
	 * However, this did not exactly yield the results that
	 * the example in the data sheet, so something a little
	 * different was done.
	 *
	 * (temp_in_celcius + 45) * 65536 / 175
	 *
	 * This was also scaled up by 10^2 and then scaled back to
	 * preserve some percision.  37449 is simply (65536 * 100) / 175
	 * and rounded.
	 */

	return (((unconverted + 45) * 100) * 37449) / 10000;
}

static int
sgp40_compute_rh_comp(int unconverted)
{
	int q;

	/*
	 * The published algorithm for this conversion is:
	 * %rh * 65535 / 100
	 *
	 * However, this did not exactly yield the results that
	 * the example in the data sheet, so something a little
	 * different was done.
	 *
	 * %rh * 65536 / 100
	 *
	 * This was also scaled up by 10^2 and then scaled back to
	 * preserve some percision.  The value is also latched to 65535
	 * as an upper limit.
	 */

	q = ((unconverted * 100) * 65536) / 10000;
	if (q > 65535)
		q = 65535;
	return q;
}

static void
sgp40_take_measurement(void *aux, VocAlgorithmParams* params)
{
	struct sgp40_sc *sc;
	sc = aux;
	uint8_t args[6];
	uint8_t buf[3];
	uint16_t rawmeasurement;
	int error;
	uint8_t crc;
	uint16_t convertedrh, convertedtemp;
	int32_t voc_index;

	mutex_enter(&sc->sc_mutex);
	convertedrh = (uint16_t)sgp40_compute_rh_comp(sc->sc_rhcomp);
	convertedtemp = (uint16_t)sgp40_compute_temp_comp(sc->sc_tempcomp);

	DPRINTF(sc, 2, ("%s: Converted RH and Temp: %04x %04x\n",
	    device_xname(sc->sc_dev), convertedrh, convertedtemp));

	args[0] = convertedrh >> 8;
	args[1] = convertedrh & 0x00ff;
	args[2] = sgp40_crc(&args[0], 2);
	args[3] = convertedtemp >> 8;
	args[4] = convertedtemp & 0x00ff;
	args[5] = sgp40_crc(&args[3], 2);

	/*
	 * The VOC algorithm has a black out time when it first starts to run
	 * and does not return any indicator that is going on, so voc_index
	 * in that case would be 0..  however, that is also a valid response
	 * otherwise, although an unlikely one.
	 */
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire iic bus for take "
		    "measurement: %d\n", device_xname(sc->sc_dev), error));
		sc->sc_voc = 0;
		sc->sc_vocvalid = false;
		goto out;
	}

	error = sgp40_cmdr(sc, SGP40_MEASURE_RAW, args, 6, buf, 3);
	iic_release_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to get measurement %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	crc = sgp40_crc(&buf[0], 2);
	DPRINTF(sc, 2, ("%s: Raw ticks and crc: %02x%02x %02x "
	    "%02x\n", device_xname(sc->sc_dev), buf[0], buf[1],
	    buf[2], crc));
	if (buf[2] != crc)
		goto out;

	rawmeasurement = buf[0] << 8;
	rawmeasurement |= buf[1];
	VocAlgorithm_process(params, rawmeasurement,
	    &voc_index);
	DPRINTF(sc, 2, ("%s: VOC index: %d\n",
	    device_xname(sc->sc_dev), voc_index));
	sc->sc_voc = voc_index;
	sc->sc_vocvalid = true;

	mutex_exit(&sc->sc_mutex);
	return;
out:
	sc->sc_voc = 0;
	sc->sc_vocvalid = false;
	mutex_exit(&sc->sc_mutex);
}

int
sgp40_verify_sysctl(SYSCTLFN_ARGS)
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

int
sgp40_verify_temp_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < -45 || t > 130)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

int
sgp40_verify_rh_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 100)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

static int
sgp40_cmddelay(uint16_t cmd)
{
	int r = -1;

	for(int i = 0;i < __arraycount(sgp40_timings);i++) {
		if (cmd == sgp40_timings[i].cmd) {
			r = sgp40_timings[i].typicaldelay;
			break;
		}
	}

	if (r == -1) {
		panic("sgp40: Bad command look up in cmd delay: cmd: %d\n",
		    cmd);
	}

	return r;
}

static int
sgp40_cmd(i2c_tag_t tag, i2c_addr_t addr, uint8_t *cmd,
    uint8_t clen, uint8_t *buf, size_t blen, int readattempts)
{
	int error;
	int cmddelay;
	uint16_t cmd16;

	cmd16 = cmd[0] << 8;
	cmd16 = cmd16 | cmd[1];

	error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, cmd, clen, NULL, 0,
	    0);
	if (error)
		return error;

	/* 
	 * Every command returns something except for turning the heater off
	 * and the general soft reset which returns nothing.
	 */
	if (cmd16 == SGP40_HEATER_OFF)
		return 0;
	/*
	 * Every command has a particular delay for how long
	 * it typically takes and the max time it will take.
	 */
	cmddelay = sgp40_cmddelay(cmd16);
	delay(cmddelay);

	for (int aint = 0; aint < readattempts; aint++) {
		error = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, NULL, 0,
		    buf, blen, 0);
		if (error == 0)
			break;
		delay(1000);
	}

	return error;
}

static int
sgp40_cmdr(struct sgp40_sc *sc, uint16_t cmd, uint8_t *extraargs,
    uint8_t argslen, uint8_t *buf, size_t blen)
{
	uint8_t fullcmd[8];
	uint8_t cmdlen;
	int n;

	/*
	 * The biggest documented command + arguments is 8 uint8_t bytes long.
	 * Catch anything that ties to have an arglen more than 6
	 */
	KASSERT(argslen <= 6);

	memset(fullcmd, 0, 8);

	fullcmd[0] = cmd >> 8;
	fullcmd[1] = cmd & 0x00ff;
	cmdlen = 2;

	n = 0;
	while (extraargs != NULL && n < argslen && cmdlen <= 7) {
		fullcmd[cmdlen] = extraargs[n];
		cmdlen++;
		n++;
	}
	DPRINTF(sc, 2, ("%s: Full command and arguments: %02x %02x %02x %02x "
	    "%02x %02x %02x %02x\n",
	    device_xname(sc->sc_dev), fullcmd[0], fullcmd[1],
	    fullcmd[2], fullcmd[3], fullcmd[4], fullcmd[5],
	    fullcmd[6], fullcmd[7]));
	return sgp40_cmd(sc->sc_tag, sc->sc_addr, fullcmd, cmdlen, buf, blen,
	    sc->sc_readattempts);
}

static	uint8_t
sgp40_crc(uint8_t * data, size_t size)
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
sgp40_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg[2];
	uint8_t buf[9];
	int error;

	/*
	 * Possible bug...  this command may not work if the chip is not idle,
	 * however, it appears to be used by a lot of other code as a probe.
	 */
	reg[0] = SGP40_GET_SERIAL_NUMBER >> 8;
	reg[1] = SGP40_GET_SERIAL_NUMBER & 0x00ff;

	error = sgp40_cmd(tag, addr, reg, 2, buf, 9, 10);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
sgp40_sysctl_init(struct sgp40_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("SGP40 controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef SGP40_DEBUG
	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), sgp40_verify_sysctl, 0,
	    &sc->sc_sgp40debug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readattempts",
	    SYSCTL_DESCR("The number of times to attempt to read the values"),
	    sgp40_verify_sysctl, 0, &sc->sc_readattempts, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ignorecrc",
	    SYSCTL_DESCR("Ignore the CRC byte"), NULL, 0, &sc->sc_ignorecrc,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, "compensation",
	    SYSCTL_DESCR("SGP40 measurement compensations"), NULL, 0, NULL, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;
	int compensation_num = cnode->sysctl_num;

	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "temperature",
	    SYSCTL_DESCR("Temperature compensation in celsius"),
	    sgp40_verify_temp_sysctl, 0, &sc->sc_tempcomp, 0, CTL_HW,
	    sysctlroot_num, compensation_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sgp40log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "humidity",
	    SYSCTL_DESCR("Humidity compensation in %RH"),
	    sgp40_verify_rh_sysctl, 0, &sc->sc_rhcomp, 0, CTL_HW,
	    sysctlroot_num, compensation_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	return 0;
}

static int
sgp40_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (matchdebug)
		printf("in match\n");

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	/* indirect config - check for configured address */
	if (ia->ia_addr != SGP40_TYPICAL_ADDR)
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

	error = sgp40_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
sgp40_attach(device_t parent, device_t self, void *aux)
{
	struct sgp40_sc *sc;
	struct i2c_attach_args *ia;
	int error, i;
	int ecount = 0;
	uint8_t buf[9];
	uint8_t tstcrc;
	uint16_t chiptestvalue;
	uint64_t serial_number = 0;
	uint8_t sn_crc1, sn_crc2, sn_crc3, sn_crcv1, sn_crcv2, sn_crcv3;
	uint8_t fs_crc, fs_crcv;
	uint16_t featureset;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_sgp40debug = 0;
	sc->sc_readattempts = 10;
	sc->sc_ignorecrc = false;
	sc->sc_stopping = false;
	sc->sc_voc = 0;
	sc->sc_vocvalid = false;
	sc->sc_tempcomp = SGP40_DEFAULT_TEMP_COMP;
	sc->sc_rhcomp = SGP40_DEFAULT_RH_COMP;
	sc->sc_sme = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_threadmutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condvar, "sgp40cv");
	sc->sc_numsensors = __arraycount(sgp40_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}
	if ((error = sgp40_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	/*
	 * Usually one would reset the chip here, but that is not possible
	 * without resetting the entire bus, so we won't do that.
	 *
	 * What we will do is make sure that the chip is idle by running the
	 * turn-the-heater command.
	 */

	error = sgp40_cmdr(sc, SGP40_HEATER_OFF, NULL, 0, NULL, 0);
	if (error) {
		aprint_error_dev(self, "Failed to turn off the heater: %d\n",
		    error);
		ecount++;
	}

	error = sgp40_cmdr(sc, SGP40_GET_SERIAL_NUMBER, NULL, 0, buf, 9);
	if (error) {
		aprint_error_dev(self, "Failed to get serial number: %d\n",
		    error);
		ecount++;
	}

	sn_crc1 = sgp40_crc(&buf[0], 2);
	sn_crc2 = sgp40_crc(&buf[3], 2);
	sn_crc3 = sgp40_crc(&buf[6], 2);
	sn_crcv1 = buf[2];
	sn_crcv2 = buf[5];
	sn_crcv3 = buf[8];
	serial_number = buf[0];
	serial_number = (serial_number << 8) | buf[1];
	serial_number = (serial_number << 8) | buf[3];
	serial_number = (serial_number << 8) | buf[4];
	serial_number = (serial_number << 8) | buf[6];
	serial_number = (serial_number << 8) | buf[7];

	DPRINTF(sc, 2, ("%s: raw serial number: %02x %02x %02x %02x %02x %02x "
	    "%02x %02x %02x\n",
	    device_xname(sc->sc_dev), buf[0], buf[1], buf[2], buf[3], buf[4],
	    buf[5], buf[6], buf[7], buf[8]));

	error = sgp40_cmdr(sc, SGP40_GET_FEATURESET, NULL, 0, buf, 3);
	if (error) {
		aprint_error_dev(self, "Failed to get featureset: %d\n",
		    error);
		ecount++;
	}

	fs_crc = sgp40_crc(&buf[0], 2);
	fs_crcv = buf[2];
	featureset = buf[0];
	featureset = (featureset << 8) | buf[1];

	DPRINTF(sc, 2, ("%s: raw feature set: %02x %02x %02x\n",
	    device_xname(sc->sc_dev), buf[0], buf[1], buf[2]));

	error = sgp40_cmdr(sc, SGP40_MEASURE_TEST, NULL, 0, buf, 3);
	if (error) {
		aprint_error_dev(self, "Failed to perform a chip test: %d\n",
		    error);
		ecount++;
	}

	tstcrc = sgp40_crc(&buf[0], 2);

	DPRINTF(sc, 2, ("%s: chip test values: %02x%02x - %02x ; %02x\n",
	    device_xname(sc->sc_dev), buf[0], buf[1], buf[2], tstcrc));

	iic_release_bus(sc->sc_tag, 0);
	if (error != 0) {
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	chiptestvalue = buf[0] << 8;
	chiptestvalue |= buf[1];

	for (i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc, sgp40_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = sgp40_sensors[i].type;
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
	sc->sc_sme->sme_refresh = sgp40_refresh;

	DPRINTF(sc, 2, ("sgp40_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
			"unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	error = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN, NULL,
	    sgp40_thread, sc, &sc->sc_thread, "%s", device_xname(sc->sc_dev));
	if (error) {
		aprint_error_dev(self,"Unable to create measurement thread\n");
		goto out;
	}

	aprint_normal_dev(self, "Sensirion SGP40, Serial number: %jx%s"
	    "Feature set word: 0x%jx%s%s%s", serial_number,
	    (sn_crc1 == sn_crcv1 && sn_crc2 == sn_crcv2 && sn_crc3 == sn_crcv3)
	    ? ", " : " (bad crc), ",
	    (uintmax_t)featureset,
	    (fs_crc == fs_crcv) ? ", " : " (bad crc), ",
	    (chiptestvalue == SGP40_TEST_RESULTS_ALL_PASSED) ?
		"All chip tests passed" :
	    (chiptestvalue == SGP40_TEST_RESULTS_SOME_FAILED) ?
		"Some chip tests failed" :
	    "Unknown test results",
	    (tstcrc == buf[2]) ? "\n" : " (bad crc)\n");
	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static void
sgp40_refresh(struct sysmon_envsys * sme, envsys_data_t * edata)
{
	struct sgp40_sc *sc;
	sc = sme->sme_cookie;

	mutex_enter(&sc->sc_mutex);
	if (sc->sc_vocvalid == true) {
		edata->value_cur = (uint32_t)sc->sc_voc;
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}
	mutex_exit(&sc->sc_mutex);
}

static int
sgp40_detach(device_t self, int flags)
{
	struct sgp40_sc *sc;

	sc = device_private(self);

	/* stop the measurement thread */
	sgp40_stop_thread(sc);

	/* Remove the sensors */
	mutex_enter(&sc->sc_mutex);
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	mutex_exit(&sc->sc_mutex);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_sgp40log);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);
	mutex_destroy(&sc->sc_threadmutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, sgp40mox, "iic,sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
sgp40mox_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_sgp40mox,
		    cfattach_ioconf_sgp40mox, cfdata_ioconf_sgp40mox);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_sgp40mox,
		      cfattach_ioconf_sgp40mox, cfdata_ioconf_sgp40mox);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
