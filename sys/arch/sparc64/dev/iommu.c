/*	$NetBSD: iommu.c,v 1.55 2002/06/20 18:26:23 eeh Exp $	*/

/*
 * Copyright (c) 2001, 2002 Eduardo Horvath
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
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <sparc64/sparc64/cache.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#ifdef DEBUG
#define IDB_BUSDMA	0x1
#define IDB_IOMMU	0x2
#define IDB_INFO	0x4
#define	IDB_SYNC	0x8
int iommudebug = 0x0;
#define DPRINTF(l, s)   do { if (iommudebug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#define iommu_strbuf_flush(i, v) do {					\
	if ((i)->sb_flush)						\
		bus_space_write_8((i)->sb_is->is_bustag, (i)->sb_sb,	\
			STRBUFREG(strbuf_pgflush), (v));		\
	} while (0)

static	int iommu_strbuf_flush_done __P((struct strbuf_ctl *));

/*
 * initialise the UltraSPARC IOMMU (SBUS or PCI):
 *	- allocate and setup the iotsb.
 *	- enable the IOMMU
 *	- initialise the streaming buffers (if they exist)
 *	- create a private DVMA map.
 */
void
iommu_init(name, is, tsbsize, iovabase)
	char *name;
	struct iommu_state *is;
	int tsbsize;
	u_int32_t iovabase;
{
	psize_t size;
	vaddr_t va;
	paddr_t pa;
	struct vm_page *m;
	struct pglist mlist;

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
	is->is_cr = (tsbsize << 16) | IOMMUCR_EN;
	is->is_tsbsize = tsbsize;
	if (iovabase == -1) {
		is->is_dvmabase = IOTSB_VSTART(is->is_tsbsize);
		is->is_dvmaend = IOTSB_VEND;
	} else {
		is->is_dvmabase = iovabase;
		is->is_dvmaend = iovabase + IOTSB_VSIZE(tsbsize);
	}

	/*
	 * Allocate memory for I/O pagetables.  They need to be physically
	 * contiguous.
	 */

	size = NBPG<<(is->is_tsbsize);
	if (uvm_pglistalloc((psize_t)size, (paddr_t)0, (paddr_t)-1,
		(paddr_t)NBPG, (paddr_t)0, &mlist, 1, 0) != 0)
		panic("iommu_init: no memory");

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		panic("iommu_init: no memory");
	is->is_tsb = (int64_t *)va;

	m = TAILQ_FIRST(&mlist);
	is->is_ptsb = VM_PAGE_TO_PHYS(m);

	/* Map the pages */
	for (; m != NULL; m = TAILQ_NEXT(m,pageq)) {
		pa = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), va, pa | PMAP_NVC,
			VM_PROT_READ|VM_PROT_WRITE,
			VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
		va += NBPG;
	}
	pmap_update(pmap_kernel());
	bzero(is->is_tsb, size);

#ifdef DEBUG
	if (iommudebug & IDB_INFO)
	{
		/* Probe the iommu */

		printf("iommu regs at: cr=%lx tsb=%lx flush=%lx\n",
			(u_long)bus_space_read_8(is->is_bustag, is->is_iommu,
				offsetof (struct iommureg, iommu_cr)),
			(u_long)bus_space_read_8(is->is_bustag, is->is_iommu,
				offsetof (struct iommureg, iommu_tsb)),
			(u_long)bus_space_read_8(is->is_bustag, is->is_iommu,
				offsetof (struct iommureg, iommu_flush)));
		printf("iommu cr=%llx tsb=%llx\n",
			(unsigned long long)bus_space_read_8(is->is_bustag,
				is->is_iommu,
				offsetof (struct iommureg, iommu_cr)),
			(unsigned long long)bus_space_read_8(is->is_bustag,
				is->is_iommu,
				offsetof (struct iommureg, iommu_tsb)));
		printf("TSB base %p phys %llx\n", (void *)is->is_tsb, 
			(unsigned long long)is->is_ptsb);
		delay(1000000); /* 1 s */
	}
