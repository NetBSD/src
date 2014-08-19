/*	$NetBSD: uturn.c,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $	*/

/*	$OpenBSD: uturn.c,v 1.6 2007/12/29 01:26:14 kettenis Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.
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

/*
 * Copyright (c) 2007 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 2004 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * References:
 * 1. Hardware Cache Coherent Input/Output. Hewlett-Packard Journal, February
 *    1996.
 * 2. PA-RISC 1.1 Architecture and Instruction Set Reference Manual,
 *    Hewlett-Packard, February 1994, Third Edition
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/mbuf.h>
#include <sys/tree.h>

#include <uvm/uvm.h>

#include <sys/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hppa/dev/cpudevs.h>

#define UTURNDEBUG
#ifdef UTURNDEBUG

#define	DPRINTF(s)	do {	\
	if (uturndebug)		\
		printf s;	\
} while(0)

int uturndebug = 0;
#else
#define	DPRINTF(s)	/* */
#endif

struct uturn_regs {
	/* Runway Supervisory Set */
	int32_t		unused1[12];
	uint32_t	io_command;		/* Offset 12 */
#define	UTURN_CMD_TLB_PURGE		33	/* Purge I/O TLB entry */
#define	UTURN_CMD_TLB_DIRECT_WRITE	35	/* I/O TLB Writes */

	uint32_t	io_status;		/* Offset 13 */
	uint32_t	io_control;		/* Offset 14 */
#define	UTURN_IOCTRL_TLB_REAL		0x00000000
#define	UTURN_IOCTRL_TLB_ERROR		0x00010000
#define	UTURN_IOCTRL_TLB_NORMAL		0x00020000

#define	UTURN_IOCTRL_MODE_OFF		0x00000000
#define	UTURN_IOCTRL_MODE_INCLUDE	0x00000080
#define	UTURN_IOCTRL_MODE_PEEK		0x00000180

#define	UTURN_VIRTUAL_MODE	\
	(UTURN_IOCTRL_TLB_NORMAL | UTURN_IOCTRL_MODE_INCLUDE)

#define	UTURN_REAL_MODE		\
	UTURN_IOCTRL_MODE_INCLUDE

	int32_t		unused2[1];

	/* Runway Auxiliary Register Set */
	uint32_t	io_err_resp;		/* Offset  0 */
	uint32_t	io_err_info;		/* Offset  1 */
	uint32_t	io_err_req;		/* Offset  2 */
	uint32_t	io_err_resp_hi;		/* Offset  3 */
	uint32_t	io_tlb_entry_m;		/* Offset  4 */
	uint32_t	io_tlb_entry_l;		/* Offset  5 */
	uint32_t	unused3[1];
	uint32_t	io_pdir_base;		/* Offset  7 */
	uint32_t	io_io_low_hv;		/* Offset  8 */
	uint32_t	io_io_high_hv;		/* Offset  9 */
	uint32_t	unused4[1];
	uint32_t	io_chain_id_mask;	/* Offset 11 */
	uint32_t	unused5[2];
	uint32_t	io_io_low;		/* Offset 14 */
	uint32_t	io_io_high;		/* Offset 15 */
};


/* Uturn supports 256 TLB entries */
#define	UTURN_CHAINID_SHIFT	8
#define	UTURN_CHAINID_MASK	0xff
#define	UTURN_TLB_ENTRIES	(1 << UTURN_CHAINID_SHIFT)

#define	UTURN_IOVP_SIZE		PAGE_SIZE
#define	UTURN_IOVP_SHIFT	PAGE_SHIFT
#define	UTURN_IOVP_MASK		PAGE_MASK

#define	UTURN_IOVA(iovp, off)	((iovp) | (off))
#define	UTURN_IOVP(iova)	((iova) & UTURN_IOVP_MASK)
#define	UTURN_IOVA_INDEX(iova)	((iova) >> UTURN_IOVP_SHIFT)

struct uturn_softc {
	device_t sc_dv;

	bus_dma_tag_t sc_dmat;
	struct uturn_regs volatile *sc_regs;
	uint64_t *sc_pdir;
	uint32_t sc_chainid_shift;

	char sc_mapname[20];
	struct extent *sc_map;

	struct hppa_bus_dma_tag sc_dmatag;
};

/*
 * per-map IOVA page table
 */
struct uturn_page_entry {
	SPLAY_ENTRY(uturn_page_entry) upe_node;
	paddr_t	upe_pa;
	vaddr_t	upe_va;
	bus_addr_t upe_iova;
};

