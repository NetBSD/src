/*	$NetBSD: obio.c,v 1.56.40.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.56.40.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/dvma.h>
#include <machine/mon.h>
#include <machine/pte.h>

#include <sun3/sun3/control.h>
#include <sun3/sun3/machdep.h>
#include <sun3/sun3/obio.h>

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_print(void *, const char *);
static int	obio_submatch(device_t, cfdata_t, const int *, void *);

struct obio_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
};

CFATTACH_DECL_NEW(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

static int obio_attached;

static int obio_bus_map(bus_space_tag_t, bus_type_t, bus_addr_t, bus_size_t,
    int, vaddr_t, bus_space_handle_t *);
static paddr_t obio_bus_mmap(bus_space_tag_t, bus_type_t, bus_addr_t,
    off_t, int, int);
static int obio_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
    struct proc *, int);

static struct sun68k_bus_space_tag obio_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus space tag */
	obio_bus_map,			/* bus_space_map */
	NULL,				/* bus_space_unmap */
	NULL,				/* bus_space_subregion */
	NULL,				/* bus_space_barrier */
	obio_bus_mmap,			/* bus_space_mmap */
	NULL,				/* bus_intr_establish */
	NULL,				/* bus_space_peek_N */
	NULL				/* bus_space_poke_N */
};

static struct sun68k_bus_dma_tag obio_dma_tag;

static int
obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (obio_attached)
		return 0;

	if (ca->ca_bustype != BUS_OBIO)
		return 0;

	if (ca->ca_name != NULL && strcmp(cf->cf_name, ca->ca_name) != 0)
		return 0;

	return 1;
}

/*
 * We need control over the order of attachment on OBIO,
 * so do "direct" style autoconfiguration with addresses
 * tried in sequence starting at zero and incrementing
 * by OBIO_INCR. Sun3 OBIO addresses are fixed forever.
 */
#define OBIO_INCR	0x020000
#define OBIO_END	0x200000

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct obio_softc *sc = device_private(self);
	struct confargs oba;
	int addr;

	obio_attached = 1;
	sc->sc_dev = self;

	aprint_normal("\n");

	sc->sc_bustag = ca->ca_bustag;
	sc->sc_dmatag = ca->ca_dmatag;

	obio_space_tag.cookie = sc;
	obio_space_tag.parent = sc->sc_bustag;

	obio_dma_tag = *sc->sc_dmatag;
	obio_dma_tag._cookie = sc;
	obio_dma_tag._dmamap_load = obio_dmamap_load;

	oba = *ca;
	oba.ca_bustag = &obio_space_tag;
	oba.ca_dmatag = &obio_dma_tag;

	/* Configure these in order of address. */
	for (addr = 0; addr < OBIO_END; addr += OBIO_INCR) {
		/* Our parent set ca->ca_bustype already. */
		oba.ca_paddr = addr;
		/* These are filled-in by obio_submatch. */
		oba.ca_intpri = -1;
		oba.ca_intvec = -1;
		(void)config_found_sm_loc(self, "obio", NULL, &oba, obio_print,
		    obio_submatch);
	}
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
obio_print(void *args, const char *name)
{

	/* Be quiet about empty OBIO locations. */
	if (name)
		return QUIET;

	/* Otherwise do the usual. */
	return bus_print(args, name);
}

int
obio_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct confargs *ca = aux;

	/*
	 * Note that a defaulted address locator can never match
	 * the value of ca->ca_paddr set by the obio_attach loop.
	 * Without this diagnostic, any device with a defaulted
	 * address locator would always be silently unmatched.
	 * Therefore, just disallow default addresses on OBIO.
	 */
#ifdef	DIAGNOSTIC
	if (cf->cf_paddr == -1)
		panic("%s: invalid address for: %s%d",
		    __func__, cf->cf_name, cf->cf_unit);
#endif

	/*
	 * Note that obio_attach calls config_found_sm() with
	 * this function as the "submatch" and ca->ca_paddr
	 * set to each of the possible OBIO locations, so we
	 * want to reject any unmatched address here.
	 */
	if (cf->cf_paddr != ca->ca_paddr)
		return 0;

	/*
	 * Note that the Sun3 does not really support vectored
	 * interrupts on OBIO, but the locator is permitted for
	 * consistency with the Sun3X.  Verify its absence...
	 */
#ifdef	DIAGNOSTIC
	if (cf->cf_intvec != -1)
		panic("%s: %s%d can not have a vector",
		    __func__, cf->cf_name, cf->cf_unit);
#endif

	/*
	 * Copy the locators into our confargs for the child.
	 * Note: ca->ca_bustype was set by our parent driver
	 * (mainbus) and ca->ca_paddr was set by obio_attach.
	 */
	ca->ca_intpri = cf->cf_intpri;
	ca->ca_intvec = -1;

	/* Now call the match function of the potential child. */
	return config_match(parent, cf, aux);
}


/*****************************************************************/

/*
 * Spacing of "interesting" OBIO mappings.  We will
 * record only those with an OBIO address that is a
 * multiple of SAVE_INCR and below SAVE_LAST.
 * The saved mappings are just one page each, which
 * is good enough for all the devices that use this.
 */
#define SAVE_SHIFT	17
#define SAVE_INCR	(1 << SAVE_SHIFT)
#define SAVE_MASK	(SAVE_INCR - 1)
#define SAVE_SLOTS	16
#define SAVE_LAST	(SAVE_SLOTS * SAVE_INCR)

