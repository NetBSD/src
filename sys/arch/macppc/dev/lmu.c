 /* $NetBSD: lmu.c,v 1.1 2020/01/10 06:24:17 macallan Exp $ */

/*-
 * Copyright (c) 2020 Michael Lorenz
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

/*
 * ambient light controller found in PowerBook5,6
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lmu.c,v 1.1 2020/01/10 06:24:17 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/time.h>
#include <sys/callout.h>

#include <dev/ofw/openfirm.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

struct lmu_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_node;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[2];
	callout_t	sc_adjust;
	int sc_thresh, sc_hyst;
};

static int	lmu_match(device_t, cfdata_t, void *);
static void	lmu_attach(device_t, device_t, void *);

static void	lmu_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	lmu_set_brightness(struct lmu_softc *, int);
static int	lmu_get_brightness(struct lmu_softc *, int);
static void	lmu_adjust(void *);

CFATTACH_DECL_NEW(lmu, sizeof(struct lmu_softc),
    lmu_match, lmu_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ "lmu-micro",		0 },
	{ "lmu-controller",	0 },
	{ NULL,			0 }
};

static int
lmu_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
lmu_attach(device_t parent, device_t self, void *aux)
{
	struct lmu_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	envsys_data_t *s;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_node = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": ambient light sensor\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = lmu_sensors_refresh;

	s = &sc->sc_sensors[0];
	s->state = ENVSYS_SINVALID;
	s->units = ENVSYS_LUX;
	strcpy(s->desc, "right");
	s->private = 0;
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	s = &sc->sc_sensors[1];
	s->state = ENVSYS_SINVALID;
	s->units = ENVSYS_LUX;
	strcpy(s->desc, "left");
	s->private = 2;
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	sysmon_envsys_register(sc->sc_sme);

	/* TODO: make this adjustable via sysctl */
	sc->sc_thresh = 300;
	sc->sc_hyst = 30;

	callout_init(&sc->sc_adjust, 0);
	callout_setfunc(&sc->sc_adjust, lmu_adjust, sc);
	callout_schedule(&sc->sc_adjust, 0);
}

static void
lmu_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct lmu_softc *sc = sme->sme_cookie;
	int ret;

	if ( edata->private < 3) {
		ret = lmu_get_brightness(sc, edata->private);
		if (ret == -1) return;
		edata->value_cur = ret;
	}
	edata->state = ENVSYS_SVALID;
}

static int
lmu_get_brightness(struct lmu_softc *sc, int reg)
{
	int error;
	uint16_t buf;
	uint8_t cmd = reg;

	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		sc->sc_addr, &cmd, 1, &buf, 2, 0);
	iic_release_bus(sc->sc_i2c, 0);
	if (error) return -1;
	return be16toh(buf);
}

static void
lmu_set_brightness(struct lmu_softc *sc, int b)
{
	int bb;
	uint8_t cmd[3];

	cmd[0] = 1;
	bb = b * 255;
	cmd[1] = (bb & 0xff00) >> 8;
	cmd[2] =  bb & 0xff;

	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		sc->sc_addr, &cmd, 3, NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

static void
lmu_adjust(void *cookie)
{
	struct lmu_softc *sc = cookie;
	int left, right, b;

	left = lmu_get_brightness(sc, 2);
	right = lmu_get_brightness(sc, 0);
	b = left > right ? left : right;

	if (b > (sc->sc_thresh + sc->sc_hyst)) {
		lmu_set_brightness(sc, 0);
	} else if (b < sc->sc_thresh) {
		lmu_set_brightness(sc, 100);
	}

	callout_schedule(&sc->sc_adjust, hz * 2);	
}
