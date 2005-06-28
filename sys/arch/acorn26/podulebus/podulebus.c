/* $NetBSD: podulebus.c,v 1.11 2005/06/28 18:29:58 drochner Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: podulebus.c,v 1.11 2005/06/28 18:29:58 drochner Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/irq.h>
#include <machine/machdep.h>
#include <machine/memcreg.h>

#include <arch/acorn26/iobus/iocreg.h>
#include <arch/acorn26/iobus/iocvar.h>
#include <dev/podulebus/podulebus.h>
#include <arch/acorn26/podulebus/podulebusreg.h>

#include "locators.h"

#include "podloader.h"
#include "unixbp.h"

#if NUNIXBP > 0
#include <arch/acorn26/podulebus/unixbpvar.h>
#endif

static int podulebus_match(struct device *, struct cfdata *, void *);
static void podulebus_attach(struct device *, struct device *, void *);
static void podulebus_probe_podule(struct device *, int);
static int podulebus_print(void *, char const *);
static int podulebus_submatch(struct device *, struct cfdata *,
			      const locdesc_t *, void *);
static void podulebus_read_chunks(struct podulebus_attach_args *, int);
static u_int8_t *podulebus_get_chunk(struct podulebus_attach_args *pa, int type);
#if NPODLOADER > 0
void podloader_read_region(struct podulebus_attach_args *pa, u_int src,
    u_int8_t *dest, size_t length);
extern register_t _podloader_call(register_t, register_t, register_t,
    void *, int);
#endif

struct podulebus_softc {
	struct	device sc_dev;
	struct	ioc_attach_args sc_ioc;
};

CFATTACH_DECL(podulebus, sizeof(struct podulebus_softc),
    podulebus_match, podulebus_attach, NULL, NULL);

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
	int ecid, w;
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
		pa.pa_mod_t = 2;
		bus_space_map(pa.pa_mod_t,
		    (bus_addr_t)MEMC_IO_BASE + slotnum * (PODULE_GAP << 2),
		    (PODULE_GAP << 2), 0, &pa.pa_mod_h);
		bus_space_read_region_1(id_bst, id_bsh, 0,
					extecid, EXTECID_SIZE);
		/* XXX If you thought that was a hack... */
		pa.pa_mod_base = pa.pa_mod_h.a1;
		pa.pa_fast_base = pa.pa_fast_h.a1;
		pa.pa_medium_base = pa.pa_medium_h.a1;
		pa.pa_slow_base = pa.pa_slow_h.a1;
		pa.pa_sync_base = pa.pa_sync_h.a1;
		pa.pa_ecid = ecid;
		pa.pa_flags1 = extecid[EXTECID_F1];
		pa.pa_manufacturer = (extecid[EXTECID_MLO] |
				      extecid[EXTECID_MHI] << 8);
		pa.pa_product = (extecid[EXTECID_PLO] |
				 extecid[EXTECID_PHI] << 8);
		pa.pa_slotnum = slotnum;
		pa.pa_ih = slotnum;
		if (pa.pa_flags1 & EXTECID_F1_CD) {
			w = pa.pa_flags1 & EXTECID_F1_W_MASK;
			if (w != EXTECID_F1_W_8BIT) {
				/* RISC OS 3 can't handle this either. */
				printf("%s:%d: ROM is not 8 bits wide; "
				    "ignoring it\n",
				    self->dv_xname, pa.pa_slotnum);
			} else {
				podulebus_read_chunks(&pa, 0);
				pa.pa_descr = podulebus_get_chunk(&pa,
							  CHUNK_DEV_DESCR);
			}
		}
		pa.pa_slotflags = 0;
		config_found_sm_loc(self, &pa, "podulebus", NULL,
				podulebus_print, podulebus_submatch);
		if (pa.pa_chunks)
			FREE(pa.pa_chunks, M_DEVBUF);
		if (pa.pa_descr)
			FREE(pa.pa_descr, M_DEVBUF);
		if (pa.pa_loader)
			FREE(pa.pa_loader, M_DEVBUF);
	} else
		printf("%s:%d: non-extended podule ignored.\n",
		       self->dv_xname, slotnum);
}

static void
podulebus_read_chunks(struct podulebus_attach_args *pa, int useloader)
{
	u_int8_t chunk[8];
	u_int ptr, nchunks, type, length, offset;

	nchunks = pa->pa_nchunks;
	if (useloader)
		ptr = 0;
	else
		ptr = EXTECID_SIZE + IRQPTR_SIZE;
	for (;;) {
#if NPODLOADER > 0
		if (useloader)
			podloader_read_region(pa, ptr, chunk, 8);
		else
#endif
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
		pa->pa_chunks[nchunks-1].pc_useloader = useloader;
	}
	pa->pa_nchunks = nchunks;
}

