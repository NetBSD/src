/*	$NetBSD: nslm7x.c,v 1.2 2000/03/07 18:39:14 groo Exp $ */

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

#include <sys/envsys.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/nslm7xvar.h>

#include <machine/intr.h>
#include <machine/bus.h>

#if defined(LMDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

struct envsys_range ranges[] = {	/* sc->sensors sub-intervals */
					/* for each unit type        */
	{ 7, 7,    ENVSYS_STEMP   },
	{ 8, 10,   ENVSYS_SFANRPM },
	{ 1, 0,    ENVSYS_SVOLTS_AC },	/* None */
	{ 0, 6,    ENVSYS_SVOLTS_DC },
	{ 1, 0,    ENVSYS_SOHMS },	/* None */
	{ 1, 0,    ENVSYS_SWATTS },	/* None */
	{ 1, 0,    ENVSYS_SAMPS }	/* None */
};

	
#define SCFLAG_OREAD	0x00000001
#define SCFLAG_OWRITE	0x00000002
#define SCFLAG_OPEN	(SCFLAG_OREAD|SCFLAG_OWRITE)

u_int8_t lm_readreg __P((struct lm_softc *, int));
void lm_writereg __P((struct lm_softc *, int, int));
void lm_refresh_sensor_data __P((struct lm_softc *));

cdev_decl(lm);

extern struct cfdriver lm_cd;

#define LMUNIT(x)	(minor(x))

u_int8_t
lm_readreg(sc, reg)
	struct lm_softc *sc;
	int reg;
{
	bus_space_write_1(sc->lm_iot, sc->lm_ioh, LMC_ADDR, reg);
	return (bus_space_read_1(sc->lm_iot, sc->lm_ioh, LMC_DATA));
}

void
lm_writereg(sc, reg, val)
	struct lm_softc *sc;
	int reg;
	int val;
{
	bus_space_write_1(sc->lm_iot, sc->lm_ioh, LMC_ADDR, reg);
	bus_space_write_1(sc->lm_iot, sc->lm_ioh, LMC_DATA, val);
}


/*
 * bus independent probe
 * pre:  lmsc contains valid busspace tag and handle
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
	int i;

	/* See if we have an LM78 or LM79 */
	i = lm_readreg(lmsc, LMD_CHIPID) & LM_ID_MASK;
	printf(": LM7");
	if (i == LM_ID_LM78)
		printf("8\n");
	else if (i == LM_ID_LM78J)
		printf("8J\n");
	else if (i == LM_ID_LM79)
		printf("9\n");
	else
		printf("? - Unknown chip ID (%x)\n", i);

	/* Start the monitoring loop */
	lm_writereg(lmsc, LMD_CONFIG, 0x01);

	/* Indicate we have never read the registers */
	timerclear(&lmsc->lastread);

	/* Initialize sensors */
	for (i = 0; i < LM_NUM_SENSORS; ++i) {
		lmsc->sensors[i].sensor = lmsc->info[i].sensor = i;
		lmsc->sensors[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		lmsc->info[i].validflags = ENVSYS_FVALID;
		lmsc->sensors[i].warnflags = ENVSYS_WARN_OK;
	}

	for (i = 0; i < 7; ++i) {
		lmsc->sensors[i].units = lmsc->info[i].units =
		    ENVSYS_SVOLTS_DC;

		lmsc->info[i].desc[0] = 'I';
		lmsc->info[i].desc[1] = 'N';
		lmsc->info[i].desc[2] = i + '0';
		lmsc->info[i].desc[3] = 0;
	}

	lmsc->sensors[7].units = ENVSYS_STEMP;
	strcpy(lmsc->info[7].desc, "Temp");

	for (i = 8; i < 11; ++i) {
		lmsc->sensors[i].units = lmsc->info[i].units = ENVSYS_SFANRPM;

		lmsc->info[i].desc[0] = 'F';
		lmsc->info[i].desc[1] = 'a';
		lmsc->info[i].desc[2] = 'n';
		lmsc->info[i].desc[3] = ' ';
		lmsc->info[i].desc[4] = i - 7 + '0';
		lmsc->info[i].desc[5] = 0;
	}
}


int	
lmopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = LMUNIT(dev);
	struct lm_softc *sc;
	
	if (unit >= lm_cd.cd_ndevs)
		return (ENXIO);
	sc = lm_cd.cd_devs[unit];
	if (sc == 0)
		return (ENXIO);

	/* XXX - add spinlocks instead! */
	if (sc->sc_flags & SCFLAG_OPEN)
		return (EBUSY);

	sc->sc_flags |= SCFLAG_OPEN;

	return 0;
}


int
lmclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct lm_softc *sc = lm_cd.cd_devs[LMUNIT(dev)];

	DPRINTF(("lmclose: pid %d flag %x mode %x\n", p->p_pid, flag, mode));

	sc->sc_flags &= ~SCFLAG_OPEN;

	return 0;
}


