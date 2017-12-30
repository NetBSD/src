/*	$NetBSD: si70xx.c,v 1.3 2017/12/30 03:18:26 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: si70xx.c,v 1.3 2017/12/30 03:18:26 christos Exp $");

/*
  Driver for the Silicon Labs SI7013/SI7020/SI7021
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
#include <dev/i2c/si70xxreg.h>
#include <dev/i2c/si70xxvar.h>


static uint8_t 	si70xx_crc(uint8_t *, size_t);
static int 	si70xx_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	si70xx_match(device_t, cfdata_t, void *);
static void 	si70xx_attach(device_t, device_t, void *);
static int 	si70xx_detach(device_t, int);
static void 	si70xx_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	si70xx_update_status(struct si70xx_sc *);
static int 	si70xx_set_heateron(struct si70xx_sc *);
static int 	si70xx_set_resolution(struct si70xx_sc *, size_t);
static int 	si70xx_set_heatervalue(struct si70xx_sc *, size_t);
static int 	si70xx_verify_sysctl(SYSCTLFN_ARGS);
static int 	si70xx_verify_sysctl_resolution(SYSCTLFN_ARGS);
static int 	si70xx_verify_sysctl_heateron(SYSCTLFN_ARGS);
static int 	si70xx_verify_sysctl_heatervalue(SYSCTLFN_ARGS);

#define SI70XX_DEBUG
#ifdef SI70XX_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_si70xxdebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(si70xxtemp, sizeof(struct si70xx_sc),
    si70xx_match, si70xx_attach, si70xx_detach, NULL);

static struct si70xx_sensor si70xx_sensors[] = {
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
	},
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
	}
};

static struct si70xx_resolution si70xx_resolutions[] = {
	{
		.text = "12bit/14bit",
		.num = 0x00,
	},
	{
		.text = "8bit/12bit",
		.num = 0x01,
	},
	{
		.text = "10bit/13bit",
		.num = 0x80,
	},
	{
		.text = "11bit/11bit",
		.num = 0x81,
	}
};

static const char si70xx_resolution_names[] =
    "12bit/14bit, 8bit/12bit, 10bit/13bit, 11bit/11bit";

static const int si70xx_heatervalues[] = {
    0xdeadbeef, 0x00, 0x01, 0x02, 0x04, 0x08, 0x0f
};

int 
si70xx_verify_sysctl(SYSCTLFN_ARGS)
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
si70xx_verify_sysctl_resolution(SYSCTLFN_ARGS)
{
	char buf[SI70XX_RES_NAME];
	struct si70xx_sc *sc;
	struct sysctlnode node;
	int error = 0;
	size_t i;

	node = *rnode;
	sc = node.sysctl_data;
	(void) memcpy(buf, sc->sc_resolution, SI70XX_RES_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(si70xx_resolutions); i++) {
		if (memcmp(node.sysctl_data, si70xx_resolutions[i].text,
		    SI70XX_RES_NAME) == 0)
			break;
	}

	if (i == __arraycount(si70xx_resolutions))
		return EINVAL;
	(void) memcpy(sc->sc_resolution, node.sysctl_data, SI70XX_RES_NAME);

	error = si70xx_set_resolution(sc, i);

	return error;
}

int 
si70xx_verify_sysctl_heateron(SYSCTLFN_ARGS)
{
	int 		error;
	bool 		t;
	struct si70xx_sc *sc;
	struct sysctlnode node;

	node = *rnode;
	sc = node.sysctl_data;
	t = sc->sc_heateron;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	sc->sc_heateron = t;
	error = si70xx_set_heateron(sc);

	return error;
}

int 
si70xx_verify_sysctl_heatervalue(SYSCTLFN_ARGS)
{
	int 		error = 0, t;
	struct si70xx_sc *sc;
	struct sysctlnode node;

	node = *rnode;
	sc = node.sysctl_data;
	t = sc->sc_heaterval;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (t < 1 || t >= __arraycount(si70xx_heatervalues))
		return (EINVAL);

	sc->sc_heaterval = t;
	error = si70xx_set_heatervalue(sc, t);

	return error;
}

static uint8_t
si70xx_dir(uint8_t cmd, size_t len)
{
	switch (cmd) {
	case SI70XX_READ_USER_REG_1:
	case SI70XX_READ_HEATER_REG:
	case SI70XX_READ_ID_PT1A:
	case SI70XX_READ_ID_PT1B:
	case SI70XX_READ_ID_PT2A:
	case SI70XX_READ_ID_PT2B:
	case SI70XX_READ_FW_VERA:
	case SI70XX_READ_FW_VERB:
		return I2C_OP_READ_WITH_STOP;
	case SI70XX_WRITE_USER_REG_1:
	case SI70XX_WRITE_HEATER_REG:
	case SI70XX_RESET:
		return I2C_OP_WRITE_WITH_STOP;
	case SI70XX_MEASURE_RH_NOHOLD:
	case SI70XX_MEASURE_TEMP_NOHOLD:
		return len == 0 ? I2C_OP_READ : I2C_OP_READ_WITH_STOP;
	default:
		panic("%s: bad command %#x\n", __func__, cmd);
		return 0;
	}
}

static int
si70xx_cmd(i2c_tag_t tag, i2c_addr_t addr, uint8_t *cmd,
    uint8_t clen, uint8_t *buf, size_t blen)
{
	uint8_t dir;
	if (clen == 0)
		dir = blen == 0 ? I2C_OP_READ : I2C_OP_READ_WITH_STOP;
	else
		dir = si70xx_dir(cmd[0], blen);

	if (dir == I2C_OP_READ || dir == I2C_OP_READ_WITH_STOP)
		memset(buf, 0, blen);

	return iic_exec(tag, dir, addr, cmd, clen, buf, blen, 0);
}

static int
si70xx_cmd0(struct si70xx_sc *sc, uint8_t *buf, size_t blen)
{
	return si70xx_cmd(sc->sc_tag, sc->sc_addr, NULL, 0, buf, blen);
}

static int
si70xx_cmd1(struct si70xx_sc *sc, uint8_t cmd, uint8_t *buf, size_t blen)
{
	return si70xx_cmd(sc->sc_tag, sc->sc_addr, &cmd, 1, buf, blen);
}

static int
si70xx_cmd2(struct si70xx_sc *sc, uint8_t cmd1, uint8_t cmd2, uint8_t *buf,
    size_t blen)
{
	uint8_t cmd[] = { cmd1, cmd2 };
	return si70xx_cmd(sc->sc_tag, sc->sc_addr, cmd, __arraycount(cmd),
	    buf, blen);
}

static int
si70xx_set_heateron(struct si70xx_sc * sc)
{
	int error;
	uint8_t userregister;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s:%s: Failed to acquire bus: %d\n",
		    device_xname(sc->sc_dev), __func__, error));
		return error;
	}

	error = si70xx_cmd1(sc, SI70XX_READ_USER_REG_1, &userregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to read user register 1: %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	DPRINTF(sc, 2, ("%s:%s: reg 1 values before: %#x\n",
	    device_xname(sc->sc_dev), __func__, userregister));
	if (sc->sc_heateron) {
		userregister |= SI70XX_HTRE_MASK;
	} else {
		userregister &= ~SI70XX_HTRE_MASK;
	}
	DPRINTF(sc, 2, ("%s:%s: user reg 1 values after: %#x\n",
	    device_xname(sc->sc_dev), __func__, userregister));

	error = si70xx_cmd1(sc, SI70XX_WRITE_USER_REG_1, &userregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to write user register 1: %d\n",
		    device_xname(sc->sc_dev), error));
	}
out:
	iic_release_bus(sc->sc_tag, 0);
	return error;
}

static int
si70xx_set_resolution(struct si70xx_sc * sc, size_t index)
{
	int error;
	uint8_t userregister;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to acquire bus: %d\n",
		    device_xname(sc->sc_dev), error));
		return error;
	}

	error = si70xx_cmd1(sc, SI70XX_READ_USER_REG_1, &userregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to read user register 1: %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	DPRINTF(sc, 2, ("%s:%s: reg 1 values before: %#x\n",
	    device_xname(sc->sc_dev), __func__, userregister));
	userregister &= (~SI70XX_RESOLUTION_MASK);
	userregister |= si70xx_resolutions[index].num;
	DPRINTF(sc, 2, ("%s:%s: reg 1 values after: %#x\n",
	    device_xname(sc->sc_dev), __func__, userregister));

	error = si70xx_cmd1(sc, SI70XX_WRITE_USER_REG_1, &userregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to write user register 1: %d\n",
		    device_xname(sc->sc_dev), error));
	}
out:
	iic_release_bus(sc->sc_tag, 0);
	return error;
}

static int
si70xx_set_heatervalue(struct si70xx_sc * sc, size_t index)
{
	int error;
	uint8_t heaterregister;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to acquire bus: %d\n",
		    device_xname(sc->sc_dev), error));
		return error;
	}
	error = si70xx_cmd1(sc, SI70XX_READ_HEATER_REG, &heaterregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to read heater register: %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	DPRINTF(sc, 2, ("%s:%s: heater values before: %#x\n",
	    device_xname(sc->sc_dev), __func__, heaterregister));
	heaterregister &= ~SI70XX_HEATER_MASK;
	heaterregister |= si70xx_heatervalues[index];
	DPRINTF(sc, 2, ("%s:%s: heater values after: %#x\n",
	    device_xname(sc->sc_dev), __func__, heaterregister));

	error = si70xx_cmd1(sc, SI70XX_WRITE_HEATER_REG, &heaterregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to write heater register: %d\n",
		    device_xname(sc->sc_dev), error));
	}
out:
	iic_release_bus(sc->sc_tag, 0);
	return error;
}

static int
si70xx_update_heater(struct si70xx_sc *sc)
{
	size_t i;
	int error;
	uint8_t heaterregister;

	error = si70xx_cmd1(sc, SI70XX_READ_HEATER_REG, &heaterregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to read heater register: %d\n",
		    device_xname(sc->sc_dev), error));
		return error;
	}

	DPRINTF(sc, 2, ("%s: read heater reg values: %02x\n",
	    device_xname(sc->sc_dev), heaterregister));

	uint8_t heat = heaterregister & SI70XX_HEATER_MASK;
	for (i = 0; i < __arraycount(si70xx_heatervalues); i++) {
		if (si70xx_heatervalues[i] == heat)
			break;
	}
	sc->sc_heaterval = i != __arraycount(si70xx_heatervalues) ? i : 0;
	return 0;
}

static int
si70xx_update_user(struct si70xx_sc *sc)
{
	size_t i;
	int error;
	uint8_t userregister;

	error = si70xx_cmd1(sc, SI70XX_READ_USER_REG_1, &userregister, 1);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to read user register 1: %d\n",
		    device_xname(sc->sc_dev), error));
		return error;
	}
	DPRINTF(sc, 2, ("%s: read user reg 1 values: %#x\n",
	    device_xname(sc->sc_dev), userregister));

	uint8_t res = userregister & SI70XX_RESOLUTION_MASK;
	for (i = 0; i < __arraycount(si70xx_resolutions); i++) {
		if (si70xx_resolutions[i].num == res)
			break;
	}

	if (i != __arraycount(si70xx_resolutions)) {
		memcpy(sc->sc_resolution, si70xx_resolutions[i].text,
		    SI70XX_RES_NAME);
	} else {
		snprintf(sc->sc_resolution, SI70XX_RES_NAME, "%02x", res);
	}

	sc->sc_vddok = (userregister & SI70XX_VDDS_MASK) == 0;
	sc->sc_heaterval = userregister & SI70XX_HTRE_MASK;
	return 0;
}

static int
si70xx_update_status(struct si70xx_sc *sc)
{
	int error1 = si70xx_update_user(sc);
	int error2 = si70xx_update_heater(sc);
	return error1 ? error1 : error2;
}

static	uint8_t
si70xx_crc(uint8_t * data, size_t size)
{
	uint8_t crc = 0;

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
si70xx_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = SI70XX_READ_USER_REG_1;
	uint8_t buf;
	int error;

	error = si70xx_cmd(tag, addr, &reg, 1, &buf, 1);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
si70xx_sysctl_init(struct si70xx_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("si70xx controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef SI70XX_DEBUG
	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), si70xx_verify_sysctl, 0,
	    &sc->sc_si70xxdebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

#ifdef HAVE_I2C_EXECV
	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "clockstretch",
	    SYSCTL_DESCR("Clockstretch value"), si70xx_verify_sysctl, 0,
	    &sc->sc_clockstretch, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;
#endif


	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readattempts",
	    SYSCTL_DESCR("The number of times to attempt to read the values"),
	    si70xx_verify_sysctl, 0, &sc->sc_readattempts, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;


	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "resolutions",
	    SYSCTL_DESCR("Valid resolutions"), 0, 0,
	    __UNCONST(si70xx_resolution_names),
	    sizeof(si70xx_resolution_names) + 1,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "resolution",
	    SYSCTL_DESCR("Resolution of RH and Temp"),
	    si70xx_verify_sysctl_resolution, 0, (void *) sc,
	    SI70XX_RES_NAME, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ignorecrc",
	    SYSCTL_DESCR("Ignore the CRC byte"), NULL, 0, &sc->sc_ignorecrc,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_BOOL, "vddok",
	    SYSCTL_DESCR("Vdd at least 1.9v"), NULL, 0, &sc->sc_vddok, 0,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "heateron",
	    SYSCTL_DESCR("Heater on"), si70xx_verify_sysctl_heateron, 0,
	    (void *)sc, 0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	return sysctl_createv(&sc->sc_si70xxlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "heaterstrength",
	    SYSCTL_DESCR("Heater strength 1 to 6"),
	    si70xx_verify_sysctl_heatervalue, 0, (void *)sc, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL);
}

static int
si70xx_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia;
	int error;
	const bool matchdebug = false;

	ia = aux;

	if (ia->ia_name) {
		/* direct config - check name */
		if (strcmp(ia->ia_name, "si70xxtemp") != 0)
			return 0;
	} else {
		/* indirect config - check for configured address */
		if (ia->ia_addr != SI70XX_TYPICAL_ADDR)
			return 0;
	}

	/*
	 * Check to see if something is really at this i2c address. This will
	 * keep phantom devices from appearing
	 */
	if (iic_acquire_bus(ia->ia_tag, 0) != 0) {
		if (matchdebug)
			printf("in match acquire bus failed\n");
		return 0;
	}

	error = si70xx_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0;
}

