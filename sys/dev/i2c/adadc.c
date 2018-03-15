/* $NetBSD: adadc.c,v 1.2.2.2 2018/03/15 09:12:05 pgoyette Exp $ */

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

/*
 * a driver for Analog Devices AD7417 temperature sensors / ADCs
 * very much macppc only for now since we need calibaration data to make sense
 * of the ADC inputs
 * info on how to get these from FreeBSD and Linux
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adadc.c,v 1.2.2.2 2018/03/15 09:12:05 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ofw/openfirm.h>

/* commands */
#define ADADC_TEMP	0x00	/* temperature, 16bit */
#define ADADC_CONFIG	0x01	/* 8bit */
#define ADADC_THYST	0x02	/* 16bit */
#define ADADC_TOTI	0x03	/* 16bit, temperature threshold */
#define ADADC_ADC	0x04	/* 16bit, ADC value */
#define ADADC_CONFIG2	0x05	/* 8bit */

/* 
 * registers
 * ADADC_TEMP has signed temperature in C in first byte, left aligned fraction
 * in 2nd byte.
 */

#define ADADC_CFG_CHANNEL_0	0x00
#define ADADC_CFG_CHANNEL_1	0x20
#define ADADC_CFG_CHANNEL_2	0x40
#define ADADC_CFG_CHANNEL_3	0x60
#define ADADC_CFG_CHANNEL_4	0x80
#define ADADC_CFG_CHANNEL_MASK	0xe0
#define ADADC_CFG_FAULT_MASK	0x18
#define ADADC_CFG_OTI_POL	0x04	/* overtemp output polarity */
#define ADADC_CFG_INTMODE	0x02	/* interrupt mode */
#define ADADC_CFG_SHUTDOWN	0x01	/* shutdown mode */


struct adadc_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[5];
	int		sc_nsensors;
	int		sc_diode_offset, sc_diode_slope;
};

static int	adadc_match(device_t, cfdata_t, void *);
static void	adadc_attach(device_t, device_t, void *);

static void	adadc_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(adadc, sizeof(struct adadc_softc),
    adadc_match, adadc_attach, NULL, NULL);

static const char * dstemp_compats[] = {
	"ad7417",
	NULL
};

/* calibaration table from Darwin via Linux */
static int slope[5] = {0, 0, 0x0320, 0x00a0, 0x1f40};

static int
adadc_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name == NULL) {
		/*
		 * XXX
		 * this driver is pretty much useless without OF, should
		 * probably remove this
		 */
		if ((ia->ia_addr & 0x2b) == 0x2b)
			return 1;
		return 0;
	} else {
		return iic_compat_match(ia, dstemp_compats);
	}
}

static void
adadc_attach(device_t parent, device_t self, void *aux)
{
	struct adadc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	envsys_data_t *s;
	int error, ch, cpuid;
	uint32_t eeprom[40];
	char loc[256];
	int which_cpu;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": AD7417\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = adadc_sensors_refresh;
	sc->sc_nsensors = 0;

	/*
	 * XXX
	 * without OpenFirmware telling us how to interpret the ADC inputs we
	 * should probably just expose the temperature and four ENVSYS_INTEGERs
	 */
	which_cpu = 0;
	ch = OF_child(ia->ia_cookie);
	while (ch != 0) {
		if (OF_getprop(ch, "location", loc, 32) > 0) {
			int reg = 0;
			OF_getprop(ch, "reg", &reg, sizeof(reg));
			s = &sc->sc_sensors[sc->sc_nsensors];
			/*
			 * this setup matches my 2x 2.5GHz PCI-X G5, Linux and
			 * FreeBSD hardcode these as well so we should be safe
			 */
			switch (reg) {
			case 0:
				if (strstr(loc, "CPU B") != NULL)
					which_cpu = 1;
				/* FALLTHROUGH */		
			case 1:
				s->units = ENVSYS_STEMP;
				break;
			case 2:
			case 4:
				s->units = ENVSYS_SAMPS;
				break;
			case 3:
				s->units = ENVSYS_SVOLTS_DC;
				break;
			default:
				s->units = ENVSYS_INTEGER;
			}
			strncpy(s->desc, loc, sizeof(s->desc));
			s->private = reg;
			sysmon_envsys_sensor_attach(sc->sc_sme, s);
			sc->sc_nsensors++;
		}
		ch = OF_peer(ch);
	}
	aprint_debug_dev(self, "monitoring CPU %d\n", which_cpu);
	if (which_cpu == 0) {
		cpuid = OF_finddevice("/u3/i2c/cpuid@a0");
	} else
		cpuid = OF_finddevice("/u3/i2c/cpuid@a2");
	error = OF_getprop(cpuid, "cpuid", eeprom, sizeof(eeprom));
	if (error >= 0) {
		sc->sc_diode_slope = eeprom[0x11] >> 16;
		sc->sc_diode_offset = (int16_t)(eeprom[0x11] & 0xffff) << 12;
	}
	sysmon_envsys_register(sc->sc_sme);
}

static void
adadc_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct adadc_softc *sc = sme->sme_cookie;
	uint8_t cmd = ADADC_CONFIG;
	int16_t data = 0;
	uint16_t rdata;
	uint8_t cfg;
	int error, ch;

		
	iic_acquire_bus(sc->sc_i2c, 0);
	if (edata->private > 0) {
		error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &cfg, 1, 0);
		ch = cfg >> 5;
		/* are we on the right channel already? */
		if (ch != edata->private) {
			/* nope */
			cfg &= 0x1f;
			cfg |= (edata->private & 7) << 5;
			iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
			    sc->sc_addr, &cmd, 1, &cfg, 1, 0);
		}
		cmd = ADADC_ADC;
		/* now read the ADC register */
		error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &rdata, 2, 0);
		rdata = be16toh(rdata) >> 6;
		if (edata->private == 1) {
			int temp;
			temp = (rdata * sc->sc_diode_slope + 
				sc->sc_diode_offset) >> 2;
			/* 16.16 fixed point */
			edata->value_cur = (temp >> 12) * 62500 + 273150000;
		} else {
			/*
			 * the input is 10bit, so converting to 8.4 fixed point
			 * is more than enough
			 */
			int temp = rdata * slope[edata->private];
			edata->value_cur = (temp >> 12) * 62500;
		}
	} else {
		cmd = ADADC_TEMP;
		/* just read the temperature register */
		error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &data, 2, 0);
		/* 8.2 bit fixed point Celsius -> microkelvin */
		edata->value_cur = ((data >> 6) * 250000) + 273150000;
	}
	iic_release_bus(sc->sc_i2c, 0);

	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->state = ENVSYS_SVALID;
	}
}