struct uturn_page_map {
	SPLAY_HEAD(uturn_page_tree, uturn_page_entry) upm_tree;
	int upm_maxpage;	/* Size of allocated page map */
	int upm_pagecnt;	/* Number of entries in use */
	struct uturn_page_entry	upm_map[1];
};

/*
 * per-map UTURN state
 */
struct uturn_map_state {
	struct uturn_softc *ums_sc;
	bus_addr_t ums_iovastart;
	bus_size_t ums_iovasize;
	struct uturn_page_map ums_map;	/* map must be last (array at end) */
};

int	uturnmatch(device_t, cfdata_t, void *);
void	uturnattach(device_t, device_t, void *);
static device_t uturn_callback(device_t, struct confargs *);

CFATTACH_DECL_NEW(uturn, sizeof(struct uturn_softc),
    uturnmatch, uturnattach, NULL, NULL);

extern struct cfdriver uturn_cd;

int uturn_dmamap_create(void *, bus_size_t, int, bus_size_t, bus_size_t, int,
    bus_dmamap_t *);
void uturn_dmamap_destroy(void *, bus_dmamap_t);
int uturn_dmamap_load(void *, bus_dmamap_t, void *, bus_size_t, struct proc *,
    int);
int uturn_dmamap_load_mbuf(void *, bus_dmamap_t, struct mbuf *, int);
int uturn_dmamap_load_uio(void *, bus_dmamap_t, struct uio *, int);
int uturn_dmamap_load_raw(void *, bus_dmamap_t, bus_dma_segment_t *, int,
    bus_size_t, int);
void uturn_dmamap_unload(void *, bus_dmamap_t);
void uturn_dmamap_sync(void *, bus_dmamap_t, bus_addr_t, bus_size_t, int);
int uturn_dmamem_alloc(void *, bus_size_t, bus_size_t, bus_size_t,
    bus_dma_segment_t *, int, int *, int);
void uturn_dmamem_free(void *, bus_dma_segment_t *, int);
int uturn_dmamem_map(void *, bus_dma_segment_t *, int, size_t, void **, int);
void uturn_dmamem_unmap(void *, void *, size_t);
paddr_t uturn_dmamem_mmap(void *, bus_dma_segment_t *, int, off_t, int, int);

static void uturn_iommu_enter(struct uturn_softc *, bus_addr_t, pa_space_t,
    vaddr_t, paddr_t);
static void uturn_iommu_remove(struct uturn_softc *, bus_addr_t, bus_size_t);

struct uturn_map_state *uturn_iomap_create(int);
void	uturn_iomap_destroy(struct uturn_map_state *);
int	uturn_iomap_insert_page(struct uturn_map_state *, vaddr_t, paddr_t);
bus_addr_t uturn_iomap_translate(struct uturn_map_state *, paddr_t);
void	uturn_iomap_clear_pages(struct uturn_map_state *);

static int uturn_iomap_load_map(struct uturn_softc *, bus_dmamap_t, int);

const struct hppa_bus_dma_tag uturn_dmat = {
	NULL,
	uturn_dmamap_create, uturn_dmamap_destroy,
	uturn_dmamap_load, uturn_dmamap_load_mbuf,
	uturn_dmamap_load_uio, uturn_dmamap_load_raw,
	uturn_dmamap_unload, uturn_dmamap_sync,

	uturn_dmamem_alloc, uturn_dmamem_free, uturn_dmamem_map,
	uturn_dmamem_unmap, uturn_dmamem_mmap
};

int
uturnmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* there will be only one */
	if (ca->ca_type.iodc_type != HPPA_TYPE_IOA ||
	    ca->ca_type.iodc_sv_model != HPPA_IOA_UTURN)
		return 0;

	if (ca->ca_type.iodc_model == 0x58 &&
	    ca->ca_type.iodc_revision >= 0x20)
		return 0;

	return 1;
}

