/*	$NetBSD: iommu.c,v 1.37 2000/01/07 10:54:11 pk Exp $ */

/*
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1995 	Paul Kranenburg
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
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
 *	This product includes software developed by Paul Kranenburg.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/iommureg.h>
#include <sparc/sparc/iommuvar.h>

struct iommu_softc {
	struct device	sc_dev;		/* base device */
	struct iommureg	*sc_reg;
	u_int		sc_pagesize;
	u_int		sc_range;
	bus_addr_t	sc_dvmabase;
	iopte_t		*sc_ptes;
	int		sc_hasiocache;
};
struct	iommu_softc *iommu_sc;/*XXX*/
int	has_iocache;
u_long	dvma_cachealign;

/*
 * Note: operations on the extent map are being protected with
 * splhigh(), since we cannot predict at which interrupt priority
 * our clients will run.
 */
struct extent *iommu_dvmamap;


/* autoconfiguration driver */
int	iommu_print __P((void *, const char *));
void	iommu_attach __P((struct device *, struct device *, void *));
int	iommu_match __P((struct device *, struct cfdata *, void *));

struct cfattach iommu_ca = {
	sizeof(struct iommu_softc), iommu_match, iommu_attach
};

/* IOMMU DMA map functions */
int	iommu_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	iommu_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	iommu_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	iommu_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	iommu_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	iommu_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	iommu_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void	iommu_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
int	iommu_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags));
int	iommu_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, int off, int prot, int flags));


struct sparc_bus_dma_tag iommu_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	iommu_dmamap_load,
	iommu_dmamap_load_mbuf,
	iommu_dmamap_load_uio,
	iommu_dmamap_load_raw,
	iommu_dmamap_unload,
	iommu_dmamap_sync,

	iommu_dmamem_alloc,
	iommu_dmamem_free,
	iommu_dmamem_map,
	_bus_dmamem_unmap,
	iommu_dmamem_mmap
};
/*
 * Print the location of some iommu-attached device (called just
 * before attaching that device).  If `iommu' is not NULL, the
 * device was found but not configured; print the iommu as well.
 * Return UNCONF (config_find ignores this if the device was configured).
 */
int
iommu_print(args, iommu)
	void *args;
	const char *iommu;
{
	struct iommu_attach_args *ia = args;

	if (iommu)
		printf("%s at %s", ia->iom_name, iommu);
	return (UNCONF);
}

int
iommu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (CPU_ISSUN4OR4C)
		return (0);
	return (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0);
}

/*
 * Attach the iommu.
 */
void
iommu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
#if defined(SUN4M)
	struct iommu_softc *sc = (struct iommu_softc *)self;
	struct mainbus_attach_args *ma = aux;
	int node;
	struct bootpath *bp;
	bus_space_handle_t bh;
	u_int pbase, pa;
	int i, mmupcrsave, s;
	iopte_t *tpte_p;
	extern u_int *kernel_iopte_table;
	extern u_int kernel_iopte_table_pa;

/*XXX-GCC!*/mmupcrsave=0;
	iommu_sc = sc;
	/*
	 * XXX there is only one iommu, for now -- do not know how to
	 * address children on others
	 */
	if (sc->sc_dev.dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}
	node = ma->ma_node;

#if 0
	if (ra->ra_vaddr)
		sc->sc_reg = (struct iommureg *)ca->ca_ra.ra_vaddr;
#else
	/*
	 * Map registers into our space. The PROM may have done this
	 * already, but I feel better if we have our own copy. Plus, the
	 * prom doesn't map the entire register set
	 *
	 * XXX struct iommureg is bigger than ra->ra_len; what are the
	 *     other fields for?
	 */
	if (bus_space_map2(
			ma->ma_bustag,
			ma->ma_iospace,
			ma->ma_paddr,
			sizeof(struct iommureg),
			0,
			0,
			&bh) != 0) {
		printf("iommu_attach: cannot map registers\n");
		return;
	}
	sc->sc_reg = (struct iommureg *)bh;
#endif

	sc->sc_hasiocache = node_has_property(node, "cache-coherence?");
	if (CACHEINFO.c_enabled == 0) /* XXX - is this correct? */
		sc->sc_hasiocache = 0;
	has_iocache = sc->sc_hasiocache; /* Set global flag */

	sc->sc_pagesize = getpropint(node, "page-size", NBPG),
	sc->sc_range = (1 << 24) <<
	    ((sc->sc_reg->io_cr & IOMMU_CTL_RANGE) >> IOMMU_CTL_RANGESHFT);
#if 0
	sc->sc_dvmabase = (0 - sc->sc_range);
