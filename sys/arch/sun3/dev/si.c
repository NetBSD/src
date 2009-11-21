/*	$NetBSD: si.c,v 1.63 2009/11/21 04:16:52 rmind Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, and Gordon W. Ross.
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
 * This file contains only the machine-dependent parts of the
 * Sun3 SCSI driver.  (Autoconfig stuff and DMA functions.)
 * The machine-independent parts are in ncr5380sbc.c
 *
 * Supported hardware includes:
 * Sun SCSI-3 on OBIO (Sun3/50,Sun3/60)
 * Sun SCSI-3 on VME (Sun3/160,Sun3/260)
 *
 * Could be made to support the Sun3/E if someone wanted to.
 *
 * Note:  Both supported variants of the Sun SCSI-3 adapter have
 * some really unusual "features" for this driver to deal with,
 * generally related to the DMA engine.  The OBIO variant will
 * ignore any attempt to write the FIFO count register while the
 * SCSI bus is in DATA_IN or DATA_OUT phase.  This is dealt with
 * by setting the FIFO count early in COMMAND or MSG_IN phase.
 *
 * The VME variant has a bit to enable or disable the DMA engine,
 * but that bit also gates the interrupt line from the NCR5380!
 * Therefore, in order to get any interrupt from the 5380, (i.e.
 * for reselect) one must clear the DMA engine transfer count and
 * then enable DMA.  This has the further complication that you
 * CAN NOT touch the NCR5380 while the DMA enable bit is set, so
 * we have to turn DMA back off before we even look at the 5380.
 *
 * What wonderfully whacky hardware this is!
 *
 * Credits, history:
 *
 * David Jones wrote the initial version of this module, which
 * included support for the VME adapter only. (no reselection).
 *
 * Gordon Ross added support for the OBIO adapter, and re-worked
 * both the VME and OBIO code to support disconnect/reselect.
 * (Required figuring out the hardware "features" noted above.)
 *
 * The autoconfiguration boilerplate came from Adam Glass.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: si.c,v 1.63 2009/11/21 04:16:52 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/dvma.h>

/* #define DEBUG XXX */

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include "sireg.h"
#include "sivar.h"

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth DMA overhead)
 */
#define	MIN_DMA_LEN 128

int si_debug = 0;
#ifdef	DEBUG
#endif

/* How long to wait for DMA before declaring an error. */
int si_dma_intr_timo = 500;	/* ticks (sec. X 100) */

static void	si_minphys(struct buf *);

/*
 * New-style autoconfig attachment. The cfattach
 * structures are in si_obio.c and si_vme.c
 */

void 
si_attach(struct si_softc *sc)
{
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	volatile struct si_regs *regs = sc->sc_regs;
	int i;

	/*
	 * Support the "options" (config file flags).
	 * Disconnect/reselect is a per-target mask.
	 * Interrupts and DMA are per-controller.
	 */
	ncr_sc->sc_no_disconnect =
	    (sc->sc_options & SI_NO_DISCONNECT);
	ncr_sc->sc_parity_disable = 
	    (sc->sc_options & SI_NO_PARITY_CHK) >> 8;
	if (sc->sc_options & SI_FORCE_POLLING)
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;

#if 1	/* XXX - Temporary */
	/* XXX - In case we think DMA is completely broken... */
	if (sc->sc_options & SI_DISABLE_DMA) {
		/* Override this function pointer. */
		ncr_sc->sc_dma_alloc = NULL;
	}
#endif
	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * Initialize fields used by the MI code
	 */
	ncr_sc->sci_r0 = &regs->sci.sci_r0;
	ncr_sc->sci_r1 = &regs->sci.sci_r1;
	ncr_sc->sci_r2 = &regs->sci.sci_r2;
	ncr_sc->sci_r3 = &regs->sci.sci_r3;
	ncr_sc->sci_r4 = &regs->sci.sci_r4;
	ncr_sc->sci_r5 = &regs->sci.sci_r5;
	ncr_sc->sci_r6 = &regs->sci.sci_r6;
	ncr_sc->sci_r7 = &regs->sci.sci_r7;

	ncr_sc->sc_rev = NCR_VARIANT_NCR5380;

	/*
	 * Allocate DMA handles.
	 */
	i = SCI_OPENINGS * sizeof(struct si_dma_handle);
	sc->sc_dma = (struct si_dma_handle *)
		malloc(i, M_DEVBUF, M_WAITOK);
	if (sc->sc_dma == NULL)
		panic("si: dvma_malloc failed");
	for (i = 0; i < SCI_OPENINGS; i++)
		sc->sc_dma[i].dh_flags = 0;

	ncr_sc->sc_channel.chan_id = 7;
	ncr_sc->sc_adapter.adapt_minphys = si_minphys;

	/*
	 *  Initialize si board itself.
	 */
	ncr5380_attach(ncr_sc);
}

