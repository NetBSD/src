/*	$NetBSD: obio_wdt.c,v 1.5 2008/11/20 20:23:05 cliff Exp $	*/

/*
 * Copyright (c) 2007 Microsoft
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
 *     This product includes software developed by Microsoft
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
 */

/*
 * obio attachment for GEMINI watchdog timers
 */
#include "opt_gemini.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio_wdt.c,v 1.5 2008/11/20 20:23:05 cliff Exp $");

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

#include <arm/gemini/gemini_obiovar.h>
#include <arm/gemini/gemini_wdtvar.h>
#include <arm/gemini/gemini_reg.h>

static int geminiwdt_match(struct device *, struct cfdata *, void *);
static void geminiwdt_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(obiowdt, sizeof(struct geminiwdt_softc),
    geminiwdt_match, geminiwdt_attach, NULL, NULL);

static int
geminiwdt_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr == OBIOCF_ADDR_DEFAULT)
		panic("geminiwdt must have addr specified in config.");

	if (obio->obio_addr == GEMINI_WATCHDOG_BASE)
		return 1;

	return 0;
}

static void
geminiwdt_attach(struct device *parent, struct device *self, void *aux)
{
	geminiwdt_softc_t *sc = device_private(self);
	struct obio_attach_args *obio = aux;

	sc->sc_dev = self;
	sc->sc_addr = obio->obio_addr;
	sc->sc_size = (obio->obio_size == OBIOCF_SIZE_DEFAULT)
		? GEMINI_WDT_WDINTERLEN + 4
		: obio->obio_size;

	sc->sc_smw.smw_name = device_xname(sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = geminiwdt_setmode;
	sc->sc_smw.smw_tickle = geminiwdt_tickle;
	sc->sc_smw.smw_period = 0;
	sc->sc_iot = obio->obio_iot;

	if (bus_space_map(sc->sc_iot, sc->sc_addr, sc->sc_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	geminiwdt_sc = sc;

	sc->sc_armed = 1;	/* fake armed so can disarm */
	geminiwdt_enable(0);	/* disable, stop, disarm */

	if (sysmon_wdog_register(&sc->sc_smw) != 0) {
		geminiwdt_sc = NULL;
		aprint_error("%s: unable to register with sysmon\n",
			     device_xname(sc->sc_dev));
	}

	aprint_normal("\n");
	aprint_naive("\n"); 
}
