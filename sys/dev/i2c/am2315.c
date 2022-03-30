/*	$NetBSD: am2315.c,v 1.7 2022/03/30 00:06:50 pgoyette Exp $	*/

/*
 * Copyright (c) 2017 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: am2315.c,v 1.7 2022/03/30 00:06:50 pgoyette Exp $");

/*
 * Driver for the Aosong AM2315
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/time.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/am2315reg.h>
#include <dev/i2c/am2315var.h>

static uint16_t am2315_crc(uint8_t *, size_t);
static int 	am2315_poke(struct am2315_sc *);
static int 	am2315_poke_m(i2c_tag_t, i2c_addr_t, const char *, bool);
static int 	am2315_match(device_t, cfdata_t, void *);
static void 	am2315_attach(device_t, device_t, void *);
static int 	am2315_detach(device_t, int);
static void 	am2315_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	am2315_verify_sysctl(SYSCTLFN_ARGS);

#define AM2315_DEBUG
#ifdef AM2315_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_am2315debug) \
	printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(am2315temp, sizeof(struct am2315_sc),
    am2315_match, am2315_attach, am2315_detach, NULL);

static struct am2315_sensor am2315_sensors[] = {
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
	},
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
	}
};

static uint16_t
am2315_crc(uint8_t *data, size_t len)
{
	uint16_t crc = 0xffff;

	for (size_t j = 0; j < len; j++) {
		crc ^= data[j];
		for (size_t i = 0; i < 8; i++) {
			if (crc & 0x01) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}

	return crc;
}

int 
am2315_verify_sysctl(SYSCTLFN_ARGS)
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

	*(int *) rnode->sysctl_data = t;

	return 0;
}

static int
am2315_cmd(i2c_tag_t tag, i2c_addr_t addr, uint8_t dir, uint8_t cmd,
    uint8_t clen, uint8_t *buf, size_t blen)
{
	uint8_t command[] = { dir, cmd, clen };
	if (buf)
		memset(buf, 0xff, blen);
	uint8_t reg = dir == AM2315_READ_REGISTERS ?
	    I2C_OP_READ_WITH_STOP : I2C_OP_WRITE_WITH_STOP;

	return iic_exec(tag, reg, addr, command,
	    __arraycount(command), buf, blen, 0);
}

static int
am2315_read_regs(struct am2315_sc *sc, uint8_t cmd, uint8_t clen, uint8_t *buf,
    size_t blen)
{
	return am2315_cmd(sc->sc_tag, sc->sc_addr, AM2315_READ_REGISTERS,
	    cmd, clen, buf, blen);
}

static int
am2315_poke(struct am2315_sc *sc)
{
	return am2315_poke_m(sc->sc_tag, sc->sc_addr, device_xname(sc->sc_dev),
	    sc->sc_am2315debug >= 2);
}

static int
am2315_poke_m(i2c_tag_t tag, i2c_addr_t addr, const char *name, bool debug)
{
	uint8_t buf[5];
	int error;

	error = am2315_cmd(tag, addr, AM2315_WRITE_REGISTERS,
	    AM2315_REGISTER_HIGH_USER1, 1, NULL, 0);
	if (debug)
		printf("%s: poke 1: %d\n", name, error);

	if (error)
		delay(2800);

	error = am2315_cmd(tag, addr, AM2315_READ_REGISTERS,
	    AM2315_REGISTER_STATUS, 1, buf, __arraycount(buf));
	if (debug)
		printf("%s: poke 2: %d %02x %02x %02x %02x%02x\n", name, error,
		    buf[0], buf[1], buf[2], buf[3], buf[4]);

	if (error)
		delay(2800);
	return error;
}

static int
am2315_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	/* indirect config - check for standard address */
	if (ia->ia_addr == AM2315_TYPICAL_ADDR)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
