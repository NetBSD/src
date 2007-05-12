/*	$NetBSD: it.c,v 1.6.2.2 2007/05/12 17:32:22 snj Exp $	*/
/*	$OpenBSD: it.c,v 1.19 2006/04/10 00:57:54 deraadt Exp $	*/

/*
 * Copyright (c) 2006-2007 Juan Romero Pardines <juan@xtrarom.org>
 * Copyright (c) 2003 Julien Bordet <zejames@greyhats.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITD TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITD TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for the iTE IT87{05,12}F hardware monitor.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: it.c,v 1.6.2.2 2007/05/12 17:32:22 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/envsys.h>
#include <sys/time.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/isa/itvar.h>

#define IT_VOLTSTART_IDX	2	/* voltage start index */
#define IT_FANSTART_IDX		7	/* fan start index */

#if defined(ITDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

/*
 * IT87-compatible chips can typically measure voltages up to 4.096 V.
 * To measure higher voltages the input is attenuated with (external)
 * resistors.  Negative voltages are measured using a reference
 * voltage.  So we have to convert the sensor values back to real
 * voltages by applying the appropriate resistor factor.
 */
#define RFACT_NONE	10000
#define RFACT(x, y)	(RFACT_NONE * ((x) + (y)) / (y))

/* autoconf(9) functions */
static int  it_isa_match(struct device *, struct cfdata *, void *);
static void it_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(it_isa, sizeof(struct it_softc),
    it_isa_match, it_isa_attach, NULL, NULL);

/* driver functions */
static int it_check(bus_space_tag_t, int);
static uint8_t it_readreg(struct it_softc *, int);
static void it_writereg(struct it_softc *, int, int);

/* envsys(9) glue */
static void it_setup_sensors(struct it_softc *);
static void it_refresh_temp(struct it_softc *, envsys_tre_data_t *);
static void it_refresh_volts(struct it_softc *, envsys_tre_data_t *);
static void it_refresh_fans(struct it_softc *, envsys_tre_data_t *);
static int it_gtredata(struct sysmon_envsys *, envsys_tre_data_t *);
static int it_streinfo(struct sysmon_envsys *, envsys_basic_info_t *);

/* voltage sensors used */
static const int it_sensorvolt[] = {
	IT_SENSORVCORE0,
	IT_SENSORV33,
	IT_SENSORV5,
	IT_SENSORV12,
	IT_SENSORVBAT
};

/* rfact values for voltage sensors */
static const int it_vrfact[] = {
	RFACT_NONE,	/* VCORE */
	RFACT_NONE,	/* +3.3V */
	RFACT(68, 100),	/* +5V   */
	RFACT(30, 10),	/* +12V  */
	RFACT_NONE	/* VBAT  */
};

static int
it_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	int rv = 0;

	/* Must supply an address */
	if (ia->ia_nio < 1)
		return 0;

	if (ISA_DIRECT_CONFIG(ia))
		return 0;

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	rv = it_check(ia->ia_iot, ia->ia_io[0].ir_addr);

	if (rv) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_size = 8;
		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
	}

	return rv;
}

static void
it_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct it_softc *sc = (struct it_softc *)self;
	struct isa_attach_args *ia = aux;
	int i;
	uint8_t idr, cr;

	ia->ia_iot = sc->sc_iot;

	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr, 8, 0,
	    &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	idr = it_readreg(sc, IT_COREID);
	if (idr == IT_REV_8712)
		aprint_normal(": IT8712F Hardware monitor\n");
	else {
		idr = it_readreg(sc, IT_VENDORID);
		if (idr == IT_REV_8705)
			aprint_normal(": IT8705F Hardware monitor\n");
		else
			aprint_normal(": iTE unknown vendor id (0x%x)\n", idr);
	}

	/* Activate monitoring */
	cr = it_readreg(sc, IT_CONFIG);
	SET(cr, 0x01);
	SET(cr, 0x08);
	it_writereg(sc, IT_CONFIG, cr);

	/* Initialize sensors */
	for (i = 0; i < IT_NUM_SENSORS; ++i) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = ENVSYS_WARN_OK;
	}

	it_setup_sensors(sc);

	/*
	 * Hook into the system monitor.
	 */
	sc->sc_sysmon.sme_ranges = it_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = it_gtredata;
	sc->sc_sysmon.sme_streinfo = it_streinfo;
	sc->sc_sysmon.sme_nsensors = IT_NUM_SENSORS;
	sc->sc_sysmon.sme_envsys_version = 1000;
	sc->sc_sysmon.sme_flags = 0;
	
	if (sysmon_envsys_register(&sc->sc_sysmon))
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

}