#endif

	/*
	 * now actually start up the IOMMU
	 */
	iommu_reset(is);

	/*
	 * Now all the hardware's working we need to allocate a dvma map.
	 */
	printf("DVMA map: %x to %x\n", 
		(unsigned int)is->is_dvmabase,
		(unsigned int)is->is_dvmaend);
	printf("IOTSB: %llx to %llx\n", 
		(unsigned long long)is->is_ptsb,
		(unsigned long long)(is->is_ptsb + size));
	is->is_dvmamap = extent_create(name,
				       is->is_dvmabase, is->is_dvmaend - NBPG,
				       M_DEVBUF, 0, 0, EX_NOWAIT);
}

/*
 * Streaming buffers don't exist on the UltraSPARC IIi; we should have
 * detected that already and disabled them.  If not, we will notice that
 * they aren't there when the STRBUF_EN bit does not remain.
 */
void
iommu_reset(is)
	struct iommu_state *is;
{
	int i;
	struct strbuf_ctl *sb;

	/* Need to do 64-bit stores */
	bus_space_write_8(is->is_bustag, is->is_iommu, IOMMUREG(iommu_tsb), 
		is->is_ptsb);

	/* Enable IOMMU in diagnostic mode */
	bus_space_write_8(is->is_bustag, is->is_iommu, IOMMUREG(iommu_cr),
		is->is_cr|IOMMUCR_DE);

	for (i=0; i<2; i++) {
		if ((sb = is->is_sb[i])) {

			/* Enable diagnostics mode? */
			bus_space_write_8(is->is_bustag, is->is_sb[i]->sb_sb, 
				STRBUFREG(strbuf_ctl), STRBUF_EN);

			/* No streaming buffers? Disable them */
			if (bus_space_read_8(is->is_bustag, 
				is->is_sb[i]->sb_sb, 
				STRBUFREG(strbuf_ctl)) == 0) {
				is->is_sb[i]->sb_flush = NULL;
			} else {
				/*
				 * locate the pa of the flush buffer.
				 */
				(void)pmap_extract(pmap_kernel(),
					(vaddr_t)is->is_sb[i]->sb_flush,
					&is->is_sb[i]->sb_flushpa);
			}
		}
	}
}

/*
 * Here are the iommu control routines. 
 */
void
iommu_enter(sb, va, pa, flags)
	struct strbuf_ctl *sb;
	vaddr_t va;
	int64_t pa;
	int flags;
{
	struct iommu_state *is = sb->sb_is;
	int strbuf = (flags & BUS_DMA_STREAMING);
	int64_t tte;

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || va > is->is_dvmaend)
		panic("iommu_enter: va %#lx not in DVMA space", va);
#endif

	/* Is the streamcache flush really needed? */
	if (sb->sb_flush) {
		iommu_strbuf_flush(sb, va);
		iommu_strbuf_flush_done(sb);
	} else
		/* If we can't flush the strbuf don't enable it. */
		strbuf = 0;

	tte = MAKEIOTTE(pa, !(flags & BUS_DMA_NOWRITE), 
		!(flags & BUS_DMA_NOCACHE), (strbuf));
#ifdef DEBUG
	tte |= (flags & 0xff000LL)<<(4*8);
#endif
	
	DPRINTF(IDB_IOMMU, ("Clearing TSB slot %d for va %p\n", 
		       (int)IOTSBSLOT(va,is->is_tsbsize), (void *)(u_long)va));
	is->is_tsb[IOTSBSLOT(va,is->is_tsbsize)] = tte;
	bus_space_write_8(is->is_bustag, is->is_iommu, 
		IOMMUREG(iommu_flush), va);
	DPRINTF(IDB_IOMMU, ("iommu_enter: va %lx pa %lx TSB[%lx]@%p=%lx\n",
		va, (long)pa, (u_long)IOTSBSLOT(va,is->is_tsbsize),
		(void *)(u_long)&is->is_tsb[IOTSBSLOT(va,is->is_tsbsize)],
		(u_long)tte));
}


/*
 * Find the value of a DVMA address (debug routine).
 */
