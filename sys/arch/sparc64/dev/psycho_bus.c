/*	$NetBSD: psycho_bus.c,v 1.3.4.1 1999/11/15 00:39:25 fvdl Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	@(#)sbus.c	8.1 (Berkeley) 6/11/93
 */

/*
 * bus space and bus dma support for UltraSPARC `psycho'.  this is largely
 * copied from the sbus code.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>
#include <sparc64/sparc64/cache.h>

#ifdef DEBUG
#define PDB_BUSMAP	0x1
#define PDB_BUSDMA	0x2
#define PDB_INTR	0x4
int psycho_busdma_debug = 0;
#define DPRINTF(l, s)   do { if (psycho_busdma_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

/*
 * here are our bus space and bus dma routines.  this is copied from the sbus
 * code.
 */
void psycho_enter __P((struct psycho_pbm *, vaddr_t, int64_t, int));
void psycho_remove __P((struct psycho_pbm *, vaddr_t, size_t));
int psycho_flush __P((struct psycho_softc *sc));

static int psycho_bus_mmap __P((bus_space_tag_t, bus_type_t, bus_addr_t,
				int, bus_space_handle_t *));
static int _psycho_bus_map __P((bus_space_tag_t, bus_type_t, bus_addr_t,
				bus_size_t, int, vaddr_t,
				bus_space_handle_t *));
static void *psycho_intr_establish __P((bus_space_tag_t, int, int,
				int (*) __P((void *)), void *));

static int psycho_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
			  bus_size_t, struct proc *, int));
static void psycho_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static void psycho_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
			   bus_size_t, int));
static int psycho_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
			   bus_size_t alignment, bus_size_t boundary,
			   bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
static void psycho_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
			   int nsegs));
static int psycho_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
			 int nsegs, size_t size, caddr_t *kvap, int flags));
static void psycho_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
			    size_t size));

bus_space_tag_t
psycho_alloc_bus_tag(pp, type)
	struct psycho_pbm *pp;
	int type;
{
	struct psycho_softc *sc = pp->pp_sc;
	bus_space_tag_t bt;

	bt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (bt == NULL)
		panic("could not allocate psycho bus tag");

	bzero(bt, sizeof *bt);
	bt->cookie = pp;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = _psycho_bus_map;
	bt->sparc_bus_mmap = psycho_bus_mmap;
	bt->sparc_intr_establish = psycho_intr_establish;
	return (bt);
}

bus_dma_tag_t
psycho_alloc_dma_tag(pp)
	struct psycho_pbm *pp;
{
	struct psycho_softc *sc = pp->pp_sc;
	bus_dma_tag_t dt, pdt = sc->sc_dmatag;

