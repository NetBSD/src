/*	$NetBSD: gio.c,v 1.1.14.1 2002/04/01 07:42:21 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>

#include "locators.h"

struct gio_softc {
	struct	device sc_dev;
};

static int	gio_match(struct device *, struct cfdata *, void *);
static void	gio_attach(struct device *, struct device *, void *);
static int	gio_print(void *, const char *);
static int	gio_search(struct device *, struct cfdata *, void *);

struct cfattach gio_ca = {
	sizeof(struct gio_softc), gio_match, gio_attach
};

static int
gio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct giobus_attach_args *gba = aux;

	if (strcmp(gba->gba_busname, match->cf_driver->cd_name) != 0)
		return 0;

	return 1;
}

static void
gio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct gio_attach_args ga;

	printf("\n");

	config_search(gio_search, self, &ga);
}

static int
gio_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct gio_attach_args *ga = aux;

	if (pnp != 0)
		return QUIET;

	if (ga->ga_slot != GIOCF_SLOT_DEFAULT)
		printf(" slot %d", ga->ga_slot);
	if (ga->ga_addr != GIOCF_ADDR_DEFAULT)
		printf(" addr 0x%x", ga->ga_addr);

	return UNCONF;
}

static int
gio_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct gio_attach_args *ga = aux;

	do {
		ga->ga_slot = cf->cf_loc[GIOCF_SLOT];
		ga->ga_addr = cf->cf_loc[GIOCF_ADDR];
		ga->ga_iot = 0;
		ga->ga_ioh = MIPS_PHYS_TO_KSEG1(ga->ga_addr);
		if ((*cf->cf_attach->ca_match)(parent, cf, ga) > 0)
			config_attach(parent, cf, ga, gio_print);
	} while (cf->cf_fstate == FSTATE_STAR);

	return 0;
}
