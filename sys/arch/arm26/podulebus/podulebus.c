/* $NetBSD: podulebus.c,v 1.2.2.3 2000/12/13 14:49:46 bouyer Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#include <sys/param.h>

__RCSID("$NetBSD: podulebus.c,v 1.2.2.3 2000/12/13 14:49:46 bouyer Exp $");

#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/irq.h>
#include <machine/memcreg.h>

#include <arch/arm26/iobus/iocreg.h>
#include <arch/arm26/iobus/iocvar.h>
#include <arch/arm26/podulebus/podulebus.h>
#include <arch/arm26/podulebus/podulebusreg.h>

#include "locators.h"

#include "unixbp.h"

#if NUNIXBP > 0
extern struct cfdriver unixbp_cd;
#endif

static int podulebus_match(struct device *, struct cfdata *, void *);
static void podulebus_attach(struct device *, struct device *, void *);
static void podulebus_probe_podule(struct device *, int);
static int podulebus_print(void *, char const *);
static int podulebus_submatch(struct device *, struct cfdata *, void *);
static void podulebus_read_chunks(struct device *,
				  struct podulebus_attach_args *);
static u_int8_t *podulebus_get_chunk(struct podulebus_attach_args *pa, int type);

struct podulebus_softc {
	struct	device sc_dev;
	struct	ioc_attach_args sc_ioc;
};

struct cfattach podulebus_ca = {
	sizeof(struct podulebus_softc), podulebus_match, podulebus_attach
};

extern struct cfdriver podulebus_cd;

/* ARGSUSED */
static int
podulebus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/* We can't usefully probe for this */
	return 1;
}

static void
podulebus_attach(struct device *parent, struct device *self, void *aux)
{
	int i;
	struct podulebus_softc *sc = (struct podulebus_softc *)self;
	struct ioc_attach_args *ioc = aux;

	sc->sc_ioc = *ioc;
	printf("\n");

	/* Iterate over the podules attaching them */
	for (i = 0; i < MAX_PODULES; i++)
		podulebus_probe_podule(self, i);
}

static void
podulebus_probe_podule(struct device *self, int slotnum)
{
	struct podulebus_softc *sc = (struct podulebus_softc *)self;
	bus_space_tag_t id_bst;
	bus_space_handle_t id_bsh;
	int ecid;
	u_int8_t extecid[EXTECID_SIZE];
	struct podulebus_attach_args pa;

	bzero(&pa, sizeof(pa));
	id_bst = sc->sc_ioc.ioc_sync_t;
	bus_space_subregion(id_bst, sc->sc_ioc.ioc_sync_h,
			    slotnum * PODULE_GAP, PODULE_GAP, &id_bsh);
	ecid = bus_space_read_1(id_bst, id_bsh, 0);
	/* Skip empty or strange slots */
	if (ecid & (ECID_NOTPRESENT | ECID_NONCONF))
		return;
	if ((ecid & ECID_ID_MASK) == ECID_ID_EXTEND) {
		pa.pa_fast_t = sc->sc_ioc.ioc_fast_t;
		bus_space_subregion(pa.pa_fast_t, sc->sc_ioc.ioc_fast_h,
				    slotnum * PODULE_GAP, PODULE_GAP,
				    &pa.pa_fast_h);
		pa.pa_medium_t = sc->sc_ioc.ioc_medium_t;
		bus_space_subregion(pa.pa_medium_t, sc->sc_ioc.ioc_medium_h,
				    slotnum * PODULE_GAP, PODULE_GAP,
				    &pa.pa_medium_h);
		pa.pa_slow_t = sc->sc_ioc.ioc_slow_t;
		bus_space_subregion(pa.pa_slow_t, sc->sc_ioc.ioc_slow_h,
				    slotnum * PODULE_GAP, PODULE_GAP,
				    &pa.pa_slow_h);
		pa.pa_sync_t = sc->sc_ioc.ioc_sync_t;
		bus_space_subregion(pa.pa_sync_t, sc->sc_ioc.ioc_sync_h,
				    slotnum * PODULE_GAP, PODULE_GAP,
				    &pa.pa_sync_h);
		/* XXX This is a hack! */
		pa.pa_memc_t = 2;
		bus_space_subregion(pa.pa_memc_t,
		    (bus_space_handle_t)MEMC_IO_BASE, slotnum * PODULE_GAP,
		    PODULE_GAP, &pa.pa_memc_h);
		bus_space_read_region_1(id_bst, id_bsh, 0,
					extecid, EXTECID_SIZE);
		pa.pa_ecid = ecid;
		pa.pa_flags1 = extecid[EXTECID_F1];
		pa.pa_manufacturer = (extecid[EXTECID_MLO] |
				      extecid[EXTECID_MHI] << 8);
		pa.pa_product = (extecid[EXTECID_PLO] |
				 extecid[EXTECID_PHI] << 8);
		pa.pa_slotnum = slotnum;
		if (pa.pa_flags1 & EXTECID_F1_CD) {
			podulebus_read_chunks(self, &pa);
			pa.pa_descr = podulebus_get_chunk(&pa,
							  CHUNK_DEV_DESCR);
		}
		config_found_sm(self, &pa,
				podulebus_print, podulebus_submatch);
		if (pa.pa_chunks)
			FREE(pa.pa_chunks, M_DEVBUF);
		if (pa.pa_descr)
			FREE(pa.pa_descr, M_DEVBUF);
	} else
		printf("%s:%d: non-extended podule ignored.\n",
		       self->dv_xname, slotnum);
}

