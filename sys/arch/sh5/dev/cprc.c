/*	$NetBSD: cprc.c,v 1.6 2003/03/13 14:37:36 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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
 * SH-5 Clock, Power and Reset Controller
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/pbridgevar.h>
#include <sh5/dev/cprcreg.h>
#include <sh5/dev/cprcvar.h>

#include "locators.h"


struct cprc_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
};

static int cprcmatch(struct device *, struct cfdata *, void *);
static void cprcattach(struct device *, struct device *, void *);
static int cprcprint(void *, const char *);

CFATTACH_DECL(cprc, sizeof(struct cprc_softc),
    cprcmatch, cprcattach, NULL, NULL);

static u_int32_t cprc_reg_read(u_int);
#ifdef notyet
static void cprc_reg_write(u_int, u_int32_t);
#endif

extern struct cfdriver cprc_cd;

static const char *cprc_subdevs[] =
    {"clock", "watchdog", "power", "reset", NULL};
static struct cprc_softc *cprc_sc;


#ifdef notyet
static int watchdog_ipl;
static int watchdog_intevt;
#endif


/*ARGSUSED*/
static int
cprcmatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct pbridge_attach_args *pa = args;

	if (strcmp(pa->pa_name, cprc_cd.cd_name))
		return (0);

	KDASSERT(cprc_sc == NULL);

	pa->pa_ipl = cf->cf_loc[PBRIDGECF_IPL];
	pa->pa_intevt = cf->cf_loc[PBRIDGECF_INTEVT];

	return (1);
}

/*ARGSUSED*/
static void
cprcattach(struct device *parent, struct device *self, void *args)
{
	struct pbridge_attach_args *pa = args;
	struct cprc_softc *sc = (struct cprc_softc *)self;
	struct device *child;
	int i;

#ifdef notyet
	watchdog_ipl = pa->pa_ipl;
	watchdog_intevt = pa->pa_intevt;
#endif

	sc->sc_bust = pa->pa_bust;
	bus_space_map(sc->sc_bust, pa->pa_offset, CPRC_REG_SIZE,0,&sc->sc_bush);

	cprc_sc = sc;

	printf(": Clock, Power and Watchdog/Reset Controller\n");

	for (i = 0; cprc_subdevs[i] != NULL; i++) {
		child = config_found(self, (void *)cprc_subdevs[i], cprcprint);
		if (i == 0 && child == NULL)
			panic("%s: no clock driver configured!",self->dv_xname);
	}
}

static int
cprcprint(void *arg, const char *cp)
{

	if (cp)
		aprint_normal("%s at %s", (const char *)arg, cp);

	return (UNCONF);
}

static u_int32_t
cprc_reg_read(u_int off)
{

	return (bus_space_read_4(cprc_sc->sc_bust, cprc_sc->sc_bush, off));
}

#ifdef notyet
static void
cprc_reg_write(u_int off, u_int32_t value)
{

	bus_space_write_4(cprc_sc->sc_bust, cprc_sc->sc_bush, off, value);
}
#endif


/******************************************************************************
 *
 * The clock controller
 */
static int clockmatch(struct device *, struct cfdata *, void *);
static void clockattach(struct device *, struct device *, void *);
CFATTACH_DECL(clock, sizeof(struct device),
    clockmatch, clockattach, NULL, NULL);
extern struct cfdriver clock_cd;
struct cprc_clocks cprc_clocks;

