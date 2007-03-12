/*	$NetBSD: ncr.c,v 1.41.26.1 2007/03/12 05:51:35 rmind Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, Gordon W. Ross, and Jens A. Nilsson.
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
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
 * This file contains the machine-dependent parts of the NCR-5380
 * controller. The machine-independent parts are in ncr5380sbc.c.
 *
 * Jens A. Nilsson.
 *
 * Credits:
 * 
 * This code is based on arch/sun3/dev/si*
 * Written by David Jones, Gordon Ross, and Adam Glass.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ncr.c,v 1.41.26.1 2007/03/12 05:51:35 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/cpu.h>
#include <machine/vsbus.h>
#include <machine/bus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/clock.h>

#include "ioconf.h"

#define MIN_DMA_LEN 128

struct si_dma_handle {
	int	dh_flags;
#define SIDH_BUSY	1
#define SIDH_OUT	2
	void *dh_addr;
	int	dh_len;
	struct	proc *dh_proc;
};

struct si_softc {
	struct	ncr5380_softc	ncr_sc;
	struct	evcnt		ncr_intrcnt;
	void *ncr_addr;
	int	ncr_off;
	int	ncr_dmaaddr;
	int	ncr_dmacount;
	int	ncr_dmadir;
	struct	si_dma_handle ncr_dma[SCI_OPENINGS];
	struct	vsbus_dma sc_vd;
	int	onlyscsi;	/* This machine needs no queueing */
};

static int ncr_dmasize;

static	int si_vsbus_match(struct device *, struct cfdata *, void *);
static	void si_vsbus_attach(struct device *, struct device *, void *);
static	void si_minphys(struct buf *);

static	void si_dma_alloc(struct ncr5380_softc *);
static	void si_dma_free(struct ncr5380_softc *);
static	void si_dma_setup(struct ncr5380_softc *);
static	void si_dma_start(struct ncr5380_softc *);
static	void si_dma_poll(struct ncr5380_softc *);
static	void si_dma_eop(struct ncr5380_softc *);
static	void si_dma_stop(struct ncr5380_softc *);
static	void si_dma_go(void *);

CFATTACH_DECL(si_vsbus, sizeof(struct si_softc),
    si_vsbus_match, si_vsbus_attach, NULL, NULL);

static int
si_vsbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vsbus_attach_args *va = aux;
	volatile char *si_csr = (char *) va->va_addr;

	if (vax_boardtype == VAX_BTYP_49 || vax_boardtype == VAX_BTYP_46
	    || vax_boardtype == VAX_BTYP_48 || vax_boardtype == VAX_BTYP_53)
		return 0;
	/* This is the way Linux autoprobes the interrupt MK-990321 */
	si_csr[12] = 0;
	si_csr[16] = 0x80;
	si_csr[0] = 0x80;
	si_csr[4] = 5; /* 0xcf */
	DELAY(100000);
	return 1;
}

static void
si_vsbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct si_softc *sc = (struct si_softc *) self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	int tweak, target;

	scb_vecalloc(va->va_cvec, (void (*)(void *)) ncr5380_intr, sc,
		SCB_ISTACK, &sc->ncr_intrcnt);
	evcnt_attach_dynamic(&sc->ncr_intrcnt, EVCNT_TYPE_INTR, NULL,
		self->dv_xname, "intr");

	/*
	 * DMA area mapin.
	 * On VS3100, split the 128K block between the two devices.
	 * On VS2000, don't care for now.
	 */
