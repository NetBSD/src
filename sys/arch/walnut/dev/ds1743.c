/*	$NetBSD: ds1743.c,v 1.1.8.2 2002/04/01 07:43:35 nathanw Exp $	*/

/*
 * Copyright (c) 2001-2002 Wasabi Sysetms, Inc.
 * Copyright (c) 1998 Mark Brinicombe.
 * Copyright (c) 1998 Causality Limited.
 * All rights reserved.
 *
 * Written by Mark Brinicombe, Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUASLITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rtc.h>
#include <machine/bus.h>

#include <walnut/dev/ds1743reg.h>
#include <walnut/dev/todclockvar.h>

struct dsrtc_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
};

static void dsrtcattach(struct device *, struct device *, void *);
static int dsrtcmatch(struct device *, struct cfdata *, void *);
#if 0	/* Nothing uses these yet */
static int ds1743_ram_read(struct dsrtc_softc *, int);
static void ds1743_ram_write(struct dsrtc_softc *, int, int);
#endif

static int dsrtc_read(void *, rtc_t *);
static int dsrtc_write(void *, rtc_t *);
static inline u_char ds1743_read(struct dsrtc_softc *, int);
static inline void ds1743_write(struct dsrtc_softc *, int, u_char);
static u_char ds1743_lock(struct dsrtc_softc *, u_char);
static void ds1743_unlock(struct dsrtc_softc *, u_char);

/* device and attach structures */
struct cfattach dsrtc_ca = {
	sizeof(struct dsrtc_softc), dsrtcmatch, dsrtcattach
};

/*
 * dsrtcmatch()
 *
 * Validate the IIC address to make sure its an RTC we understand
 */
int ds1743found = 0;

#define DS_SCRATCH_ADDR 0x1FF7

static int
dsrtcmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;
	int retval = !ds1743found;
	bus_space_handle_t h;
	u_int8_t x;

	/* match only RTC devices */
	if (strcmp(maa->mb_name, cf->cf_driver->cd_name) != 0)
		return 0;

	if (bus_space_map(0, maa->mb_addr, DS_SIZE, 0, &h)) {
		printf("%s: can't map i/o space\n", maa->mb_name);
		return 0;
	}

	/* Read one byte of what's supposed to be NVRAM */
	x = bus_space_read_1(0, h, DS_SCRATCH_ADDR);
	bus_space_write_1(0, h, DS_SCRATCH_ADDR, 0xAA);
	if (bus_space_read_1(0, h, DS_SCRATCH_ADDR) != 0xAA) {
		retval = 0;
		goto done;
	}
	
	bus_space_write_1(0, h, DS_SCRATCH_ADDR, 0x55);
	if (bus_space_read_1(0, h, DS_SCRATCH_ADDR) != 0x55) {
		retval = 0;
		goto done;
	}

	/* Restore scratch byte value */
	bus_space_write_1(0, h, DS_SCRATCH_ADDR, x);
  done:	
	bus_space_unmap(0, h, DS_SIZE);
	  
	return retval;
}

/*
 * dsrtcattach()
 *
 * Attach the rtc device
 */

