/* $NetBSD: iobus.c,v 1.2.6.2 2002/06/23 17:33:43 jdolecek Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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
 * iobus.c - when the IORQ* calls...
 */

#include <sys/param.h>

__RCSID("$NetBSD: iobus.c,v 1.2.6.2 2002/06/23 17:33:43 jdolecek Exp $");

#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/memcreg.h>

#include <arch/acorn26/iobus/iobusvar.h>

#include "locators.h"

static int iobus_match(struct device *parent, struct cfdata *cf, void *aux);
static void iobus_attach(struct device *parent, struct device *self, void *aux);
static int iobus_search_ioc(struct device *parent, struct cfdata *cf, void *aux);
static int iobus_search(struct device *parent, struct cfdata *cf, void *aux);
static int iobus_print(void *aux, const char *pnp);

struct iobus_softc {
	struct device	sc_dev;
};

struct cfattach iobus_ca = {
	sizeof(struct iobus_softc), iobus_match, iobus_attach
};

struct iobus_softc *the_iobus;

static int
iobus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/* There can be only one! */
	if (the_iobus == NULL)
		return 1;
	return 0;
}

static void
iobus_attach(struct device *parent, struct device *self, void *aux)
{

	the_iobus = (struct iobus_softc *)self;
	printf("\n");

	/*
	 * Always look for the IOC first, since stuff under there determines
	 * what else is present.
	 */
	config_search(iobus_search_ioc, self, NULL);
	config_search(iobus_search, self, NULL);
}

extern struct bus_space iobus_bs_tag;

static int
iobus_search_ioc(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args ioa;

	ioa.ioa_tag = 2;
	ioa.ioa_base = (bus_addr_t)MEMC_IO_BASE + cf->cf_loc[IOBUSCF_BASE];
	if (strcmp(cf->cf_driver->cd_name, "ioc") == 0 &&
	    (cf->cf_attach->ca_match)(parent, cf, &ioa) > 0)
		config_attach(parent, cf, &ioa, iobus_print);

	return 0;
}

static int
iobus_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args ioa;
	
	ioa.ioa_tag = 2;
	ioa.ioa_base = (bus_addr_t)MEMC_IO_BASE + cf->cf_loc[IOBUSCF_BASE];
	if ((cf->cf_attach->ca_match)(parent, cf, &ioa) > 0)
		config_attach(parent, cf, &ioa, iobus_print);

	return 0;
}

static int
iobus_print(void *aux, const char *pnp)
{
	struct iobus_attach_args *ioa = aux;

	if (ioa->ioa_base != IOBUSCF_BASE_DEFAULT)
		printf(" base 0x%06x",
		    ioa->ioa_base - (bus_addr_t)MEMC_IO_BASE);
	return UNCONF;
}