paddr_t
iommu_extract(is, dva)
	struct iommu_state *is;
	vaddr_t dva;
{
	int64_t tte = 0;
	
	if (dva >= is->is_dvmabase && dva < is->is_dvmaend)
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
iommu_remove(is, va, len)
	struct iommu_state *is;
	vaddr_t va;
	size_t len;
{

#ifdef DIAGNOSTIC
	if (va < is->is_dvmabase || va > is->is_dvmaend)
		panic("iommu_remove: va 0x%lx not in DVMA space", (u_long)va);
	if ((long)(va + len) < (long)va)
		panic("iommu_remove: va 0x%lx + len 0x%lx wraps", 
		      (long) va, (long) len);
	if (len & ~0xfffffff) 
		panic("iommu_remove: rediculous len 0x%lx", (u_long)len);
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
		if (len <= NBPG)
			len = 0;
		else
			len -= NBPG;

		/* XXX Zero-ing the entry would not require RMW */
		is->is_tsb[IOTSBSLOT(va,is->is_tsbsize)] &= ~IOTTE_V;
		bus_space_write_8(is->is_bustag, is->is_iommu, 
			IOMMUREG(iommu_flush), va);
		va += NBPG;
	}
}

static int 
iommu_strbuf_flush_done(sb)
	struct strbuf_ctl *sb;
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
	
	DPRINTF(IDB_IOMMU, ("iommu_strbuf_flush_done: flush = %lx "
		"at va = %lx pa = %lx now=%lx:%lx until = %lx:%lx\n",
		(long)*sb->sb_flush, (long)sb->sb_flush, (long)sb->sb_flushpa, 
		cur.tv_sec, cur.tv_usec,
		flushtimeout.tv_sec, flushtimeout.tv_usec));

	/* Bypass non-coherent D$ */
	while ((!ldxa(sb->sb_flushpa, ASI_PHYS_CACHED)) &&
		((cur.tv_sec <= flushtimeout.tv_sec) &&
			(cur.tv_usec <= flushtimeout.tv_usec)))
		microtime(&cur);

#ifdef DIAGNOSTIC
	if (!ldxa(sb->sb_flushpa, ASI_PHYS_CACHED)) {
		printf("iommu_strbuf_flush_done: flush timeout %p, at %p\n",
			(void *)(u_long)*sb->sb_flush,
			(void *)(u_long)sb->sb_flushpa); /* panic? */
#ifdef DDB
		Debugger();
#endif
	}
#endif
	DPRINTF(IDB_IOMMU, ("iommu_strbuf_flush_done: flushed\n"));
	return (*sb->sb_flush);
}

/*
 * IOMMU DVMA operations, common to SBUS and PCI.
 */
