/*	$NetBSD: ralink_wdog.c,v 1.2 2011/07/28 15:38:49 matt Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ra_wdog.c -- Ralink 305x Watchdog Timer driver
 *
 * Timer 1 is used as a system reset watchdog timer
 * Timer 0 is (optionally) used as a periodic watchdog service interrupt
 *
 * NetBSD sysmon watchdog is used in mode defined by RA_WDOG_DEFAULT_MODE
 * (which can be set via kernel config), or by mode passed to
 * our 'smw_setmode' function.  The mode used determines what
 * mechanism is used to periodically service the watchdog.
 *
 * KTICKLE mode is default and supports 2 variants, allowing some control
 * over the priority of the service routine:
 *
 * 1. the specified reset period is a positive integer:
 *    A callout runs the 'smw_tickle' function at IPL_SOFTCLOCK for service.
 *    If your system cannot make "forward progress" without softints running,
 *    you should use this variant.
 *
 * 2. the specified reset period is a negative integer:
 *    Timer 0 interrupt runs ra_wdog_timer0() at IPL_VM for service.
 *    If your system can make "forward progress" while spelding long times
 *    at IPL_VM, you should use this variant.
 *    The numbner is rectified 
 *
 * The reset period is defined by RA_WDOG_DEFAULT_PERIOD
 * (which can be set via kernel config), or by period passed to
 * our 'smw_setmode' function.  The interrupt service interval
 * is half the reset interval.
 *
 */

#include "rwdog.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_wdog.c,v 1.2 2011/07/28 15:38:49 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/wdog.h>

#include <mips/ralink/ralink_var.h>
#include <mips/ralink/ralink_reg.h>

#include <dev/sysmon/sysmonvar.h>

#if 0
# define DISABLE_WATCHDOG
#endif

#ifndef RA_WDOG_DEFAULT_MODE
# define RA_WDOG_DEFAULT_MODE	WDOG_MODE_KTICKLE
#endif

/*
 * PERIODs are in in seconds;
 * the counter is 16-bits;
 * maximum period depends on bus freq
 */
#ifndef RA_WDOG_DEFAULT_PERIOD
# define RA_WDOG_DEFAULT_PERIOD	10
#endif
#define WDOG_COUNT_MASK		0xffff
#define WDOG_MAX_COUNT		WDOG_COUNT_MASK
#define WDOG_MAX_PERIOD	\
		(WDOG_MAX_COUNT / (RA_BUS_FREQ / WDOG_MAX_COUNT))

static int  ra_wdog_match(device_t, cfdata_t, void *);
static void ra_wdog_attach(device_t, device_t, void *);
static int  ra_wdog_tickle(struct sysmon_wdog *);
static int  ra_wdog_timer0(void *);
static int  ra_wdog_setmode(struct sysmon_wdog *);

extern int sysmon_wdog_setmode(struct sysmon_wdog *, int, u_int);

typedef struct ra_wdog_softc {
	device_t		sc_dev;
	struct sysmon_wdog 	sc_smw;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;
	void			*sc_ih;
} ra_wdog_softc_t;


CFATTACH_DECL_NEW(rwdog, sizeof(struct ra_wdog_softc),
	ra_wdog_match, ra_wdog_attach, NULL, NULL);

static const char *wdog_modestr[WDOG_MODE_MASK+1] = {
	[ WDOG_MODE_DISARMED ] = "DISARMED",
	[ WDOG_MODE_KTICKLE  ] = "KTICKLE",
	[ WDOG_MODE_UTICKLE  ] = "UTICKLE",
	[ WDOG_MODE_ETICKLE  ] = "ETICKLE"
};

static inline void
ra_wdog_reset(const ra_wdog_softc_t *sc)
{
	uint32_t r;

	r = bus_space_read_4(sc->sc_memt, sc->sc_memh, RA_TIMER_STAT);
	r |= TIMER_1_RESET;
	bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_STAT, r);
}

static inline u_int32_t
ra_wdog_sec_to_count(u_int nsec)
{
	KASSERT(nsec <= WDOG_MAX_PERIOD);
	const u_int32_t count = (RA_BUS_FREQ / WDOG_MAX_COUNT) * nsec;
	KASSERT(count <= WDOG_MAX_COUNT);
	return count;
}