static int
it_check(bus_space_tag_t iot, int base)
{
	bus_space_handle_t ioh;
	int rv = 0;
	uint8_t cr0, cr1;
 
	if (bus_space_map(iot, base, 8, 0, &ioh))
		return 0;

	bus_space_write_1(iot, ioh, IT_ADDR, IT_RES48);
	cr0 = bus_space_read_1(iot, ioh, IT_DATA);
	bus_space_write_1(iot, ioh, IT_ADDR, IT_RES52);
	cr1 = bus_space_read_1(iot, ioh, IT_DATA);

	if (cr0 == IT_RES48_DEFAULT && cr1 == IT_RES52_DEFAULT)
		rv = 1;

	bus_space_unmap(iot, ioh, 8);
	return rv;
}

static uint8_t
it_readreg(struct it_softc *sc, int reg)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, IT_ADDR, reg);
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, IT_DATA);
}

static void
it_writereg(struct it_softc *sc, int reg, int val)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, IT_ADDR, reg);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, IT_DATA, val);
}

#define COPYDESCR(x, y)				\
	do {					\
		strlcpy((x), (y), sizeof(x));	\
	} while (0)

static void
it_setup_sensors(struct it_softc *sc)
{
	int i;

	/* temperatures */
	for (i = 0; i < IT_VOLTSTART_IDX; i++) {
		sc->sc_data[i].units = ENVSYS_STEMP;
		sc->sc_info[i].units = ENVSYS_STEMP;
	}

	COPYDESCR(sc->sc_info[0].desc, "CPU Temp");
	COPYDESCR(sc->sc_info[1].desc, "System Temp");

	/* voltages */
	for (i = IT_VOLTSTART_IDX; i < IT_FANSTART_IDX; i++) {
		sc->sc_data[i].units = ENVSYS_SVOLTS_DC;
		sc->sc_info[i].units = ENVSYS_SVOLTS_DC;
	}

	COPYDESCR(sc->sc_info[2].desc, "VCORE_A");
	COPYDESCR(sc->sc_info[3].desc, "+3.3V");
	COPYDESCR(sc->sc_info[4].desc, "+5V");
	COPYDESCR(sc->sc_info[5].desc, "+12V");
	COPYDESCR(sc->sc_info[6].desc, "VBAT");

	/* fans */
	for (i = IT_FANSTART_IDX; i < IT_NUM_SENSORS; i++) {
		sc->sc_data[i].units = ENVSYS_SFANRPM;
		sc->sc_info[i].units = ENVSYS_SFANRPM;
	}

	COPYDESCR(sc->sc_info[7].desc, "CPU Fan");
	COPYDESCR(sc->sc_info[8].desc, "System Fan");
}
#undef COPYDESCR

static void
it_refresh_temp(struct it_softc *sc, envsys_tre_data_t *tred)
{
	int sdata;

	sdata = it_readreg(sc, IT_SENSORTEMPBASE + tred->sensor);
	DPRINTF(("sdata[temp%d] 0x%x\n", tred->sensor, sdata));
	/* Convert temperature to Fahrenheit degres */
	sc->sc_data[tred->sensor].cur.data_us = sdata * 1000000 + 273150000;
}

