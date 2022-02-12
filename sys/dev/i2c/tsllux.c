/* $NetBSD: tsllux.c,v 1.4 2022/02/12 03:24:35 riastradh Exp $ */

/*-
 * Copyright (c) 2018 Jason R. Thorpe
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
__KERNEL_RCSID(0, "$NetBSD: tsllux.c,v 1.4 2022/02/12 03:24:35 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/tsl256xreg.h>

#include <dev/sysmon/sysmonvar.h>

struct tsllux_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	uint32_t	sc_poweron;

	/*
	 * Locking order is:
	 *	tsllux mutex -> i2c bus
	 */
	kmutex_t	sc_lock;

	uint8_t		sc_itime;
	uint8_t		sc_gain;
	bool		sc_cs_package;
	bool		sc_auto_gain;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensor;

	struct sysctllog *sc_sysctllog;
};

#define	TSLLUX_F_CS_PACKAGE	0x01

static int	tsllux_match(device_t, cfdata_t, void *);
static void	tsllux_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tsllux, sizeof(struct tsllux_softc),
    tsllux_match, tsllux_attach, NULL, NULL);

static const struct device_compatible_entry tsllux_compat_data[] = {
	{ .compat = "amstaos,tsl2560" },
	{ .compat = "amstaos,tsl2561" },
	DEVICE_COMPAT_EOL
};

static int	tsllux_read1(struct tsllux_softc *, uint8_t, uint8_t *);
static int	tsllux_read2(struct tsllux_softc *, uint8_t, uint16_t *);
static int	tsllux_write1(struct tsllux_softc *, uint8_t, uint8_t);
#if 0
static int	tsllux_write2(struct tsllux_softc *, uint8_t, uint16_t);
#endif

static void	tsllux_sysctl_attach(struct tsllux_softc *);

static int	tsllux_poweron(struct tsllux_softc *);
static int	tsllux_poweroff(struct tsllux_softc *);

static int	tsllux_set_integration_time(struct tsllux_softc *, uint8_t);
static int	tsllux_set_gain(struct tsllux_softc *, uint8_t);
static int	tsllux_set_autogain(struct tsllux_softc *, bool);

static int	tsllux_get_lux(struct tsllux_softc *, uint32_t *,
			       uint16_t *, uint16_t *);

static void	tsllux_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);

static int
tsllux_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	uint8_t id_reg;
	int error, match_result;

	if (iic_use_direct_match(ia, match, tsllux_compat_data, &match_result))
		return (match_result);

	switch (ia->ia_addr) {
	case TSL256x_SLAVEADDR_GND:
	case TSL256x_SLAVEADDR_FLOAT:
	case TSL256x_SLAVEADDR_VDD:
		break;

	default:
		return (0);
	}

	if (iic_acquire_bus(ia->ia_tag, 0) != 0)
		return (0);
	error = iic_smbus_read_byte(ia->ia_tag, ia->ia_addr,
	    TSL256x_REG_ID | COMMAND6x_CMD, &id_reg, 0);
	iic_release_bus(ia->ia_tag, 0);

	if (error)
		return (0);

	/* XXX This loses if we have a 2560 rev. 0. */
	if (id_reg == 0)
		return (I2C_MATCH_ADDRESS_ONLY);

	return (I2C_MATCH_ADDRESS_AND_PROBE);
}

