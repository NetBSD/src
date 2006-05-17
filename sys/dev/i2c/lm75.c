/*	$NetBSD: lm75.c,v 1.9 2006/05/17 00:10:54 kiyohara Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/lm75reg.h>

struct lmtemp_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;

	struct envsys_tre_data sc_sensor[1];
	struct envsys_basic_info sc_info[1];

	struct sysmon_envsys sc_sysmon;

	uint32_t (*sc_lmtemp_decode)(const uint8_t *);
};

static int  lmtemp_match(struct device *, struct cfdata *, void *);
static void lmtemp_attach(struct device *, struct device *, void *);

CFATTACH_DECL(lmtemp, sizeof(struct lmtemp_softc),
	lmtemp_match, lmtemp_attach, NULL, NULL);

static int	lmtemp_gtredata(struct sysmon_envsys *,
				struct envsys_tre_data *);
static int	lmtemp_streinfo(struct sysmon_envsys *,
				struct envsys_basic_info *);

static const struct envsys_range lmtemp_ranges[] = {
	{ 0, 1,		ENVSYS_STEMP },
	{ 1, 0,		-1 },
};

static int lmtemp_config_write(struct lmtemp_softc *, uint8_t);
static uint32_t lmtemp_decode_lm75(const uint8_t *);
static uint32_t lmtemp_decode_ds75(const uint8_t *);
static uint32_t lmtemp_decode_lm77(const uint8_t *);

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
	uint32_t (*lmtemp_decode)(const uint8_t *);
} lmtemptbl[] = {
	{ lmtemp_lm75,	"LM75",
	    LM75_ADDRMASK,	LM75_ADDR,	lmtemp_decode_lm75 },
	{ lmtemp_ds75,	"DS75",
	    LM75_ADDRMASK,	LM75_ADDR,	lmtemp_decode_ds75 },
	{ lmtemp_lm77,	"LM77",
	    LM77_ADDRMASK,	LM77_ADDR,	lmtemp_decode_lm77 },

	{ -1,		NULL,
	    0,			0,		NULL }
};

static int
lmtemp_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int i;

	for (i = 0; lmtemptbl[i].lmtemp_type != -1 ; i++)
		if (lmtemptbl[i].lmtemp_type == cf->cf_flags)
			break;
	if (lmtemptbl[i].lmtemp_type == -1)
		return (0);

	if ((ia->ia_addr & lmtemptbl[i].lmtemp_addrmask) ==
	    lmtemptbl[i].lmtemp_addr)
		return (1);

	return (0);
}

static void
lmtemp_attach(struct device *parent, struct device *self, void *aux)
{
	struct lmtemp_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	prop_string_t desc;
	int i;

	for (i = 0; lmtemptbl[i].lmtemp_type != -1 ; i++)
		if (lmtemptbl[i].lmtemp_type ==
		    device_cfdata(&sc->sc_dev)->cf_flags)
			break;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;

	aprint_naive(": Temperature Sensor\n");
	aprint_normal(": %s Temperature Sensor\n", lmtemptbl[i].lmtemp_name);

	/* Set the configuration of the LM75 to defaults. */
	iic_acquire_bus(sc->sc_tag, I2C_F_POLL);
	if (lmtemp_config_write(sc, 0) != 0) {
		aprint_error("%s: unable to write config register\n",
		    sc->sc_dev.dv_xname);
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		return;
	}
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	/* Initialize sensor data. */
	sc->sc_sensor[0].sensor = sc->sc_info[0].sensor = 0;
	sc->sc_sensor[0].validflags = ENVSYS_FVALID;
	sc->sc_info[0].validflags = ENVSYS_FVALID;
	sc->sc_sensor[0].warnflags = ENVSYS_WARN_OK;

	sc->sc_sensor[0].units = sc->sc_info[0].units = ENVSYS_STEMP;
	desc = prop_dictionary_get(device_properties(&sc->sc_dev),
				   "description");
	if (desc != NULL &&
	    prop_object_type(desc) == PROP_TYPE_STRING &&
	    prop_string_size(desc) > 0)
	    	strcpy(sc->sc_info[0].desc, prop_string_cstring_nocopy(desc));
	else
		strcpy(sc->sc_info[0].desc, sc->sc_dev.dv_xname);

	sc->sc_lmtemp_decode = lmtemptbl[i].lmtemp_decode;

	/* Hook into system monitor. */
	sc->sc_sysmon.sme_ranges = lmtemp_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_sensor;
	sc->sc_sysmon.sme_cookie = sc;

	sc->sc_sysmon.sme_gtredata = lmtemp_gtredata;
	sc->sc_sysmon.sme_streinfo = lmtemp_streinfo;

	sc->sc_sysmon.sme_nsensors = 1;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

