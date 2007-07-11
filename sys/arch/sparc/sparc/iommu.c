/*	$NetBSD: iommu.c,v 1.85.4.1 2007/07/11 20:02:26 mjf Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iommu.c,v 1.85.4.1 2007/07/11 20:02:26 mjf Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>

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
	int		sc_cachecoherent;
/*
 * Note: operations on the extent map are being protected with
 * splhigh(), since we cannot predict at which interrupt priority
 * our clients will run.
 */
	struct sparc_bus_dma_tag sc_dmatag;
	struct extent *sc_dvmamap;
};

/* autoconfiguration driver */
int	iommu_print(void *, const char *);
void	iommu_attach(struct device *, struct device *, void *);
int	iommu_match(struct device *, struct cfdata *, void *);

#if defined(SUN4M)
static void iommu_copy_prom_entries(struct iommu_softc *);
#endif

CFATTACH_DECL(iommu, sizeof(struct iommu_softc),
    iommu_match, iommu_attach, NULL, NULL);

/* IOMMU DMA map functions */
int	iommu_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
			bus_size_t, int, bus_dmamap_t *);
int	iommu_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
			bus_size_t, struct proc *, int);
int	iommu_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
			struct mbuf *, int);
int	iommu_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
			struct uio *, int);
int	iommu_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
			bus_dma_segment_t *, int, bus_size_t, int);
void	iommu_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	iommu_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
			bus_size_t, int);

int	iommu_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *,
			int, size_t, void **, int);
void	iommu_dmamem_unmap(bus_dma_tag_t, void *, size_t);
paddr_t	iommu_dmamem_mmap(bus_dma_tag_t, bus_dma_segment_t *,
			int, off_t, int, int);
int	iommu_dvma_alloc(struct iommu_softc *, bus_dmamap_t, vaddr_t,
			 bus_size_t, int, bus_addr_t *, bus_size_t *);

/*
 * Print the location of some iommu-attached device (called just
 * before attaching that device).  If `iommu' is not NULL, the
 * device was found but not configured; print the iommu as well.
 * Return UNCONF (config_find ignores this if the device was configured).
 */
int
iommu_print(void *args, const char *iommu)
{
	struct iommu_attach_args *ia = args;

	if (iommu)
		aprint_normal("%s at %s", ia->iom_name, iommu);
	return (UNCONF);
}

int
iommu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (CPU_ISSUN4 || CPU_ISSUN4C)
		return (0);
	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

/*
 * Attach the iommu.
 */
