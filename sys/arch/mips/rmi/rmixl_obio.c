/*	$NetBSD: rmixl_obio.c,v 1.1.2.3 2009/09/15 02:32:02 cliff Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_obio.c,v 1.1.2.3 2009/09/15 02:32:02 cliff Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_obiovar.h>

#include "locators.h"

static int  obio_match(struct device *, struct cfdata *, void *);
static void obio_attach(struct device *, struct device *, void *);
static int  obio_print(void *, const char *);
static int  obio_search(struct device *, struct cfdata *, const int *, void *);
static void obio_bus_init(struct obio_softc *);

CFATTACH_DECL(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

int obio_found;

int
obio_match(struct device * parent, struct cfdata *cf, void *aux)
{
	if (obio_found)
		return 0;
	return 1;
}

void
obio_attach(struct device * parent, struct device * self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	bus_addr_t ba;

	obio_found = 1;

	ba = (bus_addr_t)rmixl_configuration.rc_io_pbase;
	KASSERT(ba != 0);

	obio_bus_init(sc);

	aprint_normal(" addr %#lx size %#x\n", ba, RMIXL_IO_DEV_SIZE);
	aprint_naive("\n");

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
obio_search(struct device * parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct obio_softc *sc = device_private(parent);
	struct obio_attach_args obio;

	obio.obio_el_bst = sc->sc_el_bst;
	obio.obio_eb_bst = sc->sc_eb_bst;
	obio.obio_addr = cf->cf_loc[OBIOCF_ADDR];
	obio.obio_size = cf->cf_loc[OBIOCF_SIZE];
	obio.obio_mult = cf->cf_loc[OBIOCF_MULT];
	obio.obio_intr = cf->cf_loc[OBIOCF_INTR];

	if (config_match(parent, cf, &obio) > 0)
		config_attach(parent, cf, &obio, obio_print);

	return 0;
}

static void
obio_bus_init(struct obio_softc *sc)
{
	static int done = 0;

	if (done)
		return;
	done = 1;

	/* little endian space */
	if (rmixl_configuration.rc_el_memt.bs_cookie == 0)
		rmixl_el_bus_mem_init(&rmixl_configuration.rc_el_memt,
			&rmixl_configuration);

	/* big endian space */
	if (rmixl_configuration.rc_eb_memt.bs_cookie == 0)
		rmixl_eb_bus_mem_init(&rmixl_configuration.rc_eb_memt,
			&rmixl_configuration);
#ifdef NOTYET
	/* dma space for addr < 4GB */
	if (rmixl_configuration.rc_lt4G_dmat._cookie == 0)
		rmixl_dma_init(&rmixl_configuration.rc_lt4G_dmat);
	/* dma space for all memory, including >= 4GB */
	if (rmixl_configuration.rc_ge4G_dmat._cookie == 0)
		rmixl_dma_init(&rmixl_configuration.rc_ge4G_dmat);
#endif

	sc->sc_base = (bus_addr_t)rmixl_configuration.rc_io_pbase;
	sc->sc_size = (bus_size_t)RMIXL_IO_DEV_SIZE;
	sc->sc_el_bst = (bus_space_tag_t)&rmixl_configuration.rc_el_memt;
	sc->sc_eb_bst = (bus_space_tag_t)&rmixl_configuration.rc_eb_memt;
	sc->sc_lt4G_dmat = &rmixl_configuration.rc_lt4G_dmat;
	sc->sc_ge4G_dmat = &rmixl_configuration.rc_ge4G_dmat;
}

