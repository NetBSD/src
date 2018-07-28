/*	$NetBSD: lm75.c,v 1.30.2.2 2018/07/28 04:37:44 pgoyette Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lm75.c,v 1.30.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/lm75reg.h>

#ifdef macppc
#define HAVE_OF 1
#endif

#ifdef HAVE_OF
#include <dev/ofw/openfirm.h>
#endif

struct lmtemp_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;
	int sc_tmax;
	uint32_t sc_smax, sc_smin, sc_scrit;

	uint32_t (*sc_lmtemp_decode)(const uint8_t *, int);
	void (*sc_lmtemp_encode)(const uint32_t, uint8_t *, int);
};

static int  lmtemp_match(device_t, cfdata_t, void *);
static void lmtemp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lmtemp, sizeof(struct lmtemp_softc),
	lmtemp_match, lmtemp_attach, NULL, NULL);

static void	lmtemp_refresh(struct sysmon_envsys *, envsys_data_t *);
static int	lmtemp_config_write(struct lmtemp_softc *, uint8_t);
static int	lmtemp_temp_write(struct lmtemp_softc *, uint8_t, uint32_t,
				int);
static int	lmtemp_temp_read(struct lmtemp_softc *, uint8_t, uint32_t *,
				int);
static uint32_t lmtemp_decode_lm75(const uint8_t *, int);
static uint32_t lmtemp_decode_ds75(const uint8_t *, int);
static uint32_t lmtemp_decode_lm77(const uint8_t *, int);
static void	lmtemp_encode_lm75(const uint32_t, uint8_t *, int);
static void	lmtemp_encode_ds75(const uint32_t, uint8_t *, int);
static void	lmtemp_encode_lm77(const uint32_t, uint8_t *, int);
static void	lmtemp_getlim_lm75(struct sysmon_envsys *, envsys_data_t *,
				sysmon_envsys_lim_t *, uint32_t *);
static void	lmtemp_getlim_lm77(struct sysmon_envsys *, envsys_data_t *,
				sysmon_envsys_lim_t *, uint32_t *);
static void	lmtemp_setlim_lm75(struct sysmon_envsys *, envsys_data_t *,
				sysmon_envsys_lim_t *, uint32_t *);
static void	lmtemp_setlim_lm77(struct sysmon_envsys *, envsys_data_t *,
				sysmon_envsys_lim_t *, uint32_t *);

static void	lmtemp_setup_sysctl(struct lmtemp_softc *);
static int	sysctl_lm75_temp(SYSCTLFN_ARGS);

static const struct device_compatible_entry compat_data[] = {
	{ "i2c-lm75",			0 },
	{ "ds1775",			0 },
	/*
	 * see XXX in _attach() below: add code once non-lm75 matches are
	 * added here!
	 */
	{ NULL,				0 }
};

enum {
	lmtemp_lm75 = 0,
	lmtemp_ds75,
	lmtemp_lm77,
};
static const struct {
	int lmtemp_type;
	const char *lmtemp_name;
	int lmtemp_addrmask;
	int lmtemp_addr;
	uint32_t (*lmtemp_decode)(const uint8_t *, int);
	void (*lmtemp_encode)(const uint32_t, uint8_t *, int);
	void (*lmtemp_getlim)(struct sysmon_envsys *, envsys_data_t *,
		sysmon_envsys_lim_t *, uint32_t *);
	void (*lmtemp_setlim)(struct sysmon_envsys *, envsys_data_t *,
		sysmon_envsys_lim_t *, uint32_t *);
} lmtemptbl[] = {
	{ lmtemp_lm75,	"LM75",	LM75_ADDRMASK,	LM75_ADDR,
	    lmtemp_decode_lm75,	lmtemp_encode_lm75,
	    lmtemp_getlim_lm75,	lmtemp_setlim_lm75 },
	{ lmtemp_ds75,	"DS75",	LM75_ADDRMASK,	LM75_ADDR,
	    lmtemp_decode_ds75,	lmtemp_encode_ds75,
	    lmtemp_getlim_lm75,	lmtemp_setlim_lm75 },
	{ lmtemp_lm77,	"LM77",	LM77_ADDRMASK,	LM77_ADDR,
	    lmtemp_decode_lm77, lmtemp_encode_lm77,
	    lmtemp_getlim_lm77,	lmtemp_setlim_lm77 },
	{ -1,		NULL,	 0,		0,
	    NULL,		NULL,
	    NULL,		NULL }
};

