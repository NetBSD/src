/*	$NetBSD: ipaq_saip.c,v 1.5 2001/07/15 00:30:17 ichiro Exp $	*/

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
 */
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

#include <hpcarm/sa11x0/sa11x0_var.h>
#include <hpcarm/sa11x0/sa11x0_dmacreg.h>
#include <hpcarm/sa11x0/sa11x0_ppcreg.h>
#include <hpcarm/sa11x0/sa11x0_gpioreg.h>
#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_gpioreg.h>

/* prototypes */
static int	ipaq_match(struct device *, struct cfdata *, void *);
static void	ipaq_attach(struct device *, struct device *, void *);
static int 	ipaq_search(struct device *, struct cfdata *, void *);
static int	ipaq_print(void *, const char *);

/* attach structures */
struct cfattach ipaqbus_ca = {
	sizeof(struct ipaq_softc), ipaq_match, ipaq_attach
};

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
		panic("%s: unable to map Extended GPIO registers\n",
			self->dv_xname);

	sc->ipaq_egpio = EGPIO_INIT;
        bus_space_write_2(sc->sc_iot, sc->sc_egpioh, 0, sc->ipaq_egpio);

	sc->sc_ppch = psc->sc_ppch;
	sc->sc_dmach = psc->sc_dmach;

	/*
	 *  Attach each devices
	 */
	config_search(ipaq_search, self, NULL);
}

int
ipaq_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if ((*cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, ipaq_print);

        return 0;
}
