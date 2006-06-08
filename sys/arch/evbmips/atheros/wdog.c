/* $NetBSD: wdog.c,v 1.1 2006/06/08 06:15:59 gdamore Exp $ */
/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 * Watchdog timer support for the AR5312.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdog.c,v 1.1 2006/06/08 06:15:59 gdamore Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <machine/cpu.h>

#include <mips/atheros/include/ar531xreg.h>
#include <mips/atheros/include/ar531xvar.h>

#include <dev/sysmon/sysmonvar.h>

#ifndef	WDOG_DEFAULT_PERIOD
#define	WDOG_DEFAULT_PERIOD	5
#endif

static int wdog_match(struct device *, struct cfdata *, void *);
static void wdog_attach(struct device *, struct device *, void *);
static int wdog_tickle(struct sysmon_wdog *);
static int wdog_setmode(struct sysmon_wdog *);

struct wdog_softc {
	struct device		sc_dev;
	struct sysmon_wdog	sc_smw;
	int			sc_wdog_period;
	int			sc_wdog_max;
	uint32_t		sc_wdog_reload;
	uint32_t		sc_mult;
};

CFATTACH_DECL(wdog, sizeof(struct wdog_softc),
    wdog_match, wdog_attach, NULL, NULL);

static int
wdog_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

static void
wdog_attach(struct device *parent, struct device *self, void *aux)
{
	struct wdog_softc *sc = (void *)self;

	sc->sc_mult = curcpu()->ci_cpu_freq / 4;
	sc->sc_wdog_period = WDOG_DEFAULT_PERIOD;
	sc->sc_wdog_max = 0xffffffffU / sc->sc_mult;
	sc->sc_wdog_reload = sc->sc_wdog_period * sc->sc_mult;
	printf(": %d second period\n", sc->sc_wdog_period);

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = wdog_setmode;
	sc->sc_smw.smw_tickle = wdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

}

static int
wdog_tickle(struct sysmon_wdog *smw)
{
	struct wdog_softc *sc = smw->smw_cookie;

	PUTSYSREG(AR531X_SYSREG_WDOG_TIMER, sc->sc_wdog_reload);
	return (0);
}

static int
wdog_setmode(struct sysmon_wdog *smw)
{
	struct wdog_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		PUTSYSREG(AR531X_SYSREG_WDOG_CTL, AR531X_WDOG_CTL_IGNORE);
		PUTSYSREG(AR531X_SYSREG_WDOG_TIMER, 0);
	} else {

		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = sc->sc_wdog_period;
		else if (smw->smw_period != sc->sc_wdog_period) {
			/*
			 * we can't set watchdog periods in excess of
			 * the processor maximum.
			 */
			if (smw->smw_period > sc->sc_wdog_max) {
				return EOPNOTSUPP;
			}
			sc->sc_wdog_period = smw->smw_period;
			sc->sc_wdog_reload = sc->sc_wdog_period * sc->sc_mult;
		}

		PUTSYSREG(AR531X_SYSREG_WDOG_TIMER, sc->sc_wdog_reload);
		PUTSYSREG(AR531X_SYSREG_WDOG_CTL, AR531X_WDOG_CTL_RESET);
	}
	
	return (0);
}