static int
lmtemp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int i, match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	/*
	 * Indirect config - not much we can do!
	 */
	for (i = 0; lmtemptbl[i].lmtemp_type != -1 ; i++)
		if (lmtemptbl[i].lmtemp_type == cf->cf_flags)
			break;
	if (lmtemptbl[i].lmtemp_type == -1)
		return 0;

	if ((ia->ia_addr & lmtemptbl[i].lmtemp_addrmask) ==
	    lmtemptbl[i].lmtemp_addr)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
lmtemp_attach(device_t parent, device_t self, void *aux)
{
	struct lmtemp_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	char name[64];
	int i;

	sc->sc_dev = self;
	if (ia->ia_name == NULL) {
		for (i = 0; lmtemptbl[i].lmtemp_type != -1 ; i++)
			if (lmtemptbl[i].lmtemp_type ==
			    device_cfdata(self)->cf_flags)
				break;
	} else {
		if (strcmp(ia->ia_name, "ds1775") == 0) {
			i = 1;	/* LMTYPE_DS75 */
		} else {
			/* XXX - add code when adding other direct matches! */
			i = 0;
		}
	}

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;

	aprint_naive(": Temperature Sensor\n");
	if (ia->ia_name) {
		aprint_normal(": %s %s Temperature Sensor\n", ia->ia_name,
			lmtemptbl[i].lmtemp_name);
	} else {
		aprint_normal(": %s Temperature Sensor\n",
			lmtemptbl[i].lmtemp_name);
	}

	sc->sc_lmtemp_decode = lmtemptbl[i].lmtemp_decode;
	sc->sc_lmtemp_encode = lmtemptbl[i].lmtemp_encode;

	iic_acquire_bus(sc->sc_tag, I2C_F_POLL);

	/* Read temperature limit(s) and remember initial value(s). */
	if (i == lmtemp_lm77) {
		if (lmtemp_temp_read(sc, LM77_REG_TCRIT_SET_POINT,
		    &sc->sc_scrit, 1) != 0) {
			aprint_error_dev(self,
			    "unable to read low register\n");
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			return;
		}
		if (lmtemp_temp_read(sc, LM77_REG_TLOW_SET_POINT,
		    &sc->sc_smin, 1) != 0) {
			aprint_error_dev(self,
			    "unable to read low register\n");
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			return;
		}
		if (lmtemp_temp_read(sc, LM77_REG_THIGH_SET_POINT,
		    &sc->sc_smax, 1) != 0) {
			aprint_error_dev(self,
			    "unable to read high register\n");
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			return;
		}
	} else {	/* LM75 or compatible */
		if (lmtemp_temp_read(sc, LM75_REG_TOS_SET_POINT,
		    &sc->sc_smax, 1) != 0) {
			aprint_error_dev(self, "unable to read Tos register\n");
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			return;
		}
	}
	sc->sc_tmax = sc->sc_smax;

	if (i == lmtemp_lm75)
		lmtemp_setup_sysctl(sc);

	/* Set the configuration of the LM75 to defaults. */
	if (lmtemp_config_write(sc, LM75_CONFIG_FAULT_QUEUE_4) != 0) {
		aprint_error_dev(self, "unable to write config register\n");
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		return;
	}
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	sc->sc_sme = sysmon_envsys_create();
	/* Initialize sensor data. */
	sc->sc_sensor.units =  ENVSYS_STEMP;
	sc->sc_sensor.state =  ENVSYS_SINVALID;
	sc->sc_sensor.flags =  ENVSYS_FMONLIMITS;

	(void)strlcpy(name,
	    ia->ia_name? ia->ia_name : device_xname(self),
	    sizeof(sc->sc_sensor.desc));
#ifdef HAVE_OF
	int ch;
	ch = OF_child(ia->ia_cookie);
	if (ch != 0) {
		OF_getprop(ch, "location", name, 64);
	}
#endif
	(void)strlcpy(sc->sc_sensor.desc, name,
	    sizeof(sc->sc_sensor.desc));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Hook into system monitor. */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = lmtemp_refresh;
	sc->sc_sme->sme_get_limits = lmtemptbl[i].lmtemp_getlim;
	sc->sc_sme->sme_set_limits = lmtemptbl[i].lmtemp_setlim;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static int
lmtemp_config_write(struct lmtemp_softc *sc, uint8_t val)
{
	uint8_t cmdbuf[2];

	cmdbuf[0] = LM75_REG_CONFIG;
	cmdbuf[1] = val;

	return iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_address, cmdbuf, 1, &cmdbuf[1], 1, I2C_F_POLL);
}

