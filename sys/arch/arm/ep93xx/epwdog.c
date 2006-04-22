/*	$NetBSD: epwdog.c,v 1.1.10.1 2006/04/22 11:37:17 simonb Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epwdog.c,v 1.1.10.1 2006/04/22 11:37:17 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <arm/ep93xx/ep93xxvar.h> 
#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epwdogreg.h>
#include <arm/ep93xx/epwdogvar.h>

struct epwdog_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

static int epwdog_match(struct device *, struct cfdata *, void *);
static void epwdog_attach(struct device *, struct device *, void *);

CFATTACH_DECL(epwdog, sizeof(struct epwdog_softc),
	      epwdog_match, epwdog_attach, NULL, NULL);

static struct epwdog_softc *the_epwdog_sc = 0;

static int
epwdog_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
epwdog_attach(struct device *parent, struct device *self, void *aux)
{
	struct epwdog_softc *sc = (struct epwdog_softc*)self;
	struct epsoc_attach_args *sa = aux;

	printf("\n");
	sc->sc_iot = sa->sa_iot;

	if (bus_space_map(sa->sa_iot, sa->sa_addr,
			  sa->sa_size, 0, &sc->sc_ioh)){
		printf("%s: Cannot map registers", self->dv_xname);
		return;
	}

	if (!the_epwdog_sc)
		the_epwdog_sc = sc;
#ifdef DIAGNOSTIC
	else
		printf("%s is already configured\n", sc->sc_dev.dv_xname);

int
epwdog_reset(void)
{
	struct epwdog_softc *sc = the_epwdog_sc;

#ifdef DIAGNOSTIC
	if (!sc) {
		printf("epwdog not configured\n");
		return (ENXIO);
	}
#endif

	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  EP93XX_WDOG_Ctrl, EP93XX_WDOG_DISABLE);
	splhigh();
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  EP93XX_WDOG_Ctrl, EP93XX_WDOG_ENABLE);
	while (1);

	return 0; /* not reached */
}
