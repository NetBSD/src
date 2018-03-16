/* $NetBSD: fcu.c,v 1.2 2018/03/16 22:11:53 macallan Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fcu.c,v 1.2 2018/03/16 22:11:53 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kthread.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ofw/openfirm.h>

//#define FCU_DEBUG
#ifdef FCU_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

/* FCU registers, from OpenBSD's fcu.c */
#define FCU_FAN_FAIL	0x0b		/* fans states in bits 0<1-6>7 */
#define FCU_FAN_ACTIVE	0x0d
#define FCU_FANREAD(x)	0x11 + (x)*2
#define FCU_FANSET(x)	0x10 + (x)*2
#define FCU_PWM_FAIL	0x2b
#define FCU_PWM_ACTIVE	0x2d
#define FCU_PWMREAD(x)	0x30 + (x)*2

#define FCU_MAX_FANS 10

typedef struct _fcu_zone {
	bool (*filter)(const envsys_data_t *);
	int nfans;
	int fans[FCU_MAX_FANS];
	int threshold;
} fcu_zone_t; 

typedef struct _fcu_fan {
	int target;
	int reg;
	int base_rpm, max_rpm;
	int step;
	int duty;	/* for pwm fans */
} fcu_fan_t;

#define FCU_ZONE_CPU_A		0
#define FCU_ZONE_CPU_B		1
#define FCU_ZONE_CASE		2
#define FCU_ZONE_DRIVEBAY	3
#define FCU_ZONE_COUNT		4

struct fcu_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[32];
	int		sc_nsensors;
	fcu_zone_t	sc_zones[FCU_ZONE_COUNT];
	fcu_fan_t	sc_fans[FCU_MAX_FANS];
	int		sc_nfans;
	lwp_t		*sc_thread;
	bool		sc_dying, sc_pwm;
	uint8_t		sc_eeprom0[160];
	uint8_t		sc_eeprom1[160];
};

static int	fcu_match(device_t, cfdata_t, void *);
static void	fcu_attach(device_t, device_t, void *);

static void	fcu_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);

static bool is_cpu_a(const envsys_data_t *);
static bool is_cpu_b(const envsys_data_t *);
static bool is_case(const envsys_data_t *);
static bool is_drive(const envsys_data_t *);