void
uturnattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux, nca;
	struct uturn_softc *sc = device_private(self);
	bus_space_handle_t ioh;
	volatile struct uturn_regs *r;
	struct pglist pglist;
	int iova_bits;
	vaddr_t va;
	psize_t size;
	int i;

	if (bus_space_map(ca->ca_iot, ca->ca_hpa, IOMOD_HPASIZE, 0, &ioh)) {
		aprint_error(": can't map IO space\n");
		return;
	}

	sc->sc_dv = self;
	sc->sc_dmat = ca->ca_dmatag;
	sc->sc_regs = r = bus_space_vaddr(ca->ca_iot, ioh);

	aprint_normal(": %x-%x", r->io_io_low << 16, r->io_io_high << 16);
	aprint_normal(": %x-%x", r->io_io_low_hv << 16, r->io_io_high_hv << 16);

	aprint_normal(": %s rev %d\n",
	    ca->ca_type.iodc_revision < 0x10 ? "U2" : "UTurn",
	    ca->ca_type.iodc_revision & 0xf);

	/*
	 * Setup the iommu.
	 */

	/* XXX 28 bits gives us 256Mb of iova space */
	/* Calculate based on %age of RAM */
	iova_bits = 28;

	/*
	 * size is # of pdir entries (64bits) in bytes.  1 entry per IOVA
	 * page.
	 */
	size = (1 << (iova_bits - UTURN_IOVP_SHIFT)) * sizeof(uint64_t);

	/*
	 * Chainid is the upper most bits of an IOVP used to determine which
	 * TLB entry an IOVP will use.
	 */
	sc->sc_chainid_shift = iova_bits - UTURN_CHAINID_SHIFT;

	/*
	 * Allocate memory for I/O pagetables.  They need to be physically
	 * contiguous.
	 */

	if (uvm_pglistalloc(size, 0, -1, PAGE_SIZE, 0, &pglist, 1, 0) != 0)
		panic("%s: no memory", __func__);

	va = (vaddr_t)VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
	sc->sc_pdir = (int64_t *)va;

	memset(sc->sc_pdir, 0, size);

	r->io_chain_id_mask = UTURN_CHAINID_MASK << sc->sc_chainid_shift;
	r->io_pdir_base = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));

	r->io_tlb_entry_m = 0;
	r->io_tlb_entry_l = 0;

	/* for (i = UTURN_TLB_ENTRIES; i != 0; i--) { */
	for (i = 0; i < UTURN_TLB_ENTRIES; i++) {
		r->io_command =
		    UTURN_CMD_TLB_DIRECT_WRITE | (i << sc->sc_chainid_shift);
	}
	/*
	 * Go to "Virtual Mode"
	 */
	r->io_control = UTURN_VIRTUAL_MODE;

	snprintf(sc->sc_mapname, sizeof(sc->sc_mapname), "%s_map",
	    device_xname(sc->sc_dv));
	sc->sc_map = extent_create(sc->sc_mapname, 0, (1 << iova_bits),
	    0, 0, EX_NOWAIT);

	sc->sc_dmatag = uturn_dmat;
	sc->sc_dmatag._cookie = sc;

	/*
	 * U2/UTurn is actually a combination of an Upper Bus Converter (UBC)
	 * and a Lower Bus Converter (LBC).  This driver attaches to the UBC;
	 * the LBC isn't very interesting, so we skip it.  This is easy, since
	 * it always is module 63, hence the MAXMODBUS - 1 below.
	 */
	nca = *ca;
	nca.ca_hpabase = r->io_io_low << 16;
	nca.ca_dmatag = &sc->sc_dmatag;
	nca.ca_nmodules = MAXMODBUS - 1;
	pdc_scanbus(self, &nca, uturn_callback);
}

static device_t
uturn_callback(device_t self, struct confargs *ca)
{

	return config_found_sm_loc(self, "gedoens", NULL, ca, mbprint,
	    mbsubmatch);
}

/*
 * PDIR entry format (HP bit number)
 *
 * +-------+----------------+----------------------------------------------+
 * |0     3|4             15|16                                          31|
 * | PPN   | Virtual Index  |         Physical Page Number (PPN)           |
 * | [0:3] |    [0:11]      |                 [4:19]                       |
 * +-------+----------------+----------------------------------------------+
 * 
 * +-----------------------+-----------------------------------------------+
 * |0           19|20    24|   25   |       |       |      |  30   |   31  |
 * |     PPN      |  Rsvd  | PH     |Update | Rsvd  |Lock  | Safe  | Valid |
 * |    [20:39    |        | Enable |Enable |       |Enable| DMA   |       |
 * +-----------------------+-----------------------------------------------+
 *
 */

#define UTURN_PENTRY_PREFETCH	0x40
#define UTURN_PENTRY_UPDATE	0x20
#define UTURN_PENTRY_LOCK	0x04	/* eisa devices only */
#define UTURN_PENTRY_SAFEDMA	0x02	/* use safe dma - for subcacheline */
#define UTURN_PENTRY_VALID	0x01