static void
it_refresh_volts(struct it_softc *sc, envsys_tre_data_t *tred)
{
	int i, sdata;

	i = tred->sensor - IT_VOLTSTART_IDX;

	sdata = it_readreg(sc, it_sensorvolt[i]);
	DPRINTF(("sdata[volt%d] 0x%x\n", i, sdata));
	/* voltage returned as (mV >> 4) */
	sc->sc_data[tred->sensor].cur.data_s = (sdata << 4);
	/* rfact is (factor * 10^4) */
	sc->sc_data[tred->sensor].cur.data_s *= it_vrfact[i];
	/* division by 10 gets us back to uVDC */
	sc->sc_data[tred->sensor].cur.data_s /= 10;
	sc->sc_info[tred->sensor].rfact =
	    sc->sc_data[tred->sensor].cur.data_s;
}

static void
it_refresh_fans(struct it_softc *sc, envsys_tre_data_t *tred)
{
	int i, sdata, divisor, odivisor, ndivisor;

	i = tred->sensor - IT_FANSTART_IDX;
	odivisor = ndivisor = divisor = it_readreg(sc, IT_FAN);

	if ((sdata = it_readreg(sc, IT_SENSORFANBASE + i)) == 0xff) {
		if (i == 2)
			ndivisor ^= 0x40;
		else {
			ndivisor &= ~(7 << (i * 3));
			ndivisor |= ((divisor + 1) & 7) << (i * 3);
		}
	} else {
		if (i == 2)
			divisor = divisor & 1 ? 3 : 1;
		sc->sc_data[tred->sensor].cur.data_us =
		    1350000 / (sdata << (divisor & 7));
	}

	DPRINTF(("sdata[fan%d] 0x%x div: 0x%x\n", i, sdata, divisor));
	if (ndivisor != odivisor)
		it_writereg(sc, IT_FAN, ndivisor);
}

static int              
it_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct it_softc *sc = sme->sme_cookie;

	if (tred->sensor < IT_VOLTSTART_IDX)
		it_refresh_temp(sc, tred);
	else if (tred->sensor >= IT_VOLTSTART_IDX &&
	         tred->sensor < IT_FANSTART_IDX)
		it_refresh_volts(sc, tred);
	else if (tred->sensor >= IT_FANSTART_IDX)
		it_refresh_fans(sc, tred);
        else
		return 0;

	*tred = sc->sc_data[tred->sensor]; 
	return 0;
}

static int              
it_streinfo(struct sysmon_envsys *sme, envsys_basic_info_t *info)
{                                       
	struct it_softc *sc = sme->sme_cookie;
	int divisor, i;
	uint8_t sdata;          
                        
	if (sc->sc_info[info->sensor].units == ENVSYS_SVOLTS_DC)
		sc->sc_info[info->sensor].rfact = info->rfact;
	else {  
		if (sc->sc_info[info->sensor].units == ENVSYS_SFANRPM) {
			if (info->rpms == 0) {
				info->validflags = 2;
				return 0;
			}

			/* write back nominal FAN speed */
			sc->sc_info[info->sensor].rpms = info->rpms;

			/* 153 is the nominal FAN speed value */
			divisor = 1350000 / (info->rpms * 153);

			/* ...but we need lg(divisor) */
			for (i = 0; i < 7; i++) {
				if (divisor <= (1 << i))
					break;
			}
			divisor = i;

			sdata = it_readreg(sc, IT_FAN);
			/*
			 * FAN1 div is in bits <0:2>, FAN2 is in <3:5>
			 * and FAN3 is in <6>, if set divisor is 8, else 2.
			 */
			switch (info->sensor) {
			case 10: /* FAN1 */
				sdata = (sdata & 0xf8) | divisor;
				break;
			case 11: /* FAN2 */
				sdata = (sdata & 0xc7) | divisor << 3;
				break;
			default: /* FAN3 */
				if (divisor > 2)
					sdata = sdata & 0xbf;
				else 
					sdata = sdata | 0x40;
				break; 
			}
			it_writereg(sc, IT_FAN, sdata);
		}
		strlcpy(sc->sc_info[info->sensor].desc, info->desc,
		    sizeof(sc->sc_info[info->sensor].desc));
		info->validflags = ENVSYS_FVALID;
	}
	return 0;
}
