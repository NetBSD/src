/* $NetBSD: tcds_dma.c,v 1.28.16.1 2000/06/22 16:58:49 minoura Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tcds_dma.c,v 1.28.16.1 2000/06/22 16:58:49 minoura Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <machine/bus.h>

#include <dev/tc/tcvar.h>
#include <alpha/tc/tcdsreg.h>
#include <alpha/tc/tcdsvar.h>
#include <alpha/tc/ascvar.h>

void
tcds_dma_reset(sc)
	struct tcds_slotconfig *sc;
{
	/* TCDS SCSI disable/reset/enable. */
	tcds_scsi_reset(sc);			/* XXX */

	sc->sc_active = 0;			/* and of course we aren't */
}

int
tcds_dma_isintr(sc)
	struct tcds_slotconfig *sc;
{
	int x;

	x = tcds_scsi_isintr(sc, 1);

	/* XXX */
	return x;
}

/*
 * Pseudo (chained) interrupt from the asc driver to kick the
 * current running DMA transfer. I am relying on ascintr() to
 * pickup and clean errors for now
 *
 * return 1 if it was a DMA continue.
 */
int
tcds_dma_intr(sc)
	struct tcds_slotconfig *sc;
{
	struct ncr53c9x_softc *nsc = &sc->sc_asc->sc_ncr53c9x;
	bus_dmamap_t map = sc->sc_dmamap;
	u_int32_t dud;
	int trans, resid;
	u_int32_t *addr, dudmask;
	u_int8_t tcl, tcm, tch;
	bus_addr_t pa;

	NCR_DMA(("tcds_dma %d: intr", sc->sc_slot));

	if (tcds_scsi_iserr(sc))
		return (0);

	/* This is an "assertion" :) */
	if (sc->sc_active == 0)
		panic("tcds_dma_intr: DMA wasn't active");

	/* DMA has stopped */
	tcds_dma_enable(sc, 0);
	sc->sc_active = 0;

	if (sc->sc_dmasize == 0) {
		/* A "Transfer Pad" operation completed */
		tcl = NCR_READ_REG(nsc, NCR_TCL);
		tcm = NCR_READ_REG(nsc, NCR_TCM);
		NCR_DMA(("tcds_dma_intr: discarded %d bytes (tcl=%d, tcm=%d)\n",
		    tcl | (tcm << 8), tcl, tcm));
		return 0;
	}

	resid = 0;
	if (!sc->sc_ispullup &&
	    (resid = (NCR_READ_REG(nsc, NCR_FFLAG) & NCRFIFO_FF)) != 0) {
		NCR_DMA(("dmaintr: empty esp FIFO of %d ", resid));
		DELAY(1);
	}

	resid += (tcl = NCR_READ_REG(nsc, NCR_TCL));
	resid += (tcm = NCR_READ_REG(nsc, NCR_TCM)) << 8;
	if (nsc->sc_rev == NCR_VARIANT_ESP200)
		resid += (tch = NCR_READ_REG(nsc, NCR_TCH)) << 16;
	else
		tch = 0;

	if (resid == 0 && (nsc->sc_rev <= NCR_VARIANT_ESP100A) &&
	    (nsc->sc_espstat & NCRSTAT_TC) == 0)
		resid = 65536;

	trans = sc->sc_dmasize - resid;
	if (trans < 0) {			/* transferred < 0 ? */
		printf("tcds_dma %d: xfer (%d) > req (%d)\n",
		    sc->sc_slot, trans, (int)sc->sc_dmasize);
		trans = sc->sc_dmasize;
	}

