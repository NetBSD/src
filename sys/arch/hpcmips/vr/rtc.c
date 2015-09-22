/*	$NetBSD: rtc.c,v 1.34.2.1 2015/09/22 12:05:43 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.34.2.1 2015/09/22 12:05:43 skrll Exp $");

#include "opt_vr41xx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/device.h>
#include <sys/cpu.h>

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
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;
#ifndef SINGLE_VRIP_BASE
	int sc_rtcint_reg;
	int sc_tclk_h_reg, sc_tclk_l_reg;
	int sc_tclk_cnt_h_reg, sc_tclk_cnt_l_reg;
#endif /* SINGLE_VRIP_BASE */
	int64_t sc_epoch;
	struct todr_chip_handle sc_todr;
	struct timecounter sc_tc;
};

void	vrrtc_init(device_t);
int	vrrtc_get(todr_chip_handle_t, struct timeval *);
int	vrrtc_set(todr_chip_handle_t, struct timeval *);
uint32_t vrrtc_get_timecount(struct timecounter *);

struct platform_clock vr_clock = {
#define CLOCK_RATE	128
	CLOCK_RATE, vrrtc_init,
};

int	vrrtc_match(device_t, cfdata_t, void *);
void	vrrtc_attach(device_t, device_t, void *);
int	vrrtc_intr(void*, vaddr_t, uint32_t);
void	vrrtc_dump_regs(struct vrrtc_softc *);

CFATTACH_DECL_NEW(vrrtc, sizeof(struct vrrtc_softc),
    vrrtc_match, vrrtc_attach, NULL, NULL);

int
vrrtc_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

#ifndef SINGLE_VRIP_BASE
#define RTCINT_REG_W		(sc->sc_rtcint_reg)
#define TCLK_H_REG_W		(sc->sc_tclk_h_reg)
#define TCLK_L_REG_W		(sc->sc_tclk_l_reg)
#define TCLK_CNT_H_REG_W	(sc->sc_tclk_cnt_h_reg)
#define TCLK_CNT_L_REG_W	(sc->sc_tclk_cnt_l_reg)
#endif /* SINGLE_VRIP_BASE */

void
vrrtc_attach(device_t parent, device_t self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vrrtc_softc *sc = device_private(self);
	int year;

#ifndef SINGLE_VRIP_BASE
	if (va->va_addr == VR4102_RTC_ADDR) {
		sc->sc_rtcint_reg = VR4102_RTCINT_REG_W;
		sc->sc_tclk_h_reg = VR4102_TCLK_H_REG_W;
		sc->sc_tclk_l_reg = VR4102_TCLK_L_REG_W;
		sc->sc_tclk_cnt_h_reg = VR4102_TCLK_CNT_H_REG_W;
		sc->sc_tclk_cnt_l_reg = VR4102_TCLK_CNT_L_REG_W;
	} else if (va->va_addr == VR4122_RTC_ADDR) {
		sc->sc_rtcint_reg = VR4122_RTCINT_REG_W;
		sc->sc_tclk_h_reg = VR4122_TCLK_H_REG_W;
		sc->sc_tclk_l_reg = VR4122_TCLK_L_REG_W;
		sc->sc_tclk_cnt_h_reg = VR4122_TCLK_CNT_H_REG_W;
		sc->sc_tclk_cnt_l_reg = VR4122_TCLK_CNT_L_REG_W;
	} else if (va->va_addr == VR4181_RTC_ADDR) {
		sc->sc_rtcint_reg = VR4181_RTCINT_REG_W;
		sc->sc_tclk_h_reg = RTC_NO_REG_W;
		sc->sc_tclk_l_reg = RTC_NO_REG_W;
		sc->sc_tclk_cnt_h_reg = RTC_NO_REG_W;
		sc->sc_tclk_cnt_l_reg = RTC_NO_REG_W;
	} else {
		panic("%s: unknown base address 0x%lx",
		    device_xname(self), va->va_addr);
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
	/* But need to set level 1 interrupt mask register, 
	 * so register fake interrurpt handler
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

	/*
	 * Figure out the epoch, which could be either forward or
	 * backwards in time.  We assume that the start date will always
	 * be on Jan 1.
	 */
	for (year = EPOCHYEAR; year < POSIX_BASE_YEAR; year++) {
		sc->sc_epoch += days_per_year(year) * SECS_PER_DAY;
	}
	for (year = POSIX_BASE_YEAR; year < EPOCHYEAR; year++) {
		sc->sc_epoch -= days_per_year(year) * SECS_PER_DAY;
	}

	/*
	 * Initialize MI todr(9)
	 */
	sc->sc_todr.todr_settime = vrrtc_set;
	sc->sc_todr.todr_gettime = vrrtc_get;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);

	platform_clock_attach(self, &vr_clock);
}

