/*	$NetBSD: iommu.c,v 1.108.2.1 2015/09/22 12:05:52 skrll Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
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

/*
 * Copyright (c) 2001, 2002 Eduardo Horvath
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * UltraSPARC IOMMU support; used by both the sbus and pci code.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iommu.c,v 1.108.2.1 2015/09/22 12:05:52 skrll Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

#include <sys/bus.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/hypervisor.h>

#ifdef DEBUG
#define IDB_BUSDMA	0x1
#define IDB_IOMMU	0x2
#define IDB_INFO	0x4
#define	IDB_SYNC	0x8
int iommudebug = 0x0;
#define DPRINTF(l, s)   do { if (iommudebug & l) printf s; } while (0)
#define IOTTE_DEBUG(n)	(n)
#else
#define DPRINTF(l, s)
#define IOTTE_DEBUG(n)	0
#endif

#define iommu_strbuf_flush(i, v) do {					\
	if ((i)->sb_flush)						\
		bus_space_write_8((i)->sb_is->is_bustag, (i)->sb_sb,	\
			STRBUFREG(strbuf_pgflush), (v));		\
	} while (0)

static	int iommu_strbuf_flush_done(struct strbuf_ctl *);
static	void _iommu_dvmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
		bus_size_t, int);
static void iommu_enter_sun4u(struct strbuf_ctl *sb, vaddr_t va, int64_t pa, int flags);
static void iommu_enter_sun4v(struct strbuf_ctl *sb, vaddr_t va, int64_t pa, int flags);
static void iommu_remove_sun4u(struct iommu_state *is, vaddr_t va, size_t len);
static void iommu_remove_sun4v(struct iommu_state *is, vaddr_t va, size_t len);

/*
 * initialise the UltraSPARC IOMMU (SBUS or PCI):
 *	- allocate and setup the iotsb.
 *	- enable the IOMMU
 *	- initialise the streaming buffers (if they exist)
 *	- create a private DVMA map.
 */
void
iommu_init(char *name, struct iommu_state *is, int tsbsize, uint32_t iovabase)
{
	psize_t size;
	vaddr_t va;
	paddr_t pa;
	struct vm_page *pg;
	struct pglist pglist;

	DPRINTF(IDB_INFO, ("iommu_init: tsbsize %x iovabase %x\n", tsbsize, iovabase));
	
	/*
	 * Setup the iommu.
	 *
	 * The sun4u iommu is part of the SBUS or PCI controller so we will
	 * deal with it here..
	 *
	 * For sysio and psycho/psycho+ the IOMMU address space always ends at
	 * 0xffffe000, but the starting address depends on the size of the
	 * map.  The map size is 1024 * 2 ^ is->is_tsbsize entries, where each
	 * entry is 8 bytes.  The start of the map can be calculated by
	 * (0xffffe000 << (8 + is->is_tsbsize)).
	 *
	 * But sabre and hummingbird use a different scheme that seems to
	 * be hard-wired, so we read the start and size from the PROM and
	 * just use those values.
	 */
	if (strncmp(name, "pyro", 4) == 0) {
		is->is_cr = IOMMUREG_READ(is, iommu_cr);
		is->is_cr &= ~IOMMUCR_FIRE_BE;
		is->is_cr |= (IOMMUCR_FIRE_SE | IOMMUCR_FIRE_CM_EN |
		    IOMMUCR_FIRE_TE);
	} else 
		is->is_cr = IOMMUCR_EN;
	is->is_tsbsize = tsbsize;
	if (iovabase == -1) {
		is->is_dvmabase = IOTSB_VSTART(is->is_tsbsize);
		is->is_dvmaend = IOTSB_VEND - 1;
	} else {
		is->is_dvmabase = iovabase;
		is->is_dvmaend = iovabase + IOTSB_VSIZE(tsbsize) - 1;
	}

	/*
	 * Allocate memory for I/O pagetables.  They need to be physically
	 * contiguous.
	 */

	size = PAGE_SIZE << is->is_tsbsize;
	if (uvm_pglistalloc((psize_t)size, (paddr_t)0, (paddr_t)-1,
		(paddr_t)PAGE_SIZE, (paddr_t)0, &pglist, 1, 0) != 0)
		panic("iommu_init: no memory");

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY);
	if (va == 0)
		panic("iommu_init: no memory");
	is->is_tsb = (int64_t *)va;

	is->is_ptsb = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));

	/* Map the pages */
	TAILQ_FOREACH(pg, &pglist, pageq.queue) {
		pa = VM_PAGE_TO_PHYS(pg);
		pmap_kenter_pa(va, pa | PMAP_NVC,
		    VM_PROT_READ | VM_PROT_WRITE, 0);
		va += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	memset(is->is_tsb, 0, size);

#ifdef DEBUG
	if (iommudebug & IDB_INFO)
	{
		/* Probe the iommu */
		if (!CPU_ISSUN4V) {
			printf("iommu cr=%llx tsb=%llx\n",
			    (unsigned long long)bus_space_read_8(is->is_bustag,
				is->is_iommu,
				offsetof(struct iommureg, iommu_cr)),
			    (unsigned long long)bus_space_read_8(is->is_bustag,
				is->is_iommu,
				offsetof(struct iommureg, iommu_tsb)));
			printf("TSB base %p phys %llx\n", (void *)is->is_tsb,
			    (unsigned long long)is->is_ptsb);
			delay(1000000); /* 1 s */
		}
	}
#endif

	/*
	 * Now all the hardware's working we need to allocate a dvma map.
	 */
	aprint_debug("DVMA map: %x to %x\n",
		(unsigned int)is->is_dvmabase,
		(unsigned int)is->is_dvmaend);
	aprint_debug("IOTSB: %llx to %llx\n",
		(unsigned long long)is->is_ptsb,
		(unsigned long long)(is->is_ptsb + size - 1));
	is->is_dvmamap = extent_create(name,
	    is->is_dvmabase, is->is_dvmaend,
	    0, 0, EX_NOWAIT);
	if (!is->is_dvmamap)
		panic("iommu_init: extent_create() failed");
	  
	mutex_init(&is->is_lock, MUTEX_DEFAULT, IPL_HIGH);

	/*
	 * Set the TSB size.  The relevant bits were moved to the TSB
	 * base register in the PCIe host bridges.
	 */
	if (is->is_flags & IOMMU_TSBSIZE_IN_PTSB)
		is->is_ptsb |= is->is_tsbsize;
	else
		is->is_cr |= (is->is_tsbsize << 16);

	/*
	 * now actually start up the IOMMU
	 */
	iommu_reset(is);
}

