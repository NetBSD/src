/*	$NetBSD: it.c,v 1.4 2006/06/11 18:15:17 xtraeme Exp $	*/
/*	$OpenBSD: it.c,v 1.19 2006/04/10 00:57:54 deraadt Exp $	*/

/*
 * Copyright (c) 2006 Juan Romero Pardines <juan@xtrarom.org>
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
__KERNEL_RCSID(0, "$NetBSD: it.c,v 1.4 2006/06/11 18:15:17 xtraeme Exp $");

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

#if defined(ITDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

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
static void it_setup_volt(struct it_softc *, int, int);
static void it_setup_temp(struct it_softc *, int, int);
static void it_setup_fan(struct it_softc *, int, int);
static void it_refresh_temp(struct it_softc *, envsys_tre_data_t *);
static void it_refresh_volts(struct it_softc *, envsys_tre_data_t *,
    envsys_basic_info_t *);
static void it_refresh_fans(struct it_softc *, envsys_tre_data_t *);
static int it_gtredata(struct sysmon_envsys *, envsys_tre_data_t *);
static int it_streinfo(struct sysmon_envsys *, envsys_basic_info_t *);


static int
it_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	int iobase, rv = 0;

	/* Must supply an address */
	if (ia->ia_nio < 1)
		return 0;

	if (ISA_DIRECT_CONFIG(ia))
		return 0;

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	iobase = ia->ia_io[0].ir_addr;
	rv = it_check(ia->ia_iot, iobase);

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
	struct it_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase, i;
	uint8_t idr, cr;

	iot = ia->ia_iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	iobase = ia->ia_io[0].ir_addr;

	if (bus_space_map(iot, iobase, 8, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	/* Indicate we have never read the registers */
	timerclear(&sc->lastread);

	idr = it_readreg(sc, IT_COREID);
	if (idr == IT_REV_8712)
		printf(": IT8712F Hardware monitor\n");
	else {
		idr = it_readreg(sc, IT_VENDORID);
		if (idr == IT_REV_8705)
			printf(": IT8705F Hardware monitor\n");
		else
			printf("iTE unknown vendor id: 0x%x\n", idr);
	}

	it_setup_fan(sc, 0, 3);
	it_setup_volt(sc, 3, 9);
	it_setup_temp(sc, 12, 3);

	/* Activate monitoring */
	cr = it_readreg(sc, IT_CONFIG);
	cr |= 0x01 | 0x08;
	it_writereg(sc, IT_CONFIG, cr);

	/* Initialize sensors */
	for (i = 0; i < IT_NUM_SENSORS; ++i) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = ENVSYS_WARN_OK;
	}

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

static void
it_setup_volt(struct it_softc *sc, int start, int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		sc->sc_data[start + i].units = ENVSYS_SVOLTS_DC;
		sc->sc_info[start + i].units = ENVSYS_SVOLTS_DC;
	}

	sc->sc_info[start + 0].rfact = 10000;
	snprintf(sc->sc_info[start + 0].desc, sizeof(sc->sc_info[0].desc),
	    "VCORE_A");
	sc->sc_info[start + 1].rfact = 10000;
	snprintf(sc->sc_info[start + 1].desc, sizeof(sc->sc_info[1].desc),
	    "VCORE_B");
	sc->sc_info[start + 2].rfact = 10000;
	snprintf(sc->sc_info[start + 2].desc, sizeof(sc->sc_info[2].desc),
	    "+3.3V");
	sc->sc_info[start + 3].rfact = 16800;
	snprintf(sc->sc_info[start + 3].desc, sizeof(sc->sc_info[3].desc),
	    "+5V");
	sc->sc_info[start + 4].rfact = 40000;
	snprintf(sc->sc_info[start + 4].desc, sizeof(sc->sc_info[4].desc),
	    "+12V");
	sc->sc_info[start + 5].rfact = 40000;
	snprintf(sc->sc_info[start + 5].desc, sizeof(sc->sc_info[5].desc),
	    "-12V");
	sc->sc_info[start + 6].rfact = 16800;
	snprintf(sc->sc_info[start + 6].desc, sizeof(sc->sc_info[6].desc),
	    "-5V");
	sc->sc_info[start + 7].rfact = 16800;
	snprintf(sc->sc_info[start + 7].desc, sizeof(sc->sc_info[7].desc),
	    "+5VSB");
	sc->sc_info[start + 8].rfact = 10000;
	snprintf(sc->sc_info[start + 8].desc, sizeof(sc->sc_info[8].desc),
	    "VBAT");
}

