/*	$NetBSD: nslm7x.c,v 1.18 2004/04/22 00:17:11 itojun Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Squier.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: nslm7x.c,v 1.18 2004/04/22 00:17:11 itojun Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/time.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/nslm7xvar.h>

#include <machine/intr.h>
#include <machine/bus.h>

#if defined(LMDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

const struct envsys_range lm_ranges[] = {	/* sc->sensors sub-intervals */
					/* for each unit type */
	{ 7, 7,    ENVSYS_STEMP   },
	{ 8, 10,   ENVSYS_SFANRPM },
	{ 1, 0,    ENVSYS_SVOLTS_AC },	/* None */
	{ 0, 6,    ENVSYS_SVOLTS_DC },
	{ 1, 0,    ENVSYS_SOHMS },	/* None */
	{ 1, 0,    ENVSYS_SWATTS },	/* None */
	{ 1, 0,    ENVSYS_SAMPS }	/* None */
};


static void setup_fan __P((struct lm_softc *, int, int));
static void setup_temp __P((struct lm_softc *, int, int));
static void wb_setup_volt __P((struct lm_softc *));

int lm_match __P((struct lm_softc *));
int wb_match __P((struct lm_softc *));
int def_match __P((struct lm_softc *));
void lm_common_match __P((struct lm_softc *));
static int lm_generic_banksel __P((struct lm_softc *, int));

static void generic_stemp __P((struct lm_softc *, struct envsys_tre_data *));
static void generic_svolt __P((struct lm_softc *, struct envsys_tre_data *,
    struct envsys_basic_info *));
static void generic_fanrpm __P((struct lm_softc *, struct envsys_tre_data *));

void lm_refresh_sensor_data __P((struct lm_softc *));

static void wb_svolt __P((struct lm_softc *));
static void wb_stemp __P((struct lm_softc *, struct envsys_tre_data *, int));
static void wb781_fanrpm __P((struct lm_softc *, struct envsys_tre_data *));
static void wb_fanrpm __P((struct lm_softc *, struct envsys_tre_data *));

void wb781_refresh_sensor_data __P((struct lm_softc *));
void wb782_refresh_sensor_data __P((struct lm_softc *));
void wb697_refresh_sensor_data __P((struct lm_softc *));

int lm_gtredata __P((struct sysmon_envsys *, struct envsys_tre_data *));

int generic_streinfo_fan __P((struct lm_softc *, struct envsys_basic_info *,
           int, struct envsys_basic_info *));
int lm_streinfo __P((struct sysmon_envsys *, struct envsys_basic_info *));
int wb781_streinfo __P((struct sysmon_envsys *, struct envsys_basic_info *));
int wb782_streinfo __P((struct sysmon_envsys *, struct envsys_basic_info *));

struct lm_chip {
	int (*chip_match) __P((struct lm_softc *));
};

struct lm_chip lm_chips[] = {
	{ wb_match },
	{ lm_match },
	{ def_match } /* Must be last */
};


int
lm_generic_banksel(lmsc, bank)
	struct lm_softc *lmsc;
	int bank;
{

	(*lmsc->lm_writereg)(lmsc, WB_BANKSEL, bank);
	return (0);
}


/*
 * bus independent probe
 */
int
lm_probe(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	u_int8_t cr;
	int rv;

	/* Check for some power-on defaults */
	bus_space_write_1(iot, ioh, LMC_ADDR, LMD_CONFIG);

	/* Perform LM78 reset */
	bus_space_write_1(iot, ioh, LMC_DATA, 0x80);

	/* XXX - Why do I have to reselect the register? */
	bus_space_write_1(iot, ioh, LMC_ADDR, LMD_CONFIG);
	cr = bus_space_read_1(iot, ioh, LMC_DATA);

	/* XXX - spec says *only* 0x08! */
	if ((cr == 0x08) || (cr == 0x01))
		rv = 1;
	else
		rv = 0;

	DPRINTF(("lm: rv = %d, cr = %x\n", rv, cr));

	return (rv);
}


/*
 * pre:  lmsc contains valid busspace tag and handle
 */
void
lm_attach(lmsc)
	struct lm_softc *lmsc;
{
	u_int i;

	/* Install default bank selection routine, if none given. */
	if (lmsc->lm_banksel == NULL)
		lmsc->lm_banksel = lm_generic_banksel;

