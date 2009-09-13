/*	$NetBSD: rmixl_obio.c,v 1.1.2.1 2009/09/13 03:27:38 cliff Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

/*
 * On-board device autoconfiguration support for RMI {XLP, XLR, XLS} chips
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_obio.c,v 1.1.2.1 2009/09/13 03:27:38 cliff Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixl_obiovar.h>

#include "locators.h"

int	obio_match(struct device *, struct cfdata *, void *);
void	obio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(obio, sizeof(struct device),
    obio_match, obio_attach, NULL, NULL);

int	obio_print(void *, const char *);
int	obio_search(struct device *, struct cfdata *,
		    const int *, void *);

/* there can be only one */
int	obio_found;

int
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	if (obio_found)
		return 0;
	return 1;
}

void
obio_attach(struct device *parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
#if 0
	bus_space_handle_t *ioh;
	int err;
#endif

	obio_found = 1;

	sc->sc_iot = rmixl_obio_get_bus_space_tag();

	aprint_naive("\n");
	aprint_normal("\n");

#if 0
	err = bus_space_map(sc->sc_iot, RMIXL_IO_DEV_PBASE, RMIXL_IO_DEV_SIZE, 0, &ioh);
	if (err)
		panic("%s: bus_space_map failed, iot %p, addr %#lx, size %#x\n",
			__func__, sc->sc_iot, RMIXL_IO_DEV_PBASE, RMIXL_IO_DEV_SIZE);
#endif

	/*
	 * Attach on-board devices as specified in the kernel config file.
	 */
	config_search_ia(obio_search, self, "obio", NULL);
}

int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *obio = aux;

	aprint_normal(" addr 0x%08lx", obio->obio_addr);
	if (obio->obio_size != OBIOCF_SIZE_DEFAULT)
		aprint_normal("-0x%08lx", obio->obio_addr + (obio->obio_size - 1));
	if (obio->obio_mult != OBIOCF_MULT)
		aprint_normal(" mult %d", obio->obio_mult);
	if (obio->obio_intr != -1)
		aprint_normal(" intr %d", obio->obio_intr);

	return (UNCONF);
}

int
obio_search(struct device *parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct obio_attach_args obio;

	obio.obio_bst = rmixl_obio_get_bus_space_tag();
	obio.obio_addr = cf->cf_loc[OBIOCF_ADDR];
	obio.obio_size = cf->cf_loc[OBIOCF_SIZE];
	obio.obio_mult = cf->cf_loc[OBIOCF_MULT];
	obio.obio_intr = cf->cf_loc[OBIOCF_INTR];

	if (config_match(parent, cf, &obio) > 0)
		config_attach(parent, cf, &obio, obio_print);

	return 0;
}