void
iommu_attach(struct device *parent, struct device *self, void *aux)
{
#if defined(SUN4M)
	struct iommu_softc *sc = (struct iommu_softc *)self;
	struct mainbus_attach_args *ma = aux;
	struct sparc_bus_dma_tag *dmat = &sc->sc_dmatag;
	bus_space_handle_t bh;
	int node;
	int js1_implicit_iommu;
	int i, s;
	u_int iopte_table_pa;
	struct pglist mlist;
	u_int size;
	struct vm_page *m;
	vaddr_t va;

	dmat->_cookie = sc;
	dmat->_dmamap_create = iommu_dmamap_create;
	dmat->_dmamap_destroy = _bus_dmamap_destroy;
	dmat->_dmamap_load = iommu_dmamap_load;
	dmat->_dmamap_load_mbuf = iommu_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = iommu_dmamap_load_uio;
	dmat->_dmamap_load_raw = iommu_dmamap_load_raw;
	dmat->_dmamap_unload = iommu_dmamap_unload;
	dmat->_dmamap_sync = iommu_dmamap_sync;

	dmat->_dmamem_alloc = _bus_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = iommu_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = iommu_dmamem_mmap;

	/*
	 * JS1/OF device tree does not have an iommu node and sbus
	 * node is directly under root.  mainbus_attach detects this
	 * and calls us with sbus node instead so that we can attach
	 * implicit iommu and attach that sbus node under it.
	 */
	node = ma->ma_node;
	if (strcmp(prom_getpropstring(node, "name"), "sbus") == 0)
		js1_implicit_iommu = 1;
	else
		js1_implicit_iommu = 0;

	/*
	 * Map registers into our space. The PROM may have done this
	 * already, but I feel better if we have our own copy. Plus, the
	 * prom doesn't map the entire register set.
	 *
	 * XXX struct iommureg is bigger than ra->ra_len; what are the
	 *     other fields for?
	 */
	if (bus_space_map(ma->ma_bustag, ma->ma_paddr,
			  sizeof(struct iommureg), 0, &bh) != 0) {
		printf("iommu_attach: cannot map registers\n");
		return;
	}
	sc->sc_reg = (struct iommureg *)bh;

	sc->sc_cachecoherent = js1_implicit_iommu ? 0
				: node_has_property(node, "cache-coherence?");
	if (CACHEINFO.c_enabled == 0) /* XXX - is this correct? */
		sc->sc_cachecoherent = 0;

	sc->sc_pagesize = js1_implicit_iommu ? PAGE_SIZE
				: prom_getpropint(node, "page-size", PAGE_SIZE),

	/*
	 * Allocate memory for I/O pagetables.
	 * This takes 64K of contiguous physical memory to map 64M of
	 * DVMA space (starting at IOMMU_DVMA_BASE).
	 * The table must be aligned on a (-IOMMU_DVMA_BASE/pagesize)
	 * boundary (i.e. 64K for 64M of DVMA space).
	 */

	size = ((0 - IOMMU_DVMA_BASE) / sc->sc_pagesize) * sizeof(iopte_t);
	if (uvm_pglistalloc(size, vm_first_phys, vm_first_phys+vm_num_phys,
			    size, 0, &mlist, 1, 0) != 0)
		panic("iommu_attach: no memory");

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY);
	if (va == 0)
		panic("iommu_attach: no memory");

	sc->sc_ptes = (iopte_t *)va;

	m = TAILQ_FIRST(&mlist);
	iopte_table_pa = VM_PAGE_TO_PHYS(m);

	/* Map the pages */
	for (; m != NULL; m = TAILQ_NEXT(m,pageq)) {
		paddr_t pa = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va, pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE);
		va += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	/*
	 * Copy entries from current IOMMU table.
	 * XXX - Why do we need to do this?
	 */
	iommu_copy_prom_entries(sc);

	/*
	 * Now we can install our new pagetable into the IOMMU
	 */
	sc->sc_range = 0 - IOMMU_DVMA_BASE;
	sc->sc_dvmabase = IOMMU_DVMA_BASE;

	/* calculate log2(sc->sc_range/16MB) */
	i = ffs(sc->sc_range/(1 << 24)) - 1;
	if ((1 << i) != (sc->sc_range/(1 << 24)))
		panic("iommu: bad range: %d", i);

	s = splhigh();
	IOMMU_FLUSHALL(sc);

	/* Load range and physical address of PTEs */
	sc->sc_reg->io_cr = (sc->sc_reg->io_cr & ~IOMMU_CTL_RANGE) |
			  (i << IOMMU_CTL_RANGESHFT) | IOMMU_CTL_ME;
	sc->sc_reg->io_bar = (iopte_table_pa >> 4) & IOMMU_BAR_IBA;

	IOMMU_FLUSHALL(sc);
	splx(s);

	printf(": version 0x%x/0x%x, page-size %d, range %dMB\n",
		(sc->sc_reg->io_cr & IOMMU_CTL_VER) >> 24,
		(sc->sc_reg->io_cr & IOMMU_CTL_IMPL) >> 28,
		sc->sc_pagesize,
		sc->sc_range >> 20);

	sc->sc_dvmamap = extent_create("iommudvma",
					IOMMU_DVMA_BASE, IOMMU_DVMA_END,
					M_DEVBUF, 0, 0, EX_NOWAIT);
	if (sc->sc_dvmamap == NULL)
		panic("iommu: unable to allocate DVMA map");

	/*
	 * If we are attaching implicit iommu on JS1/OF we do not have
	 * an iommu node to traverse, instead mainbus_attach passed us
	 * sbus node in ma.ma_node.  Attach it as the only iommu child.
	 */
	if (js1_implicit_iommu) {
		struct iommu_attach_args ia;
		struct openprom_addr sbus_iommu_reg = { 0, 0x10001000, 0x28 };

		bzero(&ia, sizeof ia);

		/* Propagate BUS & DMA tags */
		ia.iom_bustag = ma->ma_bustag;
		ia.iom_dmatag = &sc->sc_dmatag;

		ia.iom_name = "sbus";
		ia.iom_node = node;
		ia.iom_reg = &sbus_iommu_reg;
		ia.iom_nreg = 1;

		(void) config_found(&sc->sc_dev, (void *)&ia, iommu_print);
		return;
	}

	/*
	 * Loop through ROM children (expect Sbus among them).
	 */
	for (node = firstchild(node); node; node = nextsibling(node)) {
		struct iommu_attach_args ia;

		bzero(&ia, sizeof ia);
		ia.iom_name = prom_getpropstring(node, "name");

		/* Propagate BUS & DMA tags */
		ia.iom_bustag = ma->ma_bustag;
		ia.iom_dmatag = &sc->sc_dmatag;

		ia.iom_node = node;

		ia.iom_reg = NULL;
		prom_getprop(node, "reg", sizeof(struct openprom_addr),
			&ia.iom_nreg, &ia.iom_reg);

		(void) config_found(&sc->sc_dev, (void *)&ia, iommu_print);
		if (ia.iom_reg != NULL)
			free(ia.iom_reg, M_DEVBUF);
	}