static void
tsllux_attach(device_t parent, device_t self, void *aux)
{
	struct tsllux_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	bool have_i2c;

	/* XXX IPL_NONE changes when we support threshold interrupts. */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	if (device_cfdata(self)->cf_flags & TSLLUX_F_CS_PACKAGE)
		sc->sc_cs_package = true;

	if (iic_acquire_bus(ia->ia_tag, 0) != 0) {
		return;
	}

	have_i2c = true;

	/* Power on the device and clear any pending interrupts. */
	if (tsllux_write1(sc, TSL256x_REG_CONTROL | COMMAND6x_CLEAR,
			  CONTROL6x_POWER_ON)) {
		aprint_error_dev(self, ": unable to power on device\n");
		goto out;
	}
	sc->sc_poweron = 1;

	/* Make sure interrupts are disabled. */
	if (tsllux_write1(sc, TSL256x_REG_INTERRUPT | COMMAND6x_CLEAR, 0)) {
		aprint_error_dev(self, ": unable to disable interrupts\n");
		goto out;
	}

	aprint_naive("\n");
	aprint_normal(": TSL256x Light-to-Digital converter%s\n",
		      sc->sc_cs_package ? " (CS package)" : "");

	/* Inititalize timing to reasonable defaults. */
	sc->sc_auto_gain = true;
	sc->sc_gain = TIMING6x_GAIN_16X;
	if (tsllux_set_integration_time(sc, TIMING6x_INTEG_101ms)) {
		aprint_error_dev(self, ": unable to set integration time\n");
		goto out;
	}

	tsllux_poweroff(sc);

	iic_release_bus(ia->ia_tag, 0);
	have_i2c = false;

	tsllux_sysctl_attach(sc);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = tsllux_sensors_refresh;

	sc->sc_sensor.units = ENVSYS_LUX;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	snprintf(sc->sc_sensor.desc, sizeof(sc->sc_sensor.desc),
		 "ambient light");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor);

	sysmon_envsys_register(sc->sc_sme);

 out:
	if (have_i2c) {
		if (sc->sc_poweron)
			tsllux_poweroff(sc);
		iic_release_bus(ia->ia_tag, 0);
	}
}

static int
tsllux_sysctl_cs_package(SYSCTLFN_ARGS)
{
	struct tsllux_softc *sc;
	struct sysctlnode node;
	int error;
	u_int val;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_lock);
	val = sc->sc_cs_package ? 1 : 0;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		mutex_exit(&sc->sc_lock);
		return (error);
	}

	/* CS package indicator is used only in software; no need for I2C. */

	sc->sc_cs_package = val ? true : false;
	mutex_exit(&sc->sc_lock);

	return (error);
}

static int
tsllux_sysctl_autogain(SYSCTLFN_ARGS)
{
	struct tsllux_softc *sc;
	struct sysctlnode node;
	int error;
	u_int val;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_lock);
	val = sc->sc_auto_gain ? 1 : 0;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		mutex_exit(&sc->sc_lock);
		return (error);
	}

	/* Auto-gain is a software feature; no need for I2C. */

	error = tsllux_set_autogain(sc, val ? true : false);
	mutex_exit(&sc->sc_lock);

	return (error);
}

static int
tsllux_sysctl_gain(SYSCTLFN_ARGS)
{
	struct tsllux_softc *sc;
	struct sysctlnode node;
	int error;
	u_int val;
	uint8_t new_gain;
	
	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_lock);

	switch (sc->sc_gain) {
	case TIMING6x_GAIN_1X:
		val = 1;
		break;
	
	case TIMING6x_GAIN_16X:
		val = 16;
		break;
	
	default:
		val = 1;
		break;
	}
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		mutex_exit(&sc->sc_lock);
		return (error);
	}

	switch (val) {
	case 1:
		new_gain = TIMING6x_GAIN_1X;
		break;
	
	case 16:
		new_gain = TIMING6x_GAIN_16X;
		break;
	
	default:
		mutex_exit(&sc->sc_lock);
		return (EINVAL);
	}

	if ((error = iic_acquire_bus(sc->sc_i2c, 0)) != 0) {
		mutex_exit(&sc->sc_lock);
		return (error);
	}

	error = tsllux_set_gain(sc, new_gain);
	iic_release_bus(sc->sc_i2c, 0);
	mutex_exit(&sc->sc_lock);

	return (error);
}

