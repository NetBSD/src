/*	$NetBSD: mcp980x.c,v 1.1 2013/05/06 22:04:12 rkujawa Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Microchip MCP9800/1/2/3 2-Wire High-Accuracy Temperature Sensor driver.
 * TODO: everything besides simple temperature read with default configuration.
 *
 * Note: MCP9805 is different and is supported by the sdtemp(4) driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcp980x.c,v 1.1 2013/05/06 22:04:12 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/endian.h>

#include <sys/bus.h>
#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/mcp980xreg.h>

struct mcp980x_softc {
	device_t		sc_dev;

	i2c_tag_t		sc_tag;
	i2c_addr_t		sc_addr;

	/* envsys(4) stuff */
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sensor;
	kmutex_t		sc_lock; 
};


static int mcp980x_match(device_t, cfdata_t, void *);
static void mcp980x_attach(device_t, device_t, void *);

/*static uint8_t mcp980x_reg_read_1(struct mcp980x_softc *sc, uint8_t);*/
static uint16_t mcp980x_reg_read_2(struct mcp980x_softc *sc, uint8_t reg);

static uint32_t mcp980x_temperature(struct mcp980x_softc *sc);

static void mcp980x_envsys_register(struct mcp980x_softc *);
static void mcp980x_envsys_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(mcp980x, sizeof (struct mcp980x_softc),
    mcp980x_match, mcp980x_attach, NULL, NULL);

static int
mcp980x_match(device_t parent, cfdata_t cf, void *aux)
{
	/*
	 * No sane way to probe? Perhaps at least try to match constant part
	 * of the I2Caddress.
	 */

	return 1;
}

static void
mcp980x_attach(device_t parent, device_t self, void *aux)
{
	struct mcp980x_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_addr = ia->ia_addr;
	sc->sc_tag = ia->ia_tag;

	aprint_normal(": Microchip MCP980x Temperature Sensor\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	mcp980x_envsys_register(sc);
}

static uint16_t
mcp980x_reg_read_2(struct mcp980x_softc *sc, uint8_t reg)
{
	uint8_t wbuf[2];
	uint16_t rv;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL) != 0) {
		aprint_error_dev(sc->sc_dev, "cannot acquire bus for read\n");
		return 0;
	}

	wbuf[0] = reg;

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, wbuf,
	    1, &rv, 2, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev, "cannot execute operation\n");
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		return 0;
	}
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return be16toh(rv);
}

/* Will need that later for reading config register. */ 
/*
static uint8_t
mcp980x_reg_read_1(struct mcp980x_softc *sc, uint8_t reg)
{
	uint8_t rv, wbuf[2];

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL) != 0) {
		aprint_error_dev(sc->sc_dev, "cannot acquire bus for read\n");
		return 0;
	}

	wbuf[0] = reg;

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, wbuf,
	    1, &rv, 1, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev, "cannot execute operation\n");
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		return 0;
	}
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return rv;
}*/

/* Get temperature in microKelvins. */
static uint32_t
mcp980x_temperature(struct mcp980x_softc *sc)
{
	uint16_t raw;
	uint32_t rv, uk, basedegc;

	raw = mcp980x_reg_read_2(sc, MCP980X_AMBIENT_TEMP);

	basedegc = (raw & MCP980X_AMBIENT_TEMP_DEGREES) >> 
	    MCP980X_AMBIENT_TEMP_DEGREES_SHIFT;

	uk = 1000000 * basedegc;

	if (raw & MCP980X_AMBIENT_TEMP_05DEGREE)
		uk += 500000;
	if (raw & MCP980X_AMBIENT_TEMP_025DEGREE)
		uk += 250000;
	if (raw & MCP980X_AMBIENT_TEMP_0125DEGREE)
		uk += 125000;
	if (raw & MCP980X_AMBIENT_TEMP_00625DEGREE)
		uk += 62500;

	if (raw & MCP980X_AMBIENT_TEMP_SIGN)
		rv = 273150000U - uk;
	else
		rv = 273150000U + uk;

	return rv;	
}

static void
mcp980x_envsys_register(struct mcp980x_softc *sc)
{
	sc->sc_sme = sysmon_envsys_create();

	strlcpy(sc->sc_sensor.desc, "Ambient temp",
	    sizeof(sc->sc_sensor.desc));
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		aprint_error_dev(sc->sc_dev,
		    "error attaching sensor\n");
		return;
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = mcp980x_envsys_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev, "unable to register in sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
mcp980x_envsys_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct mcp980x_softc *sc = sme->sme_cookie;

	mutex_enter(&sc->sc_lock);

	edata->value_cur = mcp980x_temperature(sc);
	edata->state = ENVSYS_SVALID;

	mutex_exit(&sc->sc_lock);
}