#endif
}

#if defined(SUN4M)
static void
iommu_copy_prom_entries(struct iommu_softc *sc)
{
	u_int pbase, pa;
	u_int range;
	iopte_t *tpte_p;
	u_int pagesz = sc->sc_pagesize;
	int use_ac = (cpuinfo.cpu_impl == 4 && cpuinfo.mxcc);
	u_int mmupcr_save;

	/*
	 * We read in the original table using MMU bypass and copy all
	 * of its entries to the appropriate place in our new table,
	 * even if the sizes are different.
	 * This is pretty easy since we know DVMA ends at 0xffffffff.
	 */

	range = (1 << 24) <<
	    ((sc->sc_reg->io_cr & IOMMU_CTL_RANGE) >> IOMMU_CTL_RANGESHFT);

	pbase = (sc->sc_reg->io_bar & IOMMU_BAR_IBA) <<
			(14 - IOMMU_BAR_IBASHFT);

	if (use_ac) {
		/*
		 * Set MMU AC bit so we'll still read from the cache
		 * in by-pass mode.
		 */
		mmupcr_save = lda(SRMMU_PCR, ASI_SRMMU);
		sta(SRMMU_PCR, ASI_SRMMU, mmupcr_save | VIKING_PCR_AC);
	} else
		mmupcr_save = 0; /* XXX - avoid GCC `uninitialized' warning */

	/* Flush entire IOMMU TLB before messing with the in-memory tables */
	IOMMU_FLUSHALL(sc);

	/*
	 * tpte_p = top of our PTE table
	 * pa     = top of current PTE table
	 * Then work downwards and copy entries until we hit the bottom
	 * of either table.
	 */
	for (tpte_p = &sc->sc_ptes[((0 - IOMMU_DVMA_BASE)/pagesz) - 1],
	     pa = (u_int)pbase + (range/pagesz - 1)*sizeof(iopte_t);
	     tpte_p >= &sc->sc_ptes[0] && pa >= (u_int)pbase;
	     tpte_p--, pa -= sizeof(iopte_t)) {

		*tpte_p = lda(pa, ASI_BYPASS);
	}

	if (use_ac) {
		/* restore mmu after bug-avoidance */
		sta(SRMMU_PCR, ASI_SRMMU, mmupcr_save);
	}
}
#endif

static void
iommu_enter(struct iommu_softc *sc, bus_addr_t dva, paddr_t pa)
{
	int pte;

	/* This routine relies on the fact that sc->sc_pagesize == PAGE_SIZE */

#ifdef DIAGNOSTIC
	if (dva < sc->sc_dvmabase)
		panic("iommu_enter: dva 0x%lx not in DVMA space", (long)dva);
#endif

	pte = atop(pa) << IOPTE_PPNSHFT;
	pte &= IOPTE_PPN;
	pte |= IOPTE_V | IOPTE_W | (sc->sc_cachecoherent ? IOPTE_C : 0);
	sc->sc_ptes[atop(dva - sc->sc_dvmabase)] = pte;
	IOMMU_FLUSHPAGE(sc, dva);
}

/*
 * iommu_remove: removes mappings created by iommu_enter
 */
static void
iommu_remove(struct iommu_softc *sc, bus_addr_t dva, bus_size_t len)
{
	u_int pagesz = sc->sc_pagesize;
	bus_addr_t base = sc->sc_dvmabase;

#ifdef DEBUG
	if (dva < base)
		panic("iommu_remove: va 0x%lx not in DVMA space", (long)dva);
#endif

	while ((long)len > 0) {
#ifdef notyet
#ifdef DEBUG
		if ((sc->sc_ptes[atop(dva - base)] & IOPTE_V) == 0)
			panic("iommu_remove: clearing invalid pte at dva 0x%lx",
			      (long)dva);
#endif
#endif
		sc->sc_ptes[atop(dva - base)] = 0;
		IOMMU_FLUSHPAGE(sc, dva);
		len -= pagesz;
		dva += pagesz;
	}
}