/*
 * Streaming buffers don't exist on the UltraSPARC IIi; we should have
 * detected that already and disabled them.  If not, we will notice that
 * they aren't there when the STRBUF_EN bit does not remain.
 */
void
iommu_reset(struct iommu_state *is)
{
	int i;
	struct strbuf_ctl *sb;

	if (CPU_ISSUN4V)
		return;
	
	IOMMUREG_WRITE(is, iommu_tsb, is->is_ptsb);

	/* Enable IOMMU in diagnostic mode */
	IOMMUREG_WRITE(is, iommu_cr, is->is_cr|IOMMUCR_DE);

	for (i = 0; i < 2; i++) {
		if ((sb = is->is_sb[i])) {

			/* Enable diagnostics mode? */
			bus_space_write_8(is->is_bustag, is->is_sb[i]->sb_sb,
				STRBUFREG(strbuf_ctl), STRBUF_EN);

			membar_Lookaside();

			/* No streaming buffers? Disable them */
			if (bus_space_read_8(is->is_bustag,
				is->is_sb[i]->sb_sb,
				STRBUFREG(strbuf_ctl)) == 0) {
				is->is_sb[i]->sb_flush = NULL;
			} else {

				/*
				 * locate the pa of the flush buffer.
				 */
				if (pmap_extract(pmap_kernel(),
				     (vaddr_t)is->is_sb[i]->sb_flush,
				     &is->is_sb[i]->sb_flushpa) == FALSE)
					is->is_sb[i]->sb_flush = NULL;
			}
		}
	}

	if (is->is_flags & IOMMU_FLUSH_CACHE)
		IOMMUREG_WRITE(is, iommu_cache_invalidate, -1ULL);
}

/*
 * Here are the iommu control routines.
 */

void
iommu_enter(struct strbuf_ctl *sb, vaddr_t va, int64_t pa, int flags)
{
	DPRINTF(IDB_IOMMU, ("iommu_enter: va %lx pa %lx flags %x\n",
	    va, (long)pa, flags));
	if (!CPU_ISSUN4V)
		iommu_enter_sun4u(sb, va, pa, flags);
	else
		iommu_enter_sun4v(sb, va, pa, flags);
}


void
iommu_enter_sun4u(struct strbuf_ctl *sb, vaddr_t va, int64_t pa, int flags)
{
	struct iommu_state *is = sb->sb_is;
	int strbuf = (flags & BUS_DMA_STREAMING);
	int64_t tte;

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || va > is->is_dvmaend)
		panic("iommu_enter: va %#lx not in DVMA space", va);
#endif

	/* Is the streamcache flush really needed? */
	if (sb->sb_flush)
		iommu_strbuf_flush(sb, va);
	else
		/* If we can't flush the strbuf don't enable it. */
		strbuf = 0;

	tte = MAKEIOTTE(pa, !(flags & BUS_DMA_NOWRITE),
		!(flags & BUS_DMA_NOCACHE), (strbuf));
#ifdef DEBUG
	tte |= (flags & 0xff000LL)<<(4*8);
#endif

	is->is_tsb[IOTSBSLOT(va,is->is_tsbsize)] = tte;
	bus_space_write_8(is->is_bustag, is->is_iommu,
		IOMMUREG(iommu_flush), va);
	DPRINTF(IDB_IOMMU, ("iommu_enter: slot %d va %lx pa %lx "
		"TSB[%lx]@%p=%lx\n", (int)IOTSBSLOT(va,is->is_tsbsize),
		va, (long)pa, (u_long)IOTSBSLOT(va,is->is_tsbsize),
		(void *)(u_long)&is->is_tsb[IOTSBSLOT(va,is->is_tsbsize)],
		(u_long)tte));
}

void
iommu_enter_sun4v(struct strbuf_ctl *sb, vaddr_t va, int64_t pa, int flags)
{
	struct iommu_state *is = sb->sb_is;
	u_int64_t tsbid = IOTSBSLOT(va, is->is_tsbsize);
	paddr_t page_list[1], addr;
	u_int64_t attr, nmapped;
	int err;

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || (va + PAGE_MASK) > is->is_dvmaend)
		panic("viommu_enter: va %#lx not in DVMA space", va);
