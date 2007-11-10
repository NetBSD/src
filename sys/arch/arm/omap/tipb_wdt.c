/*
 * OMAP watchdog timers
 */
/*
 * Copyright (c) 2007 Danger Inc.
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
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/wdog.h>

#include <machine/param.h>
#include <machine/bus.h>
#include <dev/sysmon/sysmonvar.h>

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>
#include <arm/omap/omap_wdt.h>
#include <arm/omap/omap_wdtvar.h>
#include <arm/omap/omap_wdtreg.h>

/*
 * OMAP 32K watchdog timer
 */

static int tipbwdt32k_match(struct device *, struct cfdata *, void *);
static void tipbwdt32k_attach(struct device *, struct device *, void *);

CFATTACH_DECL(tipbwdt32k, sizeof(struct omapwdt32k_softc),
    tipbwdt32k_match, tipbwdt32k_attach, NULL, NULL);

static int
tipbwdt32k_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

static void
tipbwdt32k_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapwdt32k_softc *sc = (void *)self;
	struct tipb_attach_args *tipb = aux;
	unsigned int val;

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = omapwdt32k_setmode;
	sc->sc_smw.smw_tickle = omapwdt32k_tickle;
	sc->sc_smw.smw_period = OMAPWDT32K_DEFAULT_PERIOD;
	sc->sc_iot = tipb->tipb_iot;

	if (bus_space_map(sc->sc_iot, tipb->tipb_addr, tipb->tipb_size,
			  0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, WIDR);
	aprint_normal(": rev %d.%d\n", (val & WD_REV) >> 4,
		      (val & WD_REV & 0xf));

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error("%s: unable to register with sysmon\n",
			     sc->sc_dev.dv_xname);

	omapwdt32k_sc = sc;

	/* Turn on autoidle. */

    	omapwdt_sysconfig = 
	    bus_space_read_4(omapwdt32k_sc->sc_iot,
			     omapwdt32k_sc->sc_ioh, WD_SYSCONFIG) |
	    (1 << WD_SYSCONFIG_AUTOIDLE);
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh,
			  WD_SYSCONFIG, omapwdt_sysconfig);
}
