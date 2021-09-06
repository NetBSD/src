/*	$NetBSD: dma.c,v 1.44 2021/09/06 20:55:08 andvar Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)dma.c	8.1 (Berkeley) 6/10/93
 */

/*
 * DMA driver
 */

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dma.c,v 1.44 2021/09/06 20:55:08 andvar Exp $");

#include <machine/hp300spu.h>	/* XXX param.h includes cpu.h */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <m68k/cacheops.h>

#include <hp300/dev/intiovar.h>
#include <hp300/dev/dmareg.h>
#include <hp300/dev/dmavar.h>

/*
 * The largest single request will be MAXPHYS bytes which will require
 * at most MAXPHYS/PAGE_SIZE+1 chain elements to describe, i.e. if none of
 * the buffer pages are physically contiguous (MAXPHYS/PAGE_SIZE) and the
 * buffer is not page aligned (+1).
 */
#define	DMAMAXIO	(MAXPHYS/PAGE_SIZE+1)

struct dma_chain {
	int	dc_count;
	char	*dc_addr;
};

struct dma_channel {
	struct	dmaqueue *dm_job;		/* current job */
	struct	dmadevice *dm_hwaddr;		/* registers if DMA_C */
	struct	dmaBdevice *dm_Bhwaddr;		/* registers if not DMA_C */
	char	dm_flags;			/* misc. flags */
	u_short	dm_cmd;				/* DMA controller command */
	int	dm_cur;				/* current segment */
	int	dm_last;			/* last segment */
	struct	dma_chain dm_chain[DMAMAXIO];	/* all segments */
};

struct dma_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	struct	dmareg *sc_dmareg;		/* pointer to our hardware */
	struct	dma_channel sc_chan[NDMACHAN];	/* 2 channels */
	TAILQ_HEAD(, dmaqueue) sc_queue;	/* job queue */
	struct	callout sc_debug_ch;
	char	sc_type;			/* A, B, or C */
	int	sc_ipl;				/* our interrupt level */
	void	*sc_ih;				/* interrupt cookie */
};

/* types */
#define	DMA_B	0
#define DMA_C	1

/* flags */
#define DMAF_PCFLUSH	0x01
#define DMAF_VCFLUSH	0x02
#define DMAF_NOINTR	0x04

static int	dmamatch(device_t, cfdata_t, void *);
static void	dmaattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(dma, sizeof(struct dma_softc),
    dmamatch, dmaattach, NULL, NULL);

static int	dmaintr(void *);

#ifdef DEBUG
int	dmadebug = 0;
#define DDB_WORD	0x01	/* same as DMAGO_WORD */
#define DDB_LWORD	0x02	/* same as DMAGO_LWORD */
#define	DDB_FOLLOW	0x04
#define DDB_IO		0x08

static void	dmatimeout(void *);
int	dmatimo[NDMACHAN];

long	dmahits[NDMACHAN];
long	dmamisses[NDMACHAN];
long	dmabyte[NDMACHAN];
long	dmaword[NDMACHAN];
long	dmalword[NDMACHAN];
#endif

static struct dma_softc *dma_softc;

static int
dmamatch(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	static int dmafound = 0;                /* can only have one */

	if (strcmp("dma", ia->ia_modname) != 0 || dmafound)
		return 0;

	dmafound = 1;
	return 1;
}