#endif

	attr = PCI_MAP_ATTR_READ | PCI_MAP_ATTR_WRITE;
	if (flags & BUS_DMA_READ)
		attr &= ~PCI_MAP_ATTR_READ;
	if (flags & BUS_DMA_WRITE)
		attr &= ~PCI_MAP_ATTR_WRITE;

	page_list[0] = trunc_page(pa);
	if (!pmap_extract(pmap_kernel(), (vaddr_t)page_list, &addr))
		panic("viommu_enter: pmap_extract failed");
	err = hv_pci_iommu_map(is->is_devhandle, tsbid, 1, attr,
	    addr, &nmapped);
	if (err != H_EOK || nmapped != 1)
		panic("hv_pci_iommu_map: err=%d, nmapped=%lu", err, (long unsigned int)nmapped);
}

/*
 * Find the value of a DVMA address (debug routine).
 */
paddr_t
iommu_extract(struct iommu_state *is, vaddr_t dva)
{
	int64_t tte = 0;

	if (dva >= is->is_dvmabase && dva <= is->is_dvmaend)
		tte = is->is_tsb[IOTSBSLOT(dva, is->is_tsbsize)];

	if ((tte & IOTTE_V) == 0)
		return ((paddr_t)-1L);
	return (tte & IOTTE_PAMASK);
}

/*
 * iommu_remove: removes mappings created by iommu_enter
 *
 * Only demap from IOMMU if flag is set.
 *
 * XXX: this function needs better internal error checking.
 */


void
iommu_remove(struct iommu_state *is, vaddr_t va, size_t len)
{
	DPRINTF(IDB_IOMMU, ("iommu_remove: va %lx len %zu\n", va, len));
	if (!CPU_ISSUN4V)
		iommu_remove_sun4u(is, va, len);
	else
		iommu_remove_sun4v(is, va, len);
}

void
iommu_remove_sun4u(struct iommu_state *is, vaddr_t va, size_t len)
{

	int slot;

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || va > is->is_dvmaend)
		panic("iommu_remove: va 0x%lx not in DVMA space", (u_long)va);
	if ((long)(va + len) < (long)va)
		panic("iommu_remove: va 0x%lx + len 0x%lx wraps",
		      (long) va, (long) len);
	if (len & ~0xfffffff)
		panic("iommu_remove: ridiculous len 0x%lx", (u_long)len);
#endif

	va = trunc_page(va);
	DPRINTF(IDB_IOMMU, ("iommu_remove: va %lx TSB[%lx]@%p\n",
		va, (u_long)IOTSBSLOT(va, is->is_tsbsize),
		&is->is_tsb[IOTSBSLOT(va, is->is_tsbsize)]));
	while (len > 0) {
		DPRINTF(IDB_IOMMU, ("iommu_remove: clearing TSB slot %d "
			"for va %p size %lx\n",
			(int)IOTSBSLOT(va,is->is_tsbsize), (void *)(u_long)va,
			(u_long)len));
		if (len <= PAGE_SIZE)
			len = 0;
		else
			len -= PAGE_SIZE;

#if 0
		/*
		 * XXX Zero-ing the entry would not require RMW
		 *
		 * Disabling valid bit while a page is used by a device
		 * causes an uncorrectable DMA error.
		 * Workaround to avoid an uncorrectable DMA error is
		 * eliminating the next line, but the page is mapped
		 * until the next iommu_enter call.
		 */
		is->is_tsb[IOTSBSLOT(va,is->is_tsbsize)] &= ~IOTTE_V;
		membar_StoreStore();
#endif
		IOMMUREG_WRITE(is, iommu_flush, va);

		/* Flush cache if necessary. */
		slot = IOTSBSLOT(trunc_page(va), is->is_tsbsize);
		if ((is->is_flags & IOMMU_FLUSH_CACHE) &&
		    (len == 0 || (slot % 8) == 7))
			IOMMUREG_WRITE(is, iommu_cache_flush,
			    is->is_ptsb + slot * 8);

		va += PAGE_SIZE;
	}
}

void
iommu_remove_sun4v(struct iommu_state *is, vaddr_t va, size_t len)
{
	u_int64_t tsbid = IOTSBSLOT(va, is->is_tsbsize);
	u_int64_t ndemapped;
	int err;

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || (va + PAGE_MASK) > is->is_dvmaend)
		panic("iommu_remove: va 0x%lx not in DVMA space", (u_long)va);
	if (va != trunc_page(va)) {
		printf("iommu_remove: unaligned va: %lx\n", va);
		va = trunc_page(va);
	}
#endif

	err = hv_pci_iommu_demap(is->is_devhandle, tsbid, 1, &ndemapped);
	if (err != H_EOK || ndemapped != 1)
		panic("hv_pci_iommu_unmap: err=%d", err);
}

