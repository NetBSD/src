/*	$NetBSD: rtc.c,v 1.5.2.4 2002/03/16 15:58:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura. All rights reserved.
 * Copyright (c) 1999 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_vr41xx.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/sysconf.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/rtcreg.h>

/*
 * for debugging definitions
 * 	VRRTCDEBUG	print rtc debugging information
 *	VRRTC_HEARTBEAT	print HEARTBEAT (too many print...)
 */
#ifdef VRRTCDEBUG
#ifndef VRRTCDEBUG_CONF
#define VRRTCDEBUG_CONF 0
#endif
int vrrtc_debug = VRRTCDEBUG_CONF;
#define DPRINTF(arg) if (vrrtc_debug) printf arg;
#define DDUMP_REGS(arg) if (vrrtc_debug) vrrtc_dump_regs(arg);
#else /* VRRTCDEBUG */
#define DPRINTF(arg)
#define DDUMP_REGS(arg)
#endif /* VRRTCDEBUG */

struct vrrtc_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;
#ifndef SINGLE_VRIP_BASE
	int sc_rtcint_reg;
	int sc_tclk_h_reg, sc_tclk_l_reg;
	int sc_tclk_cnt_h_reg, sc_tclk_cnt_l_reg;
#endif /* SINGLE_VRIP_BASE */
};

void	clock_init(struct device *);
void	clock_get(struct device *, time_t, struct clock_ymdhms *);
void	clock_set(struct device *, struct clock_ymdhms *);

struct platform_clock vr_clock = {
#define CLOCK_RATE	128
	CLOCK_RATE, clock_init, clock_get, clock_set,
};

int	vrrtc_match(struct device *, struct cfdata *, void *);
void	vrrtc_attach(struct device *, struct device *, void *);
int	vrrtc_intr(void*, u_int32_t, u_int32_t);
void	vrrtc_dump_regs(struct vrrtc_softc *);

struct cfattach vrrtc_ca = {
	sizeof(struct vrrtc_softc), vrrtc_match, vrrtc_attach
};

static __inline__ void vrrtc_write(struct vrrtc_softc *, int, u_int16_t);
static __inline__ u_int16_t vrrtc_read(struct vrrtc_softc *, int);
void	cvt_timehl_ymdhms(u_int32_t, u_int32_t, struct clock_ymdhms *);

extern int rtc_offset;
static int m2d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static __inline__ void
vrrtc_write(struct vrrtc_softc *sc, int port, u_int16_t val)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static __inline__ u_int16_t
vrrtc_read(struct vrrtc_softc *sc, int port)
{

	return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, port));
}

int
vrrtc_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

#ifndef SINGLE_VRIP_BASE
#define RTCINT_REG_W		(sc->sc_rtcint_reg)
#define TCLK_H_REG_W		(sc->sc_tclk_h_reg)
#define TCLK_L_REG_W		(sc->sc_tclk_l_reg)
#define TCLK_CNT_H_REG_W	(sc->sc_tclk_cnt_h_reg)
#define TCLK_CNT_L_REG_W	(sc->sc_tclk_cnt_l_reg)
#endif /* SINGLE_VRIP_BASE */

void
vrrtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vrrtc_softc *sc = (void *)self;

#ifndef SINGLE_VRIP_BASE
	if (va->va_addr == VR4102_RTC_ADDR) {
		sc->sc_rtcint_reg = VR4102_RTCINT_REG_W;
		sc->sc_tclk_h_reg = VR4102_TCLK_H_REG_W;
		sc->sc_tclk_l_reg = VR4102_TCLK_L_REG_W;
		sc->sc_tclk_cnt_h_reg = VR4102_TCLK_CNT_H_REG_W;
		sc->sc_tclk_cnt_l_reg = VR4102_TCLK_CNT_L_REG_W;
	} else
	if (va->va_addr == VR4122_RTC_ADDR) {
		sc->sc_rtcint_reg = VR4122_RTCINT_REG_W;
		sc->sc_tclk_h_reg = VR4122_TCLK_H_REG_W;
		sc->sc_tclk_l_reg = VR4122_TCLK_L_REG_W;
		sc->sc_tclk_cnt_h_reg = VR4122_TCLK_CNT_H_REG_W;
		sc->sc_tclk_cnt_l_reg = VR4122_TCLK_CNT_L_REG_W;
	} else
	if (va->va_addr == VR4181_RTC_ADDR) {
		sc->sc_rtcint_reg = VR4181_RTCINT_REG_W;
		sc->sc_tclk_h_reg = RTC_NO_REG_W;
		sc->sc_tclk_l_reg = RTC_NO_REG_W;
		sc->sc_tclk_cnt_h_reg = RTC_NO_REG_W;
		sc->sc_tclk_cnt_l_reg = RTC_NO_REG_W;
	} else {
		panic("%s: unknown base address 0x%lx\n",
		    sc->sc_dev.dv_xname, va->va_addr);
	}
