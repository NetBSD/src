/* $NetBSD: sbwdog.c,v 1.7.94.1 2009/11/23 18:28:47 matt Exp $ */

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
 * Watchdog timer support for the Broadcom BCM1250 processor.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbwdog.c,v 1.7.94.1 2009/11/23 18:28:47 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <machine/locore.h>

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/dev/sbscdvar.h>

#define	SBWDOG_DEFAULT_PERIOD	5	/* Default to 5 seconds. */

struct sbwdog_softc {
	struct device sc_dev;
	struct sysmon_wdog sc_smw;
	u_long sc_addr;
	int sc_wdog_armed;
	int sc_wdog_period;
};

static int sbwdog_match(struct device *, struct cfdata *, void *);
static void sbwdog_attach(struct device *, struct device *, void *);
static int sbwdog_tickle(struct sysmon_wdog *);
static int sbwdog_setmode(struct sysmon_wdog *);

CFATTACH_DECL(sbwdog, sizeof(struct sbwdog_softc),
    sbwdog_match, sbwdog_attach, NULL, NULL);

#define	READ_REG(rp)		(mips3_ld((volatile uint64_t *)(rp)))
#define	WRITE_REG(rp, val)	(mips3_sd((volatile uint64_t *)(rp), (val)))

static int
sbwdog_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbscd_attach_args *sa = aux;

	if (sa->sa_locs.sa_type != SBSCD_DEVTYPE_WDOG)
		return (0);

	return (1);
}

static void
sbwdog_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbwdog_softc *sc = (void *)self;
	struct sbscd_attach_args *sa = aux;

	sc->sc_wdog_period = SBWDOG_DEFAULT_PERIOD;
	sc->sc_addr = MIPS_PHYS_TO_KSEG1(sa->sa_base + sa->sa_locs.sa_offset);

	printf(": %d second period\n", sc->sc_wdog_period);

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = sbwdog_setmode;
	sc->sc_smw.smw_tickle = sbwdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

static int
sbwdog_tickle(struct sysmon_wdog *smw)
{
	struct sbwdog_softc *sc = smw->smw_cookie;

	WRITE_REG(sc->sc_addr + R_SCD_WDOG_CFG, M_SCD_WDOG_ENABLE);
	return (0);
}

static int
sbwdog_setmode(struct sysmon_wdog *smw)
{
	struct sbwdog_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		if (sc->sc_wdog_armed)
			WRITE_REG(sc->sc_addr + R_SCD_WDOG_CFG, 0);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
			sc->sc_wdog_period = SBWDOG_DEFAULT_PERIOD;
			smw->smw_period = SBWDOG_DEFAULT_PERIOD;	/* XXX needed?? */
		} else if (smw->smw_period > 8) {
			/* Maximum of 2^23 usec watchdog period. */
			return (EINVAL);
		}
		sc->sc_wdog_period = smw->smw_period;
		sc->sc_wdog_armed = 1;
		WRITE_REG(sc->sc_addr + R_SCD_WDOG_INIT,
		    sc->sc_wdog_period * V_SCD_WDOG_FREQ);

		/* Watchdog is armed by tickling it. */
		sbwdog_tickle(smw);
	}
	return (0);
}