static int
ra_wdog_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
ra_wdog_attach(device_t parent, device_t self, void *aux)
{
	ra_wdog_softc_t * const sc = device_private(self);
	const struct mainbus_attach_args *ma = aux;
	bus_space_handle_t memh;
	int error;

	aprint_naive(": Ralink watchdog controller\n");
	aprint_normal(": Ralink watchdog controller\n");
	aprint_normal_dev(self, "max period %d sec.\n", WDOG_MAX_PERIOD);

	error = bus_space_map(ma->ma_memt, RA_TIMER_BASE, 0x100, 0, &memh);
	if (error != 0) {
		aprint_error_dev(self, "unable to map registers, "
			"error=%d\n", error);
                return;
	}
 
	sc->sc_memt = ma->ma_memt;
	sc->sc_memh = memh;

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = ra_wdog_setmode;
	sc->sc_smw.smw_tickle = ra_wdog_tickle;
	sc->sc_smw.smw_period = RA_WDOG_DEFAULT_PERIOD;

	error = sysmon_wdog_register(&sc->sc_smw);
	if (error != 0)
		aprint_error_dev(self, "unable to register with sysmon, "
			"error %d\n", error);

	sc->sc_ih = ra_intr_establish(RA_IRQ_TIMER0, ra_wdog_timer0, sc, 0);
	if (sc->sc_ih == NULL)
		aprint_error_dev(self, "unable to establish interrupt\n");
			/* expect watchdog reset shortly */

	if (RA_WDOG_DEFAULT_MODE == WDOG_MODE_DISARMED) {
		/*
		 * disarm the watchdog
		 */
		bus_space_write_4(sc->sc_memt, memh, RA_TIMER_0_CNTRL, 0);
		bus_space_write_4(sc->sc_memt, memh, RA_TIMER_1_CNTRL, 0);
		aprint_normal_dev(self, "%s mode\n",
			wdog_modestr[sc->sc_smw.smw_mode]);
	} else {
		/*
		 * initialize and arm the watchdog now.
		 * if boot loader already initialized the watchdog
		 * then we are re-initializing; this will buy some time
		 * until interrupts are enabled, and will establish our
		 * (default) mode and smw_period indedpendent of the
		 * boot loader.
		 */
		error = sysmon_wdog_setmode(&sc->sc_smw, RA_WDOG_DEFAULT_MODE,
			RA_WDOG_DEFAULT_PERIOD);
		if (error != 0) {
			aprint_error_dev(self, "unable to set sysmon wdog, "
				"mode %d, error %d\n",
				RA_WDOG_DEFAULT_MODE, error);
		} else {
			aprint_normal_dev(self, "%s mode, period %d sec.\n",
				wdog_modestr[sc->sc_smw.smw_mode],
				sc->sc_smw.smw_period);
		}
	}
}

/*
 * ra_wdog_tickle - smw watchdog service function
 */
static int
ra_wdog_tickle(struct sysmon_wdog *smw)
{
	const ra_wdog_softc_t * const sc = smw->smw_cookie;
	ra_wdog_reset(sc);
	return 0;
}

/*
 * ra_wdog_timer0 - periodic watchdog service ISR
 */
static int
ra_wdog_timer0(void *arg)
{
	const ra_wdog_softc_t * const sc = arg;
	ra_wdog_reset(sc);
	return 0;
}

static int
ra_wdog_setmode(struct sysmon_wdog *smw)
{
	const ra_wdog_softc_t * const sc = smw->smw_cookie;
	u_int period = smw->smw_period;
	bool itickle = false;
	uint32_t r;

	if (((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_KTICKLE) &&
	    ((int)period < 0)) {
		itickle = true;		/* use Timer 0 */
		period = -period;
	}

	/* all configuration has to be done with the timer disabled */
	bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_0_CNTRL, 0);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_1_CNTRL, 0);

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED)
		return 0;

	if (period > WDOG_MAX_PERIOD)
		return EOPNOTSUPP;

	/* Set the new watchdog reset period in Timer 1 */
	r = ra_wdog_sec_to_count(period);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_1_LOAD, r);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_1_CNTRL,
		TIMER_EN | TIMER_MODE(TIMER_MODE_WDOG) |
			TIMER_PRESCALE(TIMER_PRESCALE_DIV_65536));

	if (itickle) {
		/* Set the new watchdog service period in Timer 0 */
		r = ra_wdog_sec_to_count(period) / 2;
		bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_0_LOAD, r);
		bus_space_write_4(sc->sc_memt, sc->sc_memh, RA_TIMER_0_CNTRL,
			TIMER_EN | TIMER_MODE(TIMER_MODE_PERIODIC) |
				TIMER_PRESCALE(TIMER_PRESCALE_DIV_65536));
	}

	return 0;
}
