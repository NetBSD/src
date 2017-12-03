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
 * obio attachment for OMAP watchdog timers
 */
#include "opt_omap.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio_wdt.c,v 1.6.2.1 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/wdog.h>

#include <machine/param.h>
#include <sys/bus.h>
#include <dev/sysmon/sysmonvar.h>

#include <arm/omap/omap2_obiovar.h>

#include <arm/omap/omap_wdtvar.h>
#include <arm/omap/omap_wdtreg.h>

static int obiowdt32k_match(device_t, cfdata_t, void *);
static void obiowdt32k_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(obiowdt32k, sizeof(struct omapwdt32k_softc),
    obiowdt32k_match, obiowdt32k_attach, NULL, NULL);

static int
obiowdt32k_match(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

static void
obiowdt32k_attach(device_t parent, device_t self, void *aux)
{
	struct omapwdt32k_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	unsigned int val;

	sc->sc_dev = self;
	sc->sc_smw.smw_name = device_xname(sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = omapwdt32k_setmode;
	sc->sc_smw.smw_tickle = omapwdt32k_tickle;
	sc->sc_smw.smw_period = OMAPWDT32K_DEFAULT_PERIOD;
	sc->sc_iot = obio->obio_iot;

	if (bus_space_map(sc->sc_iot, obio->obio_addr, obio->obio_size, 0,
		&sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, WIDR);
	aprint_normal(": rev %d.%d\n", (val & WD_REV) >> 4,
		      (val & WD_REV & 0xf));

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error("%s: unable to register with sysmon\n",
			     device_xname(sc->sc_dev));

	omapwdt32k_sc = sc;

	/* Turn on autoidle. */

    	omapwdt_sysconfig = 
	    bus_space_read_4(omapwdt32k_sc->sc_iot,
			     omapwdt32k_sc->sc_ioh, WD_SYSCONFIG) |
	    (1 << WD_SYSCONFIG_AUTOIDLE);
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh,
			  WD_SYSCONFIG, omapwdt_sysconfig);

	/*
	 * Put watchdog in known (disarmed) state.
	 *
	 * Some U-Boot versions on BeagleBone will leave the watchdog
	 * armed at boot.
	 * 
	 * XXX Revisit this, perhaps we should just start tickling an
	 * armed watchdog.
	 */

	sc->sc_armed = -1;
	omapwdt32k_enable(0);
}