#define DMASIZE (64*1024)
	if (va->va_paddr & 0x100) { /* Secondary SCSI controller */
		sc->ncr_off = DMASIZE;
		sc->onlyscsi = 1;
	}
	sc->ncr_addr = (void *)va->va_dmaaddr;
	ncr_dmasize = min(va->va_dmasize, MAXPHYS);

	/*
	 * MD function pointers used by the MI code.
	 */
	ncr_sc->sc_dma_alloc = si_dma_alloc;
	ncr_sc->sc_dma_free  = si_dma_free;
	ncr_sc->sc_dma_setup = si_dma_setup;
	ncr_sc->sc_dma_start = si_dma_start;
	ncr_sc->sc_dma_poll  = si_dma_poll;
	ncr_sc->sc_dma_eop   = si_dma_eop;
	ncr_sc->sc_dma_stop  = si_dma_stop;

	/* DMA control register offsets */
	sc->ncr_dmaaddr = 32;	/* DMA address in buffer, longword */
	sc->ncr_dmacount = 64;	/* DMA count register */
	sc->ncr_dmadir = 68;	/* Direction of DMA transfer */

	ncr_sc->sc_pio_out = ncr5380_pio_out;
	ncr_sc->sc_pio_in =  ncr5380_pio_in;

	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * Initialize fields used by the MI code.
	 */
/*	ncr_sc->sc_regt =  Unused on VAX */
	ncr_sc->sc_regh = vax_map_physmem(va->va_paddr, 1);

	/* Register offsets */
	ncr_sc->sci_r0 = 0;
	ncr_sc->sci_r1 = 4;
	ncr_sc->sci_r2 = 8;
	ncr_sc->sci_r3 = 12;
	ncr_sc->sci_r4 = 16;
	ncr_sc->sci_r5 = 20;
	ncr_sc->sci_r6 = 24;
	ncr_sc->sci_r7 = 28;

	ncr_sc->sc_rev = NCR_VARIANT_NCR5380;

	ncr_sc->sc_no_disconnect = 0xff;

	/*
	 * Get the SCSI chip target address out of NVRAM.
	 * This do not apply to the VS2000.
	 */
	tweak = clk_tweak + (va->va_paddr & 0x100 ? 3 : 0);
	if (vax_boardtype == VAX_BTYP_410)
		target = 7;
	else
		target = (clk_page[0xbc/2] >> tweak) & 7;

	printf("\n%s: NCR5380, SCSI ID %d\n", ncr_sc->sc_dev.dv_xname, target);

	ncr_sc->sc_adapter.adapt_minphys = si_minphys;
	ncr_sc->sc_channel.chan_id = target;

	/*
	 * Init the vsbus DMA resource queue struct */
	sc->sc_vd.vd_go = si_dma_go;
	sc->sc_vd.vd_arg = sc;

	/*
	 * Initialize si board itself.
	 */
	ncr5380_attach(ncr_sc);
}

/*
 * Adjust the max transfer size. The DMA buffer is only 16k on VS2000.
 */
static void
si_minphys(struct buf *bp)
{
	if (bp->b_bcount > ncr_dmasize)
		bp->b_bcount = ncr_dmasize;
}

void
si_dma_alloc(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsipi_xfer *xs = sr->sr_xs;
	struct si_dma_handle *dh;
	int xlen, i;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("si_dma_alloc: already have DMA handle");
#endif

	/* Polled transfers shouldn't allocate a DMA handle. */
	if (sr->sr_flags & SR_IMMED)
		return;

	xlen = ncr_sc->sc_datalen;

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("si_dma_alloc: len=0x%x", xlen);

	/*
	 * Find free PDMA handle.  Guaranteed to find one since we
	 * have as many PDMA handles as the driver has processes.
	 * (instances?)
	 */
	 for (i = 0; i < SCI_OPENINGS; i++) {
		if ((sc->ncr_dma[i].dh_flags & SIDH_BUSY) == 0)
			goto found;
	}
	panic("sbc: no free PDMA handles");
found:
	dh = &sc->ncr_dma[i];
	dh->dh_flags = SIDH_BUSY;
	dh->dh_addr = ncr_sc->sc_dataptr;
	dh->dh_len = xlen;
	dh->dh_proc = xs->bp->b_proc;

	/* Remember dest buffer parameters */
	if (xs->xs_control & XS_CTL_DATA_OUT)
		dh->dh_flags |= SIDH_OUT;

	sr->sr_dma_hand = dh;
}