	dt = (bus_dma_tag_t)
		malloc(sizeof(struct sparc_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (dt == NULL)
		panic("could not allocate psycho dma tag");

	bzero(dt, sizeof *dt);
	dt->_cookie = pp;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	PCOPY(_dmamap_create);
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load = psycho_dmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	PCOPY(_dmamap_load_raw);
	dt->_dmamap_unload = psycho_dmamap_unload;
	dt->_dmamap_sync = psycho_dmamap_sync;
	dt->_dmamem_alloc = psycho_dmamem_alloc;
	dt->_dmamem_free = psycho_dmamem_free;
	dt->_dmamem_map = psycho_dmamem_map;
	dt->_dmamem_unmap = psycho_dmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

/*
 * bus space support.  <sparc64/dev/psychoreg.h> has a discussion about
 * PCI physical addresses.
 */

static int get_childspace __P((int));

static int
get_childspace(type)
	int type;
{
	int ss;

	switch (type) {
	case PCI_CONFIG_BUS_SPACE:
		ss = 0x00;
		break;
	case PCI_IO_BUS_SPACE:
		ss = 0x01;
		break;
	case PCI_MEMORY_BUS_SPACE:
		ss = 0x02;
		break;
#if 0
	/* we don't do 64 bit memory space */
	case PCI_MEMORY64_BUS_SPACE:
		ss = 0x03;
		break;
#endif
	default:
		panic("get_childspace: unknown bus type");
	}

	return (ss);
}

static int
_psycho_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vaddr_t vaddr;
	bus_space_handle_t *hp;
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	int i, ss;

	DPRINTF(PDB_BUSMAP, ("_psycho_bus_map: type %d off %qx sz %qx flags %d va %p", t->type, offset, size, flags, vaddr));

	ss = get_childspace(t->type);
	DPRINTF(PDB_BUSMAP, (" cspace %d", ss));


	for (i = 0; i < pp->pp_nrange; i++) {
		bus_addr_t paddr;

		if (((pp->pp_range[i].cspace >> 24) & 0x03) != ss)
			continue;

		paddr = pp->pp_range[i].phys_lo + offset;
		paddr |= ((bus_addr_t)pp->pp_range[i].phys_hi<<32);
		DPRINTF(PDB_BUSMAP, ("\n_psycho_bus_map: mapping paddr space %lx offset %lx paddr %qx\n",
			       (long)ss, (long)offset, paddr));
		return (bus_space_map2(sc->sc_bustag, t->type, paddr,
					size, flags, vaddr, hp));
	}
	DPRINTF(PDB_BUSMAP, (" FAILED\n"));
	return (EINVAL);
}

static int
psycho_bus_mmap(t, btype, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t paddr;
	int flags;
	bus_space_handle_t *hp;
{
	bus_addr_t offset = paddr;
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	int i, ss;

	ss = get_childspace(t->type);

	DPRINTF(PDB_BUSMAP, ("_psycho_bus_mmap: type %d flags %d pa %qx\n", btype, flags, paddr));

	for (i = 0; i < pp->pp_nrange; i++) {
		bus_addr_t paddr;

		if (((pp->pp_range[i].cspace >> 24) & 0x03) != ss)
			continue;

		paddr = pp->pp_range[i].phys_lo + offset;
		paddr |= ((bus_addr_t)pp->pp_range[i].phys_hi<<32);
		DPRINTF(PDB_BUSMAP, ("\n_psycho_bus_mmap: mapping paddr space %lx offset %lx paddr %qx\n",
			       (long)ss, (long)offset, paddr));
		return (bus_space_mmap(sc->sc_bustag, 0, paddr,
				       flags, hp));
	}

	return (-1);
}

/*
 * interrupt mapping.
 *
 * this table is from the UltraSPARC IIi users manual, table 11-4.
 * we may not need it.
 */
int ino_to_ipl_table[] = {
	7,		/* PCI A, Slot 0, INTA# */
	5,		/* PCI A, Slot 0, INTB# */
	5,		/* PCI A, Slot 0, INTC# */
	2,		/* PCI A, Slot 0, INTD# */
	7,		/* PCI A, Slot 1, INTA# */
	5,		/* PCI A, Slot 1, INTB# */
	5,		/* PCI A, Slot 1, INTC# */
	2,		/* PCI A, Slot 1, INTD# */
	6,		/* PCI A, Slot 2, INTA# (unavailable) */
	5,		/* PCI A, Slot 2, INTB# (unavailable) */
	2,		/* PCI A, Slot 2, INTC# (unavailable) */
	1,		/* PCI A, Slot 2, INTD# (unavailable) */
	6,		/* PCI A, Slot 3, INTA# (unavailable) */
	4,		/* PCI A, Slot 3, INTB# (unavailable) */
	2,		/* PCI A, Slot 3, INTC# (unavailable) */
	1,		/* PCI A, Slot 3, INTD# (unavailable) */
	6,		/* PCI B, Slot 0, INTA# */
	4,		/* PCI B, Slot 0, INTB# */
	3,		/* PCI B, Slot 0, INTC# */
	1,		/* PCI B, Slot 0, INTD# */
	6,		/* PCI B, Slot 1, INTA# */
	4,		/* PCI B, Slot 1, INTB# */
	3,		/* PCI B, Slot 1, INTC# */
	1,		/* PCI B, Slot 1, INTD# */
	6,		/* PCI B, Slot 2, INTA# */
	4,		/* PCI B, Slot 2, INTB# */
	3,		/* PCI B, Slot 2, INTC# */
	1,		/* PCI B, Slot 2, INTD# */
	6,		/* PCI B, Slot 3, INTA# */
	4,		/* PCI B, Slot 3, INTB# */
	3,		/* PCI B, Slot 3, INTC# */
	1,		/* PCI B, Slot 3, INTD# */
	3,		/* SCSI */
	3,		/* Ethernet */
	2,		/* Parallel */
	8,		/* Audio Record */
	7,		/* Audio Playback */
	8,		/* Power Fail */
	7,		/* Keyboard/Mouse/Serial */
	8,		/* Floppy */
	2,		/* Spare 1 */
	4,		/* Keyboard */
	4,		/* Mouse */
	7,		/* Serial */
	0,		/* Reserved */
	0,		/* Reserved */
	8,		/* Uncorrectable ECC error */
	8,		/* Correctable ECC error */
	8,		/* PCI bus error */
	0,		/* Reserved */
	0,		/* Reserved */
};

int
psycho_intr_map(tag, pin, line, ihp)
	pcitag_t tag;
	int pin;
	int line;
	pci_intr_handle_t *ihp;
{

	if (line < 0 || line > 0x32)
		panic("psycho_intr_map: line line < 0 || line > 0x32");

	(*ihp) = line;	/* XXX should be right */
	return (0);
}

/*
 * install an interrupt handler for a PCI device
 */
void *
psycho_intr_establish(t, level, flags, handler, arg)
	bus_space_tag_t t;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	struct psycho_pbm *pp = t->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	struct intrhand *ih;
	int ino;
	long vec = level; 

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	DPRINTF(PDB_INTR, ("\npsycho_intr_establish: level %x", level));
	ino = INTINO(vec);
	DPRINTF(PDB_INTR, (" ino %x", ino));
	if ((flags & BUS_INTR_ESTABLISH_SOFTINTR) == 0) {
		int64_t *intrmapptr, *intrclrptr;
		int64_t intrmap = 0;
		int i;

		DPRINTF(PDB_INTR, ("\npsycho: intr %lx: %lx\nHunting for IRQ...\n",
		    (long)ino, intrlev[ino]));
		if ((ino & INTMAP_OBIO) == 0) {
			/*
			 * there are only 8 PCI interrupt INO's available
			 */
			i = INTPCIINOX(vec);

			intrmapptr = &((&sc->sc_regs->pcia_slot0_int)[i]);
			intrclrptr = &sc->sc_regs->pcia0_clr_int[i<<2];

			DPRINTF(PDB_INTR, ("- turning on PCI intr %d", i));
		} else {
			/*
			 * there are INTPCI_MAXOBINO (0x16) OBIO interrupts
			 * available here (i think).
			 */
			i = INTPCIOBINOX(vec);
			if (i > INTPCI_MAXOBINO)
				panic("ino %d", vec);

			intrmapptr = &((&sc->sc_regs->scsi_int_map)[i]);
			intrclrptr = &((&sc->sc_regs->scsi_clr_int)[i]);

			DPRINTF(PDB_INTR, ("- turning on OBIO intr %d", i));
		}

		/* Register the map and clear intr registers */
		ih->ih_map = intrmapptr;
		ih->ih_clr = intrclrptr;

		/*
		 * Read the current value as we can't change it besides the
		 * valid bit so so make sure only this bit is changed.
		 */
		intrmap = *intrmapptr;
		DPRINTF(PDB_INTR, ("; read intrmap = %016qx", intrmap));

		/* Enable the interrupt */
		intrmap |= INTMAP_V;
		DPRINTF(PDB_INTR, ("; addr of intrmapptr = %p", intrmapptr));
		DPRINTF(PDB_INTR, ("; writing intrmap = %016qx\n", intrmap));
		*intrmapptr = intrmap;
		DPRINTF(PDB_INTR, ("; reread intrmap = %016qx", (intrmap = *intrmapptr)));
	}
#ifdef DEBUG
	if (psycho_busdma_debug & PDB_INTR) { long i; for (i=0; i<500000000; i++); }
#endif

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_number = ino;
	ih->ih_pil = ino_to_ipl_table[ino];
	DPRINTF(PDB_INTR, ("; installing handler %p with ino %u pil %u\n", handler, (u_int)ino, (u_int)ih->ih_pil));
	intr_establish(ih->ih_pil, ih);
	return (ih);
}

/* XXXMRG XXXMRG XXXMRG everything below here looks OK .. needs testing etc..*/

/*
 * Here are the iommu control routines. 
 */
void
psycho_enter(pp, va, pa, flags)
	struct psycho_pbm *pp;
	vaddr_t va;
	int64_t pa;
	int flags;
{
	struct psycho_softc *sc = pp->pp_sc;
	int64_t tte;

#ifdef DIAGNOSTIC
	if (va < sc->sc_is.is_dvmabase)
		panic("psycho_enter: va 0x%lx not in DVMA space",va);
#endif

	tte = MAKEIOTTE(pa, !(flags&BUS_DMA_NOWRITE), !(flags&BUS_DMA_NOCACHE), 
			!(flags&BUS_DMA_COHERENT));
	
	/* Is the streamcache flush really needed? */
	bus_space_write_8(sc->sc_bustag,
	    &sc->sc_is.is_sb->strbuf_pgflush, 0, va);
	psycho_flush(sc);
	DPRINTF(PDB_BUSDMA, ("Clearing TSB slot %d for va %p\n", (int)IOTSBSLOT(va,sc->sc_is.is_tsbsize), va));
	sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)] = tte;
	bus_space_write_8(sc->sc_bustag,
	    &sc->sc_regs->psy_iommu.iommu_flush, 0, va);
	DPRINTF(PDB_BUSDMA, ("psycho_enter: va %lx pa %lx TSB[%lx]@%p=%lx\n",
		       va, (long)pa, IOTSBSLOT(va,sc->sc_is.is_tsbsize), 
		       &sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)],
		       (long)tte));
}

