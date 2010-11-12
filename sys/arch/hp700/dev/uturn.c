/*	$NetBSD: uturn.c,v 1.12 2010/11/12 13:18:57 uebayasi Exp $	*/

/*	$OpenBSD: uturn.c,v 1.6 2007/12/29 01:26:14 kettenis Exp $	*/

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

/* TODO IOA programming */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/mbuf.h>

#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>

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

struct uturn_softc {
	device_t sc_dv;

	bus_dma_tag_t sc_dmat;
	struct uturn_regs volatile *sc_regs;

	struct hppa_bus_dma_tag sc_dmatag;
};

int	uturnmatch(device_t, cfdata_t, void *);
void	uturnattach(device_t, device_t, void *);
static void uturn_callback(device_t self, struct confargs *ca);

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

	r->io_control = UTURN_REAL_MODE;

	sc->sc_dmatag = uturn_dmat;
	sc->sc_dmatag._cookie = sc;

	/*
	 * U2/UTurn is actually a combination of an Upper Bus Converter (UBC)
	 * and a Lower Bus Converter (LBC).  This driver attaches to the UBC;
	 * the LBC isn't very interesting, so we skip it.  This is easy, since
	 * it always is module 63, hence the MAXMODBUS - 1 below.
	 */
	nca = *ca;
	nca.ca_hpabase = 0;
	nca.ca_dmatag = &sc->sc_dmatag;
	nca.ca_nmodules = MAXMODBUS - 1;
	pdc_scanbus(self, &nca, uturn_callback);

	/* XXX On some machines, PDC doesn't tell us about all devices. */
	switch (cpu_hvers) {
	case HPPA_BOARD_HP809:
	case HPPA_BOARD_HP819:
	case HPPA_BOARD_HP829:
	case HPPA_BOARD_HP839:
	case HPPA_BOARD_HP849:
	case HPPA_BOARD_HP859:
	case HPPA_BOARD_HP869:

	case HPPA_BOARD_HP800D:
	case HPPA_BOARD_HP821:
		nca.ca_hpabase = r->io_io_low << 16;
		pdc_scanbus(self, &nca, uturn_callback);
		break;
	default:
		break;
	}
}

static void
uturn_callback(device_t self, struct confargs *ca)
{

	config_found_sm_loc(self, "gedoens", NULL, ca, mbprint, mbsubmatch);
}


int
uturn_dmamap_create(void *v, bus_size_t size, int nsegments, 
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamap)
{
	struct uturn_softc *sc = v;
	int err;

	DPRINTF(("%s: size %lx nsegs %d maxsegsz %lx boundary %lx flags %x\n",
 	    __func__, size, nsegments, maxsegsz, boundary, flags));
	err = bus_dmamap_create(sc->sc_dmat, size, nsegments, maxsegsz,
	    boundary, flags, dmamap);

	return err;
}

void
uturn_dmamap_destroy(void *v, bus_dmamap_t map)
{
	struct uturn_softc *sc = v;

	DPRINTF(("%s: map %p\n", __func__, map));

	bus_dmamap_destroy(sc->sc_dmat, map);
}


int
uturn_dmamap_load(void *v, bus_dmamap_t map, void *addr, bus_size_t size,
    struct proc *p, int flags)
{
	struct uturn_softc *sc = v;
	int err;
	int i;

	DPRINTF(("%s: map %p addr %p size %lx proc %p flags %x\n", __func__,
	    map, addr, size, p, flags));

	err = bus_dmamap_load(sc->sc_dmat, map, addr, size, p, flags);

	DPRINTF(("%s: nsegs %2d dm_mapsize %lx\n", __func__, map->dm_nsegs,
	    map->dm_mapsize));
	for (i = 0; i < map->dm_nsegs; i++) {
		DPRINTF(("%s: seg %2d ds_addr %lx _ds_va %lx\n", __func__, i,
		    map->dm_segs[i].ds_addr, map->dm_segs[i]._ds_va)); 
	}

	return err;
}


int
uturn_dmamap_load_mbuf(void *v, bus_dmamap_t map, struct mbuf *m, int flags)
{
	struct uturn_softc *sc = v;
	int err;
	int i;

	DPRINTF(("%s: map %p mbuf %p flags %x\n", __func__, map, m, flags));

	err = bus_dmamap_load_mbuf(sc->sc_dmat, map, m, flags);

	DPRINTF(("%s: nsegs %2d dm_mapsize %lx\n", __func__, map->dm_nsegs,
	    map->dm_mapsize));
	for (i = 0; i < map->dm_nsegs; i++)
		DPRINTF(("%s: seg %2d ds_addr %lx _ds_va %lx\n", __func__, i,
		    map->dm_segs[i].ds_addr, map->dm_segs[i]._ds_va)); 

	return err;
}