static int
iommu_strbuf_flush_done(struct strbuf_ctl *sb)
{
	struct iommu_state *is = sb->sb_is;
	struct timeval cur, flushtimeout;

#define BUMPTIME(t, usec) { \
	register volatile struct timeval *tp = (t); \
	register long us; \
 \
	tp->tv_usec = us = tp->tv_usec + (usec); \
	if (us >= 1000000) { \
		tp->tv_usec = us - 1000000; \
		tp->tv_sec++; \
	} \
}

	if (!sb->sb_flush)
		return (0);

	/*
	 * Streaming buffer flushes:
	 *
	 *   1 Tell strbuf to flush by storing va to strbuf_pgflush.  If
	 *     we're not on a cache line boundary (64-bits):
	 *   2 Store 0 in flag
	 *   3 Store pointer to flag in flushsync
	 *   4 wait till flushsync becomes 0x1
	 *
	 * If it takes more than .5 sec, something
	 * went wrong.
	 */

	*sb->sb_flush = 0;
	bus_space_write_8(is->is_bustag, sb->sb_sb,
		STRBUFREG(strbuf_flushsync), sb->sb_flushpa);

	microtime(&flushtimeout);
	cur = flushtimeout;
	BUMPTIME(&flushtimeout, 500000); /* 1/2 sec */

	DPRINTF(IDB_IOMMU, ("%s: flush = %lx at va = %lx pa = %lx now="
		"%"PRIx64":%"PRIx32" until = %"PRIx64":%"PRIx32"\n", __func__,
		(long)*sb->sb_flush, (long)sb->sb_flush, (long)sb->sb_flushpa,
		cur.tv_sec, cur.tv_usec,
		flushtimeout.tv_sec, flushtimeout.tv_usec));

	/* Bypass non-coherent D$ */
	while ((!ldxa(sb->sb_flushpa, ASI_PHYS_CACHED)) &&
	       timercmp(&cur, &flushtimeout, <=))
		microtime(&cur);

#ifdef DIAGNOSTIC
	if (!ldxa(sb->sb_flushpa, ASI_PHYS_CACHED)) {
		printf("%s: flush timeout %p, at %p\n", __func__,
			(void *)(u_long)*sb->sb_flush,
			(void *)(u_long)sb->sb_flushpa); /* panic? */
#ifdef DDB
		Debugger();
#endif
	}
#endif
	DPRINTF(IDB_IOMMU, ("%s: flushed\n", __func__));
	return (*sb->sb_flush);
}

/*
 * IOMMU DVMA operations, common to SBUS and PCI.
 */
