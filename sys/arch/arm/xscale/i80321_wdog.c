/*	$NetBSD: i80321_wdog.c,v 1.6 2003/07/15 00:24:54 lukem Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Watchdog timer support for the Intel i80321 I/O processor.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i80321_wdog.c,v 1.6 2003/07/15 00:24:54 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <machine/bus.h>
#include <arm/cpufunc.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <dev/sysmon/sysmonvar.h>

struct iopwdog_softc {
	struct device sc_dev;
	struct sysmon_wdog sc_smw;
	int sc_wdog_armed;
	int sc_wdog_period;
};

static __inline void
wdtcr_write(uint32_t val)
{

	__asm __volatile("mcr p6, 0, %0, c7, c1, 0"
		:
		: "r" (val));
}

static int
iopwdog_tickle(struct sysmon_wdog *smw)
{

	wdtcr_write(WDTCR_ENABLE1);
	wdtcr_write(WDTCR_ENABLE2);
	return (0);
}

static int
iopwdog_setmode(struct sysmon_wdog *smw)
{
	struct iopwdog_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* The i80321 watchdog can't be disarmed once armed. */
		if (sc->sc_wdog_armed)
			return (EOPNOTSUPP);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = sc->sc_wdog_period;
		else if (smw->smw_period != sc->sc_wdog_period) {
			/* Can't change the i80321 watchdog period. */
			return (EOPNOTSUPP);
		}
		sc->sc_wdog_armed = 1;
		/* Watchdog is armed by tickling it. */
		iopwdog_tickle(smw);
	}
	return (0);
}

static int
iopwdog_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iopxs_attach_args *ia = aux;

	if (strcmp(cf->cf_name, ia->ia_name) == 0)
		return (1);

	return (0);
}

static void
iopwdog_attach(struct device *parent, struct device *self, void *aux)
{
	struct iopwdog_softc *sc = (void *) self;

	/*
	 * XXX Should compute the period based on processor speed.
	 * For a 600MHz XScale core, the wdog must be tickled approx.
	 * every 7 seconds.
	 */
	sc->sc_wdog_period = 7;

	aprint_naive(": Watchdog timer\n");
	aprint_normal(": %d second period\n", sc->sc_wdog_period);

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = iopwdog_setmode;
	sc->sc_smw.smw_tickle = iopwdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

CFATTACH_DECL(iopwdog, sizeof(struct iopwdog_softc),
    iopwdog_match, iopwdog_attach, NULL, NULL);
