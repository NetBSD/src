/*	$NetBSD: obio.c,v 1.28 2026/11/14 15:01:51 nat Exp $	*/

/*
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.28 2026/11/14 15:01:51 nat Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include "locators.h"

#include <machine/autoconf.h>

#include <mac68k/obio/obiovar.h>

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_print(void *, const char *);
static int	obio_search(device_t, cfdata_t, const int *, void *);
static int	obio_rescan(device_t, const char *, const int *);

/*
 * The bus tags are needed again by obio_rescan(), which has no aux
 * argument to take them from, so keep them.
 */
struct obio_softc {
	bus_space_tag_t	sc_bst;
	bus_dma_tag_t	sc_dmat;
};

CFATTACH_DECL3_NEW(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL, obio_rescan, NULL, 0);

static int
obio_match(device_t parent, cfdata_t cf, void *aux)
{
	static bool obio_matched;

	/* Allow only one instance. */
	if (obio_matched)
		return (0);

	obio_matched = true;
	return (1);
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	struct mainbus_attach_args *mba = aux;

	sc->sc_bst = mba->mba_bst;
	sc->sc_dmat = mba->mba_dmat;

	printf("\n");

	/* Search for and attach children. */
	config_search(self, aux,
	    CFARGS(.search = obio_search));
}

static int
obio_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct obio_softc *sc = device_private(self);
	struct mainbus_attach_args mba;

	if (!ifattr_match(ifattr, "obio"))
		return 0;

	mba.mba_bst = sc->sc_bst;
	mba.mba_dmat = sc->sc_dmat;

	config_search(self, &mba,
	    CFARGS(.search = obio_search));

	return 0;
}

int
obio_print(void *args, const char *name)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)args;

	if (oa->oa_addr != (-1))
		aprint_normal(" addr %x", oa->oa_addr);

	return (UNCONF);
}

int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args *mba = (struct mainbus_attach_args *) aux;
	struct obio_attach_args oa;

	oa.oa_addr = cf->cf_loc[OBIOCF_ADDR];
	oa.oa_tag = mba->mba_bst;
	oa.oa_dmat = mba->mba_dmat;

	if (config_probe(parent, cf, &oa))
		config_attach(parent, cf, &oa, obio_print, CFARGS_NONE);

	return (0);
}