int
iommu_dvmamap_load(t, sb, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct iommu_state *is = sb->sb_is;
	int s;
	int err;
	bus_size_t sgsize;
	paddr_t curaddr;
	u_long dvmaddr, sgstart, sgend;
	bus_size_t align, boundary;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;
	pmap_t pmap;

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
	align = max(map->dm_segs[0]._ds_align, NBPG);
	s = splhigh();
	/* 
	 * If our segment size is larger than the boundary we need to 
	 * split the transfer up int little pieces ourselves.
	 */
	err = extent_alloc(is->is_dvmamap, sgsize, align, 
		(sgsize > boundary) ? 0 : boundary, 
		EX_NOWAIT|EX_BOUNDZERO, &dvmaddr);
	splx(s);

#ifdef DEBUG
	if (err || (dvmaddr == (bus_addr_t)-1))	
	{ 
		printf("iommu_dvmamap_load(): extent_alloc(%d, %x) failed!\n",
		    (int)sgsize, flags);
#ifdef DDB
		Debugger();
#endif
	}		
#endif	
	if (err != 0)
		return (err);

	if (dvmaddr == (bus_addr_t)-1)
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
	DPRINTF(IDB_INFO, ("iommu_dvmamap_load: boundary %lx boundary-1 %lx "
		"~(boundary-1) %lx\n", boundary, (boundary-1), ~(boundary-1)));
	while ((sgstart & ~(boundary - 1)) != (sgend & ~(boundary - 1))) {
		/* Oops.  We crossed a boundary.  Split the xfer. */
		DPRINTF(IDB_INFO, ("iommu_dvmamap_load: "
			"seg %d start %lx size %lx\n", seg,
			(long)map->dm_segs[seg].ds_addr, 
			map->dm_segs[seg].ds_len));
		map->dm_segs[seg].ds_len =
		    boundary - (sgstart & (boundary - 1));
		if (++seg >= map->_dm_segcnt) {
			/* Too many segments.  Fail the operation. */
			DPRINTF(IDB_INFO, ("iommu_dvmamap_load: "
				"too many segments %d\n", seg));
			s = splhigh();
			/* How can this fail?  And if it does what can we do? */
			err = extent_free(is->is_dvmamap,
				dvmaddr, sgsize, EX_NOWAIT);
			map->_dm_dvmastart = 0;
			map->_dm_dvmasize = 0;
			splx(s);
			return (E2BIG);
		}
		sgstart = roundup(sgstart, boundary);
		map->dm_segs[seg].ds_addr = sgstart;
	}
	map->dm_segs[seg].ds_len = sgend - sgstart + 1;
	DPRINTF(IDB_INFO, ("iommu_dvmamap_load: "
		"seg %d start %lx size %lx\n", seg,
		(long)map->dm_segs[seg].ds_addr, map->dm_segs[seg].ds_len));
	map->dm_nsegs = seg+1;
	map->dm_mapsize = buflen;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; buflen > 0; ) {
		/*
		 * Get the physical address for this page.
		 */
		if (pmap_extract(pmap, (vaddr_t)vaddr, &curaddr) == FALSE) {
			bus_dmamap_unload(t, map);
			return (-1);
		}

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		DPRINTF(IDB_BUSDMA,
		    ("iommu_dvmamap_load: map %p loading va %p "
			    "dva %lx at pa %lx\n",
			    map, (void *)vaddr, (long)dvmaddr,
			    (long)(curaddr & ~(NBPG-1))));
		iommu_enter(sb, trunc_page(dvmaddr), trunc_page(curaddr),
		    flags|0x4000);
			
		dvmaddr += PAGE_SIZE;
		vaddr += sgsize;
		buflen -= sgsize;
	}
#ifdef DIAGNOSTIC
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		if (map->dm_segs[seg].ds_addr < is->is_dvmabase ||
			map->dm_segs[seg].ds_addr > is->is_dvmaend) {
			printf("seg %d dvmaddr %lx out of range %x - %x\n",
				seg, (long)map->dm_segs[seg].ds_addr, 
				is->is_dvmabase, is->is_dvmaend);
			Debugger();
		}
	}
#endif
	return (0);
}


void
iommu_dvmamap_unload(t, sb, map)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_dmamap_t map;
{
	struct iommu_state *is = sb->sb_is;
	int error, s;
	bus_size_t sgsize;

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

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	
	s = splhigh();
	error = extent_free(is->is_dvmamap, map->_dm_dvmastart, 
		map->_dm_dvmasize, EX_NOWAIT);
	map->_dm_dvmastart = 0;
	map->_dm_dvmasize = 0;
	splx(s);
	if (error != 0)
		printf("warning: %qd of DVMA space lost\n", (long long)sgsize);

	/* Clear the map */
}