static int
tsllux_sysctl_itime(SYSCTLFN_ARGS)
{
	struct tsllux_softc *sc;
	struct sysctlnode node;
	int error;
	u_int val;
	uint8_t new_itime;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_lock);

	switch (sc->sc_itime) {
	case TIMING6x_INTEG_13_7ms:
		val = 13;
		break;
	
	case TIMING6x_INTEG_101ms:
		val = 101;
		break;
	
	case TIMING6x_INTEG_402ms:
	default:
		val = 402;
		break;
	}
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		mutex_exit(&sc->sc_lock);
		return (error);
	}

	switch (val) {
	case 13:
	case 14:
		new_itime = TIMING6x_INTEG_13_7ms;
		break;
	
	case 101:
		new_itime = TIMING6x_INTEG_101ms;
		break;
	
	case 402:
		new_itime = TIMING6x_INTEG_402ms;
		break;
	
	default:
		mutex_exit(&sc->sc_lock);
		return (EINVAL);
	}

	if ((error = iic_acquire_bus(sc->sc_i2c, 0)) != 0) {
		mutex_exit(&sc->sc_lock);
		return (error);
	}

	error = tsllux_set_integration_time(sc, new_itime);
	iic_release_bus(sc->sc_i2c, 0);
	mutex_exit(&sc->sc_lock);

	return (error);
}

static void
tsllux_sysctl_attach(struct tsllux_softc *sc)
{
	struct sysctllog **log = &sc->sc_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("tsl256x control"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "cs_package",
	    SYSCTL_DESCR("sensor in Chipscale (CS) package"),
	    tsllux_sysctl_cs_package, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "auto_gain",
	    SYSCTL_DESCR("auto-gain algorithm enabled"),
	    tsllux_sysctl_autogain, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;
	
	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "gain",
	    SYSCTL_DESCR("sensor gain"), tsllux_sysctl_gain, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "integration_time",
	    SYSCTL_DESCR("ADC integration time"), tsllux_sysctl_itime, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;
}

static void
tsllux_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct tsllux_softc *sc = sme->sme_cookie;
	uint32_t lux;
	int error;

	if (edata != &sc->sc_sensor) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	mutex_enter(&sc->sc_lock);

	if ((error = iic_acquire_bus(sc->sc_i2c, 0)) == 0) {
		error = tsllux_get_lux(sc, &lux, NULL, NULL);
		iic_release_bus(sc->sc_i2c, 0);
	}
	
	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = lux;
		edata->state = ENVSYS_SVALID;
	}

	mutex_exit(&sc->sc_lock);
}

/*
 * Allow pending interrupts to be cleared as part of another operation.
 */
#define	REGMASK6x		(COMMAND6x_REGMASK | COMMAND6x_CLEAR)

static int
tsllux_read1(struct tsllux_softc *sc, uint8_t reg, uint8_t *valp)
{
	reg = (reg & REGMASK6x) | COMMAND6x_CMD;
	return (iic_smbus_read_byte(sc->sc_i2c, sc->sc_addr, reg, valp, 0));
}

static int
tsllux_read2(struct tsllux_softc *sc, uint8_t reg, uint16_t *valp)
{
	reg = (reg & REGMASK6x) | COMMAND6x_CMD | COMMAND6x_WORD;
	return (iic_smbus_read_word(sc->sc_i2c, sc->sc_addr, reg, valp, 0));
}

static int
tsllux_write1(struct tsllux_softc *sc, uint8_t reg, uint8_t val)
{
	reg = (reg & REGMASK6x) | COMMAND6x_CMD;
	return (iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr, reg, val, 0));
}

#if 0
static int
tsllux_write2(struct tsllux_softc *sc, uint8_t reg, uint16_t val)
{
	reg = (reg & REGMASK6x) | COMMAND6x_CMD | COMMAND6x_WORD;
	return (iic_smbus_write_word(sc->sc_i2c, sc->sc_addr, reg, val, 0));
}
#endif

#undef REGMASK

static int
tsllux_poweron(struct tsllux_softc *sc)
{
	int error;

	if (sc->sc_poweron++ == 0) {
		uint8_t val;

		error = tsllux_write1(sc, TSL256x_REG_CONTROL,
				      CONTROL6x_POWER_ON);
		if (error)
			return (error);

		error = tsllux_read1(sc, TSL256x_REG_CONTROL, &val);
		if (error)
			return (error);

		if (val != CONTROL6x_POWER_ON) {
			aprint_error_dev(sc->sc_dev,
					 "failed to power on sensor\n");
			return (EIO);
		}
	}
	return (0);
}