/*
 * psycho_clear: clears mappings created by psycho_enter
 *
 * Only demap from IOMMU if flag is set.
 */
void
psycho_remove(pp, va, len)
	struct psycho_pbm *pp;
	vaddr_t va;
	size_t len;
{
	struct psycho_softc *sc = pp->pp_sc;

#ifdef DIAGNOSTIC
	if (va < sc->sc_is.is_dvmabase)
		panic("psycho_remove: va 0x%lx not in BUSDMA space", (long)va);
	if ((long)(va + len) < (long)va)
		panic("psycho_remove: va 0x%lx + len 0x%lx wraps", 
		      (long) va, (long) len);
	if (len & ~0xfffffff) 
		panic("psycho_remove: rediculous len 0x%lx", (long)len);
#endif

	va = trunc_page(va);
	while (len > 0) {

		/*
		 * Streaming buffer flushes:
		 * 
		 *   1 Tell strbuf to flush by storing va to strbuf_pgflush
		 * If we're not on a cache line boundary (64-bits):
		 *   2 Store 0 in flag
		 *   3 Store pointer to flag in flushsync
		 *   4 wait till flushsync becomes 0x1
		 *
		 * If it takes more than .5 sec, something went wrong.
		 */
		DPRINTF(PDB_BUSDMA, ("psycho_remove: flushing va %p TSB[%lx]@%p=%lx, %lu bytes left\n", 	       
		    (long)va, (long)IOTSBSLOT(va,sc->sc_is.is_tsbsize), 
		    (long)&sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)],
		    (long)(sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)]), 
		    (u_long)len));
		bus_space_write_8(sc->sc_bustag,
		    &sc->sc_is.is_sb->strbuf_pgflush, 0, va);
		if (len <= NBPG) {
			psycho_flush(sc);
			len = 0;
		} else len -= NBPG;
		DPRINTF(PDB_BUSDMA, ("psycho_remove: flushed va %p TSB[%lx]@%p=%lx, %lu bytes left\n", 	       
		    (long)va, (long)IOTSBSLOT(va,sc->sc_is.is_tsbsize), 
		    (long)&sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)],
		    (long)(sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)]), 
		    (u_long)len));

		sc->sc_is.is_tsb[IOTSBSLOT(va,sc->sc_is.is_tsbsize)] = 0;
		bus_space_write_8(sc->sc_bustag,
		     &sc->sc_regs->psy_iommu.iommu_flush, 0, va);
		va += NBPG;
	}
}