int
lmioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct lm_softc *sc = lm_cd.cd_devs[LMUNIT(dev)];
	struct envsys_range *rng;
	struct envsys_tre_data *tred;
	struct envsys_basic_info *binfo;
	struct timeval t, onepointfive = { 1, 500000 };
	u_int8_t sdata;
	int32_t *vers;
	int i, s;
	int divisor;

	switch (cmd) {
	case ENVSYS_VERSION:
		vers = (int32_t *)data;
		*vers = 1000;

		return (0);

	case ENVSYS_GRANGE:
		rng = (struct envsys_range *)data;
		if ((rng->units < ENVSYS_STEMP) ||
		    (rng->units > ENVSYS_SAMPS) ) {
			/* Return empty range for unsupp sensor types */
			rng->low = 1;
			rng->high = 0;
		} else {
			rng->low  = ranges[rng->units].low;
			rng->high = ranges[rng->units].high;
		}

		return (0);

	case ENVSYS_GTREDATA:
		tred = (struct envsys_tre_data *)data;
		tred->validflags = 0;

		if (tred->sensor < LM_NUM_SENSORS) {
			/* read new values at most once every 1.5 seconds */
			s = splclock();

			timeradd(&sc->lastread, &onepointfive, &t);

			i = timercmp(&mono_time, &t, >);
			if (i) {
				sc->lastread.tv_sec  = mono_time.tv_sec;
				sc->lastread.tv_usec = mono_time.tv_usec; 
			}
			splx(s);
		   
			if (i) {
				lm_refresh_sensor_data(sc);
			}

			bcopy(&sc->sensors[tred->sensor], tred,
			      sizeof(struct envsys_tre_data));
		}

		return (0);

	case ENVSYS_GTREINFO:
		binfo = (struct envsys_basic_info *)data;

		if (binfo->sensor >= LM_NUM_SENSORS)
			binfo->validflags = 0;
		else
			bcopy(&sc->info[binfo->sensor], binfo,
			      sizeof(struct envsys_basic_info));

		return (0);

	case ENVSYS_STREINFO:
		binfo = (struct envsys_basic_info *)data;

		if (binfo->sensor >= LM_NUM_SENSORS)
			binfo->validflags = 0;
		else {
			/* FAN1 and FAN2 can have divisors set, but not FAN3 */
			if ((sc->info[binfo->sensor].units == ENVSYS_SFANRPM)
			    && (binfo->sensor != 10)) {

				if (binfo->rpms == 0) {
					binfo->validflags = 0;
					return (0);
				}

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
				sdata = lm_readreg(sc, LMD_VIDFAN);
				if ( binfo->sensor == 8 ) {  /* FAN1 */
					divisor <<= 4;
					sdata = (sdata & 0xCF) | divisor;
				} else { /* FAN2 */
					divisor <<= 6;
					sdata = (sdata & 0x3F) | divisor;
				}

				lm_writereg(sc, LMD_VIDFAN, sdata);
			}

			bcopy(binfo->desc, sc->info[binfo->sensor].desc, 33);
			sc->info[binfo->sensor].desc[32] = 0;

			binfo->validflags = ENVSYS_FVALID;
		}

		return (0);

	default:
		return (ENOTTY);
	}
}


/*
 * pre:  last read occured >= 1.5 seconds ago
 * post: sensors[] current data are the latest from the chip
 */
void
lm_refresh_sensor_data(sc)
	struct lm_softc *sc;
{
	u_int8_t sdata;
	int i, divisor;

	/* Refresh our stored data for every sensor */
	for (i = 0; i < LM_NUM_SENSORS; ++i) {
		sdata = lm_readreg(sc, LMD_SENSORBASE + i);
		
		switch (sc->sensors[i].units) {
		case ENVSYS_STEMP:
			/* temp is given in deg. C, we convert to uK */
			sc->sensors[i].cur.data_us = sdata * 1000000 +
			    273150000;
			break;
			
		case ENVSYS_SVOLTS_DC:
			/* voltage returned as (mV >> 4), we convert to uVDC */
			sc->sensors[i].cur.data_s = (sdata << 4) * 1000;
			
			/* these two are negative voltages */
			if ( (i == 5) || (i == 6) )
				sc->sensors[i].cur.data_s *= -1;
			
			/*
			 * XXX - Motherboard manufacturers can wire up whatever
			 * resistors they want!  These values may have been
			 * attenuated
			 */
			
			break;
			
		case ENVSYS_SFANRPM:
			if (i == 10)
				divisor = 2;	/* Fixed divisor for FAN3 */
			else if (i == 9)	/* Bits 7 & 6 of VID/FAN  */
				divisor = (lm_readreg(sc, LMD_VIDFAN) >> 6) &
				    0x3;
			else
				divisor = (lm_readreg(sc, LMD_VIDFAN) >> 4) &
				    0x3;
			
			sc->sensors[i].cur.data_us = 1350000 /
			    (sdata << divisor);
			
			break;
			
		default:
			/* XXX - debug log something? */
			sc->sensors[i].validflags = 0;
			
			break;
		}
	}
}
