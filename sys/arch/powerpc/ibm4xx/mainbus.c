/*	$NetBSD: mainbus.c,v 1.1.2.2 2002/03/16 15:59:15 jdolecek Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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

#include "locators.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _GALAXY_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>

static int	mainbus_match(struct device *, struct cfdata *, void *);
static void	mainbus_attach(struct device *, struct device *, void *);
static int	mainbus_submatch(struct device *, struct cfdata *, void *);
static int	mainbus_print(void *, const char *);

struct mainbus_softc {
	struct device	sc_dev;
	bus_space_tag_t sc_bt;
	bus_dma_tag_t	sc_dmat;
}

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};


/*
 * Probe for the mainbus; always succeeds.
 */
static int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

static int
mainbus_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	union mainbus_attach_args *maa = aux;

	if (cf->cf_loc[MAINBUSCF_ADDR] != MAINBUSCF_ADDR_DEFAULT &&
	    cf->cf_loc[MAINBUSCF_ADDR] != maa->mba_rmb.rmb_addr)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

static int
mainbus_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_softc *sc = (struct mainbus_softc *)parent;
	struct mainbus_attach_args mba;
	int tryagain;

	do {
		mba.mba_name = cf->cf_driver->name;
		mba.mba_addr = cf->cf_loc[MAINBUSCF_ADDR];
		mba.mba_irq = cf->cf_loc[MAINBUSCF_INTR];

		tryagain = 0;
		if ((*cf->cf_attach->ca_match)(parent, cf, &mba) > 0) {
			config_attach(parent, cf, &mba, mainbus_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return (0);
}

/*
 * Attach the mainbus.
 */
static void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_softc *sc = (struct mainbus_softc *)self;
	union mainbus_attach_args mba;

	/* Create a bustag and dmatag */
	sc->sc_bt = galaxy_make_bus_space_tag(0, 0);
	sc->sc_dmat = &galaxy_default_bus_dma_tag;

	/* Attach the CPU first */
	mba.mba_name = "cpu";
	mba.mba_addr = MAINBUSCF_ADDR_DEFAULT;
	mba.mba_irq = MAINBUSCF_IRQ_DEFAULT;
	mba.mba_bt = sc->bt;
	mba.mba_dmat = sc->dmat;
	config_found(self, &mba, mainbus_print);
	
	/* Attach all other devices. */
	config_search(mainbus_search, self, NULL);
}

static int
mainbus_print(void *aux, const char *pnp)
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		printf("%s at %s", mba->mba_busname, pnp);

	if (mba->mba_rmb.rmb_addr != MAINBUSCF_ADDR_DEFAULT)
		printf(" addr 0x%08lx", mba->mba_rmb.rmb_addr);
	if (mba->mba_rmb.rmb_irq != MAINBUSCF_IRQ_DEFAULT)
		printf(" irq %d", mba->mba_rmb.rmb_irq);
	return (UNCONF);
}