am2315_attach(device_t parent, device_t self, void *aux)
{
	struct am2315_sc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t buf[11];
	int error;
	uint16_t crc, readcrc, model;
	uint8_t chipver;
	uint32_t id;
	bool modelgood, chipvergood, idgood;

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_am2315debug = 0;
	sc->sc_readcount = 2;
	sc->sc_readticks = 100;
	sc->sc_sme = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_waitmutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condwait, "am2315wait");

	sc->sc_numsensors = __arraycount(am2315_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self, "unable to create sysmon structure\n");
		return;
	}

	/* XXX: sysctl's not destroyed on failure */
	const struct sysctlnode *cnode;
	int sysctlroot_num;
	if ((error = sysctl_createv(&sc->sc_am2315log, 0, NULL, &cnode, 0,
	    CTLTYPE_NODE, device_xname(self),
	    SYSCTL_DESCR("am2315 controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto badsysctl;
	sysctlroot_num = cnode->sysctl_num;

#ifdef AM2315_DEBUG
	if ((error = sysctl_createv(&sc->sc_am2315log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), am2315_verify_sysctl, 0,
	    &sc->sc_am2315debug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto badsysctl;

#endif

	if ((error = sysctl_createv(&sc->sc_am2315log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readcount",
	    SYSCTL_DESCR("Number of times to read the sensor"),
	    am2315_verify_sysctl, 0, &sc->sc_readcount, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		goto badsysctl;

	if ((error = sysctl_createv(&sc->sc_am2315log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readticks",
	    SYSCTL_DESCR("Number of ticks between reads"),
	    am2315_verify_sysctl, 0, &sc->sc_readticks, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		goto badsysctl;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0) {
		aprint_error_dev(self,
		    "Could not acquire iic bus: %d\n", error);
		return;
	}
	am2315_poke(sc);

#define DUMP(a) \
    DPRINTF(sc, 2, ("%s: read cmd+len+%s+crcl+crch values: %02x %02x " \
	"%02x%02x %02x%02x -- %02x%02x%02x%02x%02x -- %04x %04x\n", a, \
	device_xname(self), buf[0], buf[1], buf[2], buf[3], \
	buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], \
	buf[10], crc, readcrc))

	error = am2315_read_regs(sc, AM2315_REGISTER_HIGH_MODEL, 2, buf, 6);
	if (error)
		aprint_error_dev(sc->sc_dev, "read model: %d\n", error);
	readcrc = buf[5] << 8 | buf[4];
	crc = am2315_crc(buf, 4);
	DUMP("modh+modl");
	model = buf[2] << 8 | buf[3];
	modelgood = buf[0] == AM2315_READ_REGISTERS && buf[1] == 2
	    && crc == readcrc;

	error = am2315_read_regs(sc, AM2315_REGISTER_VERSION, 1, buf, 5);
	if (error)
		aprint_error_dev(self, "read chipver: %d\n", error);
	readcrc = buf[4] << 8 | buf[3];
	crc = am2315_crc(buf, 3);
	DUMP("ver");
	chipver = buf[2];
	chipvergood = buf[0] == AM2315_READ_REGISTERS && buf[1] == 1
	    && crc == readcrc;

	error = am2315_read_regs(sc, AM2315_REGISTER_ID_PT_24_31, 2, buf, 6);
	if (error)
		aprint_error_dev(self, "read id 1: %d\n", error);
	readcrc = buf[5] << 8 | buf[4];
	crc = am2315_crc(buf, 4);
	DUMP("id1+id2");
	id = buf[2] << 8 | buf[3];
	idgood = buf[0] == AM2315_READ_REGISTERS && buf[1] == 2
	    && crc == readcrc;

	error = am2315_read_regs(sc, AM2315_REGISTER_ID_PT_8_15, 2, buf, 6);
	if (error)
		aprint_error_dev(self, "read id 2: %d\n", error);
	readcrc = buf[5] << 8 | buf[4];
	crc = am2315_crc(buf, 4);
	DUMP("id3+id4");
	id = id << 8 | buf[2];
	id = id << 8 | buf[3];
	idgood = buf[0] == AM2315_READ_REGISTERS && buf[1] == 2
	    && crc == readcrc && idgood;

	iic_release_bus(sc->sc_tag, 0);

	for (int i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc, am2315_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = am2315_sensors[i].type;
		sc->sc_sensors[i].state = ENVSYS_SINVALID;

		DPRINTF(sc, 2, ("am2315_attach: registering sensor %d (%s)\n",
		    i, sc->sc_sensors[i].desc));

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[i]);
		if (error) {
			aprint_error_dev(self, "unable to attach sensor %d\n",
			    error);
			goto badregister;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = am2315_refresh;

	DPRINTF(sc, 2, ("am2315_attach: registering with envsys\n"));

	error = sysmon_envsys_register(sc->sc_sme);
	if (error) {
		aprint_error_dev(self, "unable to register with sysmon %d\n",
		    error);
		goto badregister;
	}
	aprint_normal_dev(self, "Aosong AM2315, Model: %04x%s Version: %02x%s"
	    " ID: %08x%s",
	    model, (modelgood ? "," : "(inaccurate),"),
	    chipver, (chipvergood ? "," : "(inaccurate),"),
	    id, (idgood ? "\n" : "(inaccurate)\n"));
	return;

badsysctl:
	aprint_error_dev(self, ": can't setup sysctl tree (%d)\n", error);
	return;
badregister:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static void
am2315_refresh(struct sysmon_envsys * sme, envsys_data_t * edata)
{
	struct am2315_sc *sc;
	uint8_t buf[11], thecommand;
	uint16_t crc, readcrc;
	int  error, rv;
	uint32_t val32;
	bool istempneg = false;

	sc = sme->sme_cookie;
	edata->state = ENVSYS_SINVALID;

	mutex_enter(&sc->sc_mutex);
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire i2c bus: %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	switch (edata->sensor) {
	case AM2315_HUMIDITY_SENSOR:
		thecommand = AM2315_REGISTER_HIGH_RH;
		break;
	case AM2315_TEMP_SENSOR:
		thecommand = AM2315_REGISTER_HIGH_TEMP;
		break;
	default:
		DPRINTF(sc, 2, ("%s: bad sensor %d\n",
		    device_xname(sc->sc_dev), edata->sensor));
		goto out;
	}

	for (int count = 0; ;) {
		am2315_poke(sc);

		if ((error = am2315_read_regs(sc, thecommand, 2, buf, 6)) != 0)
			  DPRINTF(sc, 2, ("%s: Read sensor %d error: %d\n",
			      device_xname(sc->sc_dev),edata->sensor, error));

		readcrc = buf[5] << 8 | buf[4];
		crc = am2315_crc(buf, 4);

		DPRINTF(sc, 2, ("%s: read cmd+len+dh+dl+crch+crcl values: %02x"
		    " %02x %02x%02x %02x%02x -- %04x %04x -- %d\n",
		    device_xname(sc->sc_dev), buf[0], buf[1], buf[2],
		    buf[3], buf[4], buf[5], crc, readcrc, count));
		if (++count == sc->sc_readcount)
			break;
		mutex_enter(&sc->sc_waitmutex);
		rv = cv_timedwait(&sc->sc_condwait, &sc->sc_waitmutex,
		    sc->sc_readticks);
		DPRINTF(sc, 2, ("%s: wait rv: %d\n", device_xname(sc->sc_dev),
		    rv));
		mutex_exit(&sc->sc_waitmutex);
	}

	iic_release_bus(sc->sc_tag, 0);

	if (buf[0] != AM2315_READ_REGISTERS || buf[1] != 2 ||
	    crc != readcrc) {
		DPRINTF(sc, 2, ("%s: Invalid sensor data for %d\n",
		    device_xname(sc->sc_dev), edata->sensor));
		goto out;
	}

	switch (edata->sensor) {
	case AM2315_HUMIDITY_SENSOR:
		val32 = buf[2] << 8 | buf[3];
		val32 = val32 * 100000;
		DPRINTF(sc, 2, ("%s: read translated values RH: %x\n",
		    device_xname(sc->sc_dev), val32));
		edata->value_cur = val32;
		edata->state = ENVSYS_SVALID;
		break;
	case AM2315_TEMP_SENSOR:
		istempneg = (buf[2] & AM2315_TEMP_NEGATIVE);
		buf[2] = buf[2] & (~AM2315_TEMP_NEGATIVE);
		val32 = buf[2] << 8 | buf[3];
		if (istempneg) {
			val32 = 273150000 - (val32 * 100000);
		} else {
			val32 = (val32 * 100000) + 273150000;
		}
		DPRINTF(sc, 2, ("%s: read translated values TEMP: %x\n",
		    device_xname(sc->sc_dev), val32));
		edata->value_cur = val32;
		edata->state = ENVSYS_SVALID;
		break;
	default:
		panic("bad sensor %d\n", edata->sensor);
	}
out:
	mutex_exit(&sc->sc_mutex);
}

static int
am2315_detach(device_t self, int flags)
{
	struct am2315_sc *sc = device_private(self);

	mutex_enter(&sc->sc_mutex);

	/* Remove the sensors */
	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);
	mutex_exit(&sc->sc_mutex);

	/* Destroy the wait cond */
	cv_destroy(&sc->sc_condwait);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_am2315log);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_waitmutex);
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, am2315temp, "iic,sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
am2315temp_modcmd(modcmd_t cmd, void *opaque)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_am2315temp,
		    cfattach_ioconf_am2315temp, cfdata_ioconf_am2315temp);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_am2315temp,
		    cfattach_ioconf_am2315temp, cfdata_ioconf_am2315temp);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
