 /* $NetBSD: psoc.c,v 1.8 2021/06/18 23:00:47 macallan Exp $ */

/*-
 * Copyright (c) 2019 Michael Lorenz
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
 * fan controller found in 1GHz TiBook
 *
 * register values from OF:
 * fan1 - 0x20 ( status ), 0x31 ( data )
 * fan2 - 0x26 ( status ), 0x45 ( data )
 * fan status byte 0:
 * 0x5* - fan is running, 0x6* - fan stopped
 * byte 1: unknown, 0x80 seems always set, lower bits seem to fluctuate
 * byte 2: lower 6 bit seem to indicate speed
 * fan speed may be lower 6 bit of byte 2 and lower 6 of byte 1 
 * temperature sensors start at 6, two bytes each, first appears to be
 * the temperature in degrees Celsius
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: psoc.c,v 1.8 2021/06/18 23:00:47 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/time.h>

#include <dev/ofw/openfirm.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include "opt_psoc.h"
#ifdef PSOC_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

struct psoc_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_node;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[7];
	int		sc_nsensors;
	uint8_t		sc_temp[16];
	time_t		sc_last;
};

static int	psoc_match(device_t, cfdata_t, void *);
static void	psoc_attach(device_t, device_t, void *);

static void	psoc_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);

static void	psoc_dump(struct psoc_softc *);

CFATTACH_DECL_NEW(psoc, sizeof(struct psoc_softc),
    psoc_match, psoc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "Psoc" },
	DEVICE_COMPAT_EOL
};

static int
psoc_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
psoc_attach(device_t parent, device_t self, void *aux)
{
	struct psoc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	char path[256];
	envsys_data_t *s;
	int error, ih, r, i;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_node = ia->ia_cookie;
	sc->sc_last = 0;

	aprint_naive("\n");
	aprint_normal(": Psoc fan controller\n");

	error = OF_package_to_path(sc->sc_node, path, 256);
	path[error] = 0;
	DPRINTF("path [%s]\n", path);
	ih = OF_open("fan");
	OF_call_method_1("fan-init", ih, 0);
	DPRINTF("ih %08x\n", ih);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = psoc_sensors_refresh;
	sc->sc_nsensors = 0;

	psoc_dump(sc);

	for (i = 0; i < 4; i++) {
		r = i * 2 + 6;
		s = &sc->sc_sensors[sc->sc_nsensors];
		s->state = ENVSYS_SINVALID;
		s->units = ENVSYS_STEMP;
		snprintf(s->desc, 16, "temp%d", i);
		s->private = r;
		sysmon_envsys_sensor_attach(sc->sc_sme, s);
		sc->sc_nsensors++;
	}

	for (r = 0x20; r < 0x2b; r += 0x06) {
		s = &sc->sc_sensors[sc->sc_nsensors];
		s->state = ENVSYS_SINVALID;
		s->units = ENVSYS_SFANRPM;
		snprintf(s->desc, 16, "reg %02x", r);
		s->private = r;
		sysmon_envsys_sensor_attach(sc->sc_sme, s);
		sc->sc_nsensors++;
	}

	sysmon_envsys_register(sc->sc_sme);
}

static void
psoc_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct psoc_softc *sc = sme->sme_cookie;
	uint8_t cmd = 6;
	uint8_t buf[0x28];
	int error = 1, data;

	if ( edata->private < 0x20) {
		cmd = 0;
		if ((time_second - sc->sc_last) > 2) {
			iic_acquire_bus(sc->sc_i2c, 0);
			error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
				    sc->sc_addr, &cmd, 1, sc->sc_temp, 16, 0);
			iic_release_bus(sc->sc_i2c, 0);
			if (error) return;
			sc->sc_last = time_second;
		}
		error = 0;
		if (edata->private > 0) {
			data = sc->sc_temp[edata->private];
			/* Celsius -> microkelvin */
			edata->value_cur = ((int)data * 1000000) + 273150000;
		}
#ifdef PSOC_DEBUG
		if (edata->private == 6)
			psoc_dump(sc);
#endif
	} else {
		cmd = edata->private;
		iic_acquire_bus(sc->sc_i2c, 0);
		error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
			    sc->sc_addr, &cmd, 1, buf, 3, 0);
		iic_release_bus(sc->sc_i2c, 0);
		if (error) return;
		switch (buf[0] & 0xf0) {
			case 0x50:
				data = buf[edata->private - 0x20];
				edata->value_cur = ((buf[2] & 0x3f) << 6) |
						    (buf[1] & 0x3f);
				break;
			case 0x60:
				edata->value_cur = 0;
				break;
			default:
				error = 0;
		}	
	}
	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->state = ENVSYS_SVALID;
	}
}

static void
psoc_dump(struct psoc_softc *sc)
{
	int i, j;
	uint8_t data, cmd;

	iic_acquire_bus(sc->sc_i2c, 0);
	for (i = 0x20; i < 0x5f; i+= 8) {
		printf("%02x:", i);
		for (j = 0; j < 8; j++) {
			cmd = i + j;
			data = 0;
			iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
			    sc->sc_addr, &cmd, 1, &data, 1, 0);
			printf(" %02x", data);
		}
		printf("\n");
	}
	iic_release_bus(sc->sc_i2c, 0);
}