int
uturn_dmamap_load_uio(void *v, bus_dmamap_t map, struct uio *uio, int flags)
{
	struct uturn_softc *sc = v;
	int ret;
	int i;

	ret = bus_dmamap_load_uio(sc->sc_dmat, map, uio, flags);
	DPRINTF(("%s: nsegs %2d dm_mapsize %lx\n", __func__, map->dm_nsegs,
	    map->dm_mapsize));
	for (i = 0; i < map->dm_nsegs; i++)
		DPRINTF(("%s: seg %2d ds_addr %lx _ds_va %lx\n", __func__, i,
		    map->dm_segs[i].ds_addr, map->dm_segs[i]._ds_va)); 

	return ret;
}


int
uturn_dmamap_load_raw(void *v, bus_dmamap_t map, bus_dma_segment_t *segs,
    int nsegs, bus_size_t size, int flags)
{

	return 0;
}

void
uturn_dmamap_unload(void *v, bus_dmamap_t map)
{
	struct uturn_softc *sc = v;

	DPRINTF(("%s: map %p\n", __func__, map));

	bus_dmamap_unload(sc->sc_dmat, map);
}


void
uturn_dmamap_sync(void *v, bus_dmamap_t map, bus_addr_t off,
    bus_size_t len, int ops)
{
	struct uturn_softc *sc = v;

	DPRINTF(("%s: map %p off %lx len %lx ops %x\n", __func__, map, off,
	    len, ops));

	bus_dmamap_sync(sc->sc_dmat, map, off, len, ops);
}

int
uturn_dmamem_alloc(void *v, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags)
{
	struct pglist pglist;
	struct vm_page *pg;

	DPRINTF(("%s: size %lx algn %lx bndry %lx segs %p nsegs %d flags %x\n",
	    __func__, size, alignment, boundary, segs, nsegs, flags));

	size = round_page(size);

	TAILQ_INIT(&pglist);
	if (uvm_pglistalloc(size, 0, -1, alignment, boundary,
	    &pglist, 1, (flags & BUS_DMA_NOWAIT) == 0))
		return ENOMEM;

	pg = TAILQ_FIRST(&pglist);
	segs[0]._ds_va = segs[0].ds_addr = VM_PAGE_TO_PHYS(pg);
	segs[0].ds_len = size;
	*rsegs = 1;

	TAILQ_FOREACH(pg, &pglist, pageq.queue) {
		DPRINTF(("%s: pg %p (pa %lx)\n", __func__, pg, VM_PAGE_TO_PHYS(pg)));
		pmap_changebit(pg, PTE_PROT(TLB_UNCACHEABLE), 0);
	}
	pmap_update(pmap_kernel());

	return 0;
}


void
uturn_dmamem_free(void *v, bus_dma_segment_t *segs, int nsegs)
{
	struct pglist pglist;
	paddr_t pa, epa;

	DPRINTF(("%s: segs %p nsegs %d\n", __func__, segs, nsegs));

	TAILQ_INIT(&pglist);
	for (; nsegs--; segs++) {
		for (pa = segs->ds_addr, epa = pa + segs->ds_len;
		    pa < epa; pa += PAGE_SIZE) {
			struct vm_page *pg = PHYS_TO_VM_PAGE(pa);

			KASSERT(pg != NULL);
			TAILQ_INSERT_TAIL(&pglist, pg, pageq.queue);
			pmap_changebit(pg, 0, PTE_PROT(TLB_UNCACHEABLE));
			pdcache(HPPA_SID_KERNEL, pa, PAGE_SIZE);
			pdtlb(HPPA_SID_KERNEL, pa);
		}
	}
	pmap_update(pmap_kernel());
	uvm_pglistfree(&pglist);
}


int
uturn_dmamem_map(void *v, bus_dma_segment_t *segs, int nsegs, size_t size,
    void **kvap, int flags)
{

	DPRINTF(("%s: segs %p nsegs %d size %lx kvap %p flags %x: ", __func__,
	    segs, nsegs, size, kvap, flags));

	*kvap = (void *)segs[0].ds_addr;

	DPRINTF((" kvap %p\n", *kvap));

	return 0;
}


void
uturn_dmamem_unmap(void *v, void *kva, size_t size)
{

	DPRINTF(("%s: kva %p size %zu\n", __func__, kva, size));
}


paddr_t
uturn_dmamem_mmap(void *v, bus_dma_segment_t *segs, int nsegs, off_t off,
    int prot, int flags)
{
	DPRINTF(("%s: segs %p nsegs %d off %llx prot %d flags %x", __func__,
	    segs, nsegs, off, prot, flags));
		 
	return -1;
}
