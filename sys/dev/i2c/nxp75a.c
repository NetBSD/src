/* $NetBSD: nxp75a.c,v 1.1 2026/06/03 11:08:41 jdc Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: nxp75a.c,v 1.1 2026/06/03 11:08:41 jdc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/nxp75areg.h>

struct nxp75a_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;
	uint32_t sc_otemp, sc_hyst;
};

static int  nxp75a_match(device_t, cfdata_t, void *);
static void nxp75a_attach(device_t, device_t, void *);
static int nxp75a_detach(device_t, int);
void	nxp75a_refresh(struct sysmon_envsys *, envsys_data_t *);
void    nxp75a_get_limits(struct sysmon_envsys *, envsys_data_t *,
		    sysmon_envsys_lim_t *, uint32_t *);
void    nxp75a_set_limits(struct sysmon_envsys *, envsys_data_t *,
		    sysmon_envsys_lim_t *, uint32_t *);
static int nxp75a_read_temp(struct nxp75a_softc *, uint8_t, uint32_t *);
static int nxp75a_write_temp(struct nxp75a_softc *, uint8_t, uint32_t);

CFATTACH_DECL_NEW(nxp75a, sizeof(struct nxp75a_softc),
	nxp75a_match, nxp75a_attach, nxp75a_detach, NULL);


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "i2c-lm75a" },
	DEVICE_COMPAT_EOL
};

static int
nxp75a_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	/*
	 * Indirect config - assume config is correct!
	 */
	if ((ia->ia_addr & NXP75A_ADDRMASK) == NXP75A_ADDR)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
nxp75a_attach(device_t parent, device_t self, void *aux)
{
	struct nxp75a_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t reg, val;
	uint32_t tval;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;

	aprint_normal(": NXP LM75A temperature sensor\n");

	if (iic_acquire_bus(sc->sc_tag, 0)) {
		aprint_error_dev(self,
		    "unable to acquire I2C bus\n");
		return;
	}

	/* Read and save overtemp (critical limit) and hysteresis */
	reg = NXP75A_OVERTEMP;
	if (nxp75a_read_temp(sc, reg, &tval)) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(sc->sc_dev,
		    "unable to read overtemp register\n");
		return;
	}
	sc->sc_otemp = tval;
	reg = NXP75A_THYST;
	if (nxp75a_read_temp(sc, reg, &tval)) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(sc->sc_dev,
		    "unable to read hysteresis register\n");
		return;
	}
	sc->sc_hyst = tval;

	/* Read config, start conversion */
	reg = NXP75A_CONFIG;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, &val, 1, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(sc->sc_dev,
		    "unable to read config register\n");
		return;
	}

	if (val & NXP75A_CONF_SHUTDOWN) {
		val &= ~(NXP75A_CONF_SHUTDOWN);
		if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_address, &reg, 1, &val, 1, 0)) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(sc->sc_dev,
			    "unable to write config register\n");
			return;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = nxp75a_refresh;
	sc->sc_sme->sme_get_limits = nxp75a_get_limits;
	sc->sc_sme->sme_set_limits = nxp75a_set_limits;

	strlcpy(sc->sc_sensor.desc, "temperature", sizeof(sc->sc_sensor.desc));
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.flags = ENVSYS_FMONLIMITS | ENVSYS_FHAS_ENTROPY;

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		aprint_error_dev(sc->sc_dev,
		    "unable to attach sensor to sysmon\n");
		return;
	}

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}
}

static int
nxp75a_detach(device_t self, int flags)
{
	struct nxp75a_softc *sc = device_private(self);

	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}

	return 0;
}

void
nxp75a_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct nxp75a_softc *sc = sme->sme_cookie;
	uint8_t reg;
	uint32_t val;

	if (iic_acquire_bus(sc->sc_tag, 0))
		return;

	reg = NXP75A_TEMP;
	if (nxp75a_read_temp(sc, reg, &val) == 0) {
		edata->value_cur = val;
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}

	iic_release_bus(sc->sc_tag, 0);
}

void
nxp75a_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct nxp75a_softc *sc = sme->sme_cookie;
	uint8_t reg;
	uint32_t val;

	*props &= ~(PROP_CRITMAX | PROP_WARNMAX | PROP_WARNMIN);

	if (iic_acquire_bus(sc->sc_tag, 0))
		return;

	reg = NXP75A_OVERTEMP;
	if (nxp75a_read_temp(sc, reg, &val) == 0) {
		limits->sel_critmax = val;
		*props |= PROP_CRITMAX;
	}

	iic_release_bus(sc->sc_tag, 0);
}

void
nxp75a_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct nxp75a_softc *sc = sme->sme_cookie;
	uint8_t reg;
	uint32_t otemp, hyst;

	if (iic_acquire_bus(sc->sc_tag, 0))
		return;

	if (limits == NULL || *props & PROP_CRITMAX) {
		if (limits == NULL) {	/* Restore defaults */
			otemp = sc->sc_otemp;
			hyst = sc->sc_hyst;
		} else {
			otemp = limits->sel_critmax;
			/* Make sure that hyst is less than overtemp */
			hyst = otemp - (sc->sc_otemp - sc->sc_hyst);
		}
		reg = NXP75A_OVERTEMP;
		if (nxp75a_write_temp(sc, reg, otemp) == 0) {
			reg = NXP75A_THYST;
			nxp75a_write_temp(sc, reg, hyst);
		}
	}
			
	iic_release_bus(sc->sc_tag, 0);
}

static int
nxp75a_read_temp(struct nxp75a_softc *sc, uint8_t reg, uint32_t *valp)
{
	uint8_t buf[NXP75A_TEMP_LEN];
	int err;

	err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, buf, NXP75A_TEMP_LEN, 0);
	if (err)
		return err;

	/*
	 * Convert the 11 high bits from the 2 bytes to a temperature in uK.
	 * Sign-extend the MSB and add in the LSB
	 */
	*valp = (int8_t) buf[0];
	*valp = (*valp << 3) + ((buf[1] >> 5) & 0x07);
	*valp = *valp * 125000 + 273150000;

	return 0;
}

static int
nxp75a_write_temp(struct nxp75a_softc *sc, uint8_t reg, uint32_t val)
{
	uint8_t buf[NXP75A_TEMP_LEN];
	uint32_t temp;

	/* Convert the temperature in uK to 11 high bits in 2 bytes.*/
	temp = (int) (val - 273150000) / 125000 ;
	buf[0] = (temp >> 3) & 0xff;
	buf[1] = (temp << 5) & 0xe0;

	return iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_address, &reg, 1, buf, NXP75A_TEMP_LEN, 0);
}