	for (i = 0; i < sizeof(lm_chips) / sizeof(lm_chips[0]); i++)
		if (lm_chips[i].chip_match(lmsc))
			break;

	/* Start the monitoring loop */
	(*lmsc->lm_writereg)(lmsc, LMD_CONFIG, 0x01);

	/* Indicate we have never read the registers */
	timerclear(&lmsc->lastread);

	/* Initialize sensors */
	for (i = 0; i < lmsc->numsensors; ++i) {
		lmsc->sensors[i].sensor = lmsc->info[i].sensor = i;
		lmsc->sensors[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		lmsc->info[i].validflags = ENVSYS_FVALID;
		lmsc->sensors[i].warnflags = ENVSYS_WARN_OK;
	}
	/*
	 * Hook into the System Monitor.
	 */
	lmsc->sc_sysmon.sme_ranges = lm_ranges;
	lmsc->sc_sysmon.sme_sensor_info = lmsc->info;
	lmsc->sc_sysmon.sme_sensor_data = lmsc->sensors;
	lmsc->sc_sysmon.sme_cookie = lmsc;

	lmsc->sc_sysmon.sme_gtredata = lm_gtredata;
	/* sme_streinfo set in chip-specific attach */

	lmsc->sc_sysmon.sme_nsensors = lmsc->numsensors;
	lmsc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&lmsc->sc_sysmon))
		printf("%s: unable to register with sysmon\n",
		    lmsc->sc_dev.dv_xname);
}

int
lm_match(sc)
	struct lm_softc *sc;
{
	int i;

	/* See if we have an LM78 or LM79 */
	i = (*sc->lm_readreg)(sc, LMD_CHIPID) & LM_ID_MASK;
	switch(i) {
	case LM_ID_LM78:
		printf(": LM78\n");
		break;
	case LM_ID_LM78J:
		printf(": LM78J\n");
		break;
	case LM_ID_LM79:
		printf(": LM79\n");
		break;
	case LM_ID_LM81:
		printf(": LM81\n");
		break;
	default:
		return 0;
	}
	lm_common_match(sc);
	return 1;
}

int
def_match(sc)
	struct lm_softc *sc;
{
	int i;

	i = (*sc->lm_readreg)(sc, LMD_CHIPID) & LM_ID_MASK;
	printf(": Unknown chip (ID %d)\n", i);
	lm_common_match(sc);
	return 1;
}

void
lm_common_match(sc)
	struct lm_softc *sc;
{
	int i;
	sc->numsensors = LM_NUM_SENSORS;
	sc->refresh_sensor_data = lm_refresh_sensor_data;

	for (i = 0; i < 7; ++i) {
		sc->sensors[i].units = sc->info[i].units =
		    ENVSYS_SVOLTS_DC;
		snprintf(sc->info[i].desc, sizeof(sc->info[i].desc),
		    "IN %d", i);
	}

	/* default correction factors for resistors on higher voltage inputs */
	sc->info[0].rfact = sc->info[1].rfact =
	    sc->info[2].rfact = 10000;
	sc->info[3].rfact = (int)(( 90.9 / 60.4) * 10000);
	sc->info[4].rfact = (int)(( 38.0 / 10.0) * 10000);
	sc->info[5].rfact = (int)((210.0 / 60.4) * 10000);
	sc->info[6].rfact = (int)(( 90.9 / 60.4) * 10000);

	sc->sensors[7].units = ENVSYS_STEMP;
	strcpy(sc->info[7].desc, "Temp");

	setup_fan(sc, 8, 3);
	sc->sc_sysmon.sme_streinfo = lm_streinfo;
}

int
wb_match(sc)
	struct lm_softc *sc;
{
	int i, j;