static u_int8_t *
podulebus_get_chunk(struct podulebus_attach_args *pa, int type)
{
	int i;
	struct podulebus_chunk *pc;
	u_int8_t *chunk;

	for (i = 0; i < pa->pa_nchunks; i++) {
		pc = &pa->pa_chunks[i];
		if (pc->pc_type == type) {
			chunk = malloc( pc->pc_length + 1, M_DEVBUF, M_WAITOK);
#if NPODLOADER > 0
			if (pc->pc_useloader)
				podloader_read_region(pa, pc->pc_offset, chunk,
				    pc->pc_length);
			else
#endif
				bus_space_read_region_1(pa->pa_sync_t,
				    pa->pa_sync_h, pc->pc_offset, chunk,
				    pc->pc_length);
			chunk[pc->pc_length] = 0;
			return chunk;
		}
	}
	return NULL;
}

#if NPODLOADER > 0
int
podulebus_initloader(struct podulebus_attach_args *pa)
{

	if (pa->pa_loader != NULL)
		return 0;
	pa->pa_loader = podulebus_get_chunk(pa, CHUNK_RISCOS_LOADER);
	if (pa->pa_loader == NULL)
		return -1;
	podulebus_read_chunks(pa, 1);
	if (pa->pa_descr)
		FREE(pa->pa_descr, M_DEVBUF);
	pa->pa_descr = podulebus_get_chunk(pa, CHUNK_DEV_DESCR);
	return 0;
}

static register_t
podloader_call(register_t r0, register_t r1, register_t r11, void *loader,
    int entry)
{
	int s;
	register_t result;

	s = splhigh();
	result = _podloader_call(r0, r1, r11, loader, entry);
	splx(s);
	return result;
}

int
podloader_readbyte(struct podulebus_attach_args *pa, u_int addr)
{

	return podloader_call(0, addr, pa->pa_sync_base, pa->pa_loader, 0);
}

void
podloader_writebyte(struct podulebus_attach_args *pa, u_int addr, int val)
{

	podloader_call(val, addr, pa->pa_sync_base, pa->pa_loader, 1);
}

void
podloader_reset(struct podulebus_attach_args *pa)
{

	podloader_call(0, 0, pa->pa_sync_base, pa->pa_loader, 2);
}

int
podloader_callloader(struct podulebus_attach_args *pa, u_int r0, u_int r1)
{

	return podloader_call(r0, r1, pa->pa_sync_base, pa->pa_loader, 3);
}

void
podloader_read_region(struct podulebus_attach_args *pa, u_int src,
    u_int8_t *dest, size_t length)
{

	while (length--)
		*dest++ = podloader_readbyte(pa, src++);
	podloader_reset(pa);
}
#endif

static int
podulebus_print(void *aux, char const *pnp)
{
	struct podulebus_attach_args *pa = aux;
	char *p, *q;

	if (pnp) {
		if (pa->pa_descr) {
			/* Restrict description to ASCII graphic characters. */
			for (p = q = pa->pa_descr; *p != '\0'; p++)
				if (*p >= 32 && *p < 126)
					*q++ = *p;
			*q++ = 0;
			aprint_normal("%s", pa->pa_descr);
		} else
			aprint_normal("podule");
		aprint_normal(" <%04x:%04x> at %s",
		       pa->pa_manufacturer, pa->pa_product, pnp);
	}
	aprint_normal(" slot %d", pa->pa_slotnum);
	return UNCONF;
}	

static int
podulebus_submatch(struct device *parent, struct cfdata *cf,
		   const locdesc_t *ldesc, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	if (cf->cf_loc[PODULEBUSCF_SLOT] == PODULEBUSCF_SLOT_DEFAULT ||
	    cf->cf_loc[PODULEBUSCF_SLOT] == pa->pa_slotnum)
		return config_match(parent, cf, aux);
	return 0;
}

void *
podulebus_irq_establish(podulebus_intr_handle_t slot, int ipl,
			int (*func)(void *), void *arg, struct evcnt *ev)
{

	/* XXX: support for checking IRQ bit on podule? */
#if NUNIXBP > 0
	if (the_unixbp != NULL)
		return irq_establish(IRQ_UNIXBP_BASE + slot, ipl, func, arg,
		    ev);
#endif
	return irq_establish(IRQ_PIRQ, ipl, func, arg, ev);
}

void
podulebus_readcmos(struct podulebus_attach_args *pa, u_int8_t *c)
{
	int i;

	for (i = 0; i < 4; i++)
		c[i] = cmos_read(PODULE_CMOS_BASE +
		    pa->pa_slotnum * PODULE_CMOS_PERSLOT + i);
}