	NCR_DMA(("tcds_dma_intr: tcl=%d, tcm=%d, tch=%d; trans=%d, resid=%d\n",
	    tcl, tcm, tch, trans, resid));

	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    (sc->sc_ispullup ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE));

	/*
	 * Clean up unaligned DMAs into main memory.
	 */
	if (sc->sc_ispullup) {
		/* Handle unaligned starting address, length. */
		dud = bus_space_read_4(sc->sc_bst, sc->sc_bsh, sc->sc_dud0);
		if ((dud & TCDS_DUD0_VALIDBITS) != 0) {
			addr = (u_int32_t *)
			    ((paddr_t)*sc->sc_dmaaddr & ~0x3);
			dudmask = 0;
			if (dud & TCDS_DUD0_VALID00)
				panic("tcds_dma: dud0 byte 0 valid");
			if (dud & TCDS_DUD0_VALID01)
				dudmask |= TCDS_DUD_BYTE01;
			if (dud & TCDS_DUD0_VALID10)
				dudmask |= TCDS_DUD_BYTE10;
#ifdef DIAGNOSTIC
			if (dud & TCDS_DUD0_VALID11)
				dudmask |= TCDS_DUD_BYTE11;
#endif
			NCR_DMA(("dud0 at 0x%p dudmask 0x%x\n",
			    addr, dudmask));
			*addr = (*addr & ~dudmask) | (dud & dudmask);
		}
		dud = bus_space_read_4(sc->sc_bst, sc->sc_bsh, sc->sc_dud1);
		if ((dud & TCDS_DUD1_VALIDBITS) != 0) {
			pa = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
			    sc->sc_sda) << 2;
			dudmask = 0;
			if (dud & TCDS_DUD1_VALID00)
				dudmask |= TCDS_DUD_BYTE00;
			if (dud & TCDS_DUD1_VALID01)
				dudmask |= TCDS_DUD_BYTE01;
			if (dud & TCDS_DUD1_VALID10)
				dudmask |= TCDS_DUD_BYTE10;
#ifdef DIAGNOSTIC
			if (dud & TCDS_DUD1_VALID11)
				panic("tcds_dma: dud1 byte 3 valid");
#endif
			NCR_DMA(("dud1 at 0x%lx dudmask 0x%x\n",
			    pa, dudmask));
			/* XXX Fix TC_PHYS_TO_UNCACHED() */
#if defined(__alpha__)
			addr = (u_int32_t *)ALPHA_PHYS_TO_K0SEG(pa);
#elif defined(__mips__)
			addr = (u_int32_t *)MIPS_PHYS_TO_KSEG1(pa);
#else
#error TurboChannel only exists on DECs, folks...
#endif
			*addr = (*addr & ~dudmask) | (dud & dudmask);
		}
		/* XXX deal with saved residual byte? */
	}

	bus_dmamap_unload(sc->sc_dmat, map);

	*sc->sc_dmalen -= trans;
	*sc->sc_dmaaddr += trans;

#if 0 /* this is not normal operation just yet */
	if (*sc->sc_dmalen == 0 ||
	    nsc->sc_phase != nsc->sc_prevphase)
		return 0;

	/* and again */
	dma_start(sc, sc->sc_dmaaddr, sc->sc_dmalen, sc->sc_ispullup);
	return 1;
#endif
	return 0;
}

#define DMAMAX(a)	(0x02000 - ((a) & 0x1fff))

/*
 * start a dma transfer or keep it going
 */
int
tcds_dma_setup(sc, addr, len, datain, dmasize)
	struct tcds_slotconfig *sc;
	caddr_t *addr;
	size_t *len, *dmasize;
	int datain;				/* DMA into main memory */
{
	bus_dmamap_t map = sc->sc_dmamap;
	u_int32_t dic;
	size_t size;

	sc->sc_dmaaddr = addr;
	sc->sc_dmalen = len;
	sc->sc_ispullup = datain;

	NCR_DMA(("tcds_dma %d: start %d@%p,%d\n", sc->sc_slot,
		(int)*sc->sc_dmalen, *sc->sc_dmaaddr, sc->sc_ispullup));

	/*
	 * the rules say we cannot transfer more than the limit
	 * of this DMA chip (64k) and we cannot cross a 8k boundary.
	 */

	size = min(*dmasize, DMAMAX((size_t) *sc->sc_dmaaddr));
	*dmasize = sc->sc_dmasize = size;

	NCR_DMA(("dma_start: dmasize = %d\n", (int)sc->sc_dmasize));

	if (bus_dmamap_load(sc->sc_dmat, map, *addr, size,
	    NULL, BUS_DMA_NOWAIT)) {
		/*
		 * XXX Should return an error, here, but the upper-layer
		 * XXX doesn't check the return value!
		 */
		panic("tcds_dma_setup: dmamap load failed");
	}

	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    (sc->sc_ispullup ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE));

	/* Load address, set/clear unaligned transfer and read/write bits. */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, sc->sc_sda,
	    map->dm_segs[0].ds_addr >> 2);
	dic = bus_space_read_4(sc->sc_bst, sc->sc_bsh, sc->sc_dic);
	dic &= ~TCDS_DIC_ADDRMASK;
	dic |= map->dm_segs[0].ds_addr & TCDS_DIC_ADDRMASK;
	if (datain)
		dic |= TCDS_DIC_WRITE;
	else
		dic &= ~TCDS_DIC_WRITE;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, sc->sc_dic, dic);

	return (0);
}

void
tcds_dma_go(sc)
	struct tcds_slotconfig *sc;
{

	/* mark unit as DMA-active */
	sc->sc_active = 1;

	/* Start DMA */
	tcds_dma_enable(sc, 1);
}

int
tcds_dma_isactive(sc)
	struct tcds_slotconfig *sc;
{

	return (sc->sc_active);
}

int
tcds_dma_init(sc)
	struct tcds_slotconfig *sc;
{

	/*
	 * The TCDS ASIC cannot DMA across 8k boundaries, and this
	 * driver is written such that each DMA segment gets a new
	 * call to tcds_dma_setup().  Thus, the DMA map only needs
	 * to support 8k transfers.
	 */
	return (bus_dmamap_create(sc->sc_dmat, 0x2000, 1, 0x2000,
	    0x2000, BUS_DMA_NOWAIT, &sc->sc_dmamap));
}