	(*sc->lm_writereg)(sc, WB_BANKSEL, WB_BANKSEL_HBAC);
	j = (*sc->lm_readreg)(sc, WB_VENDID) << 8;
	(*sc->lm_writereg)(sc, WB_BANKSEL, 0);
	j |= (*sc->lm_readreg)(sc, WB_VENDID);
	DPRINTF(("winbond vend id 0x%x\n", j));
	if (j != WB_VENDID_WINBOND)
		return 0;
	/* read device ID */
	(*sc->lm_banksel)(sc, 0);
	j = (*sc->lm_readreg)(sc, WB_BANK0_CHIPID);
	DPRINTF(("winbond chip id 0x%x\n", j));
	switch(j) {
	case WB_CHIPID_83781:
	case WB_CHIPID_83781_2:
		printf(": W83781D\n");

		for (i = 0; i < 7; ++i) {
			sc->sensors[i].units = sc->info[i].units =
			    ENVSYS_SVOLTS_DC;
			snprintf(sc->info[i].desc, sizeof(sc->info[i].desc),
			    "IN %d", i);
		}

		/* default correction factors for higher voltage inputs */
		sc->info[0].rfact = sc->info[1].rfact =
		    sc->info[2].rfact = 10000;
		sc->info[3].rfact = (int)(( 90.9 / 60.4) * 10000);
		sc->info[4].rfact = (int)(( 38.0 / 10.0) * 10000);
		sc->info[5].rfact = (int)((210.0 / 60.4) * 10000);
		sc->info[6].rfact = (int)(( 90.9 / 60.4) * 10000);

		setup_temp(sc, 7, 3);
		setup_fan(sc, 10, 3);

		sc->numsensors = WB83781_NUM_SENSORS;
		sc->refresh_sensor_data = wb781_refresh_sensor_data;
		sc->sc_sysmon.sme_streinfo = wb781_streinfo;
		return 1;
	case WB_CHIPID_83697:
		printf(": W83697HF\n");
		wb_setup_volt(sc);
		setup_temp(sc, 9, 2);
		setup_fan(sc, 11, 3);
		sc->numsensors = WB83697_NUM_SENSORS;
		sc->refresh_sensor_data = wb697_refresh_sensor_data;
		sc->sc_sysmon.sme_streinfo = wb782_streinfo;
		return 1;
	case WB_CHIPID_83782:
		printf(": W83782D\n");
		break;
	case WB_CHIPID_83627:
		printf(": W83627HF\n");
		break;
	default:
		printf(": unknow winbond chip ID 0x%x\n", j);
		/* handle as a standart lm7x */
		lm_common_match(sc);
		return 1;
	}
	/* common code for the W83782D and W83627HF */
	wb_setup_volt(sc);
	setup_temp(sc, 9, 3);
	setup_fan(sc, 12, 3);
	sc->numsensors = WB_NUM_SENSORS;
	sc->refresh_sensor_data = wb782_refresh_sensor_data;
	sc->sc_sysmon.sme_streinfo = wb782_streinfo;
	return 1;
}

static void
wb_setup_volt(sc)
	struct lm_softc *sc;
{
	sc->sensors[0].units = sc->info[0].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[0].desc, sizeof(sc->info[0].desc), "VCORE A");
	sc->info[0].rfact = 10000;
	sc->sensors[1].units = sc->info[1].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[1].desc, sizeof(sc->info[1].desc), "VCORE B");
	sc->info[1].rfact = 10000;
	sc->sensors[2].units = sc->info[2].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[2].desc, sizeof(sc->info[2].desc), "+3.3V");
	sc->info[2].rfact = 10000;
	sc->sensors[3].units = sc->info[3].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[3].desc, sizeof(sc->info[3].desc), "+5V");
	sc->info[3].rfact = 16778;
	sc->sensors[4].units = sc->info[4].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[4].desc, sizeof(sc->info[4].desc), "+12V");
	sc->info[4].rfact = 38000;
	sc->sensors[5].units = sc->info[5].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[5].desc, sizeof(sc->info[5].desc), "-12V");
	sc->info[5].rfact = 10000;
	sc->sensors[6].units = sc->info[6].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[6].desc, sizeof(sc->info[6].desc), "-5V");
	sc->info[6].rfact = 10000;
	sc->sensors[7].units = sc->info[7].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[7].desc, sizeof(sc->info[7].desc), "+5VSB");
	sc->info[7].rfact = 15151;
	sc->sensors[8].units = sc->info[8].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->info[8].desc, sizeof(sc->info[8].desc), "VBAT");
	sc->info[8].rfact = 10000;
}

static void
setup_temp(sc, start, n)
	struct lm_softc *sc;
	int start, n;
{
	int i;

	for (i = 0; i < n; i++) {
		sc->sensors[start + i].units = ENVSYS_STEMP;
		snprintf(sc->info[start + i].desc,
		    sizeof(sc->info[start + i].desc), "Temp %d", i + 1);
	}
}