int
iommu_dvmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
	bus_size_t buflen, struct proc *p, int flags)
{
	struct strbuf_ctl *sb = (struct strbuf_ctl *)map->_dm_cookie;
	struct iommu_state *is = sb->sb_is;
	int err, needsflush;
	bus_size_t sgsize;
	paddr_t curaddr;
	u_long dvmaddr, sgstart, sgend, bmask;
	bus_size_t align, boundary, len;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;
	struct pmap *pmap;
	int slot;

	if (map->dm_nsegs) {
		/* Already in use?? */
#ifdef DIAGNOSTIC
		printf("iommu_dvmamap_load: map still in use\n");
#endif
		bus_dmamap_unload(t, map);
	}

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (buflen > map->_dm_size) {
		DPRINTF(IDB_BUSDMA,
		    ("iommu_dvmamap_load(): error %d > %d -- "
		     "map size exceeded!\n", (int)buflen, (int)map->_dm_size));
		return (EINVAL);
	}

	sgsize = round_page(buflen + ((int)vaddr & PGOFSET));

	/*
	 * A boundary presented to bus_dmamem_alloc() takes precedence
	 * over boundary in the map.
	 */
	if ((boundary = (map->dm_segs[0]._ds_boundary)) == 0)
		boundary = map->_dm_boundary;
	align = max(map->dm_segs[0]._ds_align, PAGE_SIZE);

	/*
	 * If our segment size is larger than the boundary we need to
	 * split the transfer up int little pieces ourselves.
	 */
	KASSERT(is->is_dvmamap);
	mutex_enter(&is->is_lock);
	err = extent_alloc(is->is_dvmamap, sgsize, align,
	    (sgsize > boundary) ? 0 : boundary,
	    EX_NOWAIT|EX_BOUNDZERO, &dvmaddr);
	mutex_exit(&is->is_lock);

#ifdef DEBUG
	if (err || (dvmaddr == (u_long)-1)) {
		printf("iommu_dvmamap_load(): extent_alloc(%d, %x) failed!\n",
		    (int)sgsize, flags);
#ifdef DDB
		Debugger();
#endif
	}
#endif
	if (err != 0)
		return (err);

	if (dvmaddr == (u_long)-1)
		return (ENOMEM);

	/* Set the active DVMA map */
	map->_dm_dvmastart = dvmaddr;
	map->_dm_dvmasize = sgsize;

	/*
	 * Now split the DVMA range into segments, not crossing
	 * the boundary.
	 */
	seg = 0;
	sgstart = dvmaddr + (vaddr & PGOFSET);
	sgend = sgstart + buflen - 1;
	map->dm_segs[seg].ds_addr = sgstart;
	DPRINTF(IDB_INFO, ("iommu_dvmamap_load: boundary %lx boundary - 1 %lx "
	    "~(boundary - 1) %lx\n", (long)boundary, (long)(boundary - 1),
	    (long)~(boundary - 1)));
	bmask = ~(boundary - 1);
	while ((sgstart & bmask) != (sgend & bmask) ||
	       sgend - sgstart + 1 > map->dm_maxsegsz) {
		/* Oops. We crossed a boundary or large seg. Split the xfer. */
		len = map->dm_maxsegsz;
		if ((sgstart & bmask) != (sgend & bmask))
			len = min(len, boundary - (sgstart & (boundary - 1)));
		map->dm_segs[seg].ds_len = len;
		DPRINTF(IDB_INFO, ("iommu_dvmamap_load: "
		    "seg %d start %lx size %lx\n", seg,
		    (long)map->dm_segs[seg].ds_addr,
		    (long)map->dm_segs[seg].ds_len));
		if (++seg >= map->_dm_segcnt) {
			/* Too many segments.  Fail the operation. */
			DPRINTF(IDB_INFO, ("iommu_dvmamap_load: "
			    "too many segments %d\n", seg));
			mutex_enter(&is->is_lock);
			err = extent_free(is->is_dvmamap,
			    dvmaddr, sgsize, EX_NOWAIT);
			map->_dm_dvmastart = 0;
			map->_dm_dvmasize = 0;
			mutex_exit(&is->is_lock);
			if (err != 0)
				printf("warning: %s: %" PRId64
				    " of DVMA space lost\n", __func__, sgsize);
			return (EFBIG);
		}
		sgstart += len;
		map->dm_segs[seg].ds_addr = sgstart;
	}
	map->dm_segs[seg].ds_len = sgend - sgstart + 1;
	DPRINTF(IDB_INFO, ("iommu_dvmamap_load: "
	    "seg %d start %lx size %lx\n", seg,
	    (long)map->dm_segs[seg].ds_addr, (long)map->dm_segs[seg].ds_len));
	map->dm_nsegs = seg + 1;
	map->dm_mapsize = buflen;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	needsflush = 0;
	for (; buflen > 0; ) {

		/*
		 * Get the physical address for this page.
		 */
		if (pmap_extract(pmap, (vaddr_t)vaddr, &curaddr) == FALSE) {
#ifdef DIAGNOSTIC
			printf("iommu_dvmamap_load: pmap_extract failed %lx\n", vaddr);
#endif
			bus_dmamap_unload(t, map);
			return (-1);
		}

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		DPRINTF(IDB_BUSDMA,
		    ("iommu_dvmamap_load: map %p loading va %p "
		    "dva %lx at pa %lx\n",
		    map, (void *)vaddr, (long)dvmaddr,
		    (long)trunc_page(curaddr)));
		iommu_enter(sb, trunc_page(dvmaddr), trunc_page(curaddr),
		    flags | IOTTE_DEBUG(0x4000));
		needsflush = 1;

		vaddr += sgsize;
		buflen -= sgsize;

		/* Flush cache if necessary. */
		slot = IOTSBSLOT(trunc_page(dvmaddr), is->is_tsbsize);
		if ((is->is_flags & IOMMU_FLUSH_CACHE) &&
		    (buflen <= 0 || (slot % 8) == 7))
			IOMMUREG_WRITE(is, iommu_cache_flush,
			    is->is_ptsb + slot * 8);

		dvmaddr += PAGE_SIZE;
	}
	if (needsflush)
		iommu_strbuf_flush_done(sb);
#ifdef DIAGNOSTIC
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		if (map->dm_segs[seg].ds_addr < is->is_dvmabase ||
			map->dm_segs[seg].ds_addr > is->is_dvmaend) {
			printf("seg %d dvmaddr %lx out of range %x - %x\n",
			    seg, (long)map->dm_segs[seg].ds_addr,
			    is->is_dvmabase, is->is_dvmaend);
#ifdef DDB
			Debugger();
#endif
		}
	}
#endif
	return (0);
}


void
iommu_dvmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct strbuf_ctl *sb = (struct strbuf_ctl *)map->_dm_cookie;
	struct iommu_state *is = sb->sb_is;
	int error;
	bus_size_t sgsize = map->_dm_dvmasize;

	/* Flush the iommu */
#ifdef DEBUG
	if (!map->_dm_dvmastart) {
		printf("iommu_dvmamap_unload: No dvmastart is zero\n");
#ifdef DDB
		Debugger();
#endif
	}
#endif
	iommu_remove(is, map->_dm_dvmastart, map->_dm_dvmasize);

	/* Flush the caches */
	bus_dmamap_unload(t->_parent, map);

	mutex_enter(&is->is_lock);
	error = extent_free(is->is_dvmamap, map->_dm_dvmastart,
		map->_dm_dvmasize, EX_NOWAIT);
	map->_dm_dvmastart = 0;
	map->_dm_dvmasize = 0;
	mutex_exit(&is->is_lock);
	if (error != 0)
		printf("warning: %s: %" PRId64 " of DVMA space lost\n",
		    __func__, sgsize);

	/* Clear the map */
}


int
iommu_dvmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
	bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	struct strbuf_ctl *sb = (struct strbuf_ctl *)map->_dm_cookie;
	struct iommu_state *is = sb->sb_is;
	struct vm_page *pg;
	int i, j;
	int left;
	int err, needsflush;
	bus_size_t sgsize;
	paddr_t pa;
	bus_size_t boundary, align;
	u_long dvmaddr, sgstart, sgend, bmask;
	struct pglist *pglist;
	const int pagesz = PAGE_SIZE;
	int slot;
#ifdef DEBUG
	int npg = 0;