static int
lmtemp_temp_write(struct lmtemp_softc *sc, uint8_t reg, uint32_t val, int degc)
{
	uint8_t cmdbuf[3];

	cmdbuf[0] = reg;
	sc->sc_lmtemp_encode(val, &cmdbuf[1], degc);

	return iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_address, cmdbuf, 1, &cmdbuf[1], 2, I2C_F_POLL);
}

static int
lmtemp_temp_read(struct lmtemp_softc *sc, uint8_t which, uint32_t *valp,
    int degc)
{
	int error;
	uint8_t cmdbuf[1];
	uint8_t buf[LM75_TEMP_LEN];

	cmdbuf[0] = which;

	error = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, cmdbuf, 1, buf, LM75_TEMP_LEN, 0);
	if (error)
		return error;

	*valp = sc->sc_lmtemp_decode(buf, degc);
	return 0;
}

static void
lmtemp_refresh_sensor_data(struct lmtemp_softc *sc)
{
	uint32_t val;
	int error;

	error = lmtemp_temp_read(sc, LM75_REG_TEMP, &val, 0);
	if (error) {
#if 0
		aprint_error_dev(sc->sc_dev, "unable to read temperature, error = %d\n",
		    error);
#endif
		sc->sc_sensor.state = ENVSYS_SINVALID;
		return;
	}

	sc->sc_sensor.value_cur = val;
	sc->sc_sensor.state = ENVSYS_SVALID;
}

static void
lmtemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct lmtemp_softc *sc = sme->sme_cookie;

	iic_acquire_bus(sc->sc_tag, 0);	/* also locks our instance */
	lmtemp_refresh_sensor_data(sc);
	iic_release_bus(sc->sc_tag, 0);	/* also unlocks our instance */
}

static void
lmtemp_getlim_lm75(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct lmtemp_softc *sc = sme->sme_cookie;
	uint32_t val;

	*props &= ~(PROP_CRITMAX);

	iic_acquire_bus(sc->sc_tag, 0);
	if (lmtemp_temp_read(sc, LM75_REG_TOS_SET_POINT, &val, 0) == 0) {
		limits->sel_critmax = val;
		*props |= PROP_CRITMAX;
	}
	iic_release_bus(sc->sc_tag, 0);
}

static void
lmtemp_getlim_lm77(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct lmtemp_softc *sc = sme->sme_cookie;
	uint32_t val;

	*props &= ~(PROP_CRITMAX | PROP_WARNMAX | PROP_WARNMIN);

	iic_acquire_bus(sc->sc_tag, 0);
	if (lmtemp_temp_read(sc, LM77_REG_TCRIT_SET_POINT, &val, 0) == 0) {
		limits->sel_critmax = val;
		*props |= PROP_CRITMAX;
	}
	if (lmtemp_temp_read(sc, LM77_REG_THIGH_SET_POINT, &val, 0) == 0) {
		limits->sel_warnmax = val;
		*props |= PROP_WARNMAX;
	}
	if (lmtemp_temp_read(sc, LM77_REG_TLOW_SET_POINT, &val, 0) == 0) {
		limits->sel_warnmin = val;
		*props |= PROP_WARNMIN;
	}
	iic_release_bus(sc->sc_tag, 0);
}

static void
lmtemp_setlim_lm75(struct sysmon_envsys *sme, envsys_data_t *edata, 
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct lmtemp_softc *sc = sme->sme_cookie;
	int32_t limit;

	if (*props & PROP_CRITMAX) {
		if (limits == NULL)	/* Restore defaults */
			limit = sc->sc_smax;
		else
			limit = limits->sel_critmax;
		iic_acquire_bus(sc->sc_tag, 0);
		lmtemp_temp_write(sc, LM75_REG_THYST_SET_POINT,
		    limit - 5000000, 0);
		lmtemp_temp_write(sc, LM75_REG_TOS_SET_POINT, limit, 0);
		iic_release_bus(sc->sc_tag, 0);

		/* Synchronise sysctl */
		sc->sc_tmax = (limit - 273150000) / 1000000;
	}
}

static void
lmtemp_setlim_lm77(struct sysmon_envsys *sme, envsys_data_t *edata, 
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct lmtemp_softc *sc = sme->sme_cookie;
	int32_t limit;

	iic_acquire_bus(sc->sc_tag, 0);
	if (*props & PROP_CRITMAX) {
		if (limits == NULL)	/* Restore defaults */
			limit = sc->sc_scrit;
		else
			limit = limits->sel_critmax;
		lmtemp_temp_write(sc, LM77_REG_TCRIT_SET_POINT, limit, 0);
	}
	if (*props & PROP_WARNMAX) {
		if (limits == NULL)	/* Restore defaults */
			limit = sc->sc_smax;
		else
			limit = limits->sel_warnmax;
		lmtemp_temp_write(sc, LM77_REG_THIGH_SET_POINT, limit, 0);
	}
	if (*props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			limit = sc->sc_smin;
		else
			limit = limits->sel_warnmin;
		lmtemp_temp_write(sc, LM77_REG_TLOW_SET_POINT, limit, 0);
	}
	iic_release_bus(sc->sc_tag, 0);
}