#endif
	pbase = (sc->sc_reg->io_bar & IOMMU_BAR_IBA) <<
			(14 - IOMMU_BAR_IBASHFT);

	/*
	 * Now we build our own copy of the IOMMU page tables. We need to
	 * do this since we're going to change the range to give us 64M of
	 * mappings, and thus we can move DVMA space down to 0xfd000000 to
	 * give us lots of space and to avoid bumping into the PROM, etc.
	 *
	 * XXX Note that this is rather messy.
	 */
	sc->sc_ptes = (iopte_t *) kernel_iopte_table;

	/*
	 * Now discache the page tables so that the IOMMU sees our
	 * changes.
	 */
	kvm_uncache((caddr_t)sc->sc_ptes,
	    (((0 - IOMMU_DVMA_BASE)/sc->sc_pagesize) * sizeof(iopte_t)) / NBPG);

	/*
	 * Ok. We've got to read in the original table using MMU bypass,
	 * and copy all of its entries to the appropriate place in our
	 * new table, even if the sizes are different.
	 * This is pretty easy since we know DVMA ends at 0xffffffff.
	 *
	 * XXX: PGOFSET, NBPG assume same page size as SRMMU
	 */
	if (cpuinfo.cpu_impl == 4 && cpuinfo.mxcc) {
		/* set MMU AC bit */
		sta(SRMMU_PCR, ASI_SRMMU,
		    ((mmupcrsave = lda(SRMMU_PCR, ASI_SRMMU)) | VIKING_PCR_AC));
	}

	for (tpte_p = &sc->sc_ptes[((0 - IOMMU_DVMA_BASE)/NBPG) - 1],
	     pa = (u_int)pbase - sizeof(iopte_t) +
		   ((u_int)sc->sc_range/NBPG)*sizeof(iopte_t);
	     tpte_p >= &sc->sc_ptes[0] && pa >= (u_int)pbase;
	     tpte_p--, pa -= sizeof(iopte_t)) {

		IOMMU_FLUSHPAGE(sc,
			     (tpte_p - &sc->sc_ptes[0])*NBPG + IOMMU_DVMA_BASE);
		*tpte_p = lda(pa, ASI_BYPASS);
	}
	if (cpuinfo.cpu_impl == 4 && cpuinfo.mxcc) {
		/* restore mmu after bug-avoidance */
		sta(SRMMU_PCR, ASI_SRMMU, mmupcrsave);
	}

	/*
	 * Now we can install our new pagetable into the IOMMU
	 */
	sc->sc_range = 0 - IOMMU_DVMA_BASE;
	sc->sc_dvmabase = IOMMU_DVMA_BASE;

	/* calculate log2(sc->sc_range/16MB) */
	i = ffs(sc->sc_range/(1 << 24)) - 1;
	if ((1 << i) != (sc->sc_range/(1 << 24)))
		panic("bad iommu range: %d\n",i);

	s = splhigh();
	IOMMU_FLUSHALL(sc);

	sc->sc_reg->io_cr = (sc->sc_reg->io_cr & ~IOMMU_CTL_RANGE) |
			  (i << IOMMU_CTL_RANGESHFT) | IOMMU_CTL_ME;
	sc->sc_reg->io_bar = (kernel_iopte_table_pa >> 4) & IOMMU_BAR_IBA;

	IOMMU_FLUSHALL(sc);
	splx(s);

	printf(": version 0x%x/0x%x, page-size %d, range %dMB\n",
		(sc->sc_reg->io_cr & IOMMU_CTL_VER) >> 24,
		(sc->sc_reg->io_cr & IOMMU_CTL_IMPL) >> 28,
		sc->sc_pagesize,
		sc->sc_range >> 20);

	/* Propagate bootpath */
	if (ma->ma_bp != NULL && strcmp(ma->ma_bp->name, "iommu") == 0)
		bp = ma->ma_bp + 1;
	else
		bp = NULL;

	iommu_dvmamap = extent_create("iommudvma",
					IOMMU_DVMA_BASE, IOMMU_DVMA_END,
					M_DEVBUF, 0, 0, EX_NOWAIT);
	if (iommu_dvmamap == NULL)
		panic("iommu: unable to allocate DVMA map");

	/*
	 * Loop through ROM children (expect Sbus among them).
	 */
	for (node = firstchild(node); node; node = nextsibling(node)) {
		struct iommu_attach_args ia;

		bzero(&ia, sizeof ia);
		ia.iom_name = getpropstring(node, "name");

		/* Propagate BUS & DMA tags */
		ia.iom_bustag = ma->ma_bustag;
		ia.iom_dmatag = &iommu_dma_tag;

		ia.iom_node = node;
		ia.iom_bp = bp;

		ia.iom_reg = NULL;
		getprop(node, "reg", sizeof(struct sbus_reg),
			&ia.iom_nreg, (void **)&ia.iom_reg);

		(void) config_found(&sc->sc_dev, (void *)&ia, iommu_print);
		if (ia.iom_reg != NULL)
			free(ia.iom_reg, M_DEVBUF);
	}