static void fcu_set_fan_rpm(struct fcu_softc *, fcu_fan_t *, int);
static void fcu_adjust_zone(struct fcu_softc *, int);
static void fcu_adjust(void *);

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
	int have_eeprom1 = 1;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": Fan Control Unit\n");

	if (get_cpuid(0, sc->sc_eeprom0) < 160) {
		/*
		 * XXX this should never happen, we depend on the EEPROM for
		 * calibration data to make sense of temperature and voltage
		 * sensors elsewhere, and fan parameters here.
		 */
		aprint_error_dev(self, "no EEPROM data for CPU 0\n");
		return;
	}
	if (get_cpuid(1, sc->sc_eeprom1) < 160)
		have_eeprom1 = 0;

	/* init zones */
	sc->sc_zones[FCU_ZONE_CPU_A].filter = is_cpu_a;
	sc->sc_zones[FCU_ZONE_CPU_A].threshold = 45;
	sc->sc_zones[FCU_ZONE_CPU_A].nfans = 0;
	sc->sc_zones[FCU_ZONE_CPU_B].filter = is_cpu_b;
	sc->sc_zones[FCU_ZONE_CPU_B].threshold = 45;
	sc->sc_zones[FCU_ZONE_CPU_B].nfans = 0;
	sc->sc_zones[FCU_ZONE_CASE].filter = is_case;
	sc->sc_zones[FCU_ZONE_CASE].threshold = 50;
	sc->sc_zones[FCU_ZONE_CASE].nfans = 0;
	sc->sc_zones[FCU_ZONE_DRIVEBAY].filter = is_drive;
	sc->sc_zones[FCU_ZONE_DRIVEBAY].threshold = 30;
	sc->sc_zones[FCU_ZONE_DRIVEBAY].nfans = 0;

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = fcu_sensors_refresh;

	sc->sc_sensors[0].units = ENVSYS_SFANRPM;
	sc->sc_sensors[1].state = ENVSYS_SINVALID;
	sc->sc_nfans = 0;

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

		if (s->units == ENVSYS_SFANRPM) {
			fcu_fan_t *fan = &sc->sc_fans[sc->sc_nfans];
			uint8_t *eeprom = NULL;
			uint16_t rmin, rmax;

			if (strstr(descr, "CPU A") != NULL)
				eeprom = sc->sc_eeprom0;
			if (strstr(descr, "CPU B") != NULL) {
				/*
				 * XXX
				 * this should never happen
				 */
				if (have_eeprom1 == 0) {
					eeprom = sc->sc_eeprom0;
				} else
					eeprom = sc->sc_eeprom1;
			}

			fan->reg = reg;
			fan->target = 0;
			fan->duty = 0x80;

			/* speed settings from EEPROM */
			if (strstr(descr, "PUMP") != NULL) {
				KASSERT(eeprom != NULL);
				memcpy(&rmin, &eeprom[0x54], 2);
				memcpy(&rmax, &eeprom[0x56], 2);
				fan->base_rpm = rmin;
				fan->max_rpm = rmax;
				fan->step = (rmax - rmin) / 30;
			} else if (strstr(descr, "INTAKE") != NULL) {
				KASSERT(eeprom != NULL);
				memcpy(&rmin, &eeprom[0x4c], 2);
				memcpy(&rmax, &eeprom[0x5e], 2);
				fan->base_rpm = rmin;
				fan->max_rpm = rmax;
				fan->step = (rmax - rmin) / 30;
			} else if (strstr(descr, "EXHAUST") != NULL) {
				KASSERT(eeprom != NULL);
				memcpy(&rmin, &eeprom[0x50], 2);
				memcpy(&rmax, &eeprom[0x52], 2);
				fan->base_rpm = rmin;
				fan->max_rpm = rmax;
				fan->step = (rmax - rmin) / 30;
			} else if (strstr(descr, "DRIVE") != NULL ) {
				fan->base_rpm = 1000;
				fan->max_rpm = 3000;
				fan->step = 100;
			} else {
				fan->base_rpm = 1000;
				fan->max_rpm = 3000;
				fan->step = 100;
			}

			/* now stuff them into zones */
			if (strstr(descr, "CPU A") != NULL) {
				fcu_zone_t *z = &sc->sc_zones[FCU_ZONE_CPU_A];
				z->fans[z->nfans] = sc->sc_nfans;
				z->nfans++;
			} else if (strstr(descr, "CPU B") != NULL) {
				fcu_zone_t *z = &sc->sc_zones[FCU_ZONE_CPU_B];
				z->fans[z->nfans] = sc->sc_nfans;
				z->nfans++;
			} else if ((strstr(descr, "BACKSIDE") != NULL) ||
				   (strstr(descr, "SLOT") != NULL))  {
				fcu_zone_t *z = &sc->sc_zones[FCU_ZONE_CASE];
				z->fans[z->nfans] = sc->sc_nfans;
				z->nfans++;
			} else if (strstr(descr, "DRIVE") != NULL) {
				fcu_zone_t *z = &sc->sc_zones[FCU_ZONE_DRIVEBAY];
				z->fans[z->nfans] = sc->sc_nfans;
				z->nfans++;
			}
			sc->sc_nfans++;
		}
		sysmon_envsys_sensor_attach(sc->sc_sme, s);
		sc->sc_nsensors++;
next:
		ch = OF_peer(ch);
	}		
	sysmon_envsys_register(sc->sc_sme);

	sc->sc_dying = FALSE;
	kthread_create(PRI_NONE, 0, curcpu(), fcu_adjust, sc, &sc->sc_thread,
	    "fan control"); 
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

static bool
is_cpu_a(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	if (strstr(edata->desc, "CPU A") != NULL)
		return TRUE;
	return false;
}