int 
psycho_flush(sc)
	struct psycho_softc *sc;
{
	struct iommu_state *is = &sc->sc_is;
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

	is->is_flush = 0;
	membar_sync();
	bus_space_write_8(sc->sc_bustag, &is->is_sb->strbuf_flushsync, 0, is->is_flushpa);
	membar_sync();

	microtime(&flushtimeout); 
	cur = flushtimeout;
	BUMPTIME(&flushtimeout, 500000); /* 1/2 sec */

	DPRINTF(PDB_BUSDMA, ("psycho_flush: flush = %lx at va = %lx pa = %lx now=%lx:%lx until = %lx:%lx\n", 
	    (long)is->is_flush, (long)&is->is_flush, 
	    (long)is->is_flushpa, cur.tv_sec, cur.tv_usec, 
	    flushtimeout.tv_sec, flushtimeout.tv_usec));
	/* Bypass non-coherent D$ */
	while (!ldxa(is->is_flushpa, ASI_PHYS_CACHED) && 
	       ((cur.tv_sec <= flushtimeout.tv_sec) && 
		(cur.tv_usec <= flushtimeout.tv_usec)))
		microtime(&cur);

#ifdef DIAGNOSTIC
	if (!is->is_flush) {
		printf("psycho_flush: flush timeout %p at %p\n", (long)is->is_flush, 
		       (long)is->is_flushpa); /* panic? */
#ifdef DDB
		Debugger();
#endif
	}
#endif
	DPRINTF(PDB_BUSDMA, ("psycho_flush: flushed\n"));
	return (is->is_flush);
}