/*
 * This is our record of "interesting" OBIO mappings that
 * the PROM has left in the virtual space reserved for it.
 * Each non-null array element holds the virtual address
 * of an OBIO mapping where the OBIO address mapped is:
 *     (array_index * SAVE_INCR)
 * and the length of the mapping is one page.
 */
static vaddr_t prom_mappings[SAVE_SLOTS];

/*
 * Find a virtual address for a device at physical address 'pa'.
 * If one is found among the mappings already made by the PROM
 * at power-up time, use it and return 0. Otherwise return errno
 * as a sign that a mapping will have to be created.
 */
int
find_prom_map(paddr_t pa, bus_type_t iospace, int sz, vaddr_t *vap)
{
	vsize_t off;
	vaddr_t va;

	off = pa & PGOFSET;
	pa -= off;
	sz += off;

	/* The saved mappings are all one page long. */
	if (sz > PAGE_SIZE)
		return EINVAL;

	/* Within our table? */
	if (pa >= SAVE_LAST)
		return ENOENT;

	/* Do we have this one? */
	va = prom_mappings[pa >> SAVE_SHIFT];
	if (va == 0)
		return ENOENT;

	/* Found it! */
	*vap = va + off;
	return 0;
}

/*
 * This defines the permission bits to put in our PTEs.
 * Device space is never cached, and the PROM appears to
 * leave off the "no-cache" bit, so we can do the same.
 */
#define PGBITS (PG_VALID|PG_WRITE|PG_SYSTEM)

static void
save_prom_mappings(void)
{
	paddr_t pa;
	vaddr_t segva, pgva;
	int pte, sme, i;

	segva = (vaddr_t)SUN3_MONSTART;
	while (segva < (vaddr_t)SUN3_MONEND) {
		sme = get_segmap(segva);
		if (sme == SEGINV) {
			segva += NBSG;
			continue;			/* next segment */
		}
		/*
		 * We have a valid segmap entry, so examine the
		 * PTEs for all the pages in this segment.
		 */
		pgva = segva;	/* starting page */
		segva += NBSG;	/* ending page (next seg) */
		while (pgva < segva) {
			pte = get_pte(pgva);
			if ((pte & (PG_VALID | PG_TYPE)) ==
			    (PG_VALID | PGT_OBIO)) {
				/* Have a valid OBIO mapping. */
				pa = PG_PA(pte);
				/* Is it one we want to record? */
				if ((pa < SAVE_LAST) &&
				    ((pa & SAVE_MASK) == 0)) {
					i = pa >> SAVE_SHIFT;
					if (prom_mappings[i] == 0) {
						prom_mappings[i] = pgva;
					}
				}
				/* Make sure it has the right permissions. */
				if ((pte & PGBITS) != PGBITS) {
					pte |= PGBITS;
					set_pte(pgva, pte);
				}
			}
			pgva += PAGE_SIZE;		/* next page */
		}
	}
}

/*
 * These are all the OBIO address that are required early in
 * the life of the kernel.  All are less than one page long.
 */
static paddr_t required_mappings[] = {
	/* Basically the first six OBIO devices. */
	OBIO_ZS_KBD_MS,
	OBIO_ZS_TTY_AB,
	OBIO_EEPROM,
	OBIO_CLOCK,
	OBIO_MEMERR,
	OBIO_INTERREG,
	(paddr_t)-1,	/* end marker */
};

static void
make_required_mappings(void)
{
	paddr_t *rmp;
	vaddr_t va;

	rmp = required_mappings;
	while (*rmp != (paddr_t)-1) {
		if (find_prom_map(*rmp, PMAP_OBIO, PAGE_SIZE, &va) != 0) {
			/*
			 * XXX - Ack! Need to create one!
			 * I don't think this can happen, but if
			 * it does, we can allocate a PMEG in the
			 * "high segment" and add it there. -gwr
			 */
			mon_printf("obio: no mapping for 0x%x\n", *rmp);
			sunmon_abort();
		}
		rmp++;
	}
}


/*
 * Find mappings for devices that are needed before autoconfiguration.
 * We first look for and record any useful PROM mappings, then call
 * the "init" functions for drivers that we need to use before the
 * normal autoconfiguration calls configure().  Warning: this is
 * called before pmap_bootstrap, so no allocation allowed!
 */
void
obio_init(void)
{

	save_prom_mappings();
	make_required_mappings();

	/*
	 * Find the interrupt reg mapping and turn off the
	 * interrupts, otherwise the PROM clock interrupt
	 * would poll the zs and toggle some LEDs...
	 */
	intreg_init();
}

int
obio_bus_map(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr,
    bus_size_t size, int flags, vaddr_t vaddr, bus_space_handle_t *hp)
{
	struct obio_softc *sc = t->cookie;

	return bus_space_map2(sc->sc_bustag, PMAP_OBIO, paddr, size,
	    flags | _SUN68K_BUS_MAP_USE_PROM, vaddr, hp);
}

paddr_t
obio_bus_mmap(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr, off_t off,
    int prot, int flags)
{
	struct obio_softc *sc = t->cookie;

	return bus_space_mmap2(sc->sc_bustag, PMAP_OBIO, paddr, off, prot,
	    flags);
}

static int
obio_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	int error;

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0)
		map->dm_segs[0].ds_addr &= DVMA_OBIO_SLAVE_MASK;
	return error;
}
