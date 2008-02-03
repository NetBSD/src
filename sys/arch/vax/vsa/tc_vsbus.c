/*	$NetBSD: tc_vsbus.c,v 1.1 2008/02/03 08:45:40 matt Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/scb.h>
#include <machine/vsbus.h>
#include <dev/tc/tcvar.h>

static int tcbus_match(struct device *, struct cfdata *, void *);
static void tcbus_attach(struct device *, struct device *, void *);

struct tcbus_softc {
	struct tc_softc sc_tc;
	struct tc_slotdesc sc_slots[1];
	struct vax_bus_dma_tag sc_dmatag;
	struct vax_sgmap sc_sgmap;
	struct evcnt sc_ev;
	bus_space_handle_t sc_memh;
};

static bus_dma_tag_t tcbus_dmat;

CFATTACH_DECL(tcbus, sizeof(struct tcbus_softc),
    tcbus_match, tcbus_attach, 0, 0);

static bus_dma_tag_t
tcbus_get_dma_tag(int slotno)
{
	return tcbus_dmat;
}

static void
tcbus_intr_establish(struct device *dv, void *cookie, int level,
	int (*func)(void *), void *arg)
{
	struct tcbus_softc * const sc = cookie;

	scb_vecalloc(0x51, (void (*)(void *)) func, arg, SCB_ISTACK,
	    &sc->sc_ev);
}

static void
tcbus_intr_disestablish(struct device *dv, void *cookie)
{
}

static const struct evcnt *
tcbus_intr_evcnt(struct device *dv, void *cookie)
{
	return NULL;
}

int
tcbus_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	return 0;
}

#define	KA4x_TURBO	0x30000000
#define	KA4x_TURBOMAPS	0x35800000
#define	KA4x_TURBOCSR	0x36800000

void
tcbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct vsbus_attach_args * const va = aux;
	struct tcbus_softc * const sc = (void *)self;
	struct tcbus_attach_args tba;
	struct pte *pte;
	const size_t nentries = 32768;
	int error;
	int i;

	error = bus_space_map(va->va_memt, KA4x_TURBO, 0x10000,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_memh);
	if (error) {
		aprint_error(": failed to map TC slot 0: %d\n", error);
		return;
	}

	sc->sc_slots[0].tcs_addr = sc->sc_memh;
	sc->sc_slots[0].tcs_cookie = sc;

	tba.tba_speed = TC_SPEED_12_5_MHZ;
	tba.tba_slots = sc->sc_slots;
	tba.tba_nslots = 1;
	tba.tba_intr_evcnt = tcbus_intr_evcnt;
	tba.tba_intr_establish = tcbus_intr_establish;
	tba.tba_intr_disestablish = tcbus_intr_disestablish;
	tba.tba_get_dma_tag = tcbus_get_dma_tag;

	vax_sgmap_dmatag_init(&sc->sc_dmatag, sc, nentries);
	pte = (struct pte *) vax_map_physmem(KA4x_TURBOMAPS,
	    nentries * sizeof(pte[0]));

	for (i = nentries; i > 0; )
		((uint32_t *) pte)[--i] = 0;

	sc->sc_dmatag._sgmap = &sc->sc_sgmap;
	/*
	 * Initialize the SGMAP.
	 */
	vax_sgmap_init(&sc->sc_dmatag, &sc->sc_sgmap, "tc_sgmap",
	     sc->sc_dmatag._wbase, sc->sc_dmatag._wsize, pte, 0);

	aprint_normal("\n");

	aprint_verbose("%s: 32K entry DMA SGMAP at PA 0x%x (VA %p)\n",
	     self->dv_xname, KA4x_TURBOMAPS, pte);

	tcbus_dmat = &sc->sc_dmatag;

	tcattach(parent, self, &tba);
}