static bool
is_cpu_b(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	if (strstr(edata->desc, "CPU B") != NULL)
		return TRUE;
	return false;
}

static bool
is_case(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	if ((strstr(edata->desc, "MLB") != NULL) ||
	    (strstr(edata->desc, "BACKSIDE") != NULL) ||
	    (strstr(edata->desc, "U3") != NULL))
		return TRUE;
	return false;
}

static bool
is_drive(const envsys_data_t *edata)
{
	if (edata->units != ENVSYS_STEMP)
		return false;
	if (strstr(edata->desc, "DRIVE") != NULL)
		return TRUE;
	return false;
}

static void
fcu_set_fan_rpm(struct fcu_softc *sc, fcu_fan_t *f, int speed)
{
	int error;
	uint8_t cmd;

	if (f->reg < 0x30) {
		uint16_t data;
		/* simple rpm fan, just poke the register */

		if (f->target == speed) return;
		speed = min(speed, f->max_rpm);
		speed = max(speed, f->base_rpm);
		iic_acquire_bus(sc->sc_i2c, 0);
		cmd = f->reg;
		data = (speed << 3);
		error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &data, 2, 0);
		iic_release_bus(sc->sc_i2c, 0);
	} else {
		int diff;
		int nduty = f->duty;
		uint16_t data;
		/* pwm fan, measure speed, then adjust duty cycle */
		DPRINTF("pwm fan ");
		iic_acquire_bus(sc->sc_i2c, 0);
		cmd = f->reg + 1;
		error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &data, 2, 0);
		data = data >> 3;
		diff = data - speed;
		DPRINTF("d %d s %d t %d diff %d ", f->duty, data, speed, diff);
		if (diff > 100) {
			nduty = max(20, nduty - 1);
		}
		if (diff < -100) {
			nduty = min(0xd0, nduty + 1);
		}
		cmd = f->reg;
		DPRINTF("%s nduty %d", __func__, nduty);
		if (nduty != f->duty) {
			uint8_t arg = nduty;
			error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
			    sc->sc_addr, &cmd, 1, &arg, 1, 0);
			f->duty = nduty;
			sc->sc_pwm = TRUE;

		}
		iic_release_bus(sc->sc_i2c, 0);
		DPRINTF("ok\n");
	}
	if (error) printf("boo\n");
	f->target = speed;
}

static void
fcu_adjust_zone(struct fcu_softc *sc, int which)
{
	fcu_zone_t *z = &sc->sc_zones[which];
	fcu_fan_t *f;
	int temp, i, speed, diff;
	

	if (z->nfans <= 0)
		return;

	temp = sysmon_envsys_get_max_value(z->filter, true);
	if (temp == 0) {
		/* no sensor data - leave fan alone */
		DPRINTF("nodata\n");
		return;
	}

	temp = (temp - 273150000) / 1000000;
	diff = (temp - z->threshold);

	/* now adjust each fan to the new duty cycle */
	for (i = 0; i < z->nfans; i++) {
		if (z->fans[i] > 8) {
			printf("wtf?!\n");
			continue;
		}
		f = &sc->sc_fans[z->fans[i]];
		speed = f->base_rpm + diff * f->step;
		fcu_set_fan_rpm(sc, f, speed);
	}
}

static void
fcu_adjust(void *cookie)
{
	struct fcu_softc *sc = cookie;
	int i;
	uint8_t cmd, data;

	while (!sc->sc_dying) {
		/* poke the FCU so we don't go 747 */
		iic_acquire_bus(sc->sc_i2c, 0);
		cmd = FCU_FAN_ACTIVE;
		iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &data, 1, 0);
		iic_release_bus(sc->sc_i2c, 0);
		sc->sc_pwm = FALSE;
		for (i = 0; i < FCU_ZONE_COUNT; i++)
			fcu_adjust_zone(sc, i);
		kpause("fanctrl", true, mstohz(sc->sc_pwm ? 2000 : 30000), NULL);
	}
	kthread_exit(0);
}