static int
tsllux_poweroff(struct tsllux_softc *sc)
{
	if (sc->sc_poweron && --sc->sc_poweron == 0)
		return (tsllux_write1(sc, TSL256x_REG_CONTROL,
				      CONTROL6x_POWER_OFF));
	return (0);
}

static int
tsllux_set_integration_time(struct tsllux_softc *sc, uint8_t time)
{
	int error;

	switch (time) {
	case TIMING6x_INTEG_13_7ms:
	case TIMING6x_INTEG_101ms:
	case TIMING6x_INTEG_402ms:
		break;
	
	default:
		return (EINVAL);
	}

	if ((error = tsllux_poweron(sc)) != 0)
		return (error);
	
	if ((error = tsllux_write1(sc, TSL256x_REG_TIMING,
				   time | sc->sc_gain)) != 0)
		goto out;
	
	sc->sc_itime = time;

 out:
	(void) tsllux_poweroff(sc);
	return (error);
}

static int
tsllux_set_gain0(struct tsllux_softc *sc, uint8_t gain)
{
	int error;

	if ((error = tsllux_write1(sc, TSL256x_REG_TIMING,
				   sc->sc_itime | gain)) != 0)
		return (error);
	
	sc->sc_gain = gain;
	return (0);
}

static int
tsllux_set_gain(struct tsllux_softc *sc, uint8_t gain)
{
	int error;

	switch (gain) {
	case TIMING6x_GAIN_1X:
	case TIMING6x_GAIN_16X:
		break;
	
	default:
		return (EINVAL);
	}

	if ((error = tsllux_poweron(sc)) != 0)
		return (error);

	if ((error = tsllux_set_gain0(sc, gain)) != 0)
		goto out;
	
	sc->sc_auto_gain = false;

 out:
	(void) tsllux_poweroff(sc);
	return (error);
}

static int
tsllux_set_autogain(struct tsllux_softc *sc, bool use_autogain)
{

	sc->sc_auto_gain = use_autogain;
	return (0);
}

static int
tsllux_wait_for_adcs(struct tsllux_softc *sc)
{
	int ms;

	switch (sc->sc_itime) {
	case TIMING6x_INTEG_13_7ms:
		/* Wait 15ms for 13.7ms integration */
		ms = 15;
		break;
	
	case TIMING6x_INTEG_101ms:
		/* Wait 120ms for 101ms integration */
		ms = 120;
		break;
	
	case TIMING6x_INTEG_402ms:
	default:
		/* Wait 450ms for 402ms integration */
		ms = 450;
		break;
	}

	if (ms < hztoms(1)) {
		/* Just busy-wait if we want to wait for less than 1 tick. */
		delay(ms * 1000);
	} else {
		/* Round up one tick for the case where we sleep. */
		(void) kpause("tslluxwait", false, mstohz(ms) + 1, NULL);
	}

	return (0);
}

static int
tsllux_read_adcs(struct tsllux_softc *sc, uint16_t *adc0valp,
		 uint16_t *adc1valp)
{
	int error;

	if ((error = tsllux_read2(sc, TSL256x_REG_DATA0LOW, adc0valp)) == 0)
		error = tsllux_read2(sc, TSL256x_REG_DATA1LOW, adc1valp);
	
	return (error);
}

/*
 * The following code is partially derived from Adafruit's TSL2561
 * driver for Arduino (which was in turn derived from the data sheet),
 * which carries this notice:
 * 
 * @file Adafruit_TSL2561_U.cpp
 *
 * @mainpage Adafruit TSL2561 Light/Lux sensor driver
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's TSL2561 driver for the
 * Arduino platform.  It is designed specifically to work with the
 * Adafruit TSL2561 breakout: http://www.adafruit.com/products/439
 *
 * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the breakout.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_Sensor">
 * Adafruit_Sensor</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Kevin "KTOWN" Townsend for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 *   @section  HISTORY
 *
 *   v2.0 - Rewrote driver for Adafruit_Sensor and Auto-Gain support, and
 *          added lux clipping check (returns 0 lux on sensor saturation)
 *   v1.0 - First release (previously TSL2561)
 */