#endif

	if (map->dm_nsegs) {
		/* Already in use?? */
#ifdef DIAGNOSTIC
		printf("iommu_dvmamap_load_raw: map still in use\n");
#endif
		bus_dmamap_unload(t, map);
	}

	/*
	 * A boundary presented to bus_dmamem_alloc() takes precedence
	 * over boundary in the map.
	 */
	if ((boundary = segs[0]._ds_boundary) == 0)
		boundary = map->_dm_boundary;

	align = max(segs[0]._ds_align, pagesz);

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	/* Count up the total number of pages we need */
	pa = trunc_page(segs[0].ds_addr);
	sgsize = 0;
	left = size;
	for (i = 0; left > 0 && i < nsegs; i++) {
		if (round_page(pa) != round_page(segs[i].ds_addr))
			sgsize = round_page(sgsize) +
			    (segs[i].ds_addr & PGOFSET);
		sgsize += min(left, segs[i].ds_len);
		left -= segs[i].ds_len;
		pa = segs[i].ds_addr + segs[i].ds_len;
	}
	sgsize = round_page(sgsize);

	mutex_enter(&is->is_lock);
	/*
	 * If our segment size is larger than the boundary we need to
	 * split the transfer up into little pieces ourselves.
	 */
	err = extent_alloc(is->is_dvmamap, sgsize, align,
		(sgsize > boundary) ? 0 : boundary,
		((flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT) |
		EX_BOUNDZERO, &dvmaddr);
	mutex_exit(&is->is_lock);

	if (err != 0)
		return (err);

#ifdef DEBUG
	if (dvmaddr == (u_long)-1)
	{
		printf("iommu_dvmamap_load_raw(): extent_alloc(%d, %x) failed!\n",
		    (int)sgsize, flags);
#ifdef DDB
		Debugger();
#endif
	}
#endif
	if (dvmaddr == (u_long)-1)
		return (ENOMEM);

	/* Set the active DVMA map */
	map->_dm_dvmastart = dvmaddr;
	map->_dm_dvmasize = sgsize;

	bmask = ~(boundary - 1);
	if ((pglist = segs[0]._ds_mlist) == NULL) {
		u_long prev_va = 0UL, last_va = dvmaddr;
		paddr_t prev_pa = 0;
		int end = 0, offset;
		bus_size_t len = size;

		/*
		 * This segs is made up of individual physical
		 *  segments, probably by _bus_dmamap_load_uio() or
		 * _bus_dmamap_load_mbuf().  Ignore the mlist and
		 * load each one individually.
		 */
		j = 0;
		needsflush = 0;
		for (i = 0; i < nsegs ; i++) {

			pa = segs[i].ds_addr;
			offset = (pa & PGOFSET);
			pa = trunc_page(pa);
			dvmaddr = trunc_page(dvmaddr);
			left = min(len, segs[i].ds_len);

			DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: converting "
				"physseg %d start %lx size %lx\n", i,
				(long)segs[i].ds_addr, (long)segs[i].ds_len));

			if ((pa == prev_pa) &&
				((offset != 0) || (end != offset))) {
				/* We can re-use this mapping */
				dvmaddr = prev_va;
			}

			sgstart = dvmaddr + offset;
			sgend = sgstart + left - 1;

			/* Are the segments virtually adjacent? */
			if ((j > 0) && (end == offset) &&
			    ((offset == 0) || (pa == prev_pa)) &&
			    (map->dm_segs[j-1].ds_len + left <=
			     map->dm_maxsegsz)) {
				/* Just append to the previous segment. */
				map->dm_segs[--j].ds_len += left;
				/* Restore sgstart for boundary check */
				sgstart = map->dm_segs[j].ds_addr;
				DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
					"appending seg %d start %lx size %lx\n", j,
					(long)map->dm_segs[j].ds_addr,
					(long)map->dm_segs[j].ds_len));
			} else {
				if (j >= map->_dm_segcnt) {
					iommu_remove(is, map->_dm_dvmastart,
					    last_va - map->_dm_dvmastart);
					goto fail;
				}
				map->dm_segs[j].ds_addr = sgstart;
				map->dm_segs[j].ds_len = left;
				DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
					"seg %d start %lx size %lx\n", j,
					(long)map->dm_segs[j].ds_addr,
					(long)map->dm_segs[j].ds_len));
			}
			end = (offset + left) & PGOFSET;

			/* Check for boundary issues */
			while ((sgstart & bmask) != (sgend & bmask)) {
				/* Need a new segment. */
				map->dm_segs[j].ds_len =
					boundary - (sgstart & (boundary - 1));
				DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
					"seg %d start %lx size %lx\n", j,
					(long)map->dm_segs[j].ds_addr,
					(long)map->dm_segs[j].ds_len));
				if (++j >= map->_dm_segcnt) {
					iommu_remove(is, map->_dm_dvmastart,
					    last_va - map->_dm_dvmastart);
					goto fail;
				}
				sgstart += map->dm_segs[j-1].ds_len;
				map->dm_segs[j].ds_addr = sgstart;
				map->dm_segs[j].ds_len = sgend - sgstart + 1;
			}

			if (sgsize == 0)
				panic("iommu_dmamap_load_raw: size botch");

			/* Now map a series of pages. */
			while (dvmaddr <= sgend) {
				DPRINTF(IDB_BUSDMA,
					("iommu_dvmamap_load_raw: map %p "
						"loading va %lx at pa %lx\n",
						map, (long)dvmaddr,
						(long)(pa)));
				/* Enter it if we haven't before. */
				if (prev_va != dvmaddr) {
					iommu_enter(sb, prev_va = dvmaddr,
					    prev_pa = pa,
					    flags | IOTTE_DEBUG(++npg << 12));
					needsflush = 1;

					/* Flush cache if necessary. */
					slot = IOTSBSLOT(trunc_page(dvmaddr), is->is_tsbsize);
					if ((is->is_flags & IOMMU_FLUSH_CACHE) &&
					    ((dvmaddr + pagesz) > sgend || (slot % 8) == 7))
						IOMMUREG_WRITE(is, iommu_cache_flush,
						    is->is_ptsb + slot * 8);
				}

				dvmaddr += pagesz;
				pa += pagesz;
				last_va = dvmaddr;
			}

			len -= left;
			++j;
		}
		if (needsflush)
			iommu_strbuf_flush_done(sb);

		map->dm_mapsize = size;
		map->dm_nsegs = j;