static int
lmtemp_config_write(struct lmtemp_softc *sc, uint8_t val)
{
	uint8_t cmdbuf[2];

	cmdbuf[0] = LM75_REG_CONFIG;
	cmdbuf[1] = val;

	return (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_address, cmdbuf, 1, &cmdbuf[1], 1, I2C_F_POLL));
}

static int
lmtemp_temp_read(struct lmtemp_softc *sc, uint8_t which, uint32_t *valp)
{
	int error;
	uint8_t cmdbuf[1];
	uint8_t buf[LM75_TEMP_LEN];

	cmdbuf[0] = which;

	error = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, cmdbuf, 1, buf, LM75_TEMP_LEN, 0);
	if (error)
		return (error);

	*valp = sc->sc_lmtemp_decode(buf);
	return (0);
}

static void
lmtemp_refresh_sensor_data(struct lmtemp_softc *sc)
{
	uint32_t val;
	int error;

	error = lmtemp_temp_read(sc, LM75_REG_TEMP, &val);
	if (error) {
#if 0
		printf("%s: unable to read temperature, error = %d\n",
		    sc->sc_dev.dv_xname, error);
#endif
		sc->sc_sensor[0].validflags &= ~ENVSYS_FCURVALID;
		return;
	}

	sc->sc_sensor[0].cur.data_us = val;
	sc->sc_sensor[0].validflags |= ENVSYS_FCURVALID;
}

static int
lmtemp_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct lmtemp_softc *sc = sme->sme_cookie;

	iic_acquire_bus(sc->sc_tag, 0);	/* also locks our instance */

	lmtemp_refresh_sensor_data(sc);
	*tred = sc->sc_sensor[tred->sensor];

	iic_release_bus(sc->sc_tag, 0);	/* also unlocks our instance */

	return (0);
}

static int
lmtemp_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *binfo)
{
	struct lmtemp_softc *sc = sme->sme_cookie;

	iic_acquire_bus(sc->sc_tag, 0);	/* also locks our instance */

	memcpy(sc->sc_info[binfo->sensor].desc, binfo->desc,
	    sizeof(sc->sc_info[binfo->sensor].desc));
	sc->sc_info[binfo->sensor].desc[
	    sizeof(sc->sc_info[binfo->sensor].desc) - 1] = '\0';

	iic_release_bus(sc->sc_tag, 0);	/* also unlocks our instance */

	binfo->validflags = ENVSYS_FVALID;

	return (0);
}

static uint32_t
lmtemp_decode_lm75(const uint8_t *buf)
{
	int neg, temp;
	uint32_t val;

	if (buf[0] & 1) {
		/* Below 0C */
		temp = ~buf[1] + 1;
		neg = 1;
	} else {
		temp = buf[1];
		neg = 0;
	}

	/* Temp is given in 1/2 deg. C, we convert to uK. */
	val = ((neg ? -temp : temp) / 2) * 1000000 + 273150000;
	if (temp & 1) {
		if (neg)
			val -= 500000;
		else
			val += 500000;
	}

	return (val);
}

static uint32_t
lmtemp_decode_ds75(const uint8_t *buf)
{
	int temp;

	/*
	 * Sign-extend the MSB byte, and add in the fractions of a
	 * degree contained in the LSB (precision 1/16th DegC).
	 */
	temp = (int8_t)buf[0];
	temp = (temp << 4) | ((buf[1] >> 4) & 0xf);

	/*
	 * Conversion to uK is simple.
	 */
	return (temp * 62500 + 273150000);
}

static uint32_t
lmtemp_decode_lm77(const uint8_t *buf)
{
	int temp;
	uint32_t val;

	/*
	 * Describe each bits of temperature registers on LM77.
	 *   D15 - D12:	Sign
	 *   D11 - D3 :	Bit8(MSB) - Bit0
	 */
	temp = (int8_t)buf[0];
	temp = (temp << 5) | (buf[1] >> 3);

	/* Temp is given in 1/2 deg. C, we convert to uK. */
	val = temp * 500000 + 273150000;

	return (val);
}