int
iommu_dvmamap_load_raw(t, sb, map, segs, nsegs, flags, size)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	int flags;
	bus_size_t size;
{
	struct iommu_state *is = sb->sb_is;
	struct vm_page *m;
	int i, j, s;
	int left;
	int err;
	bus_size_t sgsize;
	paddr_t pa;
	bus_size_t boundary, align;
	u_long dvmaddr, sgstart, sgend;
	struct pglist *mlist;
	int pagesz = PAGE_SIZE;
	int npg = 0; /* DEBUG */

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
	pa = segs[0].ds_addr;
	sgsize = 0;
	left = size;
	for (i=0; left && i<nsegs; i++) {
		if (round_page(pa) != round_page(segs[i].ds_addr))
			sgsize = round_page(sgsize);
		sgsize += min(left, segs[i].ds_len);
		left -= segs[i].ds_len;
		pa = segs[i].ds_addr + segs[i].ds_len;
	}
	sgsize = round_page(sgsize);

	s = splhigh();
	/* 
	 * If our segment size is larger than the boundary we need to 
	 * split the transfer up into little pieces ourselves.
	 */
	err = extent_alloc(is->is_dvmamap, sgsize, align,
		(sgsize > boundary) ? 0 : boundary,
		((flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT) |
		EX_BOUNDZERO, &dvmaddr);
	splx(s);

	if (err != 0)
		return (err);

#ifdef DEBUG
	if (dvmaddr == (bus_addr_t)-1)	
	{ 
		printf("iommu_dvmamap_load_raw(): extent_alloc(%d, %x) failed!\n",
		    (int)sgsize, flags);
		Debugger();
	}		
#endif	
	if (dvmaddr == (bus_addr_t)-1)
		return (ENOMEM);

	/* Set the active DVMA map */
	map->_dm_dvmastart = dvmaddr;
	map->_dm_dvmasize = sgsize;

	if ((mlist = segs[0]._ds_mlist) == NULL) {
		u_long prev_va = NULL;
		paddr_t prev_pa = 0;
		int end = 0, offset;

		/*
		 * This segs is made up of individual physical
		 *  segments, probably by _bus_dmamap_load_uio() or 
		 * _bus_dmamap_load_mbuf().  Ignore the mlist and
		 * load each one individually.
		 */
		map->dm_mapsize = size;

		j = 0;
		for (i = 0; i < nsegs ; i++) {

			pa = segs[i].ds_addr;
			offset = (pa & PGOFSET);
			pa = trunc_page(pa);
			dvmaddr = trunc_page(dvmaddr);
			left = min(size, segs[i].ds_len);

			DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: converting "
				"physseg %d start %lx size %lx\n", i, 
				(long)segs[i].ds_addr, segs[i].ds_len));

			if ((pa == prev_pa) && 
				((offset != 0) || (end != offset))) {
				/* We can re-use this mapping */
				dvmaddr = prev_va;
			}

			sgstart = dvmaddr + offset;
			sgend = sgstart + left - 1;

			/* Are the segments virtually adjacent? */
			if ((j > 0) && (end == offset) && 
				((offset == 0) || (pa == prev_pa))) {
				/* Just append to the previous segment. */
				map->dm_segs[--j].ds_len += left;
				DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
					"appending seg %d start %lx size %lx\n", j,
					(long)map->dm_segs[j].ds_addr, 
					map->dm_segs[j].ds_len));
			} else {
				if (j >= map->_dm_segcnt) {
					iommu_dvmamap_unload(t, sb, map);
					return (E2BIG);
				}
				map->dm_segs[j].ds_addr = sgstart;
				map->dm_segs[j].ds_len = left;
				DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
					"seg %d start %lx size %lx\n", j,
					(long)map->dm_segs[j].ds_addr,
					map->dm_segs[j].ds_len));
			}
			end = (offset + left) & PGOFSET;

			/* Check for boundary issues */
			while ((sgstart & ~(boundary - 1)) !=
				(sgend & ~(boundary - 1))) {
				/* Need a new segment. */
				map->dm_segs[j].ds_len =
					boundary - (sgstart & (boundary - 1));
				DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
					"seg %d start %lx size %lx\n", j,
					(long)map->dm_segs[j].ds_addr, 
					map->dm_segs[j].ds_len));
				if (++j >= map->_dm_segcnt) {
					iommu_dvmamap_unload(t, sb, map);
					return (E2BIG);
				}
				sgstart = roundup(sgstart, boundary);
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
				if (prev_va != dvmaddr)
					iommu_enter(sb, prev_va = dvmaddr,
						prev_pa = pa,
						flags|(++npg<<12));
				dvmaddr += pagesz;
				pa += pagesz;
			}

			size -= left;
			++j;
		}

		map->dm_nsegs = j;
#ifdef DIAGNOSTIC
		{ int seg;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		if (map->dm_segs[seg].ds_addr < is->is_dvmabase ||
			map->dm_segs[seg].ds_addr > is->is_dvmaend) {
			printf("seg %d dvmaddr %lx out of range %x - %x\n",
				seg, (long)map->dm_segs[seg].ds_addr, 
				is->is_dvmabase, is->is_dvmaend);
			Debugger();
		}
	}
		}
