/* $NetBSD: fcu.c,v 1.1.2.2 2018/03/15 09:12:05 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fcu.c,v 1.1.2.2 2018/03/15 09:12:05 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ofw/openfirm.h>

/* FCU registers, from OpenBSD's fcu.c */
#define FCU_FAN_FAIL	0x0b		/* fans states in bits 0<1-6>7 */
#define FCU_FAN_ACTIVE	0x0d
#define FCU_FANREAD(x)	0x11 + (x)*2
#define FCU_FANSET(x)	0x10 + (x)*2
#define FCU_PWM_FAIL	0x2b
#define FCU_PWM_ACTIVE	0x2d
#define FCU_PWMREAD(x)	0x30 + (x)*2

struct fcu_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[32];
	int		sc_nsensors;
};

static int	fcu_match(device_t, cfdata_t, void *);
static void	fcu_attach(device_t, device_t, void *);

static void	fcu_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(fcu, sizeof(struct fcu_softc),
    fcu_match, fcu_attach, NULL, NULL);

static const char * fcu_compats[] = {
	"fcu",
	NULL
};

static int
fcu_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name == NULL) {
		/* no ID registers on this chip */
		if (ia->ia_addr == 0x2f)
			return 1;
		return 0;
	} else {
		return iic_compat_match(ia, fcu_compats);
	}
}

static void
fcu_attach(device_t parent, device_t self, void *aux)
{
	struct fcu_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": Fan Control Unit\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = fcu_sensors_refresh;

	sc->sc_sensors[0].units = ENVSYS_SFANRPM;
	sc->sc_sensors[1].state = ENVSYS_SINVALID;

	/* round up sensors */
	int ch;

	sc->sc_nsensors = 0;
	ch = OF_child(ia->ia_cookie);
	while (ch != 0) {
		char type[32], descr[32];
		uint32_t reg;

		envsys_data_t *s = &sc->sc_sensors[sc->sc_nsensors];

		if (OF_getprop(ch, "device_type", type, 32) <= 0)
			goto next;

		if (strcmp(type, "fan-rpm-control") == 0) {
			s->units = ENVSYS_SFANRPM;
		} else if (strcmp(type, "fan-pwm-control") == 0) {
			/* XXX we get the type from the register number */
			s->units = ENVSYS_SFANRPM;
/* skip those for now since we don't really know how to interpret them */
#if 0
		} else if (strcmp(type, "power-sensor") == 0) {
			s->units = ENVSYS_SVOLTS_DC;
#endif
		} else if (strcmp(type, "gpi-sensor") == 0) {
			s->units = ENVSYS_INDICATOR;
		} else {
			/* ignore other types for now */
			goto next;
		}

		if (OF_getprop(ch, "reg", &reg, sizeof(reg)) <= 0)
			goto next;
		s->private = reg;

		if (OF_getprop(ch, "location", descr, 32) <= 0)
			goto next;
		strcpy(s->desc, descr);

		sysmon_envsys_sensor_attach(sc->sc_sme, s);
		sc->sc_nsensors++;
next:
		ch = OF_peer(ch);
	}		
	sysmon_envsys_register(sc->sc_sme);
}

static void
fcu_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct fcu_softc *sc = sme->sme_cookie;
	uint8_t cmd;
	uint16_t data = -1;
	int error;

	if (edata->units == ENVSYS_SFANRPM) {
	    	cmd = edata->private + 1;
	} else
		cmd = edata->private; 

	/* fcu is a macppc only thing so we can safely assume big endian */
	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, 1, &data, 2, 0);
	iic_release_bus(sc->sc_i2c, 0);

	if (error) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	edata->state = ENVSYS_SVALID;

	switch (edata->units) {
		case ENVSYS_SFANRPM:
			edata->value_cur = data >> 3;
			break;
		case ENVSYS_SVOLTS_DC:
			/* XXX this reads bogus */
			edata->value_cur = data * 1000;
			break;
		case ENVSYS_INDICATOR:
			/* guesswork for now */
			edata->value_cur = data >> 8;
			break;
		default:
			edata->state = ENVSYS_SINVALID;
	}	
}
