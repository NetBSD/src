/*	$NetBSD: auxfan.c,v 1.1 2026/07/26 12:53:13 jdc Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: auxfan.c,v 1.1 2026/07/26 12:53:13 jdc Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/types.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <arch/sparc64/dev/auxfanreg.h>

#define VAL_TO_SPEED(val)	((81680 * 60) / val)	/* Could be +/- 70 */

struct auxfan_softc {
	device_t	sc_dev;

	i2c_tag_t	sc_tag;
	int		sc_address;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;
};

static int auxfan_match(device_t, cfdata_t, void *);
static void auxfan_attach(device_t, device_t, void *);
void auxfan_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(auxfan, sizeof(struct auxfan_softc),
    auxfan_match, auxfan_attach, NULL, NULL);


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "SUNW,i2c-auxfan1" },
	DEVICE_COMPAT_EOL
};

static int
auxfan_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
auxfan_attach(device_t parent, device_t self, void *aux)
{
	struct auxfan_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;

	aprint_normal(": DIMM fan\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;

	sc->sc_sme->sme_refresh = auxfan_refresh;
	strlcpy(sc->sc_sensor.desc, "dimm-fan",
	    sizeof(sc->sc_sensor.desc));
	sc->sc_sensor.units = ENVSYS_SFANRPM;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.flags = ENVSYS_FMONCRITICAL;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		aprint_error_dev(sc->sc_dev,
		    "unable to attach sensor to sysmon\n");
		return;
	}
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	return;
}

void
auxfan_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct auxfan_softc *sc = sme->sme_cookie;
	int err = 0;
	uint8_t reg, val;
	uint16_t count;
	
	if ((err = iic_acquire_bus(sc->sc_tag, 0)) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	reg = AUXFAN_DIMM_FAN_SPD_LO;
	err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, &val, 1, 0);
	if (err) {
		iic_release_bus(sc->sc_tag, 0);
		edata->state = ENVSYS_SINVALID;
		return;
	}
	count = val;

	reg = AUXFAN_DIMM_FAN_SPD_HI;
	err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, &val, 1, 0);
	iic_release_bus(sc->sc_tag, 0);
	if (err) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	count += (val << 8);

	if (count == 0xffff || count == 0x0) {	/* Fan stopped */
		edata->value_cur = 0;
		edata->state = ENVSYS_SCRITICAL;
	} else {
		edata->value_cur = VAL_TO_SPEED(count);
		edata->state = ENVSYS_SVALID;
	}
}