static void
dsrtcattach(struct device *parent, struct device *self, void *aux)
{
	struct dsrtc_softc *sc = (struct dsrtc_softc *)self;
	struct mainbus_attach_args *maa = aux;
	struct todclock_attach_args ta;

	ds1743found = 1;
	
	sc->sc_iot = 0;
	sc->sc_ioh = maa->mb_addr;
	if (bus_space_map(sc->sc_iot, maa->mb_addr, DS_SIZE, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	ds1743_unlock(sc,0);	/* Make sure the clock is running */
	if ((ds1743_read(sc, DS_DAY) & DS_CTL_BF) == 0)
		printf(": lithium cell is dead, RTC unreliable");
	printf("\n");

#ifdef DEBUG
	{
		rtc_t rtc;
		dsrtc_read(sc,&rtc);
		printf("RTC: %d/%d/%02d%02d %d:%02d:%02d\n",
			rtc.rtc_mon, rtc.rtc_day, rtc.rtc_cen, rtc.rtc_year,
			rtc.rtc_hour, rtc.rtc_min, rtc.rtc_sec);
	}
#endif


	ta.ta_name = "todclock";
	ta.ta_rtc_arg = sc;
	ta.ta_rtc_write = dsrtc_write; 
	ta.ta_rtc_read = dsrtc_read;
	ta.ta_flags = 0;
	config_found(self, &ta, NULL);
}

static inline u_char
ds1743_read(struct dsrtc_softc *sc, int addr)
{

	return(bus_space_read_1(sc->sc_iot, sc->sc_ioh, addr));
}

static inline void
ds1743_write(struct dsrtc_softc *sc, int addr, u_char data)
{

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, addr, data);
}


#if 0	/* Nothing uses these yet */
static u_char
ds1743_ram_read(struct dsrtc_softc *sc, int addr)
{

	if (addr >= DS_RAM_SIZE)
		return(-1);
	return(ds1743_read(sc, addr));
}

static void
ds1743_ram_write(struct dsrtc_softc *sc, int addr, u_char val)
{

	if (addr >= DS_RAM_SIZE)
		return (-1);
	ds1743_write(sc,addr,val);
}
#endif

#define BCD(x)	((((x)/10)<<4)|(x%10))
#define unBCD(v,x)	v=x; v = ((v>>4) & 0xf)*10+(v & 0xf) 

static u_char
ds1743_lock(struct dsrtc_softc *sc, u_char mode)
{
	u_char octl,ctl;

	octl = ds1743_read(sc,DS_CENTURY);
	ctl = octl | (mode & DS_CTL_RW);
	ds1743_write(sc,DS_CENTURY, ctl);	/* Lock RTC for both reading and writing */
	return octl;
}

static void
ds1743_unlock(struct dsrtc_softc *sc, u_char key)
{
	int ctl;

	ctl = ds1743_read(sc,DS_CENTURY);
	ctl = (ctl & 0x3f) | (key & DS_CTL_RW);
	ds1743_write(sc,DS_CENTURY, ctl);	/* Enable updates */
}

static int
dsrtc_write(void * arg, rtc_t * rtc)
{
	struct dsrtc_softc *sc = arg;
	u_char key;

	key = ds1743_lock(sc,DS_CTL_W);
	
	ds1743_write(sc, DS_SECONDS, BCD(rtc->rtc_sec) & 0x7f);
	ds1743_write(sc, DS_MINUTES, BCD(rtc->rtc_min) & 0x7f);
	ds1743_write(sc, DS_HOURS, BCD(rtc->rtc_hour)  & 0x3f);
	ds1743_write(sc, DS_DATE, BCD(rtc->rtc_day)    & 0x3f);
	ds1743_write(sc, DS_MONTH, BCD(rtc->rtc_mon)   & 0x1f);
	ds1743_write(sc, DS_YEAR, BCD(rtc->rtc_year));
	ds1743_write(sc, DS_CENTURY, ((ds1743_read(sc,DS_CENTURY) & DS_CTL_RW)
				      | BCD(rtc->rtc_cen)));

	ds1743_unlock(sc,key);
	dsrtc_read(arg,rtc);
	return(1);
}

static int
dsrtc_read(void *arg, rtc_t *rtc)
{
	struct dsrtc_softc *sc = arg;
	u_char key;
	
	key = ds1743_lock(sc,DS_CTL_R);
	rtc->rtc_micro = 0;
	rtc->rtc_centi = 0;
	unBCD(rtc->rtc_sec,ds1743_read(sc, DS_SECONDS) & 0x7f);
	unBCD(rtc->rtc_min,ds1743_read(sc, DS_MINUTES) & 0x7f);
	unBCD(rtc->rtc_hour, ds1743_read(sc, DS_HOURS) & 0x3f);
	unBCD(rtc->rtc_day, ds1743_read(sc, DS_DATE)   & 0x3f);
	unBCD(rtc->rtc_mon,ds1743_read(sc, DS_MONTH)   & 0x1f);
	unBCD(rtc->rtc_year, ds1743_read(sc, DS_YEAR));
	unBCD(rtc->rtc_cen, ds1743_read(sc, DS_CENTURY) & ~DS_CTL_RW); 

	ds1743_unlock(sc,key);
	return(1);
}
