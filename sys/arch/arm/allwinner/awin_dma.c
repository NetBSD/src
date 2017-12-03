/* $NetBSD: awin_dma.c,v 1.6.16.2 2017/12/03 11:35:50 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_allwinner.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_dma.c,v 1.6.16.2 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>
#include <arm/allwinner/awin_dma.h>

static struct awin_dma_softc *awin_dma_sc;

static int	awin_dma_match(device_t, cfdata_t, void *);
static void	awin_dma_attach(device_t, device_t, void *);

#if defined(DDB)
void		awin_dma_dump_regs(void);
#endif

CFATTACH_DECL_NEW(awin_dma, sizeof(struct awin_dma_softc),
	awin_dma_match, awin_dma_attach, NULL, NULL);

static int
awin_dma_match(device_t parent, cfdata_t cf, void *aux)
{
	return awin_dma_sc == NULL;
}

static void
awin_dma_attach(device_t parent, device_t self, void *aux)
{
	struct awin_dma_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	bus_space_handle_t bsh = awin_chip_id() == AWIN_CHIP_ID_A80 ?
				 aio->aio_a80_core2_bsh : aio->aio_core_bsh;

	KASSERT(awin_dma_sc == NULL);
	awin_dma_sc = sc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_bst, bsh, loc->loc_offset,
	    loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": DMA\n");

	switch (awin_chip_id()) {
#if defined(ALLWINNER_A10) || defined(ALLWINNER_A20)
	case AWIN_CHIP_ID_A10:
	case AWIN_CHIP_ID_A20:
		awin_dma_a10_attach(sc, aio, loc);
		break;
#endif
#if defined(ALLWINNER_A31) || defined(ALLWINNER_A80)
	case AWIN_CHIP_ID_A31:
	case AWIN_CHIP_ID_A80:
		awin_dma_a31_attach(sc, aio, loc);
		break;
#endif
	}

	KASSERT(sc->sc_dc != NULL);
}

void *
awin_dma_alloc(const char *type, void (*cb)(void *), void *cbarg)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	if (sc == NULL)
		return NULL;

	return sc->sc_dc->dma_alloc(sc, type, cb, cbarg);
}

void
awin_dma_free(void *ch)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	return sc->sc_dc->dma_free(ch);
}

uint32_t
awin_dma_get_config(void *ch)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	return sc->sc_dc->dma_get_config(ch);
}

void
awin_dma_set_config(void *ch, uint32_t val)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	return sc->sc_dc->dma_set_config(ch, val);
}

int
awin_dma_transfer(void *ch, paddr_t src, paddr_t dst,
    size_t nbytes)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	return sc->sc_dc->dma_transfer(ch, src, dst, nbytes);
}

void
awin_dma_halt(void *ch)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	return sc->sc_dc->dma_halt(ch);
}

#if defined(DDB)
void
awin_dma_dump_regs(void)
{
	struct awin_dma_softc *sc = awin_dma_sc;

	switch (awin_chip_id()) {
#if defined(ALLWINNER_A10) || defined(ALLWINNER_A20)
	case AWIN_CHIP_ID_A10:
	case AWIN_CHIP_ID_A20:
		awin_dma_a10_dump_regs(sc);
		break;
#endif
#if defined(ALLWINNER_A31) || defined(ALLWINNER_A80)
	case AWIN_CHIP_ID_A31:
	case AWIN_CHIP_ID_A80:
		awin_dma_a31_dump_regs(sc);
		break;
#endif
	}
}
#endif
