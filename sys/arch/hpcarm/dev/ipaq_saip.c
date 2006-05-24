/*	$NetBSD: ipaq_saip.c,v 1.17.8.1 2006/05/24 10:56:50 yamt Exp $	*/

/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipaq_saip.c,v 1.17.8.1 2006/05/24 10:56:50 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/uio.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_dmacreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_gpioreg.h>

/* prototypes */
static int	ipaq_match(struct device *, struct cfdata *, void *);
static void	ipaq_attach(struct device *, struct device *, void *);
static int 	ipaq_search(struct device *, struct cfdata *,
				const int *, void *);
static int	ipaq_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(ipaqbus, sizeof(struct ipaq_softc),
    ipaq_match, ipaq_attach, NULL, NULL);

static int
ipaq_print(aux, name)
	void *aux;
	const char *name;
{
        return (UNCONF);
}

int
ipaq_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
ipaq_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ipaq_softc *sc = (struct ipaq_softc*)self;
	struct sa11x0_softc *psc = (struct sa11x0_softc *)parent;
	
	printf("\n");

	sc->sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
	sc->sc_gpioh = psc->sc_gpioh;

	/* Map the Extended GPIO registers */
	if (bus_space_map(sc->sc_iot, SAEGPIO_BASE, 1, 0, &sc->sc_egpioh))
		panic("%s: unable to map Extended GPIO registers",
			self->dv_xname);

	sc->ipaq_egpio = EGPIO_INIT;
        bus_space_write_2(sc->sc_iot, sc->sc_egpioh, 0, sc->ipaq_egpio);

	/* Map the SSP registers */
	if (bus_space_map(sc->sc_iot, SASSP_BASE, SASSP_NPORTS, 0, &sc->sc_ssph))
		panic("%s: unable to map SSP registers",
			self->dv_xname);

	sc->sc_ppch = psc->sc_ppch;
	sc->sc_dmach = psc->sc_dmach;

	/*
	 *  Attach each devices
	 */
	config_search_ia(ipaq_search, self, "ipaqbus", NULL);
}

int
ipaq_search(parent, cf, ldesc, aux)
	struct device *parent;
	struct cfdata *cf;
	const int *ldesc;
	void *aux;
{
	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, ipaq_print);

        return 0;
}