static void
podulebus_read_chunks(struct device *self, struct podulebus_attach_args *pa)
{
	u_int8_t chunk[8];
	u_int ptr, w, nchunks, type, length, offset;

	nchunks = 0;
	ptr = EXTECID_SIZE + IRQPTR_SIZE;
	w = pa->pa_flags1 & EXTECID_F1_W_MASK;
	if (w != EXTECID_F1_W_8BIT) {
		/* RISC OS 3 can't handle this either. */
		printf("%s:%d: ROM is not 8 bits wide; ignoring it\n",
		       self->dv_xname, pa->pa_slotnum);
		return;
	}
	for (;;) {
		bus_space_read_region_1(pa->pa_sync_t, pa->pa_sync_h,
					ptr, chunk, 8);
		ptr += 8;
		type = chunk[0];
		length = chunk[1] | chunk[2] << 8 | chunk[3] << 16;
		if (length == 0)
			break;
		offset = chunk[4] | chunk[5] << 8 |
		    chunk[6] << 16 | chunk[7] <<  24;
		if ((offset + length) > PODULE_GAP)
			continue;
		nchunks++;
		pa->pa_chunks = realloc(pa->pa_chunks,
					nchunks * sizeof(*pa->pa_chunks),
					M_DEVBUF, M_WAITOK);
		pa->pa_chunks[nchunks-1].pc_type = type;
		pa->pa_chunks[nchunks-1].pc_length = length;
		pa->pa_chunks[nchunks-1].pc_offset = offset;
	}
	pa->pa_nchunks = nchunks;
}

static u_int8_t *
podulebus_get_chunk(struct podulebus_attach_args *pa, int type)
{
	int i;
	u_int8_t *chunk;

	for (i = 0; i < pa->pa_nchunks; i++)
		if (pa->pa_chunks[i].pc_type == type) {
			MALLOC(chunk, u_int8_t *,
			       pa->pa_chunks[i].pc_length + 1,
			       M_DEVBUF, M_WAITOK);
			bus_space_read_region_1(pa->pa_sync_t, pa->pa_sync_h,
						pa->pa_chunks[i].pc_offset,
						chunk,
						pa->pa_chunks[i].pc_length);
			chunk[pa->pa_chunks[i].pc_length] = 0;
			return chunk;
		}
	return NULL;
}

static int
podulebus_print(void *aux, char const *pnp)
{
	struct podulebus_attach_args *pa = aux;

	if (pnp) {
		if (pa->pa_descr)
			printf("%s", pa->pa_descr);
		else
			printf("podule");
		printf(" <%04x:%04x> at %s",
		       pa->pa_manufacturer, pa->pa_product, pnp);
	}
	printf(" slot %d", pa->pa_slotnum);
	return UNCONF;
}	

static int
podulebus_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	if (cf->cf_loc[PODULEBUSCF_SLOT] == PODULEBUSCF_SLOT_DEFAULT ||
	    cf->cf_loc[PODULEBUSCF_SLOT] == pa->pa_slotnum)
		return (*cf->cf_attach->ca_match)(parent, cf, aux);
	return 0;
}

struct irq_handler *
podulebus_irq_establish(struct device *self, int slot, int ipl,
			int (*func)(void *), void *arg)
{

	/* XXX: support for checking IRQ bit on podule? */
#if NUNIXBP > 0
	if (unixbp_cd.cd_ndevs > 0 && unixbp_cd.cd_devs[0] != NULL)
		return irq_establish(IRQ_UNIXBP_BASE + slot, ipl, func, arg);
#endif
	return irq_establish(IRQ_PIRQ, ipl, func, arg);
}