static void
uturn_iommu_enter(struct uturn_softc *sc, bus_addr_t iova, pa_space_t sp,
    vaddr_t va, paddr_t pa)
{
	uint64_t pdir_entry;
	uint64_t *pdirp;
	uint32_t ci; /* coherent index */

	pdirp = &sc->sc_pdir[UTURN_IOVA_INDEX(iova)];

	DPRINTF(("%s: iova %lx pdir %p pdirp %p pa %lx", __func__, iova,
	    sc->sc_pdir, pdirp, pa));

	ci = lci(HPPA_SID_KERNEL, va);

	/* setup hints, etc */
	pdir_entry = (UTURN_PENTRY_LOCK | UTURN_PENTRY_SAFEDMA |
	     UTURN_PENTRY_VALID);

	/*
	 * bottom 36 bits of pa map directly into entry to form PPN[4:39]
	 * leaving last 12 bits for hints, etc.
	 */
	pdir_entry |= (pa & ~PAGE_MASK);

	/* mask off top PPN bits */
	pdir_entry &= 0x0000ffffffffffffUL;
	
	/* insert the virtual index bits */
	pdir_entry |= (((uint64_t)ci >> 12) << 48);

	/* PPN[0:3] of the 40bit PPN go in entry[0:3] */
	pdir_entry |= ((((uint64_t)pa & 0x000f000000000000UL) >> 48) << 60);

	*pdirp = pdir_entry;

	DPRINTF((": pdir_entry %llx\n", pdir_entry));

	/*
	 * We could use PDC_MODEL_CAPABILITIES here
	 */
 	fdcache(HPPA_SID_KERNEL, (vaddr_t)pdirp, sizeof(uint64_t));
}


static void
uturn_iommu_remove(struct uturn_softc *sc, bus_addr_t iova, bus_size_t size)
{
	uint32_t chain_size = 1 << sc->sc_chainid_shift;
	bus_size_t len;

	KASSERT((iova & PAGE_MASK) == 0);
	KASSERT((size & PAGE_MASK) == 0);

	DPRINTF(("%s: sc %p iova %lx size %lx\n", __func__, sc, iova, size));
	len = size;
	while (len != 0) {
		uint64_t *pdirp = &sc->sc_pdir[UTURN_IOVA_INDEX(iova)];

		/* XXX Just the valid bit??? */
		*pdirp = 0;

		/*
		* We could use PDC_MODEL_CAPABILITIES here
		*/
	 	fdcache(HPPA_SID_KERNEL, (vaddr_t)pdirp, sizeof(uint64_t));

		iova += PAGE_SIZE;
		len -= PAGE_SIZE;
	}

	len = size + chain_size;

	while (len > chain_size) {
		sc->sc_regs->io_command = UTURN_CMD_TLB_PURGE | iova;
		iova += chain_size;
		len -= chain_size;
	}
}

int
uturn_dmamap_create(void *v, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamap)
{
	struct uturn_softc *sc = v;
	bus_dmamap_t map;
	struct uturn_map_state *ums;
	int error;

	error = bus_dmamap_create(sc->sc_dmat, size, nsegments, maxsegsz,
	    boundary, flags, &map);
	if (error)
		return (error);

	ums = uturn_iomap_create(atop(round_page(size)));
	if (ums == NULL) {
		bus_dmamap_destroy(sc->sc_dmat, map);
		return (ENOMEM);
	}

	ums->ums_sc = sc;
	map->_dm_cookie = ums;
	*dmamap = map;

	return (0);
}

void
uturn_dmamap_destroy(void *v, bus_dmamap_t map)
{
	struct uturn_softc *sc = v;

	/*
	 * The specification (man page) requires a loaded
	 * map to be unloaded before it is destroyed.
	 */
	if (map->dm_nsegs)
		uturn_dmamap_unload(sc, map);

	if (map->_dm_cookie)
		uturn_iomap_destroy(map->_dm_cookie);
	map->_dm_cookie = NULL;

	bus_dmamap_destroy(sc->sc_dmat, map);
}