#endif /* SINGLE_VRIP_BASE */

	sc->sc_iot = va->va_iot;
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
	    0 /* no flags */, &sc->sc_ioh)) {
		printf("vrrtc_attach: can't map i/o space\n");
		return;
	}
	/* RTC interrupt handler is directly dispatched from CPU intr */
	vr_intr_establish(VR_INTR1, vrrtc_intr, sc);
	/* But need to set level 1 interupt mask register, 
	 * so regsiter fake interrurpt handler
	 */
	if (!(sc->sc_ih = vrip_intr_establish(va->va_vc, va->va_unit, 0,
	    IPL_CLOCK, 0, 0))) {
		printf (":can't map interrupt.\n");
		return;
	}
	/*
	 *  Rtc is attached to call this routine
	 *  before cpu_initclock() calls clock_init().
	 *  So we must disable all interrupt for now.
	 */
	/*
	 * Disable all rtc interrupts
	 */
	/* Disable Elapse compare intr */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ECMP_H_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ECMP_M_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ECMP_L_REG_W, 0);
	/* Disable RTC Long1 intr */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL1_H_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL1_L_REG_W, 0);
	/* Disable RTC Long2 intr */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL2_H_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL2_L_REG_W, 0);
	/* Disable RTC TCLK intr */
	if (TCLK_H_REG_W != RTC_NO_REG_W) {
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, TCLK_H_REG_W, 0);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, TCLK_L_REG_W, 0);
	}
	/*
	 * Clear all rtc intrrupts.
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCINT_REG_W, RTCINT_ALL);

	platform_clock_attach(sc, &vr_clock);
}

int
vrrtc_intr(void *arg, u_int32_t pc, u_int32_t statusReg)
{
	struct vrrtc_softc *sc = arg;
	struct clockframe cf;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCINT_REG_W, RTCINT_ALL);
	cf.pc = pc;
	cf.sr = statusReg;
	hardclock(&cf);
	intrcnt[HARDCLOCK]++;

#ifdef VRRTC_HEARTBEAT
	if ((intrcnt[HARDCLOCK] % (CLOCK_RATE * 5)) == 0) {
		struct clock_ymdhms dt;
		clock_get((struct device *)sc, NULL, &dt);
		printf("%s(%d): rtc_intr: %2d.%2d.%2d %02d:%02d:%02d\n",
		    __FILE__, __LINE__,
		    dt.dt_year, dt.dt_mon, dt.dt_day,
		    dt.dt_hour, dt.dt_min, dt.dt_sec);
	}
#endif
	return 0;
}

void
clock_init(struct device *dev)
{
	struct vrrtc_softc *sc = (struct vrrtc_softc *)dev;

	DDUMP_REGS(sc);
	/*
	 * Set tick (CLOCK_RATE)
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL1_H_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL1_L_REG_W,
	    RTCL1_L_HZ/CLOCK_RATE);
}

void
clock_get(struct device *dev, time_t base, struct clock_ymdhms *dt)
{

	struct vrrtc_softc *sc = (struct vrrtc_softc *)dev;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t timeh;	/* elapse time (2*timeh sec) */
	u_int32_t timel;	/* timel/32768 sec */

	timeh = bus_space_read_2(iot, ioh, ETIME_H_REG_W);
	timeh = (timeh << 16) | bus_space_read_2(iot, ioh, ETIME_M_REG_W);
	timel = bus_space_read_2(iot, ioh, ETIME_L_REG_W);

	DPRINTF(("clock_get: timeh %08x timel %08x\n", timeh, timel));

	cvt_timehl_ymdhms(timeh, timel, dt);

	DPRINTF(("clock_get: %d/%d/%d/%d/%d/%d\n", dt->dt_year, dt->dt_mon,
	    dt->dt_day, dt->dt_hour, dt->dt_min, dt->dt_sec));
}

void
clock_set(struct device *dev, struct clock_ymdhms *dt)
{
	struct vrrtc_softc *sc = (struct vrrtc_softc *)dev;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t timeh;	/* elapse time (2*timeh sec) */
	u_int32_t timel;	/* timel/32768 sec */
	int year, month, sec2;

	timeh = 0;
	timel = 0;

	DPRINTF(("clock_set: %d/%d/%d/%d/%d/%d\n", dt->dt_year, dt->dt_mon,
	    dt->dt_day, dt->dt_hour, dt->dt_min, dt->dt_sec));

	dt->dt_year += YBASE;

	DPRINTF(("clock_set: %d/%d/%d/%d/%d/%d\n", dt->dt_year, dt->dt_mon,
	    dt->dt_day, dt->dt_hour, dt->dt_min, dt->dt_sec));

	year = EPOCHYEAR;
	sec2 = LEAPYEAR4(year)?SEC2YR+SEC2DAY:SEC2YR;
	while (year < dt->dt_year) {
		year++;
		timeh += sec2;
		sec2 = LEAPYEAR4(year)?SEC2YR+SEC2DAY:SEC2YR;
	}
	month = 1; /* now month is 1..12 */
	sec2 = SEC2DAY * m2d[month-1];
	while (month < dt->dt_mon) {
		month++;
		timeh += sec2;
		sec2 = SEC2DAY * m2d[month-1];
		if (month == 2 && LEAPYEAR4(year)) /* feb. and leapyear */
			sec2 += SEC2DAY;
	}

	timeh += (dt->dt_day - 1)*SEC2DAY;

	timeh += dt->dt_hour*SEC2HOUR;

	timeh += dt->dt_min*SEC2MIN;

	timeh += dt->dt_sec/2;
	timel += (dt->dt_sec%2)*ETIME_L_HZ;

	timeh += EPOCHOFF;
	timeh -= (rtc_offset*SEC2MIN);

#ifdef VRRTCDEBUG
	cvt_timehl_ymdhms(timeh, timel, NULL);
#endif /* RTCDEBUG */

	bus_space_write_2(iot, ioh, ETIME_H_REG_W, (timeh >> 16) & 0xffff);
	bus_space_write_2(iot, ioh, ETIME_M_REG_W, timeh & 0xffff);
	bus_space_write_2(iot, ioh, ETIME_L_REG_W, timel);
}

