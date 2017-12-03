/*	$NetBSD: ds1743.c,v 1.9.12.1 2017/12/03 11:36:12 jdolecek Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ds1743.c,v 1.9.12.1 2017/12/03 11:36:12 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/clock_subr.h>

#include <machine/rtc.h>
#include <sys/bus.h>

#include <evbppc/walnut/dev/ds1743reg.h>
#include <evbppc/walnut/dev/pbusvar.h>

struct dsrtc_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
	struct todr_chip_handle sc_todr;
};

static void dsrtcattach(device_t, device_t, void *);
static int dsrtcmatch(device_t, cfdata_t, void *);
#if 0	/* Nothing uses these yet */
static int ds1743_ram_read(struct dsrtc_softc *, int);
static void ds1743_ram_write(struct dsrtc_softc *, int, int);
#endif

static int dsrtc_read(todr_chip_handle_t, struct clock_ymdhms *);
static int dsrtc_write(todr_chip_handle_t, struct clock_ymdhms *);
static inline u_char ds1743_read(struct dsrtc_softc *, int);
static inline void ds1743_write(struct dsrtc_softc *, int, u_char);
static u_char ds1743_lock(struct dsrtc_softc *, u_char);
static void ds1743_unlock(struct dsrtc_softc *, u_char);

/* device and attach structures */
CFATTACH_DECL_NEW(ds1743rtc, sizeof(struct dsrtc_softc),
    dsrtcmatch, dsrtcattach, NULL, NULL);

/*
 * dsrtcmatch()
 *
 * Validate the IIC address to make sure its an RTC we understand
 */
int ds1743found = 0;

#define DS_SCRATCH_ADDR 0x1FF7

static int
dsrtcmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct pbus_attach_args *paa = aux;
	int retval = !ds1743found;
	bus_space_handle_t h;
	u_int8_t x;

	/* match only RTC devices */
	if (strcmp(paa->pb_name, cf->cf_name) != 0)
		return 0;

	if (bus_space_map(paa->pb_bt, paa->pb_addr, DS_SIZE, 0, &h)) {
		printf("%s: can't map i/o space\n", paa->pb_name);
		return 0;
	}

	/* Read one byte of what's supposed to be NVRAM */
	x = bus_space_read_1(paa->pb_bt, h, DS_SCRATCH_ADDR);
	bus_space_write_1(paa->pb_bt, h, DS_SCRATCH_ADDR, 0xAA);
	if (bus_space_read_1(paa->pb_bt, h, DS_SCRATCH_ADDR) != 0xAA) {
		retval = 0;
		goto done;
	}
	
	bus_space_write_1(paa->pb_bt, h, DS_SCRATCH_ADDR, 0x55);
	if (bus_space_read_1(paa->pb_bt, h, DS_SCRATCH_ADDR) != 0x55) {
		retval = 0;
		goto done;
	}

	/* Restore scratch byte value */
	bus_space_write_1(paa->pb_bt, h, DS_SCRATCH_ADDR, x);
  done:	
	bus_space_unmap(paa->pb_bt, h, DS_SIZE);
	  
	return retval;
}

/*
 * dsrtcattach()
 *
 * Attach the rtc device
 */

static void
dsrtcattach(device_t parent, device_t self, void *aux)
{
	struct dsrtc_softc *sc = device_private(self);
	struct pbus_attach_args *paa = aux;

	ds1743found = 1;
	
	sc->sc_dev = self;
	sc->sc_iot = paa->pb_bt;
	if (bus_space_map(sc->sc_iot, paa->pb_addr, DS_SIZE, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	ds1743_unlock(sc, 0);	/* Make sure the clock is running */
	if ((ds1743_read(sc, DS_DAY) & DS_CTL_BF) == 0)
		printf(": lithium cell is dead, RTC unreliable");
	printf("\n");

	sc->sc_todr.todr_gettime_ymdhms = dsrtc_read;
	sc->sc_todr.todr_settime_ymdhms = dsrtc_write;
	sc->sc_todr.cookie = sc;

#ifdef DEBUG
	{
		struct clock_ymdhms dt;
		dsrtc_read(&sc->sc_todr, &dt);
		printf("RTC: %d/%d/%04d %d:%02d:%02d\n",
			dt.dt_mon, dt.dt_day, dt.dt_year,
			dt.dt_hour, dt.dt_min, dt.dt_sec);
	}
#endif

	todr_attach(&sc->sc_todr);
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
	ds1743_write(sc, addr, val);
}
#endif

#define BCD(x)		((((x) / 10) << 4) | (x % 10))
#define unBCD(v, x)	v = x; v = ((v >> 4) & 0xf) * 10 + (v & 0xf) 

static u_char
ds1743_lock(struct dsrtc_softc *sc, u_char mode)
{
	u_char octl, ctl;

	octl = ds1743_read(sc, DS_CENTURY);
	ctl = octl | (mode & DS_CTL_RW);
	ds1743_write(sc, DS_CENTURY, ctl);	/* Lock RTC for both reading and writing */
	return octl;
}

static void
ds1743_unlock(struct dsrtc_softc *sc, u_char key)
{
	int ctl;

	ctl = ds1743_read(sc, DS_CENTURY);
	ctl = (ctl & 0x3f) | (key & DS_CTL_RW);
	ds1743_write(sc, DS_CENTURY, ctl);	/* Enable updates */
}

static int
dsrtc_write(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct dsrtc_softc *sc = tch->cookie;
	u_char key;

	key = ds1743_lock(sc, DS_CTL_W);
	
	ds1743_write(sc, DS_SECONDS, bintobcd(dt->dt_sec) & 0x7f);
	ds1743_write(sc, DS_MINUTES, bintobcd(dt->dt_min) & 0x7f);
	ds1743_write(sc, DS_HOURS, bintobcd(dt->dt_hour)  & 0x3f);
	ds1743_write(sc, DS_DATE, bintobcd(dt->dt_day)    & 0x3f);
	ds1743_write(sc, DS_MONTH, bintobcd(dt->dt_mon)   & 0x1f);
	ds1743_write(sc, DS_YEAR, bintobcd(dt->dt_year % 100));
	ds1743_write(sc, DS_CENTURY, ((ds1743_read(sc, DS_CENTURY) & DS_CTL_RW)
				      | bintobcd(dt->dt_year / 100)));

	ds1743_unlock(sc, key);
	return(0);
}

static int
dsrtc_read(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct dsrtc_softc *sc = tch->cookie;
	u_char key;
	
	key = ds1743_lock(sc, DS_CTL_R);
	dt->dt_sec = bcdtobin(ds1743_read(sc, DS_SECONDS) & 0x7f);
	dt->dt_min = bcdtobin(ds1743_read(sc, DS_MINUTES) & 0x7f);
	dt->dt_hour = bcdtobin(ds1743_read(sc, DS_HOURS) & 0x3f);
	dt->dt_day = bcdtobin(ds1743_read(sc, DS_DATE)   & 0x3f);
	dt->dt_mon = bcdtobin(ds1743_read(sc, DS_MONTH)   & 0x1f);
	dt->dt_year =
	    bcdtobin(ds1743_read(sc, DS_YEAR)) +
	    bcdtobin(ds1743_read(sc, DS_CENTURY) & ~DS_CTL_RW) * 100; 

	ds1743_unlock(sc, key);
	return(0);
}