static int
tsllux_read_sensors(struct tsllux_softc *sc, uint16_t *adc0p, uint16_t *adc1p)
{
	int error;

	if ((error = tsllux_poweron(sc)) != 0)
		return (error);
	
	if ((error = tsllux_wait_for_adcs(sc)) != 0)
		goto out;
	
	error = tsllux_read_adcs(sc, adc0p, adc1p);

 out:
	(void) tsllux_poweroff(sc);
	return (error);
}

/*
 * Auto-gain thresholds:
 */
#define	TSL2561_AGC_THI_13MS	(4850)	/* Max value at Ti 13ms = 5047 */
#define	TSL2561_AGC_TLO_13MS	(100)	/* Min value at Ti 13ms = 100 */
#define	TSL2561_AGC_THI_101MS	(36000)	/* Max value at Ti 101ms = 37177 */
#define	TSL2561_AGC_TLO_101MS	(200)	/* Min value at Ti 101ms = 200 */
#define	TSL2561_AGC_THI_402MS	(63000)	/* Max value at Ti 402ms = 65535 */
#define	TSL2561_AGC_TLO_402MS	(500)	/* Min value at Ti 402ms = 500 */

static int
tsllux_get_sensor_data(struct tsllux_softc *sc, uint16_t *broadband,
		       uint16_t *ir)
{
	int error = 0;
	uint16_t adc0, adc1;
	bool did_adjust_gain, valid;
	uint16_t hi, lo;

	if (sc->sc_auto_gain == false) {
		error = tsllux_read_sensors(sc, &adc0, &adc1);
		goto out;
	}

	/* Set the hi / lo threshold based on current integration time. */
	switch (sc->sc_itime) {
	case TIMING6x_INTEG_13_7ms:
		hi = TSL2561_AGC_THI_13MS;
		lo = TSL2561_AGC_TLO_13MS;
		break;
	
	case TIMING6x_INTEG_101ms:
		hi = TSL2561_AGC_THI_101MS;
		lo = TSL2561_AGC_TLO_101MS;
		break;
	
	case TIMING6x_INTEG_402ms:
	default:
		hi = TSL2561_AGC_THI_402MS;
		lo = TSL2561_AGC_TLO_402MS;
	}

	/* Read data and adjust the gain until we have a valid range. */
	for (valid = false, did_adjust_gain = false; valid == false; ) {
		if ((error = tsllux_read_sensors(sc, &adc0, &adc1)) != 0)
			goto out;

		if (did_adjust_gain == false) {
			if (adc0 < lo && sc->sc_gain == TIMING6x_GAIN_1X) {
				/* Increase the gain and try again. */
				if ((error =
				     tsllux_set_gain0(sc,
				     		      TIMING6x_GAIN_16X)) != 0)
					goto out;
				did_adjust_gain = true;
			} else if (adc0 > hi &&
				   sc->sc_gain == TIMING6x_GAIN_16X) {
				/* Decrease the gain and try again. */
				if ((error =
				     tsllux_set_gain0(sc,
				     		      TIMING6x_GAIN_1X)) != 0)
					goto out;
				did_adjust_gain = true;
			} else {
				/*
				 * Reading is either valid or we're already
				 * at the chip's limits.
				 */
				valid = true;
			}
		} else {
			/*
			 * If we've already adjust the gain once, just
			 * return the new results.  This avoids endless
			 * loops where a value is at one extre pre-gain
			 * and at the other extreme post-gain.
			 */
			valid = true;
		}
	}

 out:
	if (error == 0) {
		if (broadband != NULL)
			*broadband = adc0;
		if (ir != NULL)
			*ir = adc1;
	}
	return (error);
}

/*
 * Clipping thresholds:
 */
#define	TSL2561_CLIPPING_13MS	(4900)
#define	TSL2561_CLIPPING_101MS	(37000)
#define	TSL2561_CLIPPING_402MS	(65000)

/*
 * Scaling factors:
 */
