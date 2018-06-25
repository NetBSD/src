/*	$NetBSD: em3027.c,v 1.1.2.1 2018/06/25 07:25:50 pgoyette Exp $ */
/*
 * Copyright (c) 2018 Valery Ushakov
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * EM Microelectronic EM3027 RTC
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: em3027.c,v 1.1.2.1 2018/06/25 07:25:50 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/clock_subr.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/em3027reg.h>
#include <dev/sysmon/sysmonvar.h>

#if 0
#define aprint_verbose_dev	aprint_normal_dev
#define aprint_debug_dev	aprint_normal_dev
#endif


struct em3027rtc_softc {
	device_t sc_dev;

	i2c_tag_t sc_tag;
	i2c_addr_t sc_addr;

	bool sc_vlow;

	struct todr_chip_handle sc_todr;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;
};


#define EM3027_CONTROL_BASE	EM3027_ONOFF
#define EM3027_WATCH_BASE	EM3027_WATCH_SEC

struct em3027rtc_watch {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t wday;
	uint8_t mon;
	uint8_t year;
};

#define EM3027_WATCH_SIZE	(EM3027_WATCH_YEAR - EM3027_WATCH_BASE + 1)
__CTASSERT(sizeof(struct em3027rtc_watch) == EM3027_WATCH_SIZE);

#define EM3027_BASE_YEAR	1980


static int em3027rtc_match(device_t, cfdata_t, void *);
static void em3027rtc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(em3027rtc, sizeof(struct em3027rtc_softc),
    em3027rtc_match, em3027rtc_attach, NULL, NULL);


static bool em3027rtc_enable_thermometer(struct em3027rtc_softc *);
static void em3027rtc_envsys_attach(struct em3027rtc_softc *);

static int em3027rtc_gettime(struct todr_chip_handle *, struct clock_ymdhms *);
static int em3027rtc_settime(struct todr_chip_handle *, struct clock_ymdhms *);

static void em3027rtc_sme_refresh(struct sysmon_envsys *, envsys_data_t *);

static int em3027rtc_iic_exec(struct em3027rtc_softc *, i2c_op_t, uint8_t,
			      void *, size_t);

static int em3027rtc_read(struct em3027rtc_softc *, uint8_t, void *, size_t);
static int em3027rtc_write(struct em3027rtc_softc *, uint8_t, void *, size_t);

static int em3027rtc_read_byte(struct em3027rtc_softc *, uint8_t, uint8_t *);
static int em3027rtc_write_byte(struct em3027rtc_softc *, uint8_t, uint8_t);



static int
em3027rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct i2c_attach_args *ia = aux;
	uint8_t reg;
	int error;

	if (ia->ia_addr != EM3027_ADDR)
		return 0;

	/* check if the device is there */
	error = iic_acquire_bus(ia->ia_tag, 0);
	if (error)
		return 0;

	error = iic_smbus_read_byte(ia->ia_tag, ia->ia_addr,
				    EM3027_ONOFF, &reg, 0);
	iic_release_bus(ia->ia_tag, 0);
	if (error)
		return 0;

	return I2C_MATCH_ADDRESS_AND_PROBE;
}