#endif
}

void
iommu_enter(va, pa)
	bus_addr_t va;
	paddr_t pa;
{
	struct iommu_softc *sc = iommu_sc;
	int pte;

#ifdef DEBUG
	if (va < sc->sc_dvmabase)
		panic("iommu_enter: va 0x%lx not in DVMA space", (long)va);
#endif

	pte = atop(pa) << IOPTE_PPNSHFT;
	pte &= IOPTE_PPN;
	pte |= IOPTE_V | IOPTE_W | (has_iocache ? IOPTE_C : 0);
	sc->sc_ptes[atop(va - sc->sc_dvmabase)] = pte;
	IOMMU_FLUSHPAGE(sc, va);
}

/*
 * iommu_clear: clears mappings created by iommu_enter
 */
void
iommu_remove(va, len)
	bus_addr_t va;
	bus_size_t len;
{
	struct iommu_softc *sc = iommu_sc;
	u_int pagesz = sc->sc_pagesize;
	bus_addr_t base = sc->sc_dvmabase;

#ifdef DEBUG
	if (va < base)
		panic("iommu_enter: va 0x%lx not in DVMA space", (long)va);
#endif

	while ((long)len > 0) {
#ifdef notyet
#ifdef DEBUG
		if ((sc->sc_ptes[atop(va - base)] & IOPTE_V) == 0)
			panic("iommu_clear: clearing invalid pte at va 0x%lx",
			      (long)va);
#endif
#endif
		sc->sc_ptes[atop(va - base)] = 0;
		IOMMU_FLUSHPAGE(sc, va);
		len -= pagesz;
		va += pagesz;
	}
}

#if 0	/* These registers aren't there??? */
void
iommu_error()
{
	struct iommu_softc *sc = X;
	struct iommureg *iop = sc->sc_reg;

	printf("iommu: afsr 0x%x, afar 0x%x\n", iop->io_afsr, iop->io_afar);
	printf("iommu: mfsr 0x%x, mfar 0x%x\n", iop->io_mfsr, iop->io_mfar);
}
int
iommu_alloc(va, len)
	u_int va, len;
{
	struct iommu_softc *sc = X;
	int off, tva, iovaddr, pte;
	paddr_t pa;

	off = (int)va & PGOFSET;
	len = round_page(len + off);
	va -= off;

if ((int)sc->sc_dvmacur + len > 0)
	sc->sc_dvmacur = sc->sc_dvmabase;

	iovaddr = tva = sc->sc_dvmacur;
	sc->sc_dvmacur += len;
	while (len) {
		(void) pmap_extract(pmap_kernel(), va, &pa);

#define IOMMU_PPNSHIFT	8
#define IOMMU_V		0x00000002
#define IOMMU_W		0x00000004

		pte = atop(pa) << IOMMU_PPNSHIFT;
		pte |= IOMMU_V | IOMMU_W;
		sta(sc->sc_ptes + atop(tva - sc->sc_dvmabase), ASI_BYPASS, pte);
		sc->sc_reg->io_flushpage = tva;
		len -= NBPG;
		va += NBPG;
		tva += NBPG;
	}
	return iovaddr + off;
}
#endif


/*
 * IOMMU DMA map functions.
 */
int
iommu_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_size_t sgsize;
	bus_addr_t dva;
	bus_addr_t boundary;
	vaddr_t va = (vaddr_t)buf;
	u_long align, voff;
	u_long ex_start, ex_end;
	pmap_t pmap;
	int s, error;

	/*
	 * Remember page offset, then truncate the buffer address to
	 * a page boundary.
	 */
	voff = va & PGOFSET;
	va &= ~PGOFSET;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	sgsize = (buflen + voff + PGOFSET) & ~PGOFSET;
	align = dvma_cachealign ? dvma_cachealign : NBPG;
	boundary = map->_dm_boundary;

	s = splhigh();

	/* Check `24 address bits' in the map's attributes */
	if ((map->_dm_flags & BUS_DMA_24BIT) != 0) {
		ex_start = D24_DVMA_BASE;
		ex_end = D24_DVMA_END;
	} else {
		ex_start = iommu_dvmamap->ex_start;
		ex_end = iommu_dvmamap->ex_end;
	}
	error = extent_alloc_subregion1(iommu_dvmamap,
					ex_start, ex_end,
					sgsize, align, va & (align-1), boundary,
					(flags & BUS_DMA_NOWAIT) == 0
						? EX_WAITOK : EX_NOWAIT,
					(u_long *)&dva);
	splx(s);

	if (error != 0)
		return (error);

	cpuinfo.cache_flush(buf, buflen);

	/*
	 * We always use just one segment.
	 */
	map->dm_mapsize = buflen;
	map->dm_nsegs = 1;
	map->dm_segs[0].ds_addr = dva + voff;
	map->dm_segs[0].ds_len = buflen;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; sgsize != 0; ) {
		paddr_t pa;
		/*
		 * Get the physical address for this page.
		 */
		(void) pmap_extract(pmap, va, &pa);

		iommu_enter(dva, pa);

		dva += NBPG;
		va += NBPG;
		sgsize -= NBPG;
	}

	return (0);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