void
cvt_timehl_ymdhms(
	u_int32_t timeh, /* 2 sec */
	u_int32_t timel, /* 1/32768 sec */
	struct clock_ymdhms *dt)
{
	u_int32_t year, month, date, hour, min, sec, sec2;

	timeh -= EPOCHOFF;

	timeh += (rtc_offset*SEC2MIN);

	year = EPOCHYEAR;
	sec2 = LEAPYEAR4(year)?SEC2YR+SEC2DAY:SEC2YR;
	while (timeh > sec2) {
		year++;
		timeh -= sec2;
		sec2 = LEAPYEAR4(year)?SEC2YR+SEC2DAY:SEC2YR;
	}

	DPRINTF(("cvt_timehl_ymdhms: timeh %08x year %d yrref %d\n", 
	    timeh, year, sec2));

	month = 0; /* now month is 0..11 */
	sec2 = SEC2DAY * m2d[month];
	while (timeh > sec2) {
		timeh -= sec2;
		month++;
		sec2 = SEC2DAY * m2d[month];
		if (month == 1 && LEAPYEAR4(year)) /* feb. and leapyear */
			sec2 += SEC2DAY;
	}
	month +=1; /* now month is 1..12 */

	DPRINTF(("cvt_timehl_ymdhms: timeh %08x month %d mref %d\n", 
	    timeh, month, sec2));

	sec2 = SEC2DAY;
	date = timeh/sec2+1; /* date is 1..31 */
	timeh -= (date-1)*sec2;

	DPRINTF(("cvt_timehl_ymdhms: timeh %08x date %d dref %d\n", 
	    timeh, date, sec2));

	sec2 = SEC2HOUR;
	hour = timeh/sec2;
	timeh -= hour*sec2;

	sec2 = SEC2MIN;
	min = timeh/sec2;
	timeh -= min*sec2;

	sec = timeh*2 + timel/ETIME_L_HZ;	

	DPRINTF(("cvt_timehl_ymdhms: hour %d min %d sec %d\n", hour, min, sec));

	if (dt) {
		dt->dt_year	= year - YBASE; /* base 1900 */
		dt->dt_mon	= month;
		dt->dt_day	= date;
		dt->dt_hour	= hour;
		dt->dt_min	= min;
		dt->dt_sec	= sec;
	}
}

void
vrrtc_dump_regs(struct vrrtc_softc *sc)
{
	int timeh;
	int timel;

	timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ETIME_H_REG_W);
	timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ETIME_M_REG_W);
	timel = (timel << 16) 
	    | bus_space_read_2(sc->sc_iot, sc->sc_ioh, ETIME_L_REG_W);
	printf("clock_init()  Elapse Time %04x%04x\n", timeh, timel);

	timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ECMP_H_REG_W);
	timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ECMP_M_REG_W);
	timel = (timel << 16) 
	    | bus_space_read_2(sc->sc_iot, sc->sc_ioh, ECMP_L_REG_W);
	printf("clock_init()  Elapse Compare %04x%04x\n", timeh, timel);

	timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL1_H_REG_W);
	timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL1_L_REG_W);
	printf("clock_init()  LONG1 %04x%04x\n", timeh, timel);

	timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL1_CNT_H_REG_W);
	timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL1_CNT_L_REG_W);
	printf("clock_init()  LONG1 CNTL %04x%04x\n", timeh, timel);

	timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL2_H_REG_W);
	timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL2_L_REG_W);
	printf("clock_init()  LONG2 %04x%04x\n", timeh, timel);

	timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL2_CNT_H_REG_W);
	timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, RTCL2_CNT_L_REG_W);
	printf("clock_init()  LONG2 CNTL %04x%04x\n", timeh, timel);

	if (TCLK_H_REG_W != RTC_NO_REG_W) {
		timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, TCLK_H_REG_W);
		timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, TCLK_L_REG_W);
		printf("clock_init()  TCLK %04x%04x\n", timeh, timel);

		timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh, TCLK_CNT_H_REG_W);
		timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh, TCLK_CNT_L_REG_W);
		printf("clock_init()  TCLK CNTL %04x%04x\n", timeh, timel);
	}
}
