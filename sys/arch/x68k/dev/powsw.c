/*	$NetBSD: powsw.c,v 1.2 2022/05/26 14:33:29 tsutsui Exp $	*/

/*
 * Copyright (c) 2011 Tetsuya Isaki. All rights reserved.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Power switch monitor
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powsw.c,v 1.2 2022/05/26 14:33:29 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/callout.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/mfp.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include "ioconf.h"

extern int power_switch_is_off;		/* XXX should be in .h */

//#define POWSW_DEBUG

#if defined(POWSW_DEBUG)
#define DPRINTF(fmt...)	printf(fmt)
#define DEBUG_LOG_ADD(c)	sc->sc_log[sc->sc_loglen++] = (c)
#define DEBUG_LOG_PRINT()	do {	\
	sc->sc_log[sc->sc_loglen] = '\0';	\
	printf("%s", sc->sc_log);	\
} while (0)
#else
#define DPRINTF(fmt...)
#define DEBUG_LOG_ADD(c)
#define DEBUG_LOG_PRINT()
#endif

/* mask */
#define POWSW_ALARM		(0x01)
#define POWSW_EXTERNAL	(0x02)
#define POWSW_FRONT		(0x04)

/* parameter */
#define POWSW_MAX_TICK	(30)
#define POWSW_THRESHOLD	(10)

struct powsw_softc {
	device_t sc_dev;
	struct sysmon_pswitch sc_smpsw;
	callout_t sc_callout;
	int sc_mask;
	int sc_prev;
	int sc_last_sw;
	int sc_tick;
	int sc_count;
#if defined(POWSW_DEBUG)
	char sc_log[100];
	int sc_loglen;
#endif
};

static int  powsw_match(device_t, cfdata_t, void *);
static void powsw_attach(device_t, device_t, void *);
static int  powsw_intr(void *);
static void powsw_softintr(void *);
static void powsw_pswitch_event(void *);
static void powsw_shutdown_check(void *);
static void powsw_reset_counter(struct powsw_softc *);
static void powsw_set_aer(struct powsw_softc *, int);

CFATTACH_DECL_NEW(powsw, sizeof(struct powsw_softc),
    powsw_match, powsw_attach, NULL, NULL);


typedef const struct {
	int vector;			/* interrupt vector */
	int mask;			/* mask bit for MFP GPIP */
	const char *name;
} powsw_desc_t;

static powsw_desc_t powsw_desc[2] = {
	{ 66, POWSW_FRONT,		"Front Switch", },
	{ 65, POWSW_EXTERNAL,	"External Power Switch", },
	/* XXX I'm not sure about alarm bit */
};


static int
powsw_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
powsw_attach(device_t parent, device_t self, void *aux)
{
	struct powsw_softc *sc = device_private(self);
	powsw_desc_t *desc;
	const char *xname;
	int unit;
	int sw;

	unit = device_unit(self);
	xname = device_xname(self);
	desc = &powsw_desc[unit];

	memset(sc, 0, sizeof(*sc));
	sc->sc_dev = self;
	sc->sc_mask = desc->mask;
	sc->sc_prev = -1;
	powsw_reset_counter(sc);

	sysmon_task_queue_init();
	sc->sc_smpsw.smpsw_name = xname;
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0)
		panic("can't register with sysmon");

	callout_init(&sc->sc_callout, 0);
	callout_setfunc(&sc->sc_callout, powsw_softintr, sc);

	if (shutdownhook_establish(powsw_shutdown_check, sc) == NULL)
		panic("%s: can't establish shutdown hook", xname);

	if (intio_intr_establish(desc->vector, xname, powsw_intr, sc) < 0)
		panic("%s: can't establish interrupt", xname);

	/* Set AER and enable interrupt */
	sw = (mfp_get_gpip() & sc->sc_mask);
	powsw_set_aer(sc, sw ? 0 : 1);
	mfp_bit_set_ierb(sc->sc_mask);

	aprint_normal(": %s\n", desc->name);
}

static int
powsw_intr(void *arg)
{
	struct powsw_softc *sc = arg;

	if (sc->sc_tick == 0) {
		mfp_bit_clear_ierb(sc->sc_mask);
		sc->sc_tick++;
		DEBUG_LOG_ADD('i');
		/*
		 * The button state seems unstable for few ticks,
		 * so wait a bit to settle.
		 */
		callout_schedule(&sc->sc_callout, 1);
	} else {
		DEBUG_LOG_ADD('x');
	}
	return 0;
}

void
powsw_softintr(void *arg)
{
	struct powsw_softc *sc = arg;
	int sw;
	int s;

	s = spl6();

	if (sc->sc_tick++ >= POWSW_MAX_TICK) {
		/* tick is over, broken switch? */
		printf("%s: unstable power switch?, ignored\n",
		    device_xname(sc->sc_dev));
		powsw_reset_counter(sc);

		mfp_bit_set_ierb(sc->sc_mask);
		splx(s);
		return;
	}

	sw = (mfp_get_gpip() & sc->sc_mask) ? 1 : 0;
	DEBUG_LOG_ADD('0' + sw);

	if (sw == sc->sc_last_sw) {
		sc->sc_count++;
	} else {
		sc->sc_last_sw = sw;
		sc->sc_count = 1;
	}

	if (sc->sc_count < POWSW_THRESHOLD) {
		callout_schedule(&sc->sc_callout, 1);
	} else {
		/* switch seems stable */
		DEBUG_LOG_PRINT();

		if (sc->sc_last_sw == sc->sc_prev) {
			/* switch state is not changed, it was a noise */
			DPRINTF(" ignore(sw=%d,prev=%d)\n", sc->sc_last_sw, sc->sc_prev);
		} else {
			/* switch state has been changed */
			sc->sc_prev = sc->sc_last_sw;
			powsw_set_aer(sc, 1 - sc->sc_prev);
			sysmon_task_queue_sched(0, powsw_pswitch_event, sc);
		}
		powsw_reset_counter(sc);
		mfp_bit_set_ierb(sc->sc_mask);	// enable interrupt
	}

	splx(s);
}

static void
powsw_pswitch_event(void *arg)
{
	struct powsw_softc *sc = arg;
	int poweroff;

	poweroff = sc->sc_prev;

	DPRINTF(" %s is %s\n", device_xname(sc->sc_dev),
	    poweroff ? "off(PRESS)" : "on(RELEASE)");

	sysmon_pswitch_event(&sc->sc_smpsw,
		poweroff ? PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
}

static void
powsw_shutdown_check(void *arg)
{
	struct powsw_softc *sc = arg;
	int poweroff;

	poweroff = sc->sc_prev;
	if (poweroff)
		power_switch_is_off = 1;
	DPRINTF("powsw_shutdown_check %s = %d\n",
		device_xname(sc->sc_dev), power_switch_is_off);
}

static void
powsw_reset_counter(struct powsw_softc *sc)
{
	sc->sc_last_sw = -1;
	sc->sc_tick = 0;
	sc->sc_count = 0;
#if defined(POWSW_DEBUG)
	sc->sc_loglen = 0;
#endif
}

static void
powsw_set_aer(struct powsw_softc *sc, int aer)
{
	KASSERT(aer == 0 || aer == 1);

	if (aer == 0) {
		mfp_bit_clear_aer(sc->sc_mask);
	} else {
		mfp_bit_set_aer(sc->sc_mask);
	}
	DPRINTF(" SetAER=%d", aer);
}