static void
setup_fan(sc, start, n)
	struct lm_softc *sc;
	int start, n;
{
	int i;
	for (i = 0; i < n; ++i) {
		sc->sensors[start + i].units = ENVSYS_SFANRPM;
		sc->info[start + i].units = ENVSYS_SFANRPM;
		snprintf(sc->info[start + i].desc,
		    sizeof(sc->info[start + i].desc), "Fan %d", i + 1);
	}
}

int
lm_gtredata(sme, tred)
	 struct sysmon_envsys *sme;
	 struct envsys_tre_data *tred;
{
	 static const struct timeval onepointfive = { 1, 500000 };
	 struct timeval t;
	 struct lm_softc *sc = sme->sme_cookie;
	 int i, s;

	 /* read new values at most once every 1.5 seconds */
	 timeradd(&sc->lastread, &onepointfive, &t);
	 s = splclock();
	 i = timercmp(&mono_time, &t, >);
	 if (i) {
		  sc->lastread.tv_sec  = mono_time.tv_sec;
		  sc->lastread.tv_usec = mono_time.tv_usec;
	 }
	 splx(s);

	 if (i)
		  sc->refresh_sensor_data(sc);

	 *tred = sc->sensors[tred->sensor];

	 return (0);
}

int
generic_streinfo_fan(sc, info, n, binfo)
	struct lm_softc *sc;
	struct envsys_basic_info *info;
	int n;
	struct envsys_basic_info *binfo;
{
	u_int8_t sdata;
	int divisor;

	/* FAN1 and FAN2 can have divisors set, but not FAN3 */
	if ((sc->info[binfo->sensor].units == ENVSYS_SFANRPM)
	    && (n < 2)) {
		if (binfo->rpms == 0) {
			binfo->validflags = 0;
			return (0);
		}

		/* write back the nominal FAN speed  */
		info->rpms = binfo->rpms;

		/* 153 is the nominal FAN speed value */
		divisor = 1350000 / (binfo->rpms * 153);

		/* ...but we need lg(divisor) */
		if (divisor <= 1)
		    divisor = 0;
		else if (divisor <= 2)
		    divisor = 1;
		else if (divisor <= 4)
		    divisor = 2;
		else
		    divisor = 3;

		/*
		 * FAN1 div is in bits <5:4>, FAN2 div is
		 * in <7:6>
		 */
		sdata = (*sc->lm_readreg)(sc, LMD_VIDFAN);
		if ( n == 0 ) {  /* FAN1 */
		    divisor <<= 4;
		    sdata = (sdata & 0xCF) | divisor;
		} else { /* FAN2 */
		    divisor <<= 6;
		    sdata = (sdata & 0x3F) | divisor;
		}

		(*sc->lm_writereg)(sc, LMD_VIDFAN, sdata);
	}
	return (0);

}

int
lm_streinfo(sme, binfo)
	 struct sysmon_envsys *sme;
	 struct envsys_basic_info *binfo;
{
	 struct lm_softc *sc = sme->sme_cookie;

	 if (sc->info[binfo->sensor].units == ENVSYS_SVOLTS_DC)
		  sc->info[binfo->sensor].rfact = binfo->rfact;
	 else {
		if (sc->info[binfo->sensor].units == ENVSYS_SFANRPM) {
			generic_streinfo_fan(sc, &sc->info[binfo->sensor],
			    binfo->sensor - 8, binfo);
		}
		memcpy(sc->info[binfo->sensor].desc, binfo->desc,
		    sizeof(sc->info[binfo->sensor].desc));
		sc->info[binfo->sensor].desc[
		    sizeof(sc->info[binfo->sensor].desc) - 1] = '\0';

		binfo->validflags = ENVSYS_FVALID;
	 }
	 return (0);
}

int
wb781_streinfo(sme, binfo)
	 struct sysmon_envsys *sme;
	 struct envsys_basic_info *binfo;
{
	 struct lm_softc *sc = sme->sme_cookie;
	 int divisor;
	 u_int8_t sdata;
	 int i;