/*ARGSUSED*/
static int
clockmatch(struct device *parent, struct cfdata *cf, void *args)
{
	const char *name = args;

	return (strcmp(name, clock_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
clockattach(struct device *parent, struct device *self, void *args)
{
	u_int64_t pllfreq;
	u_int32_t reg;

	reg = cprc_reg_read(CPRC_REG_FRQ);

	/*
	 * First, since we know the CPU clock, and its divider ratio,
	 * we can easilly figure out the PLL output clock rate from
	 * which all other clocks are derived.
	 */
	pllfreq = _sh5_ctc_ticks_per_us *
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_IFC_SHIFT) & CPRC_FRQ_MASK);
	pllfreq *= 1000000;

	cprc_clocks.cc_cpu = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_IFC_SHIFT) & CPRC_FRQ_MASK);
	cprc_clocks.cc_emi = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_EMC_SHIFT) & CPRC_FRQ_MASK);
	cprc_clocks.cc_superhyway = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_BMC_SHIFT) & CPRC_FRQ_MASK);
	cprc_clocks.cc_peripheral = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_PBC_SHIFT) & CPRC_FRQ_MASK);
	cprc_clocks.cc_pci = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_PCC_SHIFT) & CPRC_FRQ_MASK);
	cprc_clocks.cc_femi = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_FMC_SHIFT) & CPRC_FRQ_MASK);
	cprc_clocks.cc_stbus = pllfreq /
	    CPRC_FRQ2DIV((reg >> CPRC_FRQ_SBC_SHIFT) & CPRC_FRQ_MASK);

	printf(": PLL frequency - %dMHz\n", (int)(pllfreq/1000000));

	printf("%s: External Memory Clock: %dMHz, SuperHyway Clock: %dMHz\n",
	    self->dv_xname,
	    cprc_clocks.cc_emi / 1000000, cprc_clocks.cc_superhyway / 1000000);
	printf("%s: Peripheral Bus Clock: %dMHz, PCIbus Clock: %dMHz\n",
	    self->dv_xname,
	    cprc_clocks.cc_peripheral / 1000000, cprc_clocks.cc_pci / 1000000);
	printf("%s: FEMI Bus Clock: %dMHz, ST Legacy Clock: %dMHz\n",
	    self->dv_xname,
	    cprc_clocks.cc_femi / 1000000, cprc_clocks.cc_stbus / 1000000);
}


#ifdef notyet
/******************************************************************************
 *
 * The watchdog controller
 */
struct watchdog_softc {
	struct device sc_dev;
	void *sc_ih;
};
static int watchdogmatch(struct device *, struct cfdata *, void *);
static void watchdogattach(struct device *, struct device *, void *);
static int watchdogint(void *);
CFATTACH_DECL(watchdog, sizeof(struct watchdog_softc),
    watchdogmatch, watchdogattach, NULL, NULL);
extern struct cfdriver watchdog_cd;

/*ARGSUSED*/
static int
watchdogmatch(struct device *parent, struct cfdata *cf, void *args)
{
	const char *name = args;

	return (strcmp(name, watchdog_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
watchdogattach(struct device *parent, struct device *self, void *args)
{
	struct watchdog_softc *sc = (struct watchdog_sc *)self;

	if (watchdog_ipl == PBRIDGECF_IPL_DEFAULT ||
	    watchdog_intevt == PBRIDGECF_INTEVT_DEFAULT) {
		printf(": Watchdog disabled - ipl/intevt not specified\n");
		return;
	}

	/* TBD: Set up watchdog timer */

	printf(": Watchdog enabled\n");

	sc->sc_ih = sh5_intr_establish(watchdog_intevt, IST_LEVEL,
	    watchdog_ipl, watchdogint, sc);
}

/*ARGSUSED*/
static int
watchdogint(void *arg)
{

	/* TBD */
}
#endif /* notyet */


#ifdef notyet
/******************************************************************************
 *
 * The power controller
 */
static int powermatch(struct device *, struct cfdata *, void *);
static void powerattach(struct device *, struct device *, void *);
CFATTACH_DECL(power, sizeof(struct device),
    powermatch, powerattach, NULL, NULL);
extern struct cfdriver power_cd;

/*ARGSUSED*/
static int
powermatch(struct device *parent, struct cfdata *cf, void *args)
{
	const char *name = args;

	return (strcmp(name, power_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
powerattach(struct device *parent, struct device *self, void *args)
{

	printf(": Power Controller\n");
}
#endif /* notyet */


#ifdef notyet
/******************************************************************************
 *
 * The reset controller
 */
static int resetmatch(struct device *, struct cfdata *, void *);
static void resetattach(struct device *, struct device *, void *);
CFATTACH_DECL(reset, sizeof(struct device),
    resetmatch, resetattach, NULL, NULL);
extern struct cfdriver reset_cd;

/*ARGSUSED*/
static int
resetmatch(struct device *parent, struct cfdata *cf, void *args)
{
	const char *name = args;

	return (strcmp(name, reset_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
resetattach(struct device *parent, struct device *self, void *args)
{

	printf(": Reset Controller\n");

	/*
	 * XXX: Need to hook into a generic SH-5 reset front-end
	 */
}
#endif /* notyet */