/*
 * bus dma support -- XXXMRG this looks mostly OK.
 */
int
psycho_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
#if 1
	int s;
#endif
	int err;
	bus_size_t sgsize;
	paddr_t curaddr;
	u_long dvmaddr;
	vaddr_t vaddr = (vaddr_t)buf;
	pmap_t pmap;
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	if (map->dm_nsegs) {
		/* Already in use?? */
#ifdef DIAGNOSTIC
		printf("psycho_dmamap_load: map still in use\n");
#endif
		bus_dmamap_unload(t, map);
	}
#if 1
	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size) {
		DPRINTF(PDB_BUSDMA, ("psycho_dmamap_load(): error %d > %d -- map size exceeded!\n", buflen, map->_dm_size));
		return (EINVAL);
	}

	sgsize = round_page(buflen + ((int)vaddr & PGOFSET));

	/*
	 * XXX Need to implement "don't dma across this boundry".
	 */
	
	s = splhigh();
	err = extent_alloc(sc->sc_is.is_dvmamap, sgsize, NBPG,
			     map->_dm_boundary, EX_NOWAIT, (u_long *)&dvmaddr);
	splx(s);

	if (err != 0)
		return (err);

#ifdef DEBUG
	if (dvmaddr == (bus_addr_t)-1)	
	{ 
		printf("psycho_dmamap_load(): dvmamap_alloc(%d, %x) failed!\n", sgsize, flags);
		Debugger();
	}		
#endif	
	if (dvmaddr == (bus_addr_t)-1)
		return (ENOMEM);

	/*
	 * We always use just one segment.
	 */
	map->dm_mapsize = buflen;
	map->dm_nsegs = 1;
	map->dm_segs[0].ds_addr = dvmaddr + (vaddr & PGOFSET);
	map->dm_segs[0].ds_len = sgsize;

#else
	if ((err = bus_dmamap_load(t->_parent, map, buf, buflen, p, flags)))
		return (err);