#if 0	/* These registers aren't there??? */
void
iommu_error(void)
{
	struct iommu_softc *sc = X;
	struct iommureg *iop = sc->sc_reg;

	printf("iommu: afsr 0x%x, afar 0x%x\n", iop->io_afsr, iop->io_afar);
	printf("iommu: mfsr 0x%x, mfar 0x%x\n", iop->io_mfsr, iop->io_mfar);
}

int
iommu_alloc(u_int va, u_int len)
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
		len -= PAGE_SIZE;
		va += PAGE_SIZE;
		tva += PAGE_SIZE;
	}
	return iovaddr + off;
}
#endif


/*
 * IOMMU DMA map functions.
 */
int
iommu_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
		    bus_size_t maxsegsz, bus_size_t boundary, int flags,
		    bus_dmamap_t *dmamp)
{
	struct iommu_softc *sc = t->_cookie;
	bus_dmamap_t map;
	int error;

	if ((error = _bus_dmamap_create(t, size, nsegments, maxsegsz,
					boundary, flags, &map)) != 0)
		return (error);

	if ((flags & BUS_DMA_24BIT) != 0) {
		/* Limit this map to the range usable by `24-bit' devices */
		map->_dm_ex_start = D24_DVMA_BASE;
		map->_dm_ex_end = D24_DVMA_END;
	} else {
		/* Enable allocations from the entire map */
		map->_dm_ex_start = sc->sc_dvmamap->ex_start;
		map->_dm_ex_end = sc->sc_dvmamap->ex_end;
	}

	*dmamp = map;
	return (0);
}

/*
 * Internal routine to allocate space in the IOMMU map.
 */
int
iommu_dvma_alloc(struct iommu_softc *sc, bus_dmamap_t map,
		 vaddr_t va, bus_size_t len, int flags,
		 bus_addr_t *dvap, bus_size_t *sgsizep)
{
	bus_size_t sgsize;
	u_long align, voff, dvaddr;
	int s, error;
	int pagesz = PAGE_SIZE;

	/*
	 * Remember page offset, then truncate the buffer address to
	 * a page boundary.
	 */
	voff = va & (pagesz - 1);
	va &= -pagesz;

	if (len > map->_dm_size)
		return (EINVAL);

	sgsize = (len + voff + pagesz - 1) & -pagesz;
	align = dvma_cachealign ? dvma_cachealign : map->_dm_align;

	s = splhigh();
	error = extent_alloc_subregion1(sc->sc_dvmamap,
					map->_dm_ex_start, map->_dm_ex_end,
					sgsize, align, va & (align-1),
					map->_dm_boundary,
					(flags & BUS_DMA_NOWAIT) == 0
						? EX_WAITOK : EX_NOWAIT,
					&dvaddr);
	splx(s);
	*dvap = (bus_addr_t)dvaddr;
	*sgsizep = sgsize;
	return (error);
}

/*
 * Prepare buffer for DMA transfer.
 */
int
iommu_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map,
		  void *buf, bus_size_t buflen,
		  struct proc *p, int flags)
{
	struct iommu_softc *sc = t->_cookie;
	bus_size_t sgsize;
	bus_addr_t dva;
	vaddr_t va = (vaddr_t)buf;
	int pagesz = PAGE_SIZE;
	pmap_t pmap;
	int error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	/* Allocate IOMMU resources */
	if ((error = iommu_dvma_alloc(sc, map, va, buflen, flags,
					&dva, &sgsize)) != 0)
		return (error);

	if (sc->sc_cachecoherent == 0)
		cache_flush(buf, buflen); /* XXX - move to bus_dma_sync? */

	/*
	 * We always use just one segment.
	 */
	map->dm_mapsize = buflen;
	map->dm_nsegs = 1;
	map->dm_segs[0].ds_addr = dva + (va & (pagesz - 1));
	map->dm_segs[0].ds_len = buflen;
	map->dm_segs[0]._ds_sgsize = sgsize;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; sgsize != 0; ) {
		paddr_t pa;
		/*
		 * Get the physical address for this page.
		 */
		if (!pmap_extract(pmap, va, &pa)) {
			iommu_dmamap_unload(t, map);
			return (EFAULT);
		}

		iommu_enter(sc, dva, pa);

		dva += pagesz;
		va += pagesz;
		sgsize -= pagesz;
	}

	return (0);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
iommu_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map,
		       struct mbuf *m, int flags)
{