static void
em3027rtc_attach(device_t parent, device_t self, void *aux)
{
	struct em3027rtc_softc *sc = device_private(self);
	const struct i2c_attach_args *ia = aux;
	struct ctl {
		uint8_t onoff;
		uint8_t irq_ctl;
		uint8_t irq_flags;
		uint8_t status;
	} ctl;
	int error;

	aprint_naive(": Real-time Clock and Temperature Sensor\n");
	aprint_normal(": Real-time Clock and Temperature Sensor\n");

	sc->sc_dev = self;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;


	/*
	 * Control Page registers
	 */
	error = em3027rtc_read(sc, EM3027_CONTROL_BASE, &ctl, sizeof(ctl));
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to read control page (error %d)\n", error);
		return;
	}


 	/* Status */
	aprint_debug_dev(sc->sc_dev, "status=0x%02x\n", ctl.status);

	/* Complain about low voltage but continue anyway */
	if (ctl.status & EM3027_STATUS_VLOW2) {
		aprint_error_dev(sc->sc_dev, "voltage low (VLow2)\n");
		sc->sc_vlow = true;
	}
	else if (ctl.status & EM3027_STATUS_VLOW1) {
		aprint_error_dev(sc->sc_dev, "voltage low (VLow1)\n");
		sc->sc_vlow = true;
	}

	ctl.status = EM3027_STATUS_POWER_ON;


	/* On/Off */
	aprint_debug_dev(sc->sc_dev, "on/off=0x%02x\n", ctl.onoff);

	if ((ctl.onoff & EM3027_ONOFF_SR) == 0) {
		aprint_verbose_dev(sc->sc_dev, "enabling self-recovery\n");
		ctl.onoff |= EM3027_ONOFF_SR;
	}

	if ((ctl.onoff & EM3027_ONOFF_EEREF) == 0) {
		aprint_verbose_dev(sc->sc_dev, "enabling EEPROM self-refresh\n");
		ctl.onoff |= EM3027_ONOFF_EEREF;
	}

	ctl.onoff &= ~EM3027_ONOFF_TR;

	if (ctl.onoff & EM3027_ONOFF_TI) {
		aprint_verbose_dev(sc->sc_dev, "disabling timer\n");
		ctl.onoff &= ~EM3027_ONOFF_TI;
	}

	if ((ctl.onoff & EM3027_ONOFF_WA) == 0) {
		aprint_verbose_dev(sc->sc_dev, "enabling watch\n");
		ctl.onoff |= EM3027_ONOFF_WA;
	}


	/* IRQ Control/Flags */
	if (ctl.irq_ctl != 0)
		aprint_debug_dev(sc->sc_dev,
	            "irq=0x%02x - disabling all\n", ctl.irq_ctl);
	ctl.irq_ctl = 0;
	ctl.irq_flags = 0;


	/* Write them back */
	error = em3027rtc_write(sc, EM3027_CONTROL_BASE, &ctl, sizeof(ctl));
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to write control page (error %d)\n", error);
		return;
	}


	/*
	 * Attach RTC
	 */
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = em3027rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = em3027rtc_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);


	/*
	 * Attach thermometer
	 */
	em3027rtc_envsys_attach(sc);
}


static bool
em3027rtc_enable_thermometer(struct em3027rtc_softc *sc)
{
 	uint8_t eeprom_ctl;
	int error;

	error = em3027rtc_read_byte(sc, EM3027_EEPROM_CTL, &eeprom_ctl);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to read eeprom control (error %d)\n", error);
		return false;
	}

	aprint_debug_dev(sc->sc_dev, "eeprom ctl=0x%02x\n", eeprom_ctl);
	if (eeprom_ctl & EM3027_EEPROM_THERM_ENABLE)
		return true;

	eeprom_ctl |= EM3027_EEPROM_THERM_ENABLE;
	error = em3027rtc_write_byte(sc, EM3027_EEPROM_CTL, eeprom_ctl);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to write eeprom control (error %d)\n", error);
		return false;
	}

	return true;
}


static void
em3027rtc_envsys_attach(struct em3027rtc_softc *sc)
{
	int error;

	if (!em3027rtc_enable_thermometer(sc)) {
		aprint_error_dev(sc->sc_dev, "thermometer not enabled\n");
		return;
	}

	sc->sc_sme = sysmon_envsys_create();

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = em3027rtc_sme_refresh;

	sc->sc_sensor.units =  ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.flags = 0;
	strlcpy(sc->sc_sensor.desc, "temperature", sizeof(sc->sc_sensor.desc));

	error = sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to attach sensor (error %d)\n", error);
		goto out;
	}
	
	error = sysmon_envsys_register(sc->sc_sme);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon (error %d)\n", error);
		goto out;
	}

	return;

out:
	if (error) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
	}
}


static int
em3027rtc_iic_exec(struct em3027rtc_softc *sc, i2c_op_t op, uint8_t reg,
		   void *buf, size_t len)
{
	const int flags = cold ? 0 : I2C_F_POLL;
	int error;

	error = iic_acquire_bus(sc->sc_tag, flags);
	if (error)
		return error;

	error = iic_exec(sc->sc_tag, op, sc->sc_addr,
			 &reg, 1,
			 (uint8_t *)buf, len,
			 flags);

	/* XXX: horrible hack that seems to be needed on utilite */
	if (reg == EM3027_WATCH_BASE)
		DELAY(1);

	iic_release_bus(sc->sc_tag, flags);
	return error;
}