#define	TSL2561_LUX_LUXSCALE      (14)	   /* Scale by 2^14 */
#define	TSL2561_LUX_RATIOSCALE    (9)      /* Scale ratio by 2^9 */
#define	TSL2561_LUX_CHSCALE       (10)     /* Scale channel values by 2^10 */
#define	TSL2561_LUX_CHSCALE_TINT0 (0x7517) /* 322/11 * 2^TSL2561_LUX_CHSCALE */
#define	TSL2561_LUX_CHSCALE_TINT1 (0x0FE7) /* 322/81 * 2^TSL2561_LUX_CHSCALE */

/*
 * Lux factors (the datasheet explains how these magic constants
 * are used):
 */
/* T, FN and CL package values */
#define TSL2561_LUX_K1T           (0x0040)  /* 0.125 * 2^RATIO_SCALE */
#define TSL2561_LUX_B1T           (0x01f2)  /* 0.0304 * 2^LUX_SCALE */
#define TSL2561_LUX_M1T           (0x01be)  /* 0.0272 * 2^LUX_SCALE */
#define TSL2561_LUX_K2T           (0x0080)  /* 0.250 * 2^RATIO_SCALE */
#define TSL2561_LUX_B2T           (0x0214)  /* 0.0325 * 2^LUX_SCALE */
#define TSL2561_LUX_M2T           (0x02d1)  /* 0.0440 * 2^LUX_SCALE */
#define TSL2561_LUX_K3T           (0x00c0)  /* 0.375 * 2^RATIO_SCALE */
#define TSL2561_LUX_B3T           (0x023f)  /* 0.0351 * 2^LUX_SCALE */
#define TSL2561_LUX_M3T           (0x037b)  /* 0.0544 * 2^LUX_SCALE */
#define TSL2561_LUX_K4T           (0x0100)  /* 0.50 * 2^RATIO_SCALE */
#define TSL2561_LUX_B4T           (0x0270)  /* 0.0381 * 2^LUX_SCALE */
#define TSL2561_LUX_M4T           (0x03fe)  /* 0.0624 * 2^LUX_SCALE */
#define TSL2561_LUX_K5T           (0x0138)  /* 0.61 * 2^RATIO_SCALE */
#define TSL2561_LUX_B5T           (0x016f)  /* 0.0224 * 2^LUX_SCALE */
#define TSL2561_LUX_M5T           (0x01fc)  /* 0.0310 * 2^LUX_SCALE */
#define TSL2561_LUX_K6T           (0x019a)  /* 0.80 * 2^RATIO_SCALE */
#define TSL2561_LUX_B6T           (0x00d2)  /* 0.0128 * 2^LUX_SCALE */
#define TSL2561_LUX_M6T           (0x00fb)  /* 0.0153 * 2^LUX_SCALE */
#define TSL2561_LUX_K7T           (0x029a)  /* 1.3 * 2^RATIO_SCALE */
#define TSL2561_LUX_B7T           (0x0018)  /* 0.00146 * 2^LUX_SCALE */
#define TSL2561_LUX_M7T           (0x0012)  /* 0.00112 * 2^LUX_SCALE */
#define TSL2561_LUX_K8T           (0x029a)  /* 1.3 * 2^RATIO_SCALE */
#define TSL2561_LUX_B8T           (0x0000)  /* 0.000 * 2^LUX_SCALE */
#define TSL2561_LUX_M8T           (0x0000)  /* 0.000 * 2^LUX_SCALE */