#endif
		return (0);
	}
	/*
	 * This was allocated with bus_dmamem_alloc.
	 * The pages are on an `mlist'.
	 */
	map->dm_mapsize = size;
	i = 0;
	sgstart = dvmaddr;
	sgend = sgstart + size - 1;
	map->dm_segs[i].ds_addr = sgstart;
	while ((sgstart & ~(boundary - 1)) != (sgend & ~(boundary - 1))) {
		/* Oops.  We crossed a boundary.  Split the xfer. */
		map->dm_segs[i].ds_len = boundary - (sgstart & (boundary - 1));
		DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
			"seg %d start %lx size %lx\n", i,
			(long)map->dm_segs[i].ds_addr,
			map->dm_segs[i].ds_len));
		if (++i >= map->_dm_segcnt) {
			/* Too many segments.  Fail the operation. */
			s = splhigh();
			/* How can this fail?  And if it does what can we do? */
			err = extent_free(is->is_dvmamap,
				dvmaddr, sgsize, EX_NOWAIT);
			map->_dm_dvmastart = 0;
			map->_dm_dvmasize = 0;
			splx(s);
			return (E2BIG);
		}
		sgstart = roundup(sgstart, boundary);
		map->dm_segs[i].ds_addr = sgstart;
	}
	DPRINTF(IDB_INFO, ("iommu_dvmamap_load_raw: "
			"seg %d start %lx size %lx\n", i,
			(long)map->dm_segs[i].ds_addr, map->dm_segs[i].ds_len));
	map->dm_segs[i].ds_len = sgend - sgstart + 1;

	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		if (sgsize == 0)
			panic("iommu_dmamap_load_raw: size botch");
		pa = VM_PAGE_TO_PHYS(m);

		DPRINTF(IDB_BUSDMA,
		    ("iommu_dvmamap_load_raw: map %p loading va %lx at pa %lx\n",
		    map, (long)dvmaddr, (long)(pa)));
		iommu_enter(sb, dvmaddr, pa, flags|0x8000);
			
		dvmaddr += pagesz;
		sgsize -= pagesz;
	}
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
			Debugger();
		}
	}
	}
#endif
	return (0);
}

void
iommu_dvmamap_sync(t, sb, map, offset, len, ops)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	struct iommu_state *is = sb->sb_is;
	vaddr_t va = map->dm_segs[0].ds_addr + offset;
	int64_t tte;

	/*
	 * We only support one DMA segment; supporting more makes this code
         * too unweildy.
	 */

	if (ops & BUS_DMASYNC_PREREAD) {
		DPRINTF(IDB_SYNC,
		    ("iommu_dvmamap_sync: syncing va %p len %lu "
		     "BUS_DMASYNC_PREREAD\n", (void *)(u_long)va, (u_long)len));

		/* Nothing to do */;
	}
	if (ops & BUS_DMASYNC_POSTREAD) {
		DPRINTF(IDB_SYNC,
		    ("iommu_dvmamap_sync: syncing va %p len %lu "
		     "BUS_DMASYNC_POSTREAD\n", (void *)(u_long)va, (u_long)len));
#ifdef DIAGNOSTIC
		if (va < is->is_dvmabase || va >= is->is_dvmaend)
			panic("iommu_dvmamap_sync: invalid dva %p", va);
#endif
		tte = is->is_tsb[IOTSBSLOT(va, is->is_tsbsize)];

		DPRINTF(IDB_SYNC,
		    ("iommu_dvmamap_sync: syncing va %p len %lu "
		     "BUS_DMASYNC_PREWRITE\n", (void *)(u_long)va, (u_long)len));

		/* if we have a streaming buffer, flush it here first */
		if ((tte & IOTTE_STREAM) && sb->sb_flush)
			while (len > 0) {
				DPRINTF(IDB_BUSDMA,
				    ("iommu_dvmamap_sync: flushing va %p, %lu "
				     "bytes left\n", (void *)(u_long)va, 
					    (u_long)len));
				iommu_strbuf_flush(sb, va);
				if (len <= NBPG) {
					iommu_strbuf_flush_done(sb);
					len = 0;
				} else
					len -= NBPG;
				va += NBPG;
			}
	}
	if (ops & BUS_DMASYNC_PREWRITE) {
#ifdef DIAGNOSTIC
		if (va < is->is_dvmabase || va >= is->is_dvmaend)
			panic("iommu_dvmamap_sync: invalid dva %p", va);
#endif
		tte = is->is_tsb[IOTSBSLOT(va, is->is_tsbsize)];

		DPRINTF(IDB_SYNC,
		    ("iommu_dvmamap_sync: syncing va %p len %lu "
		     "BUS_DMASYNC_PREWRITE\n", (void *)(u_long)va, (u_long)len));

		/* if we have a streaming buffer, flush it here first */
		if ((tte & IOTTE_STREAM) && sb->sb_flush)
			while (len > 0) {
				DPRINTF(IDB_BUSDMA,
				    ("iommu_dvmamap_sync: flushing va %p, %lu "
				     "bytes left\n", (void *)(u_long)va, 
					    (u_long)len));
				iommu_strbuf_flush(sb, va);
				if (len <= NBPG) {
					iommu_strbuf_flush_done(sb);
					len = 0;
				} else
					len -= NBPG;
				va += NBPG;
			}
	}
	if (ops & BUS_DMASYNC_POSTWRITE) {
		DPRINTF(IDB_SYNC,
		    ("iommu_dvmamap_sync: syncing va %p len %lu "
		     "BUS_DMASYNC_POSTWRITE\n", (void *)(u_long)va, (u_long)len));
		/* Nothing to do */;
	}
}