static int
em3027rtc_read(struct em3027rtc_softc *sc, uint8_t reg, void *buf, size_t len)
{

	return em3027rtc_iic_exec(sc, I2C_OP_READ_WITH_STOP, reg, buf, len);
}


static int
em3027rtc_read_byte(struct em3027rtc_softc *sc, uint8_t reg, uint8_t *valp)
{

	return em3027rtc_read(sc, reg, valp, 1); 
}


static int
em3027rtc_write(struct em3027rtc_softc *sc, uint8_t reg, void *buf, size_t len)
{

	return em3027rtc_iic_exec(sc, I2C_OP_WRITE_WITH_STOP, reg, buf, len);
}


static int
em3027rtc_write_byte(struct em3027rtc_softc *sc, uint8_t reg, uint8_t val)
{

	return em3027rtc_write(sc, reg, &val, 1);
}


static int
em3027rtc_gettime(struct todr_chip_handle *todr, struct clock_ymdhms *dt)
{
	struct em3027rtc_softc *sc = todr->cookie;
	struct em3027rtc_watch w;
	int error;

	error = em3027rtc_read(sc, EM3027_WATCH_BASE, &w, sizeof(w));
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to read watch (error %d)\n", error);
		return error;
	}

	dt->dt_sec = bcdtobin(w.sec);
	dt->dt_min = bcdtobin(w.min);

	if (w.hour & EM3027_WATCH_HOUR_S12) {
		const int pm = w.hour & EM3027_WATCH_HOUR_PM;
		int hr;

		w.hour &= ~(EM3027_WATCH_HOUR_S12 | EM3027_WATCH_HOUR_PM);
		hr = bcdtobin(w.hour);
		if (hr == 12)
			hr = pm ? 12 : 0;
		else if (pm)
			hr += 12;

		dt->dt_hour = hr;
	}
	else {
		dt->dt_hour = bcdtobin(w.hour);
	}

	dt->dt_day = bcdtobin(w.day);
	dt->dt_wday = bcdtobin(w.wday) - 1;
	dt->dt_mon = bcdtobin(w.mon);
	dt->dt_year = bcdtobin(w.year) + EM3027_BASE_YEAR;

	return 0;
}


static int
em3027rtc_settime(struct todr_chip_handle *todr, struct clock_ymdhms *dt)
{
	struct em3027rtc_softc *sc = todr->cookie;
	struct em3027rtc_watch w;
	int error;

	w.sec = bintobcd(dt->dt_sec);
	w.min = bintobcd(dt->dt_min);
	w.hour = bintobcd(dt->dt_hour);
	w.day = bintobcd(dt->dt_day);
	w.wday = bintobcd(dt->dt_wday + 1);
	w.mon = bintobcd(dt->dt_mon);
	w.year = bintobcd(dt->dt_year - EM3027_BASE_YEAR);

	error = em3027rtc_write(sc, EM3027_WATCH_BASE, &w, sizeof(w));
	return error;
}


static void
em3027rtc_sme_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct em3027rtc_softc *sc = sme->sme_cookie;
	uint8_t status, t_raw;
	uint32_t t_uk;
	int error;

	edata->state = ENVSYS_SINVALID;

	error = em3027rtc_read_byte(sc, EM3027_STATUS, &status);
	if (error) {
		aprint_debug_dev(sc->sc_dev,
		    "failed to read status (error %d)\n", error);
		return;
	}

	if (status & (EM3027_STATUS_VLOW2 | EM3027_STATUS_VLOW1)) {
		if (!sc->sc_vlow) {
			sc->sc_vlow = true;
			aprint_error_dev(sc->sc_dev,
			    "voltage low, thermometer is disabled\n");
		}
		return;
	}
	else
		sc->sc_vlow = false;

	error = em3027rtc_read_byte(sc, EM3027_TEMP, &t_raw);
	if (error) {
		aprint_debug_dev(sc->sc_dev,
		    "failed to read temperature (error %d)\n", error);
		return;
	}


	/* convert to microkelvin */
	t_uk = ((int)t_raw + EM3027_TEMP_BASE) * 1000000 + 273150000;

	edata->value_cur = t_uk;
	edata->state = ENVSYS_SVALID;
}