void
si_dma_free(struct ncr5380_softc *ncr_sc)
{
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;

	if (dh->dh_flags & SIDH_BUSY)
		dh->dh_flags = 0;
	else
		printf("si_dma_free: free'ing unused buffer\n");

	sr->sr_dma_hand = NULL;
}

void
si_dma_setup(struct ncr5380_softc *ncr_sc)
{
	/* Do nothing here */
}

void
si_dma_start(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;

	/* Just put on queue; will call go() from below */
	if (sc->onlyscsi)
		si_dma_go(ncr_sc);
	else
		vsbus_dma_start(&sc->sc_vd);
}

/*
 * go() routine called when another transfer somewhere is finished.
 */
void
si_dma_go(void *arg)
{
	struct ncr5380_softc *ncr_sc = arg;
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;

	/*
	 * Set the VAX-DMA-specific registers, and copy the data if
	 * it is directed "outbound".
	 */
	if (dh->dh_flags & SIDH_OUT) {
		vsbus_copyfromproc(dh->dh_proc, dh->dh_addr,
		    (char *)sc->ncr_addr + sc->ncr_off, dh->dh_len);
		bus_space_write_1(ncr_sc->sc_regt, ncr_sc->sc_regh,
		    sc->ncr_dmadir, 0);
	} else {
		bus_space_write_1(ncr_sc->sc_regt, ncr_sc->sc_regh,
		    sc->ncr_dmadir, 1);
	}
	bus_space_write_4(ncr_sc->sc_regt, ncr_sc->sc_regh,
	    sc->ncr_dmacount, -dh->dh_len);
	bus_space_write_4(ncr_sc->sc_regt, ncr_sc->sc_regh,
	    sc->ncr_dmaaddr, sc->ncr_off);
	/*
	 * Now from the 5380-internal DMA registers.
	 */
	if (dh->dh_flags & SIDH_OUT) {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_OUT);
		NCR5380_WRITE(ncr_sc, sci_icmd, SCI_ICMD_DATA);
		NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode)
		    | SCI_MODE_DMA | SCI_MODE_DMA_IE);
		NCR5380_WRITE(ncr_sc, sci_dma_send, 0);
	} else {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_IN);
		NCR5380_WRITE(ncr_sc, sci_icmd, 0);
		NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode)
		    | SCI_MODE_DMA | SCI_MODE_DMA_IE);
		NCR5380_WRITE(ncr_sc, sci_irecv, 0);
	}
	ncr_sc->sc_state |= NCR_DOINGDMA;
}

/*
 * When?
 */
void
si_dma_poll(struct ncr5380_softc *ncr_sc)
{
	printf("si_dma_poll\n");
}

/*
 * When?
 */
void
si_dma_eop(struct ncr5380_softc *ncr_sc)
{
	printf("si_dma_eop\n");
}

void
si_dma_stop(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;
	int count, i;

	if (ncr_sc->sc_state & NCR_DOINGDMA) 
		ncr_sc->sc_state &= ~NCR_DOINGDMA;

	/*
	 * Sometimes the FIFO buffer isn't drained when the
	 * interrupt is posted. Just loop here and hope that
	 * it will drain soon.
	 */
	for (i = 0; i < 20000; i++) {
		count = bus_space_read_4(ncr_sc->sc_regt,
		    ncr_sc->sc_regh, sc->ncr_dmacount);
		if (count == 0)
			break;
		DELAY(100);
	}
	if (count == 0) {
		if (((dh->dh_flags & SIDH_OUT) == 0)) {
			vsbus_copytoproc(dh->dh_proc,
			    (char *)sc->ncr_addr + sc->ncr_off,
			    dh->dh_addr, dh->dh_len);
		}
		ncr_sc->sc_dataptr += dh->dh_len;
		ncr_sc->sc_datalen -= dh->dh_len;
	}

	NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode) &
	    ~(SCI_MODE_DMA | SCI_MODE_DMA_IE));
	NCR5380_WRITE(ncr_sc, sci_icmd, 0);
	if (sc->onlyscsi == 0)
		vsbus_dma_intr(); /* Try to start more transfers */
}
