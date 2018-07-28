/* $NetBSD: dstemp.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Michael Lorenz
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dstemp.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#ifdef macppc
#define HAVE_OF 1
#endif

#ifdef HAVE_OF
#include <dev/ofw/openfirm.h>
#endif

#define DSTEMP_CMD_START	0x51	/* command, no data */
#define DSTEMP_CMD_POR		0x54	/* command, no data */
#define DSTEMP_CMD_STOP		0x22	/* command, no data */
#define DSTEMP_TCURRENT		0xaa	/* current temperature, 2 bytes */
#define DSTEMP_THIGH		0xa1	/* high threshold, 2 bytes */
#define DSTEMP_TLOW		0xa2	/* low threshold, 2 bytes */
#define DSTEMP_CONFIG		0xac	/* 1 byte */

struct dstemp_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensor_temp;
};

static int	dstemp_match(device_t, cfdata_t, void *);
static void	dstemp_attach(device_t, device_t, void *);

static void	dstemp_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(dstemp, sizeof(struct dstemp_softc),
    dstemp_match, dstemp_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ "ds1631",			0 },
	{ NULL,				0 }
};

static int
dstemp_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;
	
	if ((ia->ia_addr & 0xf8) == 0x48)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
dstemp_attach(device_t parent, device_t self, void *aux)
{
	struct dstemp_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	char name[64] = "temperature";

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": DS1361\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = dstemp_sensors_refresh;

	sc->sc_sensor_temp.units = ENVSYS_STEMP;
	sc->sc_sensor_temp.state = ENVSYS_SINVALID;
#ifdef HAVE_OF
	int ch;
	ch = OF_child(ia->ia_cookie);
	if (ch != 0) {
		OF_getprop(ch, "location", name, 64);
	}
#endif
	strncpy(sc->sc_sensor_temp.desc, name, sizeof(sc->sc_sensor_temp.desc));

	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor_temp);

	sysmon_envsys_register(sc->sc_sme);
}

static void
dstemp_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct dstemp_softc *sc = sme->sme_cookie;
	uint8_t cmd = DSTEMP_TCURRENT;
	uint16_t data = 0;
	int error;

		
	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, 1, &data, 2, 0);
	iic_release_bus(sc->sc_i2c, 0);

	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur =
		    ((uint64_t)(data>>4) * 62500) +
		    + 273150000;
		edata->state = ENVSYS_SVALID;
	}
}
