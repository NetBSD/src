/* $NetBSD: admwdog.c,v 1.1.2.2 2007/03/24 14:54:51 yamt Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: admwdog.c,v 1.1.2.2 2007/03/24 14:54:51 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>
#include <sys/kernel.h>	/* for hz */

#include <machine/bus.h>

#include <dev/sysmon/sysmonvar.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_obiovar.h>
#include <mips/adm5120/dev/if_admswvar.h>

/* maximum watchdog period, seconds */
#define	ADMWDOG_MAX_PERIOD	(__SHIFTOUT_MASK(ADM5120_WDOG_WTS_MASK) / 100)

static int admwdog_setmode(struct sysmon_wdog *smw);
static int admwdog_tickle(struct sysmon_wdog *smw);

static inline uint32_t
admwdog_read(struct admsw_softc *sc)
{
	return bus_space_read_4(sc->sc_st, sc->sc_ioh, ADM5120_WDOG0);
}

static inline void
admwdog_write(struct admsw_softc *sc, uint32_t val)
{
	bus_space_write_4(sc->sc_st, sc->sc_ioh, ADM5120_WDOG0, val);
}

static void
admwdog_reset(struct admsw_softc *sc)
{
	int s;
	uint32_t wdog0;

	wdog0 = __SHIFTIN(sc->sc_smw.smw_period * 100, ADM5120_WDOG_WTS_MASK) |
	    __SHIFTIN(1, ADM5120_WDOG_WT_MASK);

	s = splhigh();

	admwdog_write(sc, wdog0);
	(void)admwdog_read(sc);
	admwdog_write(sc, ADM5120_WDOG0_WTTR | wdog0);

	splx(s);
}

static int
admwdog_setmode(struct sysmon_wdog *smw)
{
	struct admsw_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		admwdog_write(sc, 0);
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		smw->smw_period = 32;
	else if (smw->smw_period > ADMWDOG_MAX_PERIOD)
		return EINVAL;

	admwdog_reset(sc);

	return 0;
}

static int
admwdog_tickle(struct sysmon_wdog *smw)
{
	struct admsw_softc *sc = smw->smw_cookie;

	(void)admwdog_read(sc);
	return 0;
}

void
admwdog_attach(struct admsw_softc *sc)
{
	struct sysmon_wdog *smw = &sc->sc_smw;

	/* deactivate watchdog */
	admwdog_write(sc, 0);

	smw->smw_name = device_xname(&sc->sc_dev);
	smw->smw_cookie = sc;
	smw->smw_setmode = admwdog_setmode;
	smw->smw_tickle = admwdog_tickle;
	smw->smw_period = ADMWDOG_MAX_PERIOD;

	sysmon_wdog_register(&sc->sc_smw);
}