	panic("_bus_dmamap_load_mbuf: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
iommu_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map,
		      struct uio *uio, int flags)
{

	panic("_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
iommu_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
		      bus_dma_segment_t *segs, int nsegs, bus_size_t size,
		      int flags)
{
	struct iommu_softc *sc = t->_cookie;
	struct vm_page *m;
	paddr_t pa;
	bus_addr_t dva;
	bus_size_t sgsize;
	struct pglist *mlist;
	int pagesz = PAGE_SIZE;
	int error;

	map->dm_nsegs = 0;

	/* Allocate IOMMU resources */
	if ((error = iommu_dvma_alloc(sc, map, segs[0]._ds_va, size,
				      flags, &dva, &sgsize)) != 0)
		return (error);

	/*
	 * Note DVMA address in case bus_dmamem_map() is called later.
	 * It can then insure cache coherency by choosing a KVA that
	 * is aligned to `ds_addr'.
	 */
	segs[0].ds_addr = dva;
	segs[0].ds_len = size;

	map->dm_segs[0].ds_addr = dva;
	map->dm_segs[0].ds_len = size;
	map->dm_segs[0]._ds_sgsize = sgsize;

	/* Map physical pages into IOMMU */
	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		if (sgsize == 0)
			panic("iommu_dmamap_load_raw: size botch");
		pa = VM_PAGE_TO_PHYS(m);
		iommu_enter(sc, dva, pa);
		dva += pagesz;
		sgsize -= pagesz;
	}

	map->dm_nsegs = 1;
	map->dm_mapsize = size;

	return (0);
}

/*
 * Unload an IOMMU DMA map.
 */
void
iommu_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct iommu_softc *sc = t->_cookie;
	bus_dma_segment_t *segs = map->dm_segs;
	int nsegs = map->dm_nsegs;
	bus_addr_t dva;
	bus_size_t len;
	int i, s, error;

	for (i = 0; i < nsegs; i++) {
		dva = segs[i].ds_addr & -PAGE_SIZE;
		len = segs[i]._ds_sgsize;

		iommu_remove(sc, dva, len);
		s = splhigh();
		error = extent_free(sc->sc_dvmamap, dva, len, EX_NOWAIT);
		splx(s);
		if (error != 0)
			printf("warning: %ld of DVMA space lost\n", (long)len);
	}

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * DMA map synchronization.
 */
void
iommu_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map,
		  bus_addr_t offset, bus_size_t len, int ops)
{

	/*
	 * XXX Should flush CPU write buffers.
	 */
}

/*
 * Map DMA-safe memory.
 */
int
iommu_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
		 size_t size, void **kvap, int flags)
{
	struct iommu_softc *sc = t->_cookie;
	struct vm_page *m;
	vaddr_t va;
	bus_addr_t addr;
	struct pglist *mlist;
	int cbit;
	u_long align;
	int pagesz = PAGE_SIZE;

	if (nsegs != 1)
		panic("iommu_dmamem_map: nsegs = %d", nsegs);

	cbit = sc->sc_cachecoherent ? 0 : PMAP_NC;
	align = dvma_cachealign ? dvma_cachealign : pagesz;

	size = round_page(size);

	/*
	 * In case the segment has already been loaded by
	 * iommu_dmamap_load_raw(), find a region of kernel virtual
	 * addresses that can accommodate our aligment requirements.
	 */
	va = _bus_dma_valloc_skewed(size, 0, align,
				    segs[0].ds_addr & (align - 1));
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (void *)va;

	/*
	 * Map the pages allocated in _bus_dmamem_alloc() to the
	 * kernel virtual address space.
	 */
	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {

		if (size == 0)
			panic("iommu_dmamem_map: size botch");

		addr = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va, addr | cbit, VM_PROT_READ | VM_PROT_WRITE);
#if 0
			if (flags & BUS_DMA_COHERENT)
				/* XXX */;
#endif
		va += pagesz;
		size -= pagesz;
	}
	pmap_update(pmap_kernel());

	return (0);
}

void
iommu_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("iommu_dmamem_unmap");
#endif

	size = round_page(size);
	pmap_kremove((vaddr_t)kva, size);
	pmap_update(pmap_kernel());
	uvm_unmap(kernel_map, (vaddr_t)kva, (vaddr_t)kva + size);
}


/*
 * mmap(2)'ing DMA-safe memory.
 */
paddr_t
iommu_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
		  off_t off, int prot, int flags)
{

	panic("_bus_dmamem_mmap: not implemented");
}