static uint32_t
lmtemp_decode_lm75(const uint8_t *buf, int degc)
{
	int temp;
	uint32_t val;

	/*
	 * LM75 temps are the most-significant 9 bits of a 16-bit reg.
	 * sign-extend the MSB and add in the 0.5 from the LSB
	 */
	temp = (int8_t) buf[0];
	temp = (temp << 1) + ((buf[1] >> 7) & 0x1);

	/* Temp is given in 1/2 deg. C, we convert to C or uK. */
	if (degc)
		val = temp / 2;
	else
		val = temp * 500000 + 273150000;

	return val;
}

static uint32_t
lmtemp_decode_ds75(const uint8_t *buf, int degc)
{
	int temp;

	/*
	 * Sign-extend the MSB byte, and add in the fractions of a
	 * degree contained in the LSB (precision 1/16th DegC).
	 */
	temp = (int8_t)buf[0];
	temp = (temp << 4) | ((buf[1] >> 4) & 0xf);

	/*
	 * Conversion to C or uK is simple.
	 */
	if (degc)
		return temp / 16;
	else
		return (temp * 62500 + 273150000);
}

static uint32_t
lmtemp_decode_lm77(const uint8_t *buf, int degc)
{
	int temp;
	uint32_t val;

	/*
	 * Describe each bits of temperature registers on LM77.
	 *   D15 - D12:	Sign
	 *   D11 - D3 :	Bit8(MSB) - Bit0
	 */
	temp = (int8_t)buf[0];
	temp = (temp << 5) | ((buf[1] >> 3) & 0x1f);

	/* Temp is given in 1/2 deg. C, we convert to C or uK. */
	if (degc)
		val = temp / 2;
	else
		val = temp * 500000 + 273150000;

	return val;
}

static void lmtemp_encode_lm75(const uint32_t val, uint8_t *buf, int degc)
{
	int temp;

	/* Convert from C or uK to register format */
	if (degc)
		temp = val * 2;
	else
		temp = (val - 273150000) / 500000;
	buf[0] = (temp >> 1) & 0xff;
	buf[1] = (temp & 1) << 7;
}

static void lmtemp_encode_ds75(const uint32_t val, uint8_t *buf, int degc)
{
	int temp;

	/* Convert from C or uK to register format */
	if (degc)
		temp = val * 16;
	else
		temp = (val - 273150000) / 62500;
	buf[0] = (temp >> 4) & 0xff;
	buf[1] = (temp & 0xf) << 4;
}

static void lmtemp_encode_lm77(const uint32_t val, uint8_t *buf, int degc)
{
	int temp;

	/* Convert from C or uK to register format */
	if (degc)
		temp = val * 2;
	else
		temp = (val - 273150000) / 500000;
	buf[0] = (temp >> 5) & 0xff;
	buf[1] = (temp & 0x1f) << 3;
}

static void
lmtemp_setup_sysctl(struct lmtemp_softc *sc)
{
	const struct sysctlnode *me = NULL, *node = NULL;

	sysctl_createv(NULL, 0, NULL, &me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "temp", "Threshold temperature",
	    sysctl_lm75_temp, 1, (void *)sc, 0,
	    CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
}

static int
sysctl_lm75_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct lmtemp_softc *sc = node.sysctl_data;
	int temp;

	if (newp) {

		/* we're asked to write */	
		node.sysctl_data = &sc->sc_tmax;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			temp = *(int *)node.sysctl_data;
			sc->sc_tmax = temp;
			iic_acquire_bus(sc->sc_tag, I2C_F_POLL);
			lmtemp_temp_write(sc, LM75_REG_THYST_SET_POINT,
			    sc->sc_tmax - 5, 1);
			lmtemp_temp_write(sc, LM75_REG_TOS_SET_POINT,
			    sc->sc_tmax, 1);
			iic_release_bus(sc->sc_tag, I2C_F_POLL);

			/* Synchronise envsys - calls lmtemp_getlim_lm75() */
			sysmon_envsys_update_limits(sc->sc_sme, &sc->sc_sensor);
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_data = &sc->sc_tmax;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

SYSCTL_SETUP(sysctl_lmtemp_setup, "sysctl lmtemp subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}