/* CS package values */
#define TSL2561_LUX_K1C           (0x0043)  /* 0.130 * 2^RATIO_SCALE */
#define TSL2561_LUX_B1C           (0x0204)  /* 0.0315 * 2^LUX_SCALE */
#define TSL2561_LUX_M1C           (0x01ad)  /* 0.0262 * 2^LUX_SCALE */
#define TSL2561_LUX_K2C           (0x0085)  /* 0.260 * 2^RATIO_SCALE */
#define TSL2561_LUX_B2C           (0x0228)  /* 0.0337 * 2^LUX_SCALE */
#define TSL2561_LUX_M2C           (0x02c1)  /* 0.0430 * 2^LUX_SCALE */
#define TSL2561_LUX_K3C           (0x00c8)  /* 0.390 * 2^RATIO_SCALE */
#define TSL2561_LUX_B3C           (0x0253)  /* 0.0363 * 2^LUX_SCALE */
#define TSL2561_LUX_M3C           (0x0363)  /* 0.0529 * 2^LUX_SCALE */
#define TSL2561_LUX_K4C           (0x010a)  /* 0.520 * 2^RATIO_SCALE */
#define TSL2561_LUX_B4C           (0x0282)  /* 0.0392 * 2^LUX_SCALE */
#define TSL2561_LUX_M4C           (0x03df)  /* 0.0605 * 2^LUX_SCALE */
#define TSL2561_LUX_K5C           (0x014d)  /* 0.65 * 2^RATIO_SCALE */
#define TSL2561_LUX_B5C           (0x0177)  /* 0.0229 * 2^LUX_SCALE */
#define TSL2561_LUX_M5C           (0x01dd)  /* 0.0291 * 2^LUX_SCALE */
#define TSL2561_LUX_K6C           (0x019a)  /* 0.80 * 2^RATIO_SCALE */
#define TSL2561_LUX_B6C           (0x0101)  /* 0.0157 * 2^LUX_SCALE */
#define TSL2561_LUX_M6C           (0x0127)  /* 0.0180 * 2^LUX_SCALE */
#define TSL2561_LUX_K7C           (0x029a)  /* 1.3 * 2^RATIO_SCALE */
#define TSL2561_LUX_B7C           (0x0037)  /* 0.00338 * 2^LUX_SCALE */
#define TSL2561_LUX_M7C           (0x002b)  /* 0.00260 * 2^LUX_SCALE */
#define TSL2561_LUX_K8C           (0x029a)  /* 1.3 * 2^RATIO_SCALE */
#define TSL2561_LUX_B8C           (0x0000)  /* 0.000 * 2^LUX_SCALE */
#define TSL2561_LUX_M8C           (0x0000)  /* 0.000 * 2^LUX_SCALE */

struct lux_factor_table_entry {
	uint16_t	k;
	uint16_t	b;
	uint16_t	m;
};

static const struct lux_factor_table_entry lux_factor_table[] = {
	{ TSL2561_LUX_K1T,	TSL2561_LUX_B1T,	TSL2561_LUX_M1T },
	{ TSL2561_LUX_K2T,	TSL2561_LUX_B2T,	TSL2561_LUX_M2T },
	{ TSL2561_LUX_K3T,	TSL2561_LUX_B3T,	TSL2561_LUX_M3T },
	{ TSL2561_LUX_K4T,	TSL2561_LUX_B4T,	TSL2561_LUX_M4T },
	{ TSL2561_LUX_K5T,	TSL2561_LUX_B5T,	TSL2561_LUX_M5T },
	{ TSL2561_LUX_K6T,	TSL2561_LUX_B6T,	TSL2561_LUX_M6T },
	{ TSL2561_LUX_K7T,	TSL2561_LUX_B7T,	TSL2561_LUX_M7T },
	{ TSL2561_LUX_K8T,	TSL2561_LUX_B8T,	TSL2561_LUX_M8T },
};
static const int lux_factor_table_last_entry =
    (sizeof(lux_factor_table) / sizeof(lux_factor_table[0])) - 1;

static const struct lux_factor_table_entry lux_factor_table_cs_package[] = {
	{ TSL2561_LUX_K1C,	TSL2561_LUX_B1C,	TSL2561_LUX_M1C },
	{ TSL2561_LUX_K2C,	TSL2561_LUX_B2C,	TSL2561_LUX_M2C },
	{ TSL2561_LUX_K3C,	TSL2561_LUX_B3C,	TSL2561_LUX_M3C },
	{ TSL2561_LUX_K4C,	TSL2561_LUX_B4C,	TSL2561_LUX_M4C },
	{ TSL2561_LUX_K5C,	TSL2561_LUX_B5C,	TSL2561_LUX_M5C },
	{ TSL2561_LUX_K6C,	TSL2561_LUX_B6C,	TSL2561_LUX_M6C },
	{ TSL2561_LUX_K7C,	TSL2561_LUX_B7C,	TSL2561_LUX_M7C },
	{ TSL2561_LUX_K8C,	TSL2561_LUX_B8C,	TSL2561_LUX_M8C },
};
static const int lux_factor_table_cs_package_last_entry =
    (sizeof(lux_factor_table_cs_package) /
     sizeof(lux_factor_table_cs_package[0])) - 1;

