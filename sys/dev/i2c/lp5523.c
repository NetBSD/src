/* $NetBSD: lp5523.c,v 1.1.2.2 2013/05/16 15:51:29 khorben Exp $ */

/*
 * Texas Instruments LP5523 Programmable 9-Output LED Driver
 *
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Pierre Pronchery (khorben@defora.org).
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
__KERNEL_RCSID(0, "$NetBSD: lp5523.c,v 1.1.2.2 2013/05/16 15:51:29 khorben Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/i2c/lp5523reg.h>

#define LP5523_DEGC2UK(t) ((t * 1000000) + 273150000)

/* constants */
enum lp5523_leds {
	LP5523_LED_0 = 0,
	LP5523_LED_1,
	LP5523_LED_2,
	LP5523_LED_3,
	LP5523_LED_4,
	LP5523_LED_5,
	LP5523_LED_6,
	LP5523_LED_7,
	LP5523_LED_8
};
#define LP5523_LED_LAST		LP5523_LED_8
#define LP5523_LED_CNT		(LP5523_LED_LAST + 1)

/* variables */
struct lp5523_softc {
	device_t		sc_dev;
	i2c_tag_t		sc_i2c;
	i2c_addr_t		sc_addr;

	struct sysctllog	*sc_sysctllog;

	/* temperature sensor */
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_temp_sensor;
};

static int	lp5523_match(device_t, cfdata_t, void *);
static void	lp5523_attach(device_t, device_t, void *);
static void	lp5523_attach_envsys(struct lp5523_softc *);
static void	lp5523_attach_sysctl(struct lp5523_softc *);

static int	lp5523_reset(struct lp5523_softc *);

static int	lp5523_sysctl(SYSCTLFN_ARGS);

static void	lp5523_refresh_temperature(struct sysmon_envsys *,
		envsys_data_t *);

static int	lp5523_read_1(struct lp5523_softc *, uint8_t, uint8_t *);
static int	lp5523_write_1(struct lp5523_softc *, uint8_t, uint8_t);

CFATTACH_DECL_NEW(lp5523led, sizeof(struct lp5523_softc),
	lp5523_match, lp5523_attach, NULL, NULL);

static int
lp5523_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
lp5523_attach(device_t parent, device_t self, void *aux)
{
	struct lp5523_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	lp5523_reset(sc);

	aprint_normal(": LED driver\n");
	aprint_naive(": LED driver\n");

	lp5523_attach_sysctl(sc);

	lp5523_attach_envsys(sc);

	if (!pmf_device_register(sc->sc_dev, NULL, NULL)) {
		aprint_error_dev(sc->sc_dev,
		    "could not establish power handler\n");
	}
}

static void
lp5523_attach_envsys(struct lp5523_softc *sc)
{
	const int flags = ENVSYS_FMONNOTSUPP | ENVSYS_FHAS_ENTROPY
		| ENVSYS_FVALID_MIN | ENVSYS_FVALID_MAX;

	sc->sc_sme = sysmon_envsys_create();

	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_refresh = lp5523_refresh_temperature;

	sc->sc_temp_sensor.flags = flags;
	sc->sc_temp_sensor.units = ENVSYS_STEMP;
	sc->sc_temp_sensor.state = ENVSYS_SINVALID;
	sc->sc_temp_sensor.value_min = LP5523_DEGC2UK(-41);
	sc->sc_temp_sensor.value_max = LP5523_DEGC2UK(89);

	strlcat(sc->sc_temp_sensor.desc, "temperature",
			sizeof(sc->sc_temp_sensor.desc));

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_temp_sensor) != 0
			|| sysmon_envsys_register(sc->sc_sme) != 0)
	{
		aprint_error_dev(sc->sc_dev, "unable to attach the temperature"
				" sensor\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	lp5523_refresh_temperature(sc->sc_sme, &sc->sc_temp_sensor);
}

static void
lp5523_attach_sysctl(struct lp5523_softc *sc)
{
	struct sysctllog **log = &sc->sc_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	unsigned int i;
	char buf[8];
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW,
			CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &rnode, CTLFLAG_PERMANENT,
			CTLTYPE_NODE, device_xname(sc->sc_dev),
			SYSCTL_DESCR("lp5523led control"), NULL, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);
	if (error)
		return;

	for (i = 0; i < LP5523_LED_CNT; i++) {
		snprintf(buf, sizeof(buf), "led%u", i);
		error = sysctl_createv(log, 0, &rnode, &cnode,
				CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
				CTLTYPE_INT, buf, SYSCTL_DESCR("LED enable"),
				lp5523_sysctl, 0, (void *)sc, 0, CTL_CREATE,
				CTL_EOL);
		if (error)
			break;
	}
}

static int
lp5523_reset(struct lp5523_softc *sc)
{
	uint8_t u8;

	iic_acquire_bus(sc->sc_i2c, 0);
	lp5523_write_1(sc, LP5523_REG_RESET, 0xff);

	u8 = LP5523_REG_ENGINE_CNTRL1_CHIP_EN;
	lp5523_write_1(sc, LP5523_REG_ENGINE_CNTRL1, u8);
	iic_release_bus(sc->sc_i2c, 0);

	return 0;
}

static int
lp5523_sysctl(SYSCTLFN_ARGS)
{
	struct lp5523_softc *sc;
	struct sysctlnode node;
	u_int led;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	iic_acquire_bus(sc->sc_i2c, 0);
	/* FIXME really implement */
	led = 0;
	iic_release_bus(sc->sc_i2c, 0);

	node.sysctl_data = &led;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	iic_acquire_bus(sc->sc_i2c, 0);
	/* FIXME really implement */
	error = 0;
	iic_release_bus(sc->sc_i2c, 0);

	return error;
}

static void
lp5523_refresh_temperature(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct lp5523_softc *sc = sme->sme_cookie;
	int8_t s8;

	iic_acquire_bus(sc->sc_i2c, 0);
	lp5523_read_1(sc, LP5523_REG_TEMP_READ, &s8);
	iic_release_bus(sc->sc_i2c, 0);
	if (s8 >= -41 && s8 <= 89) {
		sc->sc_temp_sensor.state = ENVSYS_SVALID;
		sc->sc_temp_sensor.value_cur = LP5523_DEGC2UK(s8);
	} else {
		sc->sc_temp_sensor.state = ENVSYS_SINVALID;
	}
}

static int
lp5523_read_1(struct lp5523_softc *sc, uint8_t reg, uint8_t *val)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
			&reg, sizeof(reg), val, sizeof(*val), 0);
}

static int
lp5523_write_1(struct lp5523_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t data[2] = { reg, val };

	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
			NULL, 0, data, sizeof(data), 0);
}