static void
it_setup_temp(struct it_softc *sc, int start, int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		sc->sc_data[start + i].units = ENVSYS_STEMP;
		sc->sc_info[start + i].units = ENVSYS_STEMP;
	}
	snprintf(sc->sc_info[start + 0].desc,
	    sizeof(sc->sc_info[start + 0].desc), "CPU Temp");
	snprintf(sc->sc_info[start + 1].desc,
	    sizeof(sc->sc_info[start + 1].desc), "Chassis Temp");
	snprintf(sc->sc_info[start + 2].desc,
	    sizeof(sc->sc_info[start + 2].desc), "External Temp");
}

static void
it_setup_fan(struct it_softc *sc, int start, int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		sc->sc_data[start + i].units = ENVSYS_SFANRPM;
		sc->sc_info[start + i].units = ENVSYS_SFANRPM;
	}
	snprintf(sc->sc_info[start + 0].desc,
	    sizeof(sc->sc_info[start + 0].desc), "CPU Fan");
	snprintf(sc->sc_info[start + 1].desc,
	    sizeof(sc->sc_info[start + 1].desc), "Chassis Fan");
	snprintf(sc->sc_info[start + 2].desc,
	    sizeof(sc->sc_info[start + 2].desc), "External Fan");
}

static void
it_refresh_temp(struct it_softc *sc, envsys_tre_data_t *tred)
{
	int i, sdata;

	for (i = 0; i < 3; i++) {
		sdata = it_readreg(sc, IT_SENSORTEMPBASE + i);
		DPRINTF(("sdata[temp%d] 0x%x\n", i, sdata));
		/* Convert temperature to Fahrenheit degres */
		tred[i].cur.data_us = sdata * 1000000 + 273150000;
	}
}

static void
it_refresh_volts(struct it_softc *sc, envsys_tre_data_t *tred,
    envsys_basic_info_t *info)
{
	int i, sdata;

	for (i = 0; i < 9; i++) {
		sdata = it_readreg(sc, IT_SENSORVOLTBASE + i);
		DPRINTF(("sdata[volt%d] 0x%x\n", i, sdata));
		/* voltage returned as (mV >> 4) */
		tred[i].cur.data_s = (sdata << 4);
		/* rfact is (factor * 10^4) */
		tred[i].cur.data_s *= info[i].rfact;
		/*
		 * xtraeme: looks like on my motherboard, these two values
		 * are null, so disable them.
		 */
		if (tred[i].cur.data_s != 0) {
			if (i == 5 || i == 6)
				tred[i].cur.data_s -=
			    	    (info[i].rfact - 10000) * IT_VREF;
		}
		/* division by 10 gets us back to uVDC */
		tred[i].cur.data_s /= 10;

	}
}

static void
it_refresh_fans(struct it_softc *sc, envsys_tre_data_t *tred)
{
	int i, sdata, divisor, odivisor, ndivisor;

	odivisor = ndivisor = divisor = it_readreg(sc, IT_FAN);
	for (i = 0; i < 3; i++, divisor >>= 3) {
		if ((sdata = it_readreg(sc, IT_SENSORFANBASE + i)) == 0xff) {
			if (i == 2)
				ndivisor ^= 0x40;
			else {
				ndivisor &= ~(7 << (i * 3));
				ndivisor |= ((divisor + 1) & 7) << (i * 3);
			}
		} else if (sdata == 0) {
			tred[i].cur.data_us = 0;
			sc->sc_data[i].validflags &=
			    (ENVSYS_FVALID|ENVSYS_FCURVALID);
			sc->sc_info[i].validflags &= ENVSYS_FVALID;
		} else {
			if (i == 2)
				divisor = divisor & 1 ? 3 : 1;
			tred[i].cur.data_us =
			    1350000 / (sdata << (divisor & 7));
		}
		DPRINTF(("sdata[%d] 0x%x div: 0x%x\n", i, sdata, divisor));
	}
	if (ndivisor != odivisor)
		it_writereg(sc, IT_FAN, ndivisor);
}

static int              
it_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct it_softc *sc = sme->sme_cookie;
	static const struct timeval onepointfive = { 0, 500000 };
	struct timeval tv, utv;

	/* read new values at most once every 0.5 seconds */
	getmicrouptime(&utv);
	timeradd(&sc->lastread, &onepointfive, &tv);
	if (timercmp(&utv, &tv, >)) {
		sc->lastread  = utv;
		/* Refresh our stored data for every sensor */
		it_refresh_temp(sc, &sc->sc_data[12]); 
		it_refresh_volts(sc, &sc->sc_data[3], &sc->sc_info[3]);
		it_refresh_fans(sc, &sc->sc_data[0]);
	}
        
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