	 if (sc->info[binfo->sensor].units == ENVSYS_SVOLTS_DC)
		  sc->info[binfo->sensor].rfact = binfo->rfact;
	 else {
		if (sc->info[binfo->sensor].units == ENVSYS_SFANRPM) {
			if (binfo->rpms == 0) {
				binfo->validflags = 0;
				return (0);
			}

			/* write back the nominal FAN speed  */
			sc->info[binfo->sensor].rpms = binfo->rpms;

			/* 153 is the nominal FAN speed value */
			divisor = 1350000 / (binfo->rpms * 153);

			/* ...but we need lg(divisor) */
			for (i = 0; i < 7; i++) {
				if (divisor <= (1 << i))
				 	break;
			}
			divisor = i;

			if (binfo->sensor == 10 || binfo->sensor == 11) {
				/*
				 * FAN1 div is in bits <5:4>, FAN2 div
				 * is in <7:6>
				 */
				sdata = (*sc->lm_readreg)(sc, LMD_VIDFAN);
				if ( binfo->sensor == 10 ) {  /* FAN1 */
					 sdata = (sdata & 0xCF) |
					     ((divisor & 0x3) << 4);
				} else { /* FAN2 */
					 sdata = (sdata & 0x3F) |
					     ((divisor & 0x3) << 6);
				}
				(*sc->lm_writereg)(sc, LMD_VIDFAN, sdata);
			} else {
				/* FAN3 is in WB_PIN <7:6> */
				sdata = (*sc->lm_readreg)(sc, WB_PIN);
				sdata = (sdata & 0x3F) |
				     ((divisor & 0x3) << 6);
				(*sc->lm_writereg)(sc, WB_PIN, sdata);
			}
		}
		memcpy(sc->info[binfo->sensor].desc, binfo->desc,
		    sizeof(sc->info[binfo->sensor].desc));
		sc->info[binfo->sensor].desc[
		    sizeof(sc->info[binfo->sensor].desc) - 1] = '\0';

		binfo->validflags = ENVSYS_FVALID;
	 }
	 return (0);
}

int
wb782_streinfo(sme, binfo)
	 struct sysmon_envsys *sme;
	 struct envsys_basic_info *binfo;
{
	 struct lm_softc *sc = sme->sme_cookie;
	 int divisor;
	 u_int8_t sdata;
	 int i;

	 if (sc->info[binfo->sensor].units == ENVSYS_SVOLTS_DC)
		  sc->info[binfo->sensor].rfact = binfo->rfact;
	 else {
	 	if (sc->info[binfo->sensor].units == ENVSYS_SFANRPM) {
			if (binfo->rpms == 0) {
				binfo->validflags = 0;
				return (0);
			}

			/* write back the nominal FAN speed  */
			sc->info[binfo->sensor].rpms = binfo->rpms;

			/* 153 is the nominal FAN speed value */
			divisor = 1350000 / (binfo->rpms * 153);

			/* ...but we need lg(divisor) */
			for (i = 0; i < 7; i++) {
				if (divisor <= (1 << i))
				 	break;
			}
			divisor = i;

			if (binfo->sensor == 12 || binfo->sensor == 13) {
				/*
				 * FAN1 div is in bits <5:4>, FAN2 div
				 * is in <7:6>
				 */
				sdata = (*sc->lm_readreg)(sc, LMD_VIDFAN);
				if ( binfo->sensor == 12 ) {  /* FAN1 */
					 sdata = (sdata & 0xCF) |
					     ((divisor & 0x3) << 4);
				} else { /* FAN2 */
					 sdata = (sdata & 0x3F) |
					     ((divisor & 0x3) << 6);
				}
				(*sc->lm_writereg)(sc, LMD_VIDFAN, sdata);
			} else {
				/* FAN3 is in WB_PIN <7:6> */
				sdata = (*sc->lm_readreg)(sc, WB_PIN);
				sdata = (sdata & 0x3F) |
				     ((divisor & 0x3) << 6);
				(*sc->lm_writereg)(sc, WB_PIN, sdata);
			}
			/* Bit 2 of divisor is in WB_BANK0_FANBAT */
			(*sc->lm_banksel)(sc, 0);
			sdata = (*sc->lm_readreg)(sc, WB_BANK0_FANBAT);
			sdata &= ~(0x20 << (binfo->sensor - 12));
			sdata |= (divisor & 0x4) << (binfo->sensor - 9);
			(*sc->lm_writereg)(sc, WB_BANK0_FANBAT, sdata);
		}

		memcpy(sc->info[binfo->sensor].desc, binfo->desc,
		    sizeof(sc->info[binfo->sensor].desc));
		sc->info[binfo->sensor].desc[
		    sizeof(sc->info[binfo->sensor].desc) - 1] = '\0';

		binfo->validflags = ENVSYS_FVALID;
	}
	return (0);
}