static void
si_minphys(struct buf *bp)
{

	if (bp->b_bcount > MAX_DMA_LEN) {
#ifdef	DEBUG
		if (si_debug) {
			printf("%s len = 0x%x.\n", __func__, bp->b_bcount);
			Debugger();
		}
#endif
		bp->b_bcount = MAX_DMA_LEN;
	}
	minphys(bp);
}


#define CSR_WANT (SI_CSR_SBC_IP | SI_CSR_DMA_IP | \
	SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR )

int
si_intr(void *arg)
{
	struct si_softc *sc = arg;
	volatile struct si_regs *si = sc->sc_regs;
	int dma_error, claimed;
	u_short csr;

	claimed = 0;
	dma_error = 0;

	/* SBC interrupt? DMA interrupt? */
	csr = si->si_csr;
	NCR_TRACE("si_intr: csr=0x%x\n", csr);

	if (csr & SI_CSR_DMA_CONFLICT) {
		dma_error |= SI_CSR_DMA_CONFLICT;
		printf("%s: DMA conflict\n", __func__);
	}
	if (csr & SI_CSR_DMA_BUS_ERR) {
		dma_error |= SI_CSR_DMA_BUS_ERR;
		printf("%s: DMA bus error\n", __func__);
	}
	if (dma_error) {
		if (sc->ncr_sc.sc_state & NCR_DOINGDMA)
			sc->ncr_sc.sc_state |= NCR_ABORTING;
		/* Make sure we will call the main isr. */
		csr |= SI_CSR_DMA_IP;
	}

	if (csr & (SI_CSR_SBC_IP | SI_CSR_DMA_IP)) {
		claimed = ncr5380_intr(&sc->ncr_sc);
#ifdef	DEBUG
		if (!claimed) {
			printf("%s: spurious from SBC\n", __func__);
			if (si_debug & 4)
				Debugger();	/* XXX */
		}
#endif
		/* Yes, we DID cause this interrupt. */
		claimed = 1;
	}

	return claimed;
}


/*****************************************************************
 * Common functions for DMA
 ****************************************************************/

/*
 * Allocate a DMA handle and put it in sc->sc_dma.  Prepare
 * for DMA transfer.  On the Sun3, this means mapping the buffer
 * into DVMA space.  dvma_mapin() flushes the cache for us.
 */