static void
dmaattach(device_t parent, device_t self, void *aux)
{
	struct dma_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	struct dma_channel *dc;
	struct dmareg *dma;
	int i;
	char rev;

	sc->sc_dev = self;

	/* There's just one. */
	dma_softc = sc;

	sc->sc_bst = ia->ia_bst;
	if (bus_space_map(sc->sc_bst, ia->ia_iobase, INTIO_DEVSIZE, 0,
	     &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	dma = bus_space_vaddr(sc->sc_bst, sc->sc_bsh);
	sc->sc_dmareg = dma;

	/*
	 * Determine the DMA type.  A DMA_A or DMA_B will fail the
	 * following probe.
	 *
	 * XXX Don't know how to easily differentiate the A and B cards,
	 * so we just hope nobody has an A card (A cards will work if
	 * splbio works out to ipl 3).
	 */
	if (hp300_bus_space_probe(sc->sc_bst, sc->sc_bsh, DMA_ID2, 1) == 0) {
		rev = 'B';
#if !defined(HP320)
		aprint_normal("\n");
		panic("%s: DMA card requires hp320 support", __func__);
#endif
	} else
		rev = dma->dma_id[2];

	sc->sc_type = (rev == 'B') ? DMA_B : DMA_C;

	TAILQ_INIT(&sc->sc_queue);
	callout_init(&sc->sc_debug_ch, 0);

	for (i = 0; i < NDMACHAN; i++) {
		dc = &sc->sc_chan[i];
		dc->dm_job = NULL;
		switch (i) {
		case 0:
			dc->dm_hwaddr = &dma->dma_chan0;
			dc->dm_Bhwaddr = &dma->dma_Bchan0;
			break;

		case 1:
			dc->dm_hwaddr = &dma->dma_chan1;
			dc->dm_Bhwaddr = &dma->dma_Bchan1;
			break;

		default:
			aprint_normal("\n");
			panic("%s: more than 2 channels?", __func__);
			/* NOTREACHED */
		}
	}

#ifdef DEBUG
	/* make sure timeout is really not needed */
	callout_reset(&sc->sc_debug_ch, 30 * hz, dmatimeout, sc);
#endif

	aprint_normal(": 98620%c, 2 channels, %d-bit DMA\n",
	    rev, (rev == 'B') ? 16 : 32);

	/*
	 * Defer hooking up our interrupt until the first
	 * DMA-using controller has hooked up theirs.
	 */
	sc->sc_ih = NULL;
}

/*
 * Compute the ipl and (re)establish the interrupt handler
 * for the DMA controller.
 */
void
dmacomputeipl(void)
{
	struct dma_softc *sc = dma_softc;

	if (sc->sc_ih != NULL)
		intr_disestablish(sc->sc_ih);

	/*
	 * Our interrupt level must be as high as the highest
	 * device using DMA (i.e. splbio).
	 */
	sc->sc_ipl = PSLTOIPL(ipl2psl_table[IPL_VM]);
	sc->sc_ih = intr_establish(dmaintr, sc, sc->sc_ipl, IPL_VM);
}

int
dmareq(struct dmaqueue *dq)
{
	struct dma_softc *sc = dma_softc;
	int i, chan, s;

#if 1
	s = splhigh();	/* XXXthorpej */
#else
	s = splbio();
#endif

	chan = dq->dq_chan;
	for (i = NDMACHAN - 1; i >= 0; i--) {
		/*
		 * Can we use this channel?
		 */
		if ((chan & (1 << i)) == 0)
			continue;

		/*
		 * We can use it; is it busy?
		 */
		if (sc->sc_chan[i].dm_job != NULL)
			continue;

		/*
		 * Not busy; give the caller this channel.
		 */
		sc->sc_chan[i].dm_job = dq;
		dq->dq_chan = i;
		splx(s);
		return 1;
	}

	/*
	 * Couldn't get a channel now; put this in the queue.
	 */
	TAILQ_INSERT_TAIL(&sc->sc_queue, dq, dq_list);
	splx(s);
	return 0;
}

void
dmafree(struct dmaqueue *dq)
{
	int unit = dq->dq_chan;
	struct dma_softc *sc = dma_softc;
	struct dma_channel *dc = &sc->sc_chan[unit];
	struct dmaqueue *dn;
	int chan, s;

#if 1
	s = splhigh();	/* XXXthorpej */
#else
	s = splbio();
#endif

#ifdef DEBUG
	dmatimo[unit] = 0;
#endif

	DMA_CLEAR(dc);

#if defined(CACHE_HAVE_PAC) || defined(M68040)
	/*
	 * XXX we may not always go thru the flush code in dmastop()
	 */
	if (dc->dm_flags & DMAF_PCFLUSH) {
		PCIA();
		dc->dm_flags &= ~DMAF_PCFLUSH;
	}
#endif

#if defined(CACHE_HAVE_VAC)
	if (dc->dm_flags & DMAF_VCFLUSH) {
		/*
		 * 320/350s have VACs that may also need flushing.
		 * In our case we only flush the supervisor side
		 * because we know that if we are DMAing to user
		 * space, the physical pages will also be mapped
		 * in kernel space (via vmapbuf) and hence cache-
		 * inhibited by the pmap module due to the multiple
		 * mapping.
		 */
		DCIS();
		dc->dm_flags &= ~DMAF_VCFLUSH;
	}
#endif

	/*
	 * Channel is now free.  Look for another job to run on this
	 * channel.
	 */
	dc->dm_job = NULL;
	chan = 1 << unit;
	for (dn = TAILQ_FIRST(&sc->sc_queue); dn != NULL;
	    dn = TAILQ_NEXT(dn, dq_list)) {
		if (dn->dq_chan & chan) {
			/* Found one... */
			TAILQ_REMOVE(&sc->sc_queue, dn, dq_list);
			dc->dm_job = dn;
			dn->dq_chan = dq->dq_chan;
			splx(s);

			/* Start the initiator. */
			(*dn->dq_start)(dn->dq_softc);
			return;
		}
	}
	splx(s);
}

void
dmago(int unit, char *addr, int count, int flags)
{
	struct dma_softc *sc = dma_softc;
	struct dma_channel *dc = &sc->sc_chan[unit];
	char *dmaend = NULL;
	int seg, tcount;

	if (count > MAXPHYS)
		panic("dmago: count > MAXPHYS");

#if defined(HP320)
	if (sc->sc_type == DMA_B && (flags & DMAGO_LWORD))
		panic("dmago: no can do 32-bit DMA");
#endif

#ifdef DEBUG
	if (dmadebug & DDB_FOLLOW)
		printf("dmago(%d, %p, %x, %x)\n",
		       unit, addr, count, flags);
	if (flags & DMAGO_LWORD)
		dmalword[unit]++;
	else if (flags & DMAGO_WORD)
		dmaword[unit]++;
	else
		dmabyte[unit]++;
#endif
	/*
	 * Build the DMA chain
	 */
	for (seg = 0; count > 0; seg++) {
		dc->dm_chain[seg].dc_addr = (char *) kvtop(addr);
#if defined(M68040)
		/*
		 * Push back dirty cache lines
		 */
		if (mmutype == MMU_68040)
			DCFP((paddr_t)dc->dm_chain[seg].dc_addr);
#endif
		if (count < (tcount = PAGE_SIZE - ((int)addr & PGOFSET)))
			tcount = count;
		dc->dm_chain[seg].dc_count = tcount;
		addr += tcount;
		count -= tcount;
		if (flags & DMAGO_LWORD)
			tcount >>= 2;
		else if (flags & DMAGO_WORD)
			tcount >>= 1;

		/*
		 * Try to compact the DMA transfer if the pages are adjacent.
		 * Note: this will never happen on the first iteration.
		 */
		if (dc->dm_chain[seg].dc_addr == dmaend
#if defined(HP320)
		    /* only 16-bit count on 98620B */
		    && (sc->sc_type != DMA_B ||
			dc->dm_chain[seg - 1].dc_count + tcount <= 65536)
#endif
		) {
#ifdef DEBUG
			dmahits[unit]++;
#endif
			dmaend += dc->dm_chain[seg].dc_count;
			dc->dm_chain[--seg].dc_count += tcount;
		} else {
#ifdef DEBUG
			dmamisses[unit]++;
#endif
			dmaend = dc->dm_chain[seg].dc_addr +
			    dc->dm_chain[seg].dc_count;
			dc->dm_chain[seg].dc_count = tcount;
		}
	}
	dc->dm_cur = 0;
	dc->dm_last = --seg;
	dc->dm_flags = 0;
	/*
	 * Set up the command word based on flags
	 */
	dc->dm_cmd = DMA_ENAB | DMA_IPL(sc->sc_ipl) | DMA_START;
	if ((flags & DMAGO_READ) == 0)
		dc->dm_cmd |= DMA_WRT;
	if (flags & DMAGO_LWORD)
		dc->dm_cmd |= DMA_LWORD;
	else if (flags & DMAGO_WORD)
		dc->dm_cmd |= DMA_WORD;
	if (flags & DMAGO_PRI)
		dc->dm_cmd |= DMA_PRI;

#if defined(M68040)
	/*
	 * On the 68040 we need to flush (push) the data cache before a
	 * DMA (already done above) and flush again after DMA completes.
	 * In theory we should only need to flush prior to a write DMA
	 * and purge after a read DMA but if the entire page is not
	 * involved in the DMA we might purge some valid data.
	 */
	if (mmutype == MMU_68040 && (flags & DMAGO_READ))
		dc->dm_flags |= DMAF_PCFLUSH;
#endif

#if defined(CACHE_HAVE_PAC)
	/*
	 * Remember if we need to flush external physical cache when
	 * DMA is done.  We only do this if we are reading (writing memory).
	 */
	if (ectype == EC_PHYS && (flags & DMAGO_READ))
		dc->dm_flags |= DMAF_PCFLUSH;
#endif

#if defined(CACHE_HAVE_VAC)
	if (ectype == EC_VIRT && (flags & DMAGO_READ))
		dc->dm_flags |= DMAF_VCFLUSH;
#endif

	/*
	 * Remember if we can skip the dma completion interrupt on
	 * the last segment in the chain.
	 */
	if (flags & DMAGO_NOINT) {
		if (dc->dm_cur == dc->dm_last)
			dc->dm_cmd &= ~DMA_ENAB;
		else
			dc->dm_flags |= DMAF_NOINTR;
	}
#ifdef DEBUG
	if (dmadebug & DDB_IO) {
		if (((dmadebug&DDB_WORD) && (dc->dm_cmd&DMA_WORD)) ||
		    ((dmadebug&DDB_LWORD) && (dc->dm_cmd&DMA_LWORD))) {
			printf("dmago: cmd %x, flags %x\n",
			    dc->dm_cmd, dc->dm_flags);
			for (seg = 0; seg <= dc->dm_last; seg++)
				printf("  %d: %d@%p\n", seg,
				    dc->dm_chain[seg].dc_count,
				    dc->dm_chain[seg].dc_addr);
		}
	}
	dmatimo[unit] = 1;
#endif
	DMA_ARM(sc, dc);
}

void
dmastop(int unit)
{
	struct dma_softc *sc = dma_softc;
	struct dma_channel *dc = &sc->sc_chan[unit];

#ifdef DEBUG
	if (dmadebug & DDB_FOLLOW)
		printf("dmastop(%d)\n", unit);
	dmatimo[unit] = 0;
#endif
	DMA_CLEAR(dc);

#if defined(CACHE_HAVE_PAC) || defined(M68040)
	if (dc->dm_flags & DMAF_PCFLUSH) {
		PCIA();
		dc->dm_flags &= ~DMAF_PCFLUSH;
	}
#endif

#if defined(CACHE_HAVE_VAC)
	if (dc->dm_flags & DMAF_VCFLUSH) {
		/*
		 * 320/350s have VACs that may also need flushing.
		 * In our case we only flush the supervisor side
		 * because we know that if we are DMAing to user
		 * space, the physical pages will also be mapped
		 * in kernel space (via vmapbuf) and hence cache-
		 * inhibited by the pmap module due to the multiple
		 * mapping.
		 */
		DCIS();
		dc->dm_flags &= ~DMAF_VCFLUSH;
	}
#endif

	/*
	 * We may get this interrupt after a device service routine
	 * has freed the dma channel.  So, ignore the intr if there's
	 * nothing on the queue.
	 */
	if (dc->dm_job != NULL)
		(*dc->dm_job->dq_done)(dc->dm_job->dq_softc);
}

static int
dmaintr(void *arg)
{
	struct dma_softc *sc = arg;
	struct dma_channel *dc;
	int i, stat;
	int found = 0;

#ifdef DEBUG
	if (dmadebug & DDB_FOLLOW)
		printf("dmaintr\n");
#endif
	for (i = 0; i < NDMACHAN; i++) {
		dc = &sc->sc_chan[i];
		stat = DMA_STAT(dc);
		if ((stat & DMA_INTR) == 0)
			continue;
		found++;
#ifdef DEBUG
		if (dmadebug & DDB_IO) {
			if (((dmadebug&DDB_WORD) && (dc->dm_cmd&DMA_WORD)) ||
			    ((dmadebug&DDB_LWORD) && (dc->dm_cmd&DMA_LWORD)))
			 	printf("dmaintr: flags %x unit %d stat %x "
				    "next %d\n",
				    dc->dm_flags, i, stat, dc->dm_cur + 1);
		}
		if (stat & DMA_ARMED)
			printf("dma channel %d: intr when armed\n", i);
#endif
		/*
		 * Load the next segment, or finish up if we're done.
		 */
		dc->dm_cur++;
		if (dc->dm_cur <= dc->dm_last) {
#ifdef DEBUG
			dmatimo[i] = 1;
#endif
			/*
			 * If we're the last segment, disable the
			 * completion interrupt, if necessary.
			 */
			if (dc->dm_cur == dc->dm_last &&
			    (dc->dm_flags & DMAF_NOINTR))
				dc->dm_cmd &= ~DMA_ENAB;
			DMA_CLEAR(dc);
			DMA_ARM(sc, dc);
		} else
			dmastop(i);
	}
	return found;
}

#ifdef DEBUG
static void
dmatimeout(void *arg)
{
	int i, s;
	struct dma_softc *sc = arg;

	for (i = 0; i < NDMACHAN; i++) {
		s = splbio();
		if (dmatimo[i]) {
			if (dmatimo[i] > 1)
				printf("dma channel %d timeout #%d\n",
				    i, dmatimo[i]-1);
			dmatimo[i]++;
		}
		splx(s);
	}
	callout_reset(&sc->sc_debug_ch, 30 * hz, dmatimeout, sc);
}
#endif