#endif

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	dvmaddr = trunc_page(map->dm_segs[0].ds_addr);
	sgsize = round_page(buflen + ((int)vaddr & PGOFSET));
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

		DPRINTF(PDB_BUSDMA, ("psycho_dmamap_load: map %p loading va %lx at pa %lx\n",
		    map, (long)dvmaddr, (long)(curaddr & ~(NBPG-1))));
		psycho_enter(pp, trunc_page(dvmaddr), trunc_page(curaddr), flags);
			
		dvmaddr += PAGE_SIZE;
		vaddr += sgsize;
		buflen -= sgsize;
	}
	return (0);
}

void
psycho_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	vaddr_t addr;
	int len;
#if 1
	int error, s;
	bus_addr_t dvmaddr;
	bus_size_t sgsize;
#endif
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	if (map->dm_nsegs != 1)
		panic("psycho_dmamap_unload: nsegs = %d", map->dm_nsegs);

	addr = trunc_page(map->dm_segs[0].ds_addr);
	len = map->dm_segs[0].ds_len;

	DPRINTF(PDB_BUSDMA, ("psycho_dmamap_unload: map %p removing va %lx size %lx\n",
	    map, (long)addr, (long)len));
	psycho_remove(pp, addr, len);
#if 1
	dvmaddr = (map->dm_segs[0].ds_addr & ~PGOFSET);
	sgsize = map->dm_segs[0].ds_len;

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	
	/* Unmapping is bus dependent */
	s = splhigh();
	error = extent_free(sc->sc_is.is_dvmamap, dvmaddr, sgsize, EX_NOWAIT);
	splx(s);
	if (error != 0)
		printf("warning: %qd of DVMA space lost\n", (long long)sgsize);
	cache_flush((caddr_t)dvmaddr, (u_int) sgsize);	
#else
	bus_dmamap_unload(t->_parent, map);
#endif
}

void
psycho_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;
	vaddr_t va = map->dm_segs[0].ds_addr + offset;

	/*
	 * We only support one DMA segment; supporting more makes this code
         * too unweildy.
	 */

	if (ops & BUS_DMASYNC_PREREAD) {
		DPRINTF(PDB_BUSDMA, ("psycho_dmamap_sync: syncing va %p len %lu BUS_DMASYNC_PREREAD\n", 	       
		    (long)va, (u_long)len));

		/* Nothing to do */;
	}
	if (ops & BUS_DMASYNC_POSTREAD) {
		/*
		 * We should sync the IOMMU streaming caches here first.
		 */
		DPRINTF(PDB_BUSDMA, ("psycho_dmamap_sync: syncing va %p len %lu BUS_DMASYNC_POSTREAD\n", 	       
		    (long)va, (u_long)len));
		while (len > 0) {
			
			/*
			 * Streaming buffer flushes:
			 * 
			 *   1 Tell strbuf to flush by storing va to strbuf_pgflush
			 * If we're not on a cache line boundary (64-bits):
			 *   2 Store 0 in flag
			 *   3 Store pointer to flag in flushsync
			 *   4 wait till flushsync becomes 0x1
			 *
			 * If it takes more than .5 sec, something went wrong.
			 */
			DPRINTF(PDB_BUSDMA, ("psycho_dmamap_sync: flushing va %p, %lu bytes left\n", 	       
			    (long)va, (u_long)len));
			bus_space_write_8(sc->sc_bustag,
			    &sc->sc_is.is_sb->strbuf_pgflush, 0, va);
			if (len <= NBPG) {
				psycho_flush(sc);
				len = 0;
			} else
				len -= NBPG;
			va += NBPG;
		}
	}
	if (ops & BUS_DMASYNC_PREWRITE) {
		DPRINTF(PDB_BUSDMA, ("psycho_dmamap_sync: syncing va %p len %lu BUS_DMASYNC_PREWRITE\n", 	       
		    (long)va, (u_long)len));
		/* Nothing to do */;
	}
	if (ops & BUS_DMASYNC_POSTWRITE) {
		DPRINTF(PDB_BUSDMA, ("psycho_dmamap_sync: syncing va %p len %lu BUS_DMASYNC_POSTWRITE\n",
		    (long)va, (u_long)len));
		/* Nothing to do */;
	}
	bus_dmamap_sync(t->_parent, map, offset, len, ops);
}