void 
si_dma_alloc(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsipi_xfer *xs = sr->sr_xs;
	struct si_dma_handle *dh;
	int i, xlen;
	void *addr;

#ifdef	DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("%s: already have DMA handle", __func__);
#endif

	addr = ncr_sc->sc_dataptr;
	xlen = ncr_sc->sc_datalen;

	/* If the DMA start addr is misaligned then do PIO */
	if (((vaddr_t)addr & 1) || (xlen & 1)) {
		printf("%s: misaligned.\n", __func__);
		return;
	}

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("%s: xlen=0x%x", __func__, xlen);

	/*
	 * Never attempt single transfers of more than 63k, because
	 * our count register may be only 16 bits (an OBIO adapter).
	 * This should never happen since already bounded by minphys().
	 * XXX - Should just segment these...
	 */
	if (xlen > MAX_DMA_LEN) {
		printf("%s: excessive xlen=0x%x\n", __func__, xlen);
		Debugger();
		ncr_sc->sc_datalen = xlen = MAX_DMA_LEN;
	}

	/* Find free DMA handle.  Guaranteed to find one since we have
	   as many DMA handles as the driver has processes. */
	for (i = 0; i < SCI_OPENINGS; i++) {
		if ((sc->sc_dma[i].dh_flags & SIDH_BUSY) == 0)
			goto found;
	}
	panic("si: no free DMA handles.");
found:

	dh = &sc->sc_dma[i];
	dh->dh_flags = SIDH_BUSY;

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, addr, xlen, NULL,
	    BUS_DMA_NOWAIT) != 0)
		panic("%s: can't load dmamap", device_xname(ncr_sc->sc_dev));
	dh->dh_dmaaddr = sc->sc_dmap->dm_segs[0].ds_addr;
	dh->dh_dmalen  = xlen;

	/* Copy the "write" flag for convenience. */
	if (xs->xs_control & XS_CTL_DATA_OUT)
		dh->dh_flags |= SIDH_OUT;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, dh->dh_dmalen,
	    (dh->dh_flags & SIDH_OUT) == 0 ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

#if 0
	/*
	 * Some machines might not need to remap B_PHYS buffers.
	 * The sun3 does not map B_PHYS buffers into DVMA space,
	 * (they are mapped into normal KV space) so on the sun3
	 * we must always remap to a DVMA address here. Re-map is
	 * cheap anyway, because it's done by segments, not pages.
	 */
	if (xs->bp && (xs->bp->b_flags & B_PHYS))
		dh->dh_flags |= SIDH_PHYS;
#endif

	/* success */
	sr->sr_dma_hand = dh;

	return;
}


void 
si_dma_free(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;

#ifdef	DIAGNOSTIC
	if (dh == NULL)
		panic("%s: no DMA handle", __func__);
#endif

	if (ncr_sc->sc_state & NCR_DOINGDMA)
		panic("%s: free while in progress", __func__);

	if (dh->dh_flags & SIDH_BUSY) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, dh->dh_dmalen,
		    (dh->dh_flags & SIDH_OUT) == 0 ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
		dh->dh_dmaaddr = 0;
		dh->dh_flags = 0;
	}
	sr->sr_dma_hand = NULL;
}


#define	CSR_MASK (SI_CSR_SBC_IP | SI_CSR_DMA_IP | \
		SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR)
#define	POLL_TIMO	50000	/* X100 = 5 sec. */

/*
 * Poll (spin-wait) for DMA completion.
 * Called right after xx_dma_start(), and
 * xx_dma_stop() will be called next.
 * Same for either VME or OBIO.
 */
void 
si_dma_poll(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	volatile struct si_regs *si = sc->sc_regs;
	int tmo;

	/* Make sure DMA started successfully. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		return;

	/*
	 * XXX: The Sun driver waits for ~SI_CSR_DMA_ACTIVE here
	 * XXX: (on obio) or even worse (on vme) a 10mS. delay!
	 * XXX: I really doubt that is necessary...
	 */

	/* Wait for any "DMA complete" or error bits. */
	tmo = POLL_TIMO;
	for (;;) {
		if (si->si_csr & CSR_MASK)
			break;
		if (--tmo <= 0) {
			printf("si: DMA timeout (while polling)\n");
			/* Indicate timeout as MI code would. */
			sr->sr_flags |= SR_OVERDUE;
			break;
		}
		delay(100);
	}
	NCR_TRACE("si_dma_poll: waited %d\n",
			  POLL_TIMO - tmo);

#ifdef	DEBUG
	if (si_debug & 2) {
		printf("%s: done, csr=0x%x\n", __func__, si->si_csr);
	}
#endif
}