int
vrrtc_intr(void *arg, vaddr_t pc, uint32_t status)
{
	struct vrrtc_softc *sc = arg;
	struct clockframe cf;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCINT_REG_W, RTCINT_ALL);
	cf.pc = pc;
	cf.sr = status;
	cf.intr = (curcpu()->ci_idepth > 1);
	hardclock(&cf);

	return 0;
}

void
vrrtc_init(device_t self)
{
	struct vrrtc_softc *sc = device_private(self);

	DDUMP_REGS(sc);
	/*
	 * Set tick (CLOCK_RATE)
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, RTCL1_H_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
	    RTCL1_L_REG_W, RTCL1_L_HZ / CLOCK_RATE);

	/*
	 * Initialize timecounter.
	 */
	sc->sc_tc.tc_get_timecount = vrrtc_get_timecount;
	sc->sc_tc.tc_name = "vrrtc";
	sc->sc_tc.tc_counter_mask = 0xffff;
	sc->sc_tc.tc_frequency = ETIME_L_HZ;
	sc->sc_tc.tc_priv = sc;
	sc->sc_tc.tc_quality = 100;
	tc_init(&sc->sc_tc);
}

uint32_t
vrrtc_get_timecount(struct timecounter *tc)
{
	struct vrrtc_softc *sc = (struct vrrtc_softc *)tc->tc_priv;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	return bus_space_read_2(iot, ioh, ETIME_L_REG_W);
}

int
vrrtc_get(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct vrrtc_softc *sc = (struct vrrtc_softc *)tch->cookie;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	uint32_t timeh;		/* elapse time (2*timeh sec) */
	uint32_t timel;		/* timel/32768 sec */
	uint64_t sec, usec;

	timeh = bus_space_read_2(iot, ioh, ETIME_H_REG_W);
	timeh = (timeh << 16) | bus_space_read_2(iot, ioh, ETIME_M_REG_W);
	timel = bus_space_read_2(iot, ioh, ETIME_L_REG_W);

	DPRINTF(("clock_get: timeh %08x timel %08x\n", timeh, timel));

	timeh -= EPOCHOFF;
	sec = (uint64_t)timeh * 2;
	sec -= sc->sc_epoch;
	tvp->tv_sec = sec;
	tvp->tv_sec += timel / ETIME_L_HZ;

	/* scale from 32kHz to 1MHz */
	usec = (timel % ETIME_L_HZ);
	usec *= 1000000;
	usec /= ETIME_L_HZ;
	tvp->tv_usec = usec;

	return 0;
}

int
vrrtc_set(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct vrrtc_softc *sc = (struct vrrtc_softc *)tch->cookie;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	uint32_t timeh;		/* elapse time (2*timeh sec) */
	uint32_t timel;		/* timel/32768 sec */
	int64_t sec, cnt;

	sec = tvp->tv_sec + sc->sc_epoch;
	sec += sc->sc_epoch;
	timeh = EPOCHOFF + (sec / 2);
	timel = sec % 2;

	cnt = tvp->tv_usec;
	/* scale from 1MHz to 32kHz */
	cnt *= ETIME_L_HZ;
	cnt /= 1000000;
	timel += (uint32_t)cnt;

	bus_space_write_2(iot, ioh, ETIME_H_REG_W, (timeh >> 16) & 0xffff);
	bus_space_write_2(iot, ioh, ETIME_M_REG_W, timeh & 0xffff);
	bus_space_write_2(iot, ioh, ETIME_L_REG_W, timel);

	return 0;
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

		timeh = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
		    TCLK_CNT_H_REG_W);
		timel = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
		    TCLK_CNT_L_REG_W);
		printf("clock_init()  TCLK CNTL %04x%04x\n", timeh, timel);
	}
}