#ifdef DIAGNOSTIC
		{ int seg;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		if (map->dm_segs[seg].ds_addr < is->is_dvmabase ||
		    map->dm_segs[seg].ds_addr > is->is_dvmaend) {
			printf("seg %d dvmaddr %lx out of range %x - %x\n",
				seg, (long)map->dm_segs[seg].ds_addr,
				is->is_dvmabase, is->is_dvmaend);
#ifdef DDB
			Debugger();
#endif
		}
	}
		}
#endif
		return (0);
	}

	/*
	 * This was allocated with bus_dmamem_alloc.
	 * The pages are on a `pglist'.
	 */
	i = 0;
	sgstart = dvmaddr;
	sgend = sgstart + size - 1;
	map->dm_segs[i].ds_addr = sgstart;
	while ((sgstart & bmask) != (sgend & bmask)) {
		/* Oops.  We crossed a boundary.  Split the xfer. */
		map->dm_segs[i].ds_len = boundary - (sgstart & (boundary - 1));
		DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
			"seg %d start %lx size %lx\n", i,
			(long)map->dm_segs[i].ds_addr,
			(long)map->dm_segs[i].ds_len));
		if (++i >= map->_dm_segcnt) {
			/* Too many segments.  Fail the operation. */
			goto fail;
		}
		sgstart += map->dm_segs[i-1].ds_len;
		map->dm_segs[i].ds_addr = sgstart;
	}
	DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
			"seg %d start %lx size %lx\n", i,
			(long)map->dm_segs[i].ds_addr, (long)map->dm_segs[i].ds_len));
	map->dm_segs[i].ds_len = sgend - sgstart + 1;

	needsflush = 0;
	TAILQ_FOREACH(pg, pglist, pageq.queue) {
		if (sgsize == 0)
			panic("iommu_dmamap_load_raw: size botch");
		pa = VM_PAGE_TO_PHYS(pg);

		DPRINTF(IDB_BUSDMA,
		    ("iommu_dvmamap_load_raw: map %p loading va %lx at pa %lx\n",
		    map, (long)dvmaddr, (long)(pa)));
		iommu_enter(sb, dvmaddr, pa, flags | IOTTE_DEBUG(0x8000));
		needsflush = 1;

		sgsize -= pagesz;

		/* Flush cache if necessary. */
		slot = IOTSBSLOT(trunc_page(dvmaddr), is->is_tsbsize);
		if ((is->is_flags & IOMMU_FLUSH_CACHE) &&
		    (sgsize == 0 || (slot % 8) == 7))
			IOMMUREG_WRITE(is, iommu_cache_flush,
			    is->is_ptsb + slot * 8);

		dvmaddr += pagesz;
	}
	if (needsflush)
		iommu_strbuf_flush_done(sb);
	map->dm_mapsize = size;
	map->dm_nsegs = i+1;
#ifdef DIAGNOSTIC
	{ int seg;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		if (map->dm_segs[seg].ds_addr < is->is_dvmabase ||
			map->dm_segs[seg].ds_addr > is->is_dvmaend) {
			printf("seg %d dvmaddr %lx out of range %x - %x\n",
				seg, (long)map->dm_segs[seg].ds_addr,
				is->is_dvmabase, is->is_dvmaend);
#ifdef DDB
			Debugger();
#endif
		}
	}
	}
#endif
	return (0);

fail:
	mutex_enter(&is->is_lock);
	err = extent_free(is->is_dvmamap, map->_dm_dvmastart, sgsize,
	    EX_NOWAIT);
	map->_dm_dvmastart = 0;
	map->_dm_dvmasize = 0;
	mutex_exit(&is->is_lock);
	if (err != 0)
		printf("warning: %s: %" PRId64 " of DVMA space lost\n",
		    __func__, sgsize);
	return (EFBIG);
}


/*
 * Flush an individual dma segment, returns non-zero if the streaming buffers
 * need flushing afterwards.
 */
static int
iommu_dvmamap_sync_range(struct strbuf_ctl *sb, vaddr_t va, bus_size_t len)
{
	vaddr_t vaend;
	struct iommu_state *is = sb->sb_is;

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || va > is->is_dvmaend)
		panic("invalid va: %llx", (long long)va);