int
psycho_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	paddr_t curaddr;
	u_long dvmaddr;
	vm_page_t m;
	struct pglist *mlist;
	int error;
	int n, s;
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	if ((error = bus_dmamem_alloc(t->_parent, size, alignment, 
				     boundary, segs, nsegs, rsegs, flags)))
		return (error);

	/*
	 * Allocate a DVMA mapping for our new memory.
	 */
	for (n = 0; n < *rsegs; n++) {
#if 1
		s = splhigh();
		if (extent_alloc(sc->sc_is.is_dvmamap, segs[0].ds_len, alignment,
				 boundary, EX_NOWAIT, (u_long *)&dvmaddr)) {
			splx(s);
				/* Free what we got and exit */
			bus_dmamem_free(t->_parent, segs, nsegs);
			return (ENOMEM);
		}
		splx(s);
#else
  		dvmaddr = dvmamap_alloc(segs[0].ds_len, flags);
		dvmaddr = dvmamap_alloc(segs[0].ds_len, flags);
		if (dvmaddr == (bus_addr_t)-1) {
			/* Free what we got and exit */
			bus_dmamem_free(t->_parent, segs, nsegs);
			return (ENOMEM);
		}
#endif
		segs[n].ds_addr = dvmaddr;
		size = segs[n].ds_len;
		mlist = segs[n]._ds_mlist;

		/* Map memory into DVMA space */
		for (m = mlist->tqh_first; m != NULL; m = m->pageq.tqe_next) {
			curaddr = VM_PAGE_TO_PHYS(m);
			DPRINTF(PDB_BUSDMA, ("psycho_dmamem_alloc: map %p loading va %lx at pa %lx\n",
			    (long)m, (long)dvmaddr, (long)(curaddr & ~(NBPG-1))));
			psycho_enter(pp, dvmaddr, curaddr, flags);
			dvmaddr += PAGE_SIZE;
		}
	}
	return (0);
}

void
psycho_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	vaddr_t addr;
	int len;
	int n, s, error;
	struct psycho_pbm *pp = (struct psycho_pbm *)t->_cookie;
	struct psycho_softc *sc = pp->pp_sc;

	for (n = 0; n < nsegs; n++) {
		addr = segs[n].ds_addr;
		len = segs[n].ds_len;
		psycho_remove(pp, addr, len);
#if 1
		s = splhigh();
		error = extent_free(sc->sc_is.is_dvmamap, addr, len, EX_NOWAIT);
		splx(s);
		if (error != 0)
			printf("warning: %ld of DVMA space lost\n", (long)len);
#else
		dvmamap_free(addr, len);
#endif
	}
	bus_dmamem_free(t->_parent, segs, nsegs);
}

/*
 * Map the DVMA mappings into the kernel pmap.
 * Check the flags to see whether we're streaming or coherent.
 */
int
psycho_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vm_page_t m;
	vaddr_t va;
	bus_addr_t addr;
	struct pglist *mlist;
	int cbit;

	/* 
	 * digest flags:
	 */
	cbit = 0;
	if (flags & BUS_DMA_COHERENT)	/* Disable vcache */
		cbit |= PMAP_NVC;
	if (flags & BUS_DMA_NOCACHE)	/* sideffects */
		cbit |= PMAP_NC;
	/*
	 * Now take this and map it into the CPU since it should already
	 * be in the the IOMMU.
	 */
	*kvap = (caddr_t)va = segs[0].ds_addr;
	mlist = segs[0]._ds_mlist;
	for (m = mlist->tqh_first; m != NULL; m = m->pageq.tqe_next) {

		if (size == 0)
			panic("psycho_dmamem_map: size botch");

		addr = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), va, addr | cbit,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return (0);
}

/*
 * Unmap DVMA mappings from kernel
 */
void
psycho_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{
	
#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("psycho_dmamem_unmap");
#endif
	
	size = round_page(size);
	pmap_remove(pmap_kernel(), (vaddr_t)kva, size);
}