iommu_dmamap_load_mbuf(t, map, m, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
{

	panic("_bus_dmamap_load: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
iommu_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{

	panic("_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
iommu_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
iommu_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	bus_addr_t addr;
	bus_size_t len;
	int s, error;

	if (map->dm_nsegs != 1)
		panic("_bus_dmamap_unload: nsegs = %d", map->dm_nsegs);

	addr = map->dm_segs[0].ds_addr;
	len = map->dm_segs[0].ds_len;
	len = ((addr & PGOFSET) + len + PGOFSET) & ~PGOFSET;
	addr &= ~PGOFSET;

	iommu_remove(addr, len);
	s = splhigh();
	error = extent_free(iommu_dvmamap, addr, len, EX_NOWAIT);
	splx(s);
	if (error != 0)
		printf("warning: %ld of DVMA space lost\n", (long)len);

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
iommu_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/*
	 * XXX Should flush CPU write buffers.
	 */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
iommu_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	paddr_t pa;
	bus_addr_t dva;
	vm_page_t m;
	int s, error;
	u_long ex_start, ex_end;
	struct pglist *mlist;

	size = round_page(size);
	error = _bus_dmamem_alloc_common(t, size, alignment, boundary,
					 segs, nsegs, rsegs, flags);
	if (error != 0)
		return (error);

	s = splhigh();

	if ((flags & BUS_DMA_24BIT) != 0) {
		ex_start = D24_DVMA_BASE;
		ex_end = D24_DVMA_END;
	} else {
		ex_start = iommu_dvmamap->ex_start;
		ex_end = iommu_dvmamap->ex_end;
	}

	error = extent_alloc_subregion(iommu_dvmamap,
					ex_start, ex_end,
					size, alignment, boundary,
					(flags & BUS_DMA_NOWAIT) == 0
						? EX_WAITOK : EX_NOWAIT,
					(u_long *)&dva);
	splx(s);
	if (error != 0)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	segs[0].ds_addr = dva;
	segs[0].ds_len = size;
	*rsegs = 1;

	mlist = segs[0]._ds_mlist;
	/* Map memory into DVMA space */
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		pa = VM_PAGE_TO_PHYS(m);

		iommu_enter(dva, pa);
		dva += PAGE_SIZE;
	}

	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
iommu_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	bus_addr_t addr;
	bus_size_t len;
	int s, error;

	if (nsegs != 1)
		panic("bus_dmamem_free: nsegs = %d", nsegs);

	addr = segs[0].ds_addr;
	len = segs[0].ds_len;

	iommu_remove(addr, len);
	s = splhigh();
	error = extent_free(iommu_dvmamap, addr, len, EX_NOWAIT);
	splx(s);
	if (error != 0)
		printf("warning: %ld of DVMA space lost\n", (long)len);
	/*
	 * Return the list of pages back to the VM system.
	 */
	_bus_dmamem_free_common(t, segs, nsegs);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
iommu_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vm_page_t m;
	vaddr_t va, sva;
	bus_addr_t addr;
	struct pglist *mlist;
	int cbit;
	size_t oversize;
	u_long align;

	if (nsegs != 1)
		panic("iommu_dmamem_map: nsegs = %d", nsegs);

	cbit = has_iocache ? 0 : PMAP_NC;
	align = dvma_cachealign ? dvma_cachealign : PAGE_SIZE;

	size = round_page(size);

	/*
	 * Find a region of kernel virtual addresses that can accomodate
	 * our aligment requirements.
	 */
	oversize = size + align - PAGE_SIZE;
	sva = uvm_km_valloc(kernel_map, oversize);
	if (sva == 0)
		return (ENOMEM);

	/* Compute start of aligned region */
	va = sva;
	va += ((segs[0].ds_addr & (align - 1)) + align - va) & (align - 1);

	/* Return excess virtual addresses */
	if (va != sva)
		(void)uvm_unmap(kernel_map, sva, va);
	if (va + size != sva + oversize)
		(void)uvm_unmap(kernel_map, va + size, sva + oversize);


	*kvap = (caddr_t)va;
	mlist = segs[0]._ds_mlist;

	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {

		if (size == 0)
			panic("iommu_dmamem_map: size botch");

		addr = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), va, addr | cbit,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
#if 0
			if (flags & BUS_DMA_COHERENT)
				/* XXX */;
#endif
		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return (0);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
int
iommu_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs, off, prot, flags;
{

	panic("_bus_dmamem_mmap: not implemented");
}
