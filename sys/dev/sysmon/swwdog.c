/*	$NetBSD: swwdog.c,v 1.6.8.1 2007/07/11 20:08:24 mjf Exp $	*/

/*
 * Copyright (c) 2004, 2005 Steven M. Bellovin
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Steven M. Bellovin
 * 4. The name of the author contributors may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: swwdog.c,v 1.6.8.1 2007/07/11 20:08:24 mjf Exp $");

/*
 *
 * Software watchdog timer
 *
 */
#include <sys/param.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/wdog.h>
#include <dev/sysmon/sysmonvar.h>

#include "swwdog.h"

struct swwdog_softc {
	struct sysmon_wdog sc_smw;
	struct callout sc_c;
	char sc_name[20];
	int sc_wdog_armed;
} sc_wdog[NSWWDOG];

void	swwdogattach(int);

static int swwdog_setmode(struct sysmon_wdog *);
static int swwdog_tickle(struct sysmon_wdog *);

static int swwdog_arm(struct swwdog_softc *);
static int swwdog_disarm(struct swwdog_softc *);

static void swwdog_panic(void *);

int swwdog_reboot = 0;		/* set for panic instead of reboot */

#define	SWDOG_DEFAULT	60		/* 60-second default period */

void
swwdogattach(int count __unused)
{
	int i;

	for (i = 0; i < NSWWDOG; i++) {
		struct swwdog_softc *sc = &sc_wdog[i];

		snprintf(sc->sc_name, sizeof sc->sc_name, "swwdog%d", i);
		printf("%s: ", sc->sc_name);
		sc->sc_smw.smw_name = sc->sc_name;
		sc->sc_smw.smw_cookie = sc;
		sc->sc_smw.smw_setmode = swwdog_setmode;
		sc->sc_smw.smw_tickle = swwdog_tickle;
		sc->sc_smw.smw_period = SWDOG_DEFAULT;
		callout_init(&sc->sc_c, 0);
		callout_setfunc(&sc->sc_c, swwdog_panic, sc);

		if (sysmon_wdog_register(&sc->sc_smw) == 0)
			printf("software watchdog initialized\n");
		else
			printf("unable to register software watchdog "
			    "with sysmon\n");
	}
}

static int
swwdog_setmode(struct sysmon_wdog *smw)
{
	struct swwdog_softc *sc = smw->smw_cookie;
	int error = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		error = swwdog_disarm(sc);
	} else {
		if (smw->smw_period == 0)
			return EINVAL;
		else if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			sc->sc_smw.smw_period = SWDOG_DEFAULT;
		else
			sc->sc_smw.smw_period = smw->smw_period;
		error = swwdog_arm(sc);
	}
	return error;
}

static int
swwdog_tickle(struct sysmon_wdog *smw)
{

	return swwdog_arm(smw->smw_cookie);
}

static int
swwdog_arm(struct swwdog_softc *sc)
{

	callout_schedule(&sc->sc_c, sc->sc_smw.smw_period * hz);
	return 0;
}

static int
swwdog_disarm(struct swwdog_softc *sc)
{

	callout_stop(&sc->sc_c);
	return 0;
}

static void
swwdog_panic(void *vsc)
{
	struct swwdog_softc *sc = vsc;
	int do_panic;

	do_panic = swwdog_reboot;
	swwdog_reboot = 1;
	callout_schedule(&sc->sc_c, 60 * hz);	/* deliberate double-panic */

	printf("%s: %d second timer expired", sc->sc_name,
	    sc->sc_smw.smw_period);

	if (do_panic)
		panic("watchdog timer expired");
	else
		cpu_reboot(0, NULL);
}