static int
tsllux_get_lux(struct tsllux_softc *sc, uint32_t *luxp,
	       uint16_t *raw_broadband, uint16_t *raw_ir)
{
	uint32_t channel0, channel1, scale, ratio, lux = 0;
	uint16_t broadband, ir;
	uint16_t clip_threshold;
	const struct lux_factor_table_entry *table;
	int idx, last_entry, error;
	int32_t temp;

	if ((error = tsllux_get_sensor_data(sc, &broadband, &ir)) != 0)
		return (error);
	
	if (luxp == NULL) {
		/*
		 * Caller doesn't want the calculated Lux value, so
		 * don't bother calculating it.  Maybe they just want
		 * the raw sensor data?
		 */
		goto out;
	}

	/*
	 * Check to see if the sensor is saturated.  If so,
	 * just return a "max brightness" value.
	 */
	switch (sc->sc_itime) {
	case TIMING6x_INTEG_13_7ms:
		clip_threshold = TSL2561_CLIPPING_13MS;
		break;
	
	case TIMING6x_INTEG_101ms:
		clip_threshold = TSL2561_CLIPPING_101MS;
		break;
	
	case TIMING6x_INTEG_402ms:
	default:
		clip_threshold = TSL2561_CLIPPING_402MS;
		break;
	}

	if (broadband > clip_threshold || ir > clip_threshold) {
		lux = 65536;
		goto out;
	}

	/* Get correct scale factor based on integration time. */
	switch (sc->sc_itime) {
	case TIMING6x_INTEG_13_7ms:
		scale = TSL2561_LUX_CHSCALE_TINT0;
		break;
	
	case TIMING6x_INTEG_101ms:
		scale = TSL2561_LUX_CHSCALE_TINT1;
		break;
	
	case TIMING6x_INTEG_402ms:
	default:
		scale = (1 << TSL2561_LUX_CHSCALE);
	}

	/* Scale for gain. */
	if (sc->sc_gain == TIMING6x_GAIN_1X)
		scale <<= 4;
	
	/* Scale the channel values. */
	channel0 = ((uint32_t)broadband * scale) >> TSL2561_LUX_CHSCALE;
	channel1 = ((uint32_t)ir * scale) >> TSL2561_LUX_CHSCALE;

	/* Find the ratio of the channel values (ir / broadband) */
	if (channel0 != 0)
		ratio = (channel1 << (TSL2561_LUX_RATIOSCALE + 1)) / channel0;
	else
		ratio = 0;
	
	/* Round the ratio value. */
	ratio = (ratio + 1) >> 1;

	if (sc->sc_cs_package) {
		table = lux_factor_table_cs_package;
		last_entry = lux_factor_table_cs_package_last_entry;
	} else {
		table = lux_factor_table;
		last_entry = lux_factor_table_last_entry;
	}

	/*
	 * The table is arranged such that we compare <= against
	 * the key, and if all else fails, we use the last entry.
	 * The pseudo-code in the data sheet shows what's going on.
	 */
	for (idx = 0; idx < last_entry; idx++) {
		if (ratio <= table[idx].k)
			break;
	}

	temp = ((channel0 * table[idx].b) - (channel1 * table[idx].m));

	/* Do not allow negative Lux value. */
	if (temp < 0)
		temp = 0;
	
	/* Round lsb (2^(LUX_SCALE-1)) */
	temp += (1 << (TSL2561_LUX_LUXSCALE-1));

	/* Strip off fractional portion */
	lux = temp >> TSL2561_LUX_LUXSCALE;

 out:
	if (error == 0) {
		if (luxp != NULL)
			*luxp = lux;
		if (raw_broadband != NULL)
			*raw_broadband = broadband;
		if (raw_ir != NULL)
			*raw_ir = ir;
	}
	return (error);
}