static int
uturn_iomap_load_map(struct uturn_softc *sc, bus_dmamap_t map, int flags)
{
	struct uturn_map_state *ums = map->_dm_cookie;
	struct uturn_page_map *upm = &ums->ums_map;
	struct uturn_page_entry *e;
	int err, seg, s;
	paddr_t pa, paend;
	vaddr_t va;
	bus_size_t sgsize;
	bus_size_t align, boundary;
	u_long iovaddr;
	bus_addr_t iova;
	int i;

	/* XXX */
	boundary = map->_dm_boundary;
	align = PAGE_SIZE;

	uturn_iomap_clear_pages(ums);

	for (seg = 0; seg < map->dm_nsegs; seg++) {
		struct hppa_bus_dma_segment *ds = &map->dm_segs[seg];

		paend = round_page(ds->ds_addr + ds->ds_len);
		for (pa = trunc_page(ds->ds_addr), va = trunc_page(ds->_ds_va);
		     pa < paend; pa += PAGE_SIZE, va += PAGE_SIZE) {
			err = uturn_iomap_insert_page(ums, va, pa);
			if (err) {
				printf("iomap insert error: %d for "
				    "va 0x%lx pa 0x%lx\n", err, va, pa);
				bus_dmamap_unload(sc->sc_dmat, map);
				uturn_iomap_clear_pages(ums);
			}
		}
	}

	sgsize = ums->ums_map.upm_pagecnt * PAGE_SIZE;
	/* XXXNH */
	s = splhigh();
	err = extent_alloc(sc->sc_map, sgsize, align, boundary,
	    EX_NOWAIT | EX_BOUNDZERO, &iovaddr);
	splx(s);
	if (err)
		return (err);

	ums->ums_iovastart = iovaddr;
	ums->ums_iovasize = sgsize;

	iova = iovaddr;
	for (i = 0, e = upm->upm_map; i < upm->upm_pagecnt; ++i, ++e) {
		e->upe_iova = iova;
		uturn_iommu_enter(sc, e->upe_iova, HPPA_SID_KERNEL, e->upe_va,
		    e->upe_pa);
		iova += PAGE_SIZE;
	}

	for (seg = 0; seg < map->dm_nsegs; seg++) {
		struct hppa_bus_dma_segment *ds = &map->dm_segs[seg];
		ds->ds_addr = uturn_iomap_translate(ums, ds->ds_addr);
	}

	return (0);
}

int
uturn_dmamap_load(void *v, bus_dmamap_t map, void *addr, bus_size_t size,
    struct proc *p, int flags)
{
	struct uturn_softc *sc = v;
	int err;

	err = bus_dmamap_load(sc->sc_dmat, map, addr, size, p, flags);
	if (err)
		return (err);

	return uturn_iomap_load_map(sc, map, flags);
}

int
uturn_dmamap_load_mbuf(void *v, bus_dmamap_t map, struct mbuf *m, int flags)
{
	struct uturn_softc *sc = v;
	int err;

	err = bus_dmamap_load_mbuf(sc->sc_dmat, map, m, flags);
	if (err)
		return (err);

	return uturn_iomap_load_map(sc, map, flags);
}

int
uturn_dmamap_load_uio(void *v, bus_dmamap_t map, struct uio *uio, int flags)
{
	struct uturn_softc *sc = v;

	printf("load_uio\n");

	return (bus_dmamap_load_uio(sc->sc_dmat, map, uio, flags));
}

int
uturn_dmamap_load_raw(void *v, bus_dmamap_t map, bus_dma_segment_t *segs,
    int nsegs, bus_size_t size, int flags)
{
	struct uturn_softc *sc = v;

	printf("load_raw\n");

	return (bus_dmamap_load_raw(sc->sc_dmat, map, segs, nsegs, size, flags));
}

void
uturn_dmamap_unload(void *v, bus_dmamap_t map)
{
	struct uturn_softc *sc = v;
	struct uturn_map_state *ums = map->_dm_cookie;
	struct uturn_page_map *upm = &ums->ums_map;
	struct uturn_page_entry *e;
	int err, i, s;

	/* Remove the IOMMU entries. */
	for (i = 0, e = upm->upm_map; i < upm->upm_pagecnt; ++i, ++e)
		uturn_iommu_remove(sc, e->upe_iova, PAGE_SIZE);

	/* Clear the iomap. */
	uturn_iomap_clear_pages(ums);

	bus_dmamap_unload(sc->sc_dmat, map);

	s = splhigh();
	err = extent_free(sc->sc_map, ums->ums_iovastart,
	    ums->ums_iovasize, EX_NOWAIT);
	ums->ums_iovastart = 0;
	ums->ums_iovasize = 0;
	splx(s);
	if (err)
		printf("warning: %ld of IOVA space lost\n", ums->ums_iovasize);
}

void
uturn_dmamap_sync(void *v, bus_dmamap_t map, bus_addr_t off,
    bus_size_t len, int ops)
{
	/* Nothing to do; DMA is cache-coherent. */
}