int
iommu_dvmamem_alloc(t, sb, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{

	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_alloc: sz %llx align %llx bound %llx "
	   "segp %p flags %d\n", (unsigned long long)size,
	   (unsigned long long)alignment, (unsigned long long)boundary,
	   segs, flags));
	return (bus_dmamem_alloc(t->_parent, size, alignment, boundary,
	    segs, nsegs, rsegs, flags|BUS_DMA_DVMA));
}

void
iommu_dvmamem_free(t, sb, segs, nsegs)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_dma_segment_t *segs;
	int nsegs;
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
iommu_dvmamem_map(t, sb, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct vm_page *m;
	vaddr_t va;
	bus_addr_t addr;
	struct pglist *mlist;
	int cbit;

	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_map: segp %p nsegs %d size %lx\n",
	    segs, nsegs, size));

	/*
	 * Allocate some space in the kernel map, and then map these pages
	 * into this space.
	 */
	size = round_page(size);
	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	/* 
	 * digest flags:
	 */
	cbit = 0;
	if (flags & BUS_DMA_COHERENT)	/* Disable vcache */
		cbit |= PMAP_NVC;
	if (flags & BUS_DMA_NOCACHE)	/* sideffects */
		cbit |= PMAP_NC;

	/*
	 * Now take this and map it into the CPU.
	 */
	mlist = segs[0]._ds_mlist;
	for (m = mlist->tqh_first; m != NULL; m = m->pageq.tqe_next) {
#ifdef DIAGNOSTIC
		if (size == 0)
			panic("iommu_dvmamem_map: size botch");
#endif
		addr = VM_PAGE_TO_PHYS(m);
		DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_map: "
		    "mapping va %lx at %llx\n", va, (unsigned long long)addr | cbit));
		pmap_enter(pmap_kernel(), va, addr | cbit,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
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
iommu_dvmamem_unmap(t, sb, kva, size)
	bus_dma_tag_t t;
	struct strbuf_ctl *sb;
	caddr_t kva;
	size_t size;
{
	
	DPRINTF(IDB_BUSDMA, ("iommu_dvmamem_unmap: kvm %p size %lx\n",
	    kva, size));
	    
#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("iommu_dvmamem_unmap");
#endif
	
	size = round_page(size);
	pmap_remove(pmap_kernel(), (vaddr_t)kva, size);
	pmap_update(pmap_kernel());
#if 0
	/*
	 * XXX ? is this necessary? i think so and i think other
	 * implementations are missing it.
	 */
	uvm_km_free(kernel_map, (vaddr_t)kva, size);
#endif
}