static void
generic_stemp(sc, sensor)
	struct lm_softc *sc;
	struct envsys_tre_data *sensor;
{
	int sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + 7);
	DPRINTF(("sdata[temp] 0x%x\n", sdata));
	/* temp is given in deg. C, we convert to uK */
	sensor->cur.data_us = sdata * 1000000 + 273150000;
}

static void
generic_svolt(sc, sensors, infos)
	struct lm_softc *sc;
	struct envsys_tre_data *sensors;
	struct envsys_basic_info *infos;
{
	int i, sdata;

	for (i = 0; i < 7; i++) {
		sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + i);
		DPRINTF(("sdata[volt%d] 0x%x\n", i, sdata));
		/* voltage returned as (mV >> 4), we convert to uVDC */
		sensors[i].cur.data_s = (sdata << 4);
		/* rfact is (factor * 10^4) */
		sensors[i].cur.data_s *= infos[i].rfact;
		/* division by 10 gets us back to uVDC */
		sensors[i].cur.data_s /= 10;

		/* these two are negative voltages */
		if ( (i == 5) || (i == 6) )
			sensors[i].cur.data_s *= -1;
	}
}

static void
generic_fanrpm(sc, sensors)
	struct lm_softc *sc;
	struct envsys_tre_data *sensors;
{
	int i, sdata, divisor;
	for (i = 0; i < 3; i++) {
		sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + 8 + i);
		DPRINTF(("sdata[fan%d] 0x%x\n", i, sdata));
		if (i == 2)
			divisor = 2;	/* Fixed divisor for FAN3 */
		else if (i == 1)	/* Bits 7 & 6 of VID/FAN  */
			divisor = ((*sc->lm_readreg)(sc, LMD_VIDFAN) >> 6) & 0x3;
		else
			divisor = ((*sc->lm_readreg)(sc, LMD_VIDFAN) >> 4) & 0x3;

		if (sdata == 0xff || sdata == 0x00) {
			sensors[i].cur.data_us = 0;
		} else {
			sensors[i].cur.data_us = 1350000 / (sdata << divisor);
		}
	}
}

/*
 * pre:  last read occurred >= 1.5 seconds ago
 * post: sensors[] current data are the latest from the chip
 */
void
lm_refresh_sensor_data(sc)
	struct lm_softc *sc;
{
	/* Refresh our stored data for every sensor */
	generic_stemp(sc, &sc->sensors[7]);
	generic_svolt(sc, &sc->sensors[0], &sc->info[0]);
	generic_fanrpm(sc, &sc->sensors[8]);
}

static void
wb_svolt(sc)
	struct lm_softc *sc;
{
	int i, sdata;
	for (i = 0; i < 9; ++i) {
		if (i < 7) {
			sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + i);
		} else {
			/* from bank5 */
			(*sc->lm_banksel)(sc, 5);
			sdata = (*sc->lm_readreg)(sc, (i == 7) ?
			    WB_BANK5_5VSB : WB_BANK5_VBAT);
		}
		DPRINTF(("sdata[volt%d] 0x%x\n", i, sdata));
		/* voltage returned as (mV >> 4), we convert to uV */
		sdata =  sdata << 4;
		/* special case for negative voltages */
		if (i == 5) {
			/*
			 * -12Vdc, assume Winbond recommended values for
			 * resistors
			 */
			sdata = ((sdata * 1000) - (3600 * 805)) / 195;
		} else if (i == 6) {
			/*
			 * -5Vdc, assume Winbond recommended values for
			 * resistors
			 */
			sdata = ((sdata * 1000) - (3600 * 682)) / 318;
		}
		/* rfact is (factor * 10^4) */
		sc->sensors[i].cur.data_s = sdata * sc->info[i].rfact;
		/* division by 10 gets us back to uVDC */
		sc->sensors[i].cur.data_s /= 10;
	}
}