#endif

	if ((is->is_tsb[IOTSBSLOT(va, is->is_tsbsize)] & IOTTE_STREAM) == 0) {
		DPRINTF(IDB_SYNC, 
			("iommu_dvmamap_sync_range: attempting to flush "
			 "non-streaming entry\n"));
		return (0);
	}

	vaend = round_page(va + len) - 1;
	va = trunc_page(va);

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || vaend > is->is_dvmaend)
		panic("invalid va range: %llx to %llx (%x to %x)",
		    (long long)va, (long long)vaend,
		    is->is_dvmabase,
		    is->is_dvmaend);
#endif

	for ( ; va <= vaend; va += PAGE_SIZE) {
		DPRINTF(IDB_SYNC,
		    ("iommu_dvmamap_sync_range: flushing va %p\n",
		    (void *)(u_long)va));
		iommu_strbuf_flush(sb, va);
	}

	return (1);
}

static void
_iommu_dvmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
	bus_size_t len, int ops)
{
	struct strbuf_ctl *sb = (struct strbuf_ctl *)map->_dm_cookie;
	bus_size_t count;
	int i, needsflush = 0;

	if (!sb->sb_flush)
		return;

	for (i = 0; i < map->dm_nsegs; i++) {
		if (offset < map->dm_segs[i].ds_len)
			break;
		offset -= map->dm_segs[i].ds_len;
	}

	if (i == map->dm_nsegs)
		panic("%s: segment too short %llu", __func__,
		    (unsigned long long)offset);

	if (ops & (BUS_DMASYNC_PREREAD | BUS_DMASYNC_POSTWRITE)) {
		/* Nothing to do */;
	}

	if (ops & (BUS_DMASYNC_POSTREAD | BUS_DMASYNC_PREWRITE)) {

		for (; len > 0 && i < map->dm_nsegs; i++) {
			count = MIN(map->dm_segs[i].ds_len - offset, len);
			if (count > 0 && 
			    iommu_dvmamap_sync_range(sb,
				map->dm_segs[i].ds_addr + offset, count))
				needsflush = 1;
			offset = 0;
			len -= count;
		}
#ifdef DIAGNOSTIC
		if (i == map->dm_nsegs && len > 0)
			panic("%s: leftover %llu", __func__,
			    (unsigned long long)len);
#endif

		if (needsflush)
			iommu_strbuf_flush_done(sb);
	}
}

void
iommu_dvmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
	bus_size_t len, int ops)
{

	/* If len is 0, then there is nothing to do */
	if (len == 0)
		return;

	if (ops & (BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE)) {
		/* Flush the CPU then the IOMMU */
		bus_dmamap_sync(t->_parent, map, offset, len, ops);
		_iommu_dvmamap_sync(t, map, offset, len, ops);
	}
	if (ops & (BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE)) {
		/* Flush the IOMMU then the CPU */
		_iommu_dvmamap_sync(t, map, offset, len, ops);
		bus_dmamap_sync(t->_parent, map, offset, len, ops);
	}
}

int
iommu_dvmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
	bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
	int flags)
{

	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_alloc: sz %llx align %llx bound %llx "
	   "segp %p flags %d\n", (unsigned long long)size,
	   (unsigned long long)alignment, (unsigned long long)boundary,
	   segs, flags));
	return (bus_dmamem_alloc(t->_parent, size, alignment, boundary,
	    segs, nsegs, rsegs, flags|BUS_DMA_DVMA));
}

void
iommu_dvmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{

	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_free: segp %p nsegs %d\n",
	    segs, nsegs));
	bus_dmamem_free(t->_parent, segs, nsegs);
}

/*
 * Map the DVMA mappings into the kernel pmap.
 * Check the flags to see whether we're streaming or coherent.
 */
int
iommu_dvmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
	size_t size, void **kvap, int flags)
{
	struct vm_page *pg;
	vaddr_t va;
	bus_addr_t addr;
	struct pglist *pglist;
	int cbit;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;

	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_map: segp %p nsegs %d size %lx\n",
	    segs, nsegs, size));

	/*
	 * Allocate some space in the kernel map, and then map these pages
	 * into this space.
	 */
	size = round_page(size);
	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);
	if (va == 0)
		return (ENOMEM);

	*kvap = (void *)va;

	/*
	 * digest flags:
	 */
	cbit = 0;
	if (flags & BUS_DMA_COHERENT)	/* Disable vcache */
		cbit |= PMAP_NVC;
	if (flags & BUS_DMA_NOCACHE)	/* side effects */
		cbit |= PMAP_NC;

	/*
	 * Now take this and map it into the CPU.
	 */
	pglist = segs[0]._ds_mlist;
	TAILQ_FOREACH(pg, pglist, pageq.queue) {
#ifdef DIAGNOSTIC
		if (size == 0)
			panic("iommu_dvmamem_map: size botch");
#endif
		addr = VM_PAGE_TO_PHYS(pg);
		DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_map: "
		    "mapping va %lx at %llx\n", va, (unsigned long long)addr | cbit));
		pmap_kenter_pa(va, addr | cbit,
		    VM_PROT_READ | VM_PROT_WRITE, 0);
		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	return (0);
}

/*
 * Unmap DVMA mappings from kernel
 */
void
iommu_dvmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_unmap: kvm %p size %lx\n",
	    kva, size));

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("iommu_dvmamem_unmap");
#endif

	size = round_page(size);
	pmap_kremove((vaddr_t)kva, size);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, (vaddr_t)kva, size, UVM_KMF_VAONLY);
}
