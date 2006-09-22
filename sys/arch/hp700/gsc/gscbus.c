/*	$NetBSD: gscbus.c,v 1.13 2006/09/22 14:08:04 skrll Exp $	*/

/*	$OpenBSD: gscbus.c,v 1.13 2001/08/01 20:32:04 miod Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Sample IO layouts:
 * 712:
 *
 * f0100000 -- lasi0
 * f0102000 -- lpt0
 * f0104000 -- audio0
 * f0105000 -- com0
 * f0106000 -- siop0
 * f0107000 -- ie0
 * f0108000 -- kbd0
 * f0108100 -- pms0
 * f010a000 -- fdc0
 * f010c000 -- *lasi0
 * f0200000 -- wax0
 * f8000000 -- sti0
 * fffbe000 -- cpu0
 * fffbf000 -- mem0
 *
 * 725/50:
 *
 * f0820000 -- dma
 * f0821000 -- hil
 * f0822000 -- com1
 * f0823000 -- com0
 * f0824000 -- lpt0
 * f0825000 -- siop0
 * f0826000 -- ie0
 * f0827000 -- dma reset
 * f0828000 -- timers
 * f0829000 -- domain kbd
 * f082f000 -- asp0
 * f1000000 -- audio0
 * fc000000 -- eisa0
 * fffbe000 -- cpu0
 * fffbf000 -- mem0
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gscbus.c,v 1.13 2006/09/22 14:08:04 skrll Exp $");

#define GSCDEBUG

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>

#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <machine/cpufunc.h>

#include <hp700/gsc/gscbusvar.h>
#include <hp700/hp700/machdep.h>

int	gscmatch(struct device *, struct cfdata *, void *);
void	gscattach(struct device *, struct device *, void *);

struct gsc_softc {
	struct device sc_dev;
	struct gsc_attach_args sc_ga;
	void *sc_ih;
};

CFATTACH_DECL(gsc, sizeof(struct gsc_softc),
    gscmatch, gscattach, NULL, NULL);

/*
 * pdc_scanbus calls this function back with each module
 * it finds on the GSC bus.  We call the IC-specific function
 * to fix up the module's attach arguments, then we match
 * and attach it.
 */
static void gsc_module_callback(struct device *, struct confargs *);
static void
gsc_module_callback(struct device *self, struct confargs *ca)
{
	struct gsc_softc *sc = (struct gsc_softc *)self;
	struct gsc_attach_args ga;

	/* Make the GSC attach args. */
	ga = sc->sc_ga;
	ga.ga_ca = *ca;
	ga.ga_iot = sc->sc_ga.ga_iot;
	ga.ga_dmatag = sc->sc_ga.ga_dmatag;
	(*sc->sc_ga.ga_fix_args)(sc->sc_ga.ga_fix_args_cookie, &ga);

	config_found_sm_loc(self, "gsc", NULL, &ga, mbprint, mbsubmatch);
}

int
gscmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct gsc_attach_args *ga = aux;

	return !strcmp(ga->ga_name, "gsc");
}

void
gscattach(struct device *parent, struct device *self, void *aux)
{
	struct gsc_softc *sc = (struct gsc_softc *)self;
	struct gsc_attach_args *ga = aux;

	sc->sc_ga = *ga;

#ifdef USELEDS
	if (machine_ledaddr)
		printf(": %sleds", machine_ledword? "word" : "");
#endif

	printf ("\n");

	/* Add the I/O subsystem's interrupt register. */
	ga->ga_int_reg->int_reg_dev = parent->dv_xname;
	sc->sc_ih = hp700_intr_establish(&sc->sc_dev, IPL_NONE,
					 NULL, ga->ga_int_reg,
					 &int_reg_cpu, ga->ga_irq);

	pdc_scanbus(self, &ga->ga_ca, gsc_module_callback);
}

int
gscprint(void *aux, const char *pnp)
{

	return (UNCONF);
}