static void
si70xx_attach(device_t parent, device_t self, void *aux)
{
	struct si70xx_sc *sc;
	struct i2c_attach_args *ia;
	int error, i;
	int ecount = 0;
	uint8_t buf[8];
	uint8_t testcrcpt1[4];
	uint8_t testcrcpt2[4];
	uint8_t crc1 = 0, crc2 = 0;
	uint8_t readcrc1 = 0, readcrc2 = 0;
	uint8_t fwversion, model;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_si70xxdebug = 0;
#ifdef HAVE_I2C_EXECV
	sc->sc_clockstretch = 2048;
#endif
	sc->sc_readattempts = 25;
	sc->sc_ignorecrc = false;
	sc->sc_sme = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_numsensors = __arraycount(si70xx_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}
	if ((error = si70xx_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}
	error = si70xx_cmd1(sc, SI70XX_RESET, NULL, 0);
	if (error != 0)
		aprint_error_dev(self, "Reset failed: %d\n", error);

	delay(15000);	/* 15 ms max */

	error = si70xx_cmd2(sc, SI70XX_READ_ID_PT1A, SI70XX_READ_ID_PT1B,
	    buf, 8);
	if (error) {
		aprint_error_dev(self, "Failed to read first part of ID: %d\n",
		    error);
		ecount++;
	}
	testcrcpt1[0] = buf[0];
	testcrcpt1[1] = buf[2];
	testcrcpt1[2] = buf[4];
	testcrcpt1[3] = buf[6];
	readcrc1 = buf[7];
	crc1 = si70xx_crc(testcrcpt1, 4);

	DPRINTF(sc, 2, ("%s: read 1 values: %02x%02x%02x%02x%02x%02x%02x%02x "
	    "- %02x\n", device_xname(sc->sc_dev), buf[0], buf[1],
	    buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
	    crc1));

	error = si70xx_cmd2(sc, SI70XX_READ_ID_PT2A, SI70XX_READ_ID_PT2B,
	    buf, 8);
	if (error != 0) {
		aprint_error_dev(self, "Failed to read second part of ID: %d\n",
		    error);
		ecount++;
	}
	model = testcrcpt2[0] = buf[0];
	testcrcpt2[1] = buf[1];
	testcrcpt2[2] = buf[3];
	testcrcpt2[3] = buf[4];
	readcrc2 = buf[5];
	crc2 = si70xx_crc(testcrcpt2, 4);

	DPRINTF(sc, 2, ("%s: read 2 values: %02x%02x%02x%02x%02x%02x - %02x\n",
	    device_xname(sc->sc_dev), buf[0], buf[1], buf[2],
	    buf[3], buf[4], buf[5], crc2));

	error = si70xx_cmd2(sc, SI70XX_READ_FW_VERA, SI70XX_READ_FW_VERB,
	    buf, 8);

	if (error) {
		aprint_error_dev(self, "Failed to read firware version: %d\n",
		    error);
		ecount++;
	}
	fwversion = buf[0];
	DPRINTF(sc, 2, ("%s: read fw values: %#x\n", device_xname(sc->sc_dev),
	    fwversion));

	error = si70xx_update_status(sc);
	iic_release_bus(sc->sc_tag, 0);
	if (error != 0) {
		aprint_error_dev(self, "Failed to update status: %x\n", error);
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	for (i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc, si70xx_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = si70xx_sensors[i].type;
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
	sc->sc_sme->sme_refresh = si70xx_refresh;

	DPRINTF(sc, 2, ("si70xx_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
			"unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}
	if (ecount != 0) {
		aprint_normal_dev(self, "Could not read model, "
		    "probably an HTU21D\n");
		return;
	}

	char modelstr[64];
	switch (model) {
	case 0:
	case 0xff:
		snprintf(modelstr, sizeof(modelstr), "Engineering Sample");
	case 13:
	case 20:
	case 21:
		snprintf(modelstr, sizeof(modelstr), "SI70%d", model);
		break;
	default:
		snprintf(modelstr, sizeof(modelstr), "Unknown SI70%d", model);
		break;
	}

	const char *fwversionstr;
	switch (fwversion) {
	case 0xff:
		fwversionstr = "1.0";
		break;
	case 0x20:
		fwversionstr = "2.0";
		break;
	default:
		fwversionstr = "unknown";
		break;
	}

	aprint_normal_dev(self, "Silicon Labs Model: %s, "
	    "Firmware version: %s, "
	    "Serial number: %02x%02x%02x%02x%02x%02x%02x%02x%s",
	    modelstr, fwversionstr, testcrcpt1[0], testcrcpt1[1],
	    testcrcpt1[2], testcrcpt1[3], testcrcpt2[0], testcrcpt2[1],
	    testcrcpt2[2], testcrcpt2[3],
	    (crc1 == readcrc1 && crc2 == readcrc2) ? "\n" : " (bad crc)\n");
	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static int
si70xx_exec(struct si70xx_sc *sc, uint8_t cmd, envsys_data_t *edata)
{
	int error;
	int xdelay;
	const char *name;
	int64_t mul, offs;
	uint8_t buf[3];

	switch (cmd) {
	case SI70XX_MEASURE_RH_NOHOLD:
		/*
		 * The published conversion for RH is: %RH =
		 * ((125 * RHCODE) / 65536) - 6
		 * 
		 * The sysmon infrastructure for RH wants %RH *
		 * 10^6 The result will fit in 32 bits, but
		 * the intermediate values will not.
		 */
		mul = 125000000;
		offs = -6000000;
		/*
		 * Conversion times for %RH in ms
		 * 
		 *        	Typical Max
		 * 12-bit	10.0	12.0
		 * 11-bit	 5.8	 7.0
		 * 10-bit	 3.7	 4.5
		 *  8-bit	 2.6	 3.1
		 * 
		 * A call to read %RH will also read temperature.  The
		 * conversion time will be the amount of time above
		 * plus the amount of time for temperature below
		 */
		xdelay = 10500;
		name = "RH";
		break;
	case SI70XX_MEASURE_TEMP_NOHOLD:
		/*
		 * The published conversion for temp is:
		 * degree C = ((175.72 * TEMPCODE) / 65536) -
		 * 46.85
		 * 
		 * The sysmon infrastructure for temp wants
		 * microkelvin.  This is simple, as degree C
		 * converts directly with K with simple
		 * addition. The result will fit in 32 bits,
		 * but the intermediate values will not.
		 */
		mul = 175720000;
		offs = 226300000;
		/*
		 * Conversion times for temperature in ms
	 	 * 
		 *		Typical	Max
		 * 14-bit	7.0	10.8
		 * 13-bit	4.0	 6.2
		 * 12-bit	2.4	 3.8
		 * 11-bit	1.5	 2.4
		 */
		xdelay = 4750;
		name = "TEMP";
		break;
	default:
		return EINVAL;
	}

#if HAVE_I2C_EXECV
	memset(buf, 0, sizeof(buf));
	error = iic_execv(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &cmd, 1, buf, sizeof(buf), 0, I2C_ATTR_CLOCKSTRETCH,
	    sc->sc_clockstretch, I2C_ATTR_EOL);
#else
	/*
	 * The lower level driver must support the ability to
	 * do a zero length read, otherwise this breaks
	 */
	error = si70xx_cmd1(sc, cmd, buf, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to read NO HOLD %s %d %d\n",
		    device_xname(sc->sc_dev), name, 1, error));
		return error;
	}

	/*
	 * It will probably be at least this long... we would
	 * not have to do this sort of thing if clock
	 * stretching worked.  Even this is a problem for the
	 * RPI without a patch to remove a [apparently] not
	 * needed KASSERT()
	 */
	delay(xdelay);

	for (int aint = 0; aint < sc->sc_readattempts; aint++) {
		error = si70xx_cmd0(sc, buf, sizeof(buf));
		if (error == 0)
			break;
		DPRINTF(sc, 2, ("%s: Failed to read NO HOLD RH"
		    " %d %d\n", device_xname(sc->sc_dev), 2, error));
		delay(1000);
	}
#endif

	DPRINTF(sc, 2, ("%s: %s values: %02x%02x%02x - %02x\n",
	    device_xname(sc->sc_dev), name, buf[0], buf[1], buf[2],
	    si70xx_crc(buf, 2)));

	uint8_t crc;
	if (sc->sc_ignorecrc) {
		crc = buf[2];
	} else {
		crc = si70xx_crc(buf, 2);
	}

	if (crc != buf[2]) {
		DPRINTF(sc, 2, ("%s: Bad CRC for %s: %#x and %#x\n",
		    device_xname(sc->sc_dev), name, crc, buf[2]));
		return EINVAL;
	}

	uint16_t val16 = (buf[0] << 8) | buf[1];
	uint64_t val64 = ((mul * val16) >> 16) + offs;
	DPRINTF(sc, 2, ("%s: %s calculated values: %x %#jx\n",
	    device_xname(sc->sc_dev), name, val16, (uintmax_t)val64));
	edata->value_cur = (uint32_t) val64;
	edata->state = ENVSYS_SVALID;
	return 0;
}

static void
si70xx_refresh(struct sysmon_envsys * sme, envsys_data_t * edata)
{
	struct si70xx_sc *sc;
	int 		error;

	sc = sme->sme_cookie;
	edata->state = ENVSYS_SINVALID;

	mutex_enter(&sc->sc_mutex);
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire i2c bus: %x\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}
	error = si70xx_update_status(sc);
	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to update status in refresh %d\n",
		    device_xname(sc->sc_dev), error));
		goto out1;
	}
	switch (edata->sensor) {
	case SI70XX_HUMIDITY_SENSOR:
		error = si70xx_exec(sc, SI70XX_MEASURE_RH_NOHOLD, edata);
		break;

	case SI70XX_TEMP_SENSOR:
		error = si70xx_exec(sc, SI70XX_MEASURE_TEMP_NOHOLD, edata);
		break;
	default:
		error = EINVAL;
		break;
	}

	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to get new status in refresh %d\n",
		    device_xname(sc->sc_dev), error));
	}
out1:
	iic_release_bus(sc->sc_tag, 0);
out:
	mutex_exit(&sc->sc_mutex);
}

static int
si70xx_detach(device_t self, int flags)
{
	struct si70xx_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);

	/* Remove the sensors */
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	mutex_exit(&sc->sc_mutex);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_si70xxlog);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, si70xxtemp, "i2cexec,sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
si70xxtemp_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_si70xxtemp,
		    cfattach_ioconf_si70xxtemp, cfdata_ioconf_si70xxtemp);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_si70xxtemp,
		      cfattach_ioconf_si70xxtemp, cfdata_ioconf_si70xxtemp);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