int
uturn_dmamem_alloc(void *v, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags)
{
	struct uturn_softc *sc = v;

	return (bus_dmamem_alloc(sc->sc_dmat, size, alignment, boundary,
	    segs, nsegs, rsegs, flags));
}

void
uturn_dmamem_free(void *v, bus_dma_segment_t *segs, int nsegs)
{
	struct uturn_softc *sc = v;

	bus_dmamem_free(sc->sc_dmat, segs, nsegs);
}

int
uturn_dmamem_map(void *v, bus_dma_segment_t *segs, int nsegs, size_t size,
    void **kvap, int flags)
{
	struct uturn_softc *sc = v;

	return (bus_dmamem_map(sc->sc_dmat, segs, nsegs, size, kvap, flags));
}

void
uturn_dmamem_unmap(void *v, void *kva, size_t size)
{
	struct uturn_softc *sc = v;

	bus_dmamem_unmap(sc->sc_dmat, kva, size);
}

paddr_t
uturn_dmamem_mmap(void *v, bus_dma_segment_t *segs, int nsegs, off_t off,
    int prot, int flags)
{
	struct uturn_softc *sc = v;

	return (bus_dmamem_mmap(sc->sc_dmat, segs, nsegs, off, prot, flags));
}

/*
 * Utility function used by splay tree to order page entries by pa.
 */
static inline int
upe_compare(struct uturn_page_entry *a, struct uturn_page_entry *b)
{
	return ((a->upe_pa > b->upe_pa) ? 1 :
		(a->upe_pa < b->upe_pa) ? -1 : 0);
}

SPLAY_PROTOTYPE(uturn_page_tree, uturn_page_entry, upe_node, upe_compare);

SPLAY_GENERATE(uturn_page_tree, uturn_page_entry, upe_node, upe_compare);

/*
 * Create a new iomap.
 */
struct uturn_map_state *
uturn_iomap_create(int n)
{
	struct uturn_map_state *ums;

	/* Safety for heavily fragmented data, such as mbufs */
	n += 4;
	if (n < 16)
		n = 16;

	ums = malloc(sizeof(*ums) + (n - 1) * sizeof(ums->ums_map.upm_map[0]),
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ums == NULL)
		return (NULL);

	/* Initialize the map. */
	ums->ums_map.upm_maxpage = n;
	SPLAY_INIT(&ums->ums_map.upm_tree);

	return (ums);
}

/*
 * Destroy an iomap.
 */
void
uturn_iomap_destroy(struct uturn_map_state *ums)
{
	KASSERT(ums->ums_map.upm_pagecnt == 0);

	free(ums, M_DEVBUF);
}

/*
 * Insert a pa entry in the iomap.
 */
int
uturn_iomap_insert_page(struct uturn_map_state *ums, vaddr_t va, paddr_t pa)
{
	struct uturn_page_map *upm = &ums->ums_map;
	struct uturn_page_entry *e;

	if (upm->upm_pagecnt >= upm->upm_maxpage) {
		struct uturn_page_entry upe;

		upe.upe_pa = pa;
		if (SPLAY_FIND(uturn_page_tree, &upm->upm_tree, &upe))
			return (0);

		return (ENOMEM);
	}

	e = &upm->upm_map[upm->upm_pagecnt];

	e->upe_pa = pa;
	e->upe_va = va;
	e->upe_iova = 0;

	e = SPLAY_INSERT(uturn_page_tree, &upm->upm_tree, e);

	/* Duplicates are okay, but only count them once. */
	if (e)
		return (0);

	++upm->upm_pagecnt;

	return (0);
}

/*
 * Translate a physical address (pa) into a IOVA address.
 */
bus_addr_t
uturn_iomap_translate(struct uturn_map_state *ums, paddr_t pa)
{
	struct uturn_page_map *upm = &ums->ums_map;
	struct uturn_page_entry *e;
	struct uturn_page_entry pe;
	paddr_t offset = pa & PAGE_MASK;

	pe.upe_pa = trunc_page(pa);

	e = SPLAY_FIND(uturn_page_tree, &upm->upm_tree, &pe);

	if (e == NULL) {
		panic("couldn't find pa %lx\n", pa);
		return 0;
	}

	return (e->upe_iova | offset);
}

/*
 * Clear the iomap table and tree.
 */
void
uturn_iomap_clear_pages(struct uturn_map_state *ums)
{
	ums->ums_map.upm_pagecnt = 0;
	SPLAY_INIT(&ums->ums_map.upm_tree);
}