static void
wb_stemp(sc, sensors, n)
	struct lm_softc *sc;
	struct  envsys_tre_data *sensors;
	int n;
{
	int sdata;
	/* temperatures. Given in dC, we convert to uK */
	sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + 7);
	DPRINTF(("sdata[temp0] 0x%x\n", sdata));
	sensors[0].cur.data_us = sdata * 1000000 + 273150000;
	/* from bank1 */
	if ((*sc->lm_banksel)(sc, 1))
		sensors[1].validflags &= ~ENVSYS_FCURVALID;
	else {
		sdata = (*sc->lm_readreg)(sc, WB_BANK1_T2H) << 1;
		sdata |=  ((*sc->lm_readreg)(sc, WB_BANK1_T2L) & 0x80) >> 7;
		DPRINTF(("sdata[temp1] 0x%x\n", sdata));
		sensors[1].cur.data_us = (sdata * 1000000) / 2 + 273150000;
	}
	if (n < 3)
		return;
	/* from bank2 */
	if ((*sc->lm_banksel)(sc, 2))
		sensors[2].validflags &= ~ENVSYS_FCURVALID;
	else {
		sdata = (*sc->lm_readreg)(sc, WB_BANK2_T3H) << 1;
		sdata |=  ((*sc->lm_readreg)(sc, WB_BANK2_T3L) & 0x80) >> 7;
		DPRINTF(("sdata[temp2] 0x%x\n", sdata));
		sensors[2].cur.data_us = (sdata * 1000000) / 2 + 273150000;
	}
}

static void
wb781_fanrpm(sc, sensors)
	struct lm_softc *sc;
	struct envsys_tre_data *sensors;
{
	int i, divisor, sdata;
	(*sc->lm_banksel)(sc, 0);
	for (i = 0; i < 3; i++) {
		sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + i + 8);
		DPRINTF(("sdata[fan%d] 0x%x\n", i, sdata));
		if (i == 0)
			divisor = ((*sc->lm_readreg)(sc, LMD_VIDFAN) >> 4) & 0x3;
		else if (i == 1)
			divisor = ((*sc->lm_readreg)(sc, LMD_VIDFAN) >> 6) & 0x3;
		else
			divisor = ((*sc->lm_readreg)(sc, WB_PIN) >> 6) & 0x3;

		DPRINTF(("sdata[%d] 0x%x div 0x%x\n", i, sdata, divisor));
		if (sdata == 0xff || sdata == 0x00) {
			sensors[i].cur.data_us = 0;
		} else {
			sensors[i].cur.data_us = 1350000 /
			    (sdata << divisor);
		}
	}
}

static void
wb_fanrpm(sc, sensors)
	struct lm_softc *sc;
	struct envsys_tre_data *sensors;
{
	int i, divisor, sdata;
	(*sc->lm_banksel)(sc, 0);
	for (i = 0; i < 3; i++) {
		sdata = (*sc->lm_readreg)(sc, LMD_SENSORBASE + i + 8);
		DPRINTF(("sdata[fan%d] 0x%x\n", i, sdata));
		if (i == 0)
			divisor = ((*sc->lm_readreg)(sc, LMD_VIDFAN) >> 4) & 0x3;
		else if (i == 1)
			divisor = ((*sc->lm_readreg)(sc, LMD_VIDFAN) >> 6) & 0x3;
		else
			divisor = ((*sc->lm_readreg)(sc, WB_PIN) >> 6) & 0x3;
		divisor |= ((*sc->lm_readreg)(sc, WB_BANK0_FANBAT) >> (i + 3)) & 0x4;

		DPRINTF(("sdata[%d] 0x%x div 0x%x\n", i, sdata, divisor));
		if (sdata == 0xff || sdata == 0x00) {
			sensors[i].cur.data_us = 0;
		} else {
			sensors[i].cur.data_us = 1350000 /
			    (sdata << divisor);
		}
	}
}

void
wb781_refresh_sensor_data(sc)
	struct lm_softc *sc;
{
	/* Refresh our stored data for every sensor */
	/* we need to reselect bank0 to access common registers */
	(*sc->lm_banksel)(sc, 0);
	generic_svolt(sc, &sc->sensors[0], &sc->info[0]);
	(*sc->lm_banksel)(sc, 0);
	wb_stemp(sc, &sc->sensors[7], 3);
	(*sc->lm_banksel)(sc, 0);
	wb781_fanrpm(sc, &sc->sensors[10]);
}

void
wb782_refresh_sensor_data(sc)
	struct lm_softc *sc;
{
	/* Refresh our stored data for every sensor */
	wb_svolt(sc);
	wb_stemp(sc, &sc->sensors[9], 3);
	wb_fanrpm(sc, &sc->sensors[12]);
}

void
wb697_refresh_sensor_data(sc)
	struct lm_softc *sc;
{
	/* Refresh our stored data for every sensor */
	wb_svolt(sc);
	wb_stemp(sc, &sc->sensors[9], 2);
	wb_fanrpm(sc, &sc->sensors[11]);
}
