/* $NetBSD: lm95221.c,v 1.1 2026/06/03 11:07:01 jdc Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lm95221.c,v 1.1 2026/06/03 11:07:01 jdc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/lm95221reg.h>

#define LM95221_LOCAL	0
#define LM95221_REMOTE1	1
#define LM95221_REMOTE2	2
#define LM95221_MISSING 0x01
#define LM95221_SIGNED	0x02

struct lm95221ts_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[LM95221_NSENSORS];
	int sc_map[LM95221_NSENSORS];		/* Sysmon numbers to sensors */
	uint8_t	sc_reg_m[LM95221_NSENSORS];	/* MSB register */
	uint8_t	sc_reg_l[LM95221_NSENSORS];	/* LSB register */
	uint8_t sc_flags[LM95221_NSENSORS];	/* Flags for remotes */
};

static int lm95221ts_match(device_t, cfdata_t, void *);
static void lm95221ts_attach(device_t, device_t, void *);
static int lm95221ts_detach(device_t, int);
void lm95221ts_refresh(struct sysmon_envsys *, envsys_data_t *);
static int lm95221ts_read_temp(struct lm95221ts_softc *, uint8_t, uint8_t,
    uint8_t, uint32_t *);

CFATTACH_DECL_NEW(lm95221ts, sizeof(struct lm95221ts_softc),
	lm95221ts_match, lm95221ts_attach, lm95221ts_detach, NULL);


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "i2c-lm95221" },
	DEVICE_COMPAT_EOL
};

static int
lm95221ts_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	/*
	 * Indirect config - assume config is correct!
	 */
	if (ia->ia_addr == LM95221_ADDR)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
lm95221ts_attach(device_t parent, device_t self, void *aux)
{
	struct lm95221ts_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t reg, val;
	int i, map;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;

	aprint_normal(": LM95221 temperature sensor\n");

	/* Read status, start conversion */
	for (i = 0; i < LM95221_NSENSORS; i++)
		sc->sc_flags[i] = 0;

	if (iic_acquire_bus(sc->sc_tag, 0)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to acquire i2c bus\n");
		return;
	}

	reg = LM95221_STATUS;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, &val, 1, 0)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to read status register\n");
		return;
	}
	if (val & LM95221_STAT_NO_REMOTE1)
		sc->sc_flags[LM95221_REMOTE1] |= LM95221_MISSING;
	if (val & LM95221_STAT_NO_REMOTE2)
		sc->sc_flags[LM95221_REMOTE2] |= LM95221_MISSING;

	reg = LM95221_CONFIG;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, &val, 1, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(sc->sc_dev,
		    "unable to read config register\n");
		return;
	}
	if (val & LM95221_CONF_REMOTE1)
		sc->sc_flags[LM95221_REMOTE1] |= LM95221_SIGNED;
	if (val & LM95221_CONF_REMOTE2)
		sc->sc_flags[LM95221_REMOTE2] |= LM95221_SIGNED;

	if (val & LM95221_CONF_STANDBY) {
		val &= ~(LM95221_CONF_STANDBY);
		if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_address, &reg, 1, &val, 1, 0)) {
			aprint_error_dev(sc->sc_dev,
			    "unable to write config register\n");
			iic_release_bus(sc->sc_tag, 0);
			return;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = lm95221ts_refresh;

	strlcpy(sc->sc_sensor[LM95221_LOCAL].desc, "local",
	    sizeof(sc->sc_sensor[LM95221_LOCAL].desc));
	sc->sc_reg_m[LM95221_LOCAL] = LM95221_LOCAL_MSB;
	sc->sc_reg_l[LM95221_LOCAL] = LM95221_LOCAL_LSB;

	strlcpy(sc->sc_sensor[LM95221_REMOTE1].desc, "remote 1",
	    sizeof(sc->sc_sensor[LM95221_REMOTE1].desc));
	sc->sc_reg_m[LM95221_REMOTE1] = LM95221_REMOTE1_MSB;
	sc->sc_reg_l[LM95221_REMOTE1] = LM95221_REMOTE1_LSB;

	strlcpy(sc->sc_sensor[LM95221_REMOTE2].desc, "remote 2",
	    sizeof(sc->sc_sensor[LM95221_REMOTE2].desc));
	sc->sc_reg_m[LM95221_REMOTE2] = LM95221_REMOTE2_MSB;
	sc->sc_reg_l[LM95221_REMOTE2] = LM95221_REMOTE2_LSB;

	for (i = 0; i < LM95221_NSENSORS; i++) {
		if (i == LM95221_REMOTE1 &&
		    (sc->sc_flags[LM95221_REMOTE1] & LM95221_MISSING))
			continue;
		if (i == LM95221_REMOTE2 &&
		    (sc->sc_flags[LM95221_REMOTE2] & LM95221_MISSING))
			continue;

		sc->sc_sensor[i].units = ENVSYS_STEMP;
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
		sc->sc_sensor[i].flags = ENVSYS_FHAS_ENTROPY;

		if (sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			sc->sc_sme = NULL;
			aprint_error_dev(sc->sc_dev,
			    "unable to attach sensor %d to sysmon\n", i);
			return;
		}
		map = sc->sc_sensor[i].sensor;
		sc->sc_map[map] = i;
	}

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}
}

static int
lm95221ts_detach(device_t self, int flags)
{
	struct lm95221ts_softc *sc = device_private(self);

	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}

	return 0;
}

void
lm95221ts_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct lm95221ts_softc *sc = sme->sme_cookie;
	uint32_t val;
	uint8_t reg_m, reg_l, flags;
	int map;

	map = sc->sc_map[edata->sensor];
	reg_m = sc->sc_reg_m[map];
	reg_l = sc->sc_reg_l[map];
	flags = sc->sc_flags[map];

	if (iic_acquire_bus(sc->sc_tag, 0))
		return;

	if (lm95221ts_read_temp(sc, reg_m, reg_l, flags, &val) == 0) {
		edata->value_cur = val;
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}

	iic_release_bus(sc->sc_tag, 0);
}

static int
lm95221ts_read_temp(struct lm95221ts_softc *sc, uint8_t reg_m, uint8_t reg_l,
    uint8_t flags, uint32_t *valp)
{
	uint8_t buf_m, buf_l;
	int err;

	err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg_m, 1, &buf_m, 1, 0);
	if (err)
		return err;
	err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg_l, 1, &buf_l, 1, 0);
	if (err)
		return err;

	/*
	 * Convert the 11 high bits from the 2 bytes to a temperature in uK.
	 * Maybe sign-extend the MSB, and add in the LSB
	 */
	if (flags & LM95221_SIGNED)
		*valp = buf_m;
	else
		*valp = (int8_t) buf_m;
	*valp = (*valp << 3) + ((buf_l >> 5) & 0x07);
	*valp = *valp * 125000 + 273150000;

	return 0;
}
