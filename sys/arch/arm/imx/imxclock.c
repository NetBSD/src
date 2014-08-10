/*	$NetBSD: imxclock.c,v 1.6.12.1 2014/08/10 06:53:51 tls Exp $ */
/*
 * Copyright (c) 2009, 2010  Genetec corp.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec corp.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * common part for i.MX31 and i.MX51
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imxclock.c,v 1.6.12.1 2014/08/10 06:53:51 tls Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/imx/imxclockvar.h>
#include <arm/imx/imxepitreg.h>

static u_int imx_epit_get_timecount(struct timecounter *);
static int imxclock_intr(void *);

static struct timecounter imx_epit_timecounter = {
	imx_epit_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	0,			/* frequency */
	"epit",			/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static volatile uint32_t imxclock_base;
struct imxclock_softc *imxclock = NULL;

void
cpu_initclocks(void)
{
	uint32_t reg;
	u_int freq;

	if (epit1_sc != NULL)
		imxclock = epit1_sc;
	else if (epit2_sc != NULL)
		imxclock = epit2_sc;
	else
		panic("%s: driver has not been initialized!", __FUNCTION__);

	freq = imxclock_get_timerfreq(imxclock);
	imx_epit_timecounter.tc_frequency = freq;
	tc_init(&imx_epit_timecounter);

	aprint_verbose_dev(imxclock->sc_dev,
	    "timer clock frequency %d\n", freq);

	imxclock->sc_reload_value = freq / hz - 1;

	/* stop timers */
	bus_space_write_4(imxclock->sc_iot, imxclock->sc_ioh, EPIT_EPITCR, 0);

	aprint_normal("clock: hz=%d stathz = %d\n", hz, stathz);

	bus_space_write_4(imxclock->sc_iot, imxclock->sc_ioh, EPIT_EPITLR,
			  imxclock->sc_reload_value);
	bus_space_write_4(imxclock->sc_iot, imxclock->sc_ioh, EPIT_EPITCMPR, 0);

	reg = EPITCR_ENMOD | EPITCR_IOVW | EPITCR_RLD | imxclock->sc_clksrc;
	bus_space_write_4(imxclock->sc_iot, imxclock->sc_ioh,
	    EPIT_EPITCR, reg);
	reg |= EPITCR_EN | EPITCR_OCIEN | EPITCR_WAITEN | EPITCR_DOZEN |
		EPITCR_STOPEN;
	bus_space_write_4(imxclock->sc_iot, imxclock->sc_ioh,
	    EPIT_EPITCR, reg);

	epit1_sc->sc_ih = intr_establish(imxclock->sc_intr, IPL_CLOCK,
	    IST_LEVEL, imxclock_intr, NULL);
}

void
setstatclockrate(int schz)
{
}

static int
imxclock_intr(void *arg)
{
	struct imxclock_softc *sc = imxclock;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EPIT_EPITSR, 1);
	atomic_add_32(&imxclock_base, sc->sc_reload_value);

	hardclock((struct clockframe *)arg);

	return 1;
}

u_int
imx_epit_get_timecount(struct timecounter *tc)
{
	uint32_t counter;
	uint32_t base;
	u_int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	counter = bus_space_read_4(imxclock->sc_iot, imxclock->sc_ioh, EPIT_EPITCNT);
	base = imxclock_base;
	restore_interrupts(oldirqstate);

	return base - counter;
}
