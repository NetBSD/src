/*	$NetBSD: sw.c,v 1.13 2003/07/15 00:04:56 lukem Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, Gordon W. Ross, and Jason R. Thorpe.
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
 * This file contains only the machine-dependent parts of the
 * Sun4 SCSI driver.  (Autoconfig stuff and DMA functions.)
 * The machine-independent parts are in ncr5380sbc.c
 *
 * Supported hardware includes:
 * Sun "SCSI Weird" on OBIO (sw: Sun 4/100-series)
 * Sun SCSI-3 on VME (si: Sun 4/200-series, others)
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
 * David Jones wrote the initial version of this module for NetBSD/sun3,
 * which included support for the VME adapter only. (no reselection).
 *
 * Gordon Ross added support for the Sun 3 OBIO adapter, and re-worked
 * both the VME and OBIO code to support disconnect/reselect.
 * (Required figuring out the hardware "features" noted above.)
 *
 * The autoconfiguration boilerplate came from Adam Glass.
 *
 * Jason R. Thorpe ported the autoconfiguration and VME portions to
 * NetBSD/sparc, and added initial support for the 4/100 "SCSI Weird",
 * a wacky OBIO variant of the VME SCSI-3.  Many thanks to Chuck Cranor
 * for lots of helpful tips and suggestions.  Thanks also to Paul Kranenburg
 * and Chris Torek for bits of insight needed along the way.  Thanks to
 * David Gilbert and Andrew Gillham who risked filesystem life-and-limb
 * for the sake of testing.  Andrew Gillham helped work out the bugs
 * the 4/100 DMA code.
 */

/*
 * NOTE: support for the 4/100 "SCSI Weird" is not complete!  DMA
 * works, but interrupts (and, thus, reselection) don't.  I don't know
 * why, and I don't have a machine to test this on further.
 *
 * DMA, DMA completion interrupts, and reselection work fine on my
 * 4/260 with modern SCSI-II disks attached.  I've had reports of
 * reselection failing on Sun Shoebox-type configurations where
 * there are multiple non-SCSI devices behind Emulex or Adaptec
 * bridges.  These devices pre-date the SCSI-I spec, and might not
 * bahve the way the 5380 code expects.  For this reason, only
 * DMA is enabled by default in this driver.
 *
 *	Jason R. Thorpe <thorpej@NetBSD.ORG>
 *	December 8, 1995
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sw.c,v 1.13 2003/07/15 00:04:56 lukem Exp $");

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#ifndef DDB
#define	Debugger()
#endif

#ifndef DEBUG
#define DEBUG XXX
#endif

#define COUNT_SW_LEFTOVERS	XXX	/* See sw DMA completion code */

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <sparc/dev/swreg.h>

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth DMA overhead)
 */
#define	MIN_DMA_LEN 128

/*
 * Transfers lager than 65535 bytes need to be split-up.
 * (Some of the FIFO logic has only 16 bits counters.)
 * Make the size an integer multiple of the page size
 * to avoid buf/cluster remap problems.  (paranoid?)
 */
#define	MAX_DMA_LEN 0xE000

#ifdef	DEBUG
int sw_debug = 0;
#endif

/*
 * This structure is used to keep track of mapped DMA requests.
 */
struct sw_dma_handle {
	int 		dh_flags;
#define	SIDH_BUSY	0x01		/* This DH is in use */
#define	SIDH_OUT	0x02		/* DMA does data out (write) */
	u_char		*dh_addr;	/* KVA of start of buffer */
	int 		dh_maplen;	/* Original data length */
	long		dh_startingpa;	/* PA of buffer; for "sw" */
	bus_dmamap_t	dh_dmamap;
#define dh_dvma	dh_dmamap->dm_segs[0].ds_addr /* VA of buffer in DVMA space */
};

/*
 * The first structure member has to be the ncr5380_softc
 * so we can just cast to go back and fourth between them.
 */
struct sw_softc {
	struct ncr5380_softc	ncr_sc;
	bus_space_tag_t		sc_bustag;	/* bus tags */
	bus_dma_tag_t		sc_dmatag;

	struct sw_dma_handle *sc_dma;
	int		sc_xlen;	/* length of current DMA segment. */
	int		sc_options;	/* options for this instance. */
};

/*
 * Options.  By default, DMA is enabled and DMA completion interrupts
 * and reselect are disabled.  You may enable additional features
 * the `flags' directive in your kernel's configuration file.
 *
 * Alternatively, you can patch your kernel with DDB or some other
 * mechanism.  The sc_options member of the softc is OR'd with
 * the value in sw_options.
 *
 * On the "sw", interrupts (and thus) reselection don't work, so they're
 * disabled by default.  DMA is still a little dangerous, too.
 *
 * Note, there's a separate sw_options to make life easier.
 */
#define	SW_ENABLE_DMA	0x01	/* Use DMA (maybe polled) */
#define	SW_DMA_INTR	0x02	/* DMA completion interrupts */
#define	SW_DO_RESELECT	0x04	/* Allow disconnect/reselect */
#define	SW_OPTIONS_MASK	(SW_ENABLE_DMA|SW_DMA_INTR|SW_DO_RESELECT)
#define SW_OPTIONS_BITS	"\10\3RESELECT\2DMA_INTR\1DMA"
int sw_options = SW_ENABLE_DMA;

static int	sw_match __P((struct device *, struct cfdata *, void *));
static void	sw_attach __P((struct device *, struct device *, void *));
static int	sw_intr __P((void *));
static void	sw_reset_adapter __P((struct ncr5380_softc *));
static void	sw_minphys __P((struct buf *));

void	sw_dma_alloc __P((struct ncr5380_softc *));
void	sw_dma_free __P((struct ncr5380_softc *));
void	sw_dma_poll __P((struct ncr5380_softc *));

void	sw_dma_setup __P((struct ncr5380_softc *));
void	sw_dma_start __P((struct ncr5380_softc *));
void	sw_dma_eop __P((struct ncr5380_softc *));
void	sw_dma_stop __P((struct ncr5380_softc *));

void	sw_intr_on __P((struct ncr5380_softc *));
void	sw_intr_off __P((struct ncr5380_softc *));

/* Shorthand bus space access */
#define SWREG_READ(sc, index) \
	bus_space_read_4((sc)->sc_regt, (sc)->sc_regh, index)
#define SWREG_WRITE(sc, index, v) \
	bus_space_write_4((sc)->sc_regt, (sc)->sc_regh, index, v)


/* The Sun "SCSI Weird" 4/100 obio controller. */
CFATTACH_DECL(sw, sizeof(struct sw_softc),
    sw_match, sw_attach, NULL, NULL);

static int
sw_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	/* Nothing but a Sun 4/100 is going to have these devices. */
	if (cpuinfo.cpu_type != CPUTYP_4_100)
		return (0);

	if (uoba->uoba_isobio4 == 0)
		return (0);

	/* Make sure there is something there... */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				1,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

static void
sw_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct sw_softc *sc = (struct sw_softc *) self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	bus_space_handle_t bh;
	char bits[64];
	int i;

	sc->sc_dmatag = oba->oba_dmatag;

	/* Map the controller registers. */
	if (bus_space_map(oba->oba_bustag, oba->oba_paddr,
			  SWREG_BANK_SZ,
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}

	ncr_sc->sc_regt = oba->oba_bustag;
	ncr_sc->sc_regh = bh;

	sc->sc_options = sw_options;

	ncr_sc->sc_dma_setup = sw_dma_setup;
	ncr_sc->sc_dma_start = sw_dma_start;
	ncr_sc->sc_dma_eop   = sw_dma_stop;
	ncr_sc->sc_dma_stop  = sw_dma_stop;
	ncr_sc->sc_intr_on   = sw_intr_on;
	ncr_sc->sc_intr_off  = sw_intr_off;

	/*
	 * Establish interrupt channel.
	 * Default interrupt priority always is 3.  At least, that's
	 * what my board seems to be at.  --thorpej
	 */
	if (oba->oba_pri == -1)
		oba->oba_pri = 3;

	(void)bus_intr_establish(oba->oba_bustag, oba->oba_pri, IPL_BIO,
				 sw_intr, sc);

	printf(" pri %d\n", oba->oba_pri);


	/*
	 * Pull in the options flags.  Allow the user to completely
	 * override the default values.
	 */
	if ((ncr_sc->sc_dev.dv_cfdata->cf_flags & SW_OPTIONS_MASK) != 0)
		sc->sc_options =
		    (ncr_sc->sc_dev.dv_cfdata->cf_flags & SW_OPTIONS_MASK);

	/*
	 * Initialize fields used by the MI code
	 */

	/* NCR5380 register bank offsets */
	ncr_sc->sci_r0 = 0;
	ncr_sc->sci_r1 = 1;
	ncr_sc->sci_r2 = 2;
	ncr_sc->sci_r3 = 3;
	ncr_sc->sci_r4 = 4;
	ncr_sc->sci_r5 = 5;
	ncr_sc->sci_r6 = 6;
	ncr_sc->sci_r7 = 7;

	ncr_sc->sc_rev = NCR_VARIANT_NCR5380;

	/*
	 * MD function pointers used by the MI code.
	 */
	ncr_sc->sc_pio_out = ncr5380_pio_out;
	ncr_sc->sc_pio_in =  ncr5380_pio_in;
	ncr_sc->sc_dma_alloc = sw_dma_alloc;
	ncr_sc->sc_dma_free  = sw_dma_free;
	ncr_sc->sc_dma_poll  = sw_dma_poll;

	ncr_sc->sc_flags = 0;
	if ((sc->sc_options & SW_DO_RESELECT) == 0)
		ncr_sc->sc_no_disconnect = 0xFF;
	if ((sc->sc_options & SW_DMA_INTR) == 0)
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;
	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;


	/*
	 * Allocate DMA handles.
	 */
	i = SCI_OPENINGS * sizeof(struct sw_dma_handle);
	sc->sc_dma = (struct sw_dma_handle *)malloc(i, M_DEVBUF, M_NOWAIT);
	if (sc->sc_dma == NULL)
		panic("sw: DMA handle malloc failed");

	for (i = 0; i < SCI_OPENINGS; i++) {
		sc->sc_dma[i].dh_flags = 0;

		/* Allocate a DMA handle */
		if (bus_dmamap_create(
				sc->sc_dmatag,	/* tag */
				MAXPHYS,	/* size */
				1,		/* nsegments */
				MAXPHYS,	/* maxsegsz */
				0,		/* boundary */
				BUS_DMA_NOWAIT,
				&sc->sc_dma[i].dh_dmamap) != 0) {

			printf("%s: DMA buffer map create error\n",
				ncr_sc->sc_dev.dv_xname);
			return;
		}
	}

	if (sc->sc_options) {
		printf("%s: options=%s\n", ncr_sc->sc_dev.dv_xname,
			bitmask_snprintf(sc->sc_options, SW_OPTIONS_BITS,
			    bits, sizeof(bits)));
	}

	ncr_sc->sc_channel.chan_id = 7;
	ncr_sc->sc_adapter.adapt_minphys = sw_minphys;

	/* Initialize sw board */
	sw_reset_adapter(ncr_sc);

	/* Attach the ncr5380 chip driver */
	ncr5380_attach(ncr_sc);
}

static void
sw_minphys(struct buf *bp)
{
	if (bp->b_bcount > MAX_DMA_LEN) {
#ifdef DEBUG
		if (sw_debug) {
			printf("sw_minphys len = 0x%x.\n", MAX_DMA_LEN);
			Debugger();
		}
#endif
		bp->b_bcount = MAX_DMA_LEN;
	}
	minphys(bp);
}

#define CSR_WANT (SW_CSR_SBC_IP | SW_CSR_DMA_IP | \
	SW_CSR_DMA_CONFLICT | SW_CSR_DMA_BUS_ERR )

static int
sw_intr(void *arg)
{
	struct sw_softc *sc = arg;
	struct ncr5380_softc *ncr_sc = (struct ncr5380_softc *)arg;
	int dma_error, claimed;
	u_short csr;

	claimed = 0;
	dma_error = 0;

	/* SBC interrupt? DMA interrupt? */
	csr = SWREG_READ(ncr_sc, SWREG_CSR);

	NCR_TRACE("sw_intr: csr=0x%x\n", csr);

	if (csr & SW_CSR_DMA_CONFLICT) {
		dma_error |= SW_CSR_DMA_CONFLICT;
		printf("sw_intr: DMA conflict\n");
	}
	if (csr & SW_CSR_DMA_BUS_ERR) {
		dma_error |= SW_CSR_DMA_BUS_ERR;
		printf("sw_intr: DMA bus error\n");
	}
	if (dma_error) {
		if (sc->ncr_sc.sc_state & NCR_DOINGDMA)
			sc->ncr_sc.sc_state |= NCR_ABORTING;
		/* Make sure we will call the main isr. */
		csr |= SW_CSR_DMA_IP;
	}

	if (csr & (SW_CSR_SBC_IP | SW_CSR_DMA_IP)) {
		claimed = ncr5380_intr(&sc->ncr_sc);
#ifdef DEBUG
		if (!claimed) {
			printf("sw_intr: spurious from SBC\n");
			if (sw_debug & 4) {
				Debugger();	/* XXX */
			}
		}
#endif
	}

	return (claimed);
}


static void
sw_reset_adapter(struct ncr5380_softc *ncr_sc)
{

#ifdef	DEBUG
	if (sw_debug) {
		printf("sw_reset_adapter\n");
	}
#endif

	/*
	 * The reset bits in the CSR are active low.
	 */
	SWREG_WRITE(ncr_sc, SWREG_CSR, 0);
	delay(10);
	SWREG_WRITE(ncr_sc, SWREG_CSR, SW_CSR_SCSI_RES);

	SWREG_WRITE(ncr_sc, SWREG_DMA_ADDR, 0);
	SWREG_WRITE(ncr_sc, SWREG_DMA_CNT, 0);
	delay(10);
	SWREG_WRITE(ncr_sc, SWREG_CSR, SW_CSR_SCSI_RES | SW_CSR_INTR_EN);

	SCI_CLR_INTR(ncr_sc);
}


/*****************************************************************
 * Common functions for DMA
 ****************************************************************/

/*
 * Allocate a DMA handle and put it in sc->sc_dma.  Prepare
 * for DMA transfer.  On the Sun4, this means mapping the buffer
 * into DVMA space.
 */
void
sw_dma_alloc(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sw_softc *sc = (struct sw_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsipi_xfer *xs = sr->sr_xs;
	struct sw_dma_handle *dh;
	int i, xlen;
	u_long addr;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("sw_dma_alloc: already have DMA handle");
#endif

#if 1	/* XXX - Temporary */
	/* XXX - In case we think DMA is completely broken... */
	if ((sc->sc_options & SW_ENABLE_DMA) == 0)
		return;
#endif

	addr = (u_long) ncr_sc->sc_dataptr;
	xlen = ncr_sc->sc_datalen;

	/* If the DMA start addr is misaligned then do PIO */
	if ((addr & 1) || (xlen & 1)) {
		printf("sw_dma_alloc: misaligned.\n");
		return;
	}

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("sw_dma_alloc: xlen=0x%x", xlen);

	/* Find free DMA handle.  Guaranteed to find one since we have
	   as many DMA handles as the driver has processes. */
	for (i = 0; i < SCI_OPENINGS; i++) {
		if ((sc->sc_dma[i].dh_flags & SIDH_BUSY) == 0)
			goto found;
	}
	panic("sw: no free DMA handles.");

found:
	dh = &sc->sc_dma[i];
	dh->dh_flags = SIDH_BUSY;
	dh->dh_addr = (u_char *)addr;
	dh->dh_maplen  = xlen;

	/* Copy the "write" flag for convenience. */
	if ((xs->xs_control & XS_CTL_DATA_OUT) != 0)
		dh->dh_flags |= SIDH_OUT;

	/*
	 * Double-map the buffer into DVMA space.  If we can't re-map
	 * the buffer, we print a warning and fall back to PIO mode.
	 *
	 * NOTE: it is not safe to sleep here!
	 */
	if (bus_dmamap_load(sc->sc_dmatag, dh->dh_dmamap,
			    (caddr_t)addr, xlen, NULL, BUS_DMA_NOWAIT) != 0) {
		/* Can't remap segment */
		printf("sw_dma_alloc: can't remap 0x%lx/0x%x, doing PIO\n",
			addr, dh->dh_maplen);
		dh->dh_flags = 0;
		return;
	}
	bus_dmamap_sync(sc->sc_dmatag, dh->dh_dmamap, addr, xlen,
			(dh->dh_flags & SIDH_OUT)
				? BUS_DMASYNC_PREWRITE
				: BUS_DMASYNC_PREREAD);

	/* success */
	sr->sr_dma_hand = dh;

	return;
}


void
sw_dma_free(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sw_softc *sc = (struct sw_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sw_dma_handle *dh = sr->sr_dma_hand;

#ifdef DIAGNOSTIC
	if (dh == NULL)
		panic("sw_dma_free: no DMA handle");
#endif

	if (ncr_sc->sc_state & NCR_DOINGDMA)
		panic("sw_dma_free: free while in progress");

	if (dh->dh_flags & SIDH_BUSY) {
		/* Give back the DVMA space. */
		bus_dmamap_sync(sc->sc_dmatag, dh->dh_dmamap,
				dh->dh_dvma, dh->dh_maplen,
				(dh->dh_flags & SIDH_OUT)
					? BUS_DMASYNC_POSTWRITE
					: BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmatag, dh->dh_dmamap);
		dh->dh_flags = 0;
	}
	sr->sr_dma_hand = NULL;
}


/*
 * Poll (spin-wait) for DMA completion.
 * Called right after xx_dma_start(), and
 * xx_dma_stop() will be called next.
 * Same for either VME or OBIO.
 */
void
sw_dma_poll(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sci_req *sr = ncr_sc->sc_current;
	int tmo, csr_mask, csr;

	/* Make sure DMA started successfully. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		return;

	csr_mask = SW_CSR_SBC_IP | SW_CSR_DMA_IP |
		SW_CSR_DMA_CONFLICT | SW_CSR_DMA_BUS_ERR;

	tmo = 50000;	/* X100 = 5 sec. */
	for (;;) {
		csr = SWREG_READ(ncr_sc, SWREG_CSR);
		if (csr & csr_mask)
			break;
		if (--tmo <= 0) {
			printf("%s: DMA timeout (while polling)\n",
			    ncr_sc->sc_dev.dv_xname);
			/* Indicate timeout as MI code would. */
			sr->sr_flags |= SR_OVERDUE;
			break;
		}
		delay(100);
	}

#ifdef	DEBUG
	if (sw_debug) {
		printf("sw_dma_poll: done, csr=0x%x\n", csr);
	}
#endif
}


/*
 * This is called when the bus is going idle,
 * so we want to enable the SBC interrupts.
 * That is controlled by the DMA enable!
 * Who would have guessed!
 * What a NASTY trick!
 *
 * XXX THIS MIGHT NOT WORK RIGHT!
 */
void
sw_intr_on(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	u_int32_t csr;

	sw_dma_setup(ncr_sc);
	csr = SWREG_READ(ncr_sc, SWREG_CSR);
	csr |= SW_CSR_DMA_EN;	/* XXX - this bit is for vme only?! */
	SWREG_WRITE(ncr_sc, SWREG_CSR, csr);
}

/*
 * This is called when the bus is idle and we are
 * about to start playing with the SBC chip.
 *
 * XXX THIS MIGHT NOT WORK RIGHT!
 */
void
sw_intr_off(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	u_int32_t csr;

	csr = SWREG_READ(ncr_sc, SWREG_CSR);
	csr &= ~SW_CSR_DMA_EN;
	SWREG_WRITE(ncr_sc, SWREG_CSR, csr);
}


/*
 * This function is called during the COMMAND or MSG_IN phase
 * that precedes a DATA_IN or DATA_OUT phase, in case we need
 * to setup the DMA engine before the bus enters a DATA phase.
 *
 * On the OBIO version we just clear the DMA count and address
 * here (to make sure it stays idle) and do the real setup
 * later, in dma_start.
 */
void
sw_dma_setup(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	u_int32_t csr;

	/* No FIFO to reset on "sw". */

	/* Set direction (assume recv here) */
	csr = SWREG_READ(ncr_sc, SWREG_CSR);
	csr &= ~SW_CSR_SEND;
	SWREG_WRITE(ncr_sc, SWREG_CSR, csr);

	SWREG_WRITE(ncr_sc, SWREG_DMA_ADDR, 0);
	SWREG_WRITE(ncr_sc, SWREG_DMA_CNT, 0);
}


void
sw_dma_start(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sw_softc *sc = (struct sw_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sw_dma_handle *dh = sr->sr_dma_hand;
	u_long dva;
	int xlen, adj, adjlen;
	u_int mode;
	u_int32_t csr;

	/*
	 * Get the DVMA mapping for this segment.
	 */
	dva = (u_long)(dh->dh_dvma);
	if (dva & 1)
		panic("sw_dma_start: bad dva=0x%lx", dva);

	xlen = ncr_sc->sc_datalen;
	xlen &= ~1;
	sc->sc_xlen = xlen;	/* XXX: or less... */

#ifdef	DEBUG
	if (sw_debug & 2) {
		printf("sw_dma_start: dh=%p, dva=0x%lx, xlen=%d\n",
		    dh, dva, xlen);
	}
#endif

	/*
	 * Set up the DMA controller.
	 * Note that (dh->dh_len < sc_datalen)
	 */

	/* Set direction (send/recv) */
	csr = SWREG_READ(ncr_sc, SWREG_CSR);
	if (dh->dh_flags & SIDH_OUT) {
		csr |= SW_CSR_SEND;
	} else {
		csr &= ~SW_CSR_SEND;
	}
	SWREG_WRITE(ncr_sc, SWREG_CSR, csr);

	/*
	 * The "sw" needs longword aligned transfers.  We
	 * detect a shortword aligned transfer here, and adjust the
	 * DMA transfer by 2 bytes.  These two bytes are read/written
	 * in PIO mode just before the DMA is started.
	 */
	adj = 0;
	if (dva & 2) {
		adj = 2;
#ifdef DEBUG
		if (sw_debug & 2)
			printf("sw_dma_start: adjusted up %d bytes\n", adj);
#endif
	}

	/* We have to frob the address on the "sw". */
	dh->dh_startingpa = (dva | 0xF00000);
	SWREG_WRITE(ncr_sc, SWREG_DMA_ADDR, (u_int)(dh->dh_startingpa + adj));
	SWREG_WRITE(ncr_sc, SWREG_DMA_CNT, xlen - adj);

	/*
	 * Acknowledge the phase change.  (After DMA setup!)
	 * Put the SBIC into DMA mode, and start the transfer.
	 */
	if (dh->dh_flags & SIDH_OUT) {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_OUT);
		if (adj) {
			adjlen = ncr5380_pio_out(ncr_sc, PHASE_DATA_OUT,
			    adj, dh->dh_addr);
			if (adjlen != adj)
				printf("%s: bad outgoing adj, %d != %d\n",
				    ncr_sc->sc_dev.dv_xname, adjlen, adj);
		}
		SCI_CLR_INTR(ncr_sc);
		NCR5380_WRITE(ncr_sc, sci_icmd, SCI_ICMD_DATA);
		mode = NCR5380_READ(ncr_sc, sci_mode);
		mode |= (SCI_MODE_DMA | SCI_MODE_DMA_IE);
		NCR5380_WRITE(ncr_sc, sci_mode, mode);
		NCR5380_WRITE(ncr_sc, sci_dma_send, 0); 	/* start it */
	} else {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_IN);
		if (adj) {
			adjlen = ncr5380_pio_in(ncr_sc, PHASE_DATA_IN,
			    adj, dh->dh_addr);
			if (adjlen != adj)
				printf("%s: bad incoming adj, %d != %d\n",
				    ncr_sc->sc_dev.dv_xname, adjlen, adj);
		}
		SCI_CLR_INTR(ncr_sc);
		NCR5380_WRITE(ncr_sc, sci_icmd, 0);
		mode = NCR5380_READ(ncr_sc, sci_mode);
		mode |= (SCI_MODE_DMA | SCI_MODE_DMA_IE);
		NCR5380_WRITE(ncr_sc, sci_mode, mode);
		NCR5380_WRITE(ncr_sc, sci_irecv, 0); 	/* start it */
	}

	/* Let'er rip! */
	csr |= SW_CSR_DMA_EN;
	SWREG_WRITE(ncr_sc, SWREG_CSR, csr);

	ncr_sc->sc_state |= NCR_DOINGDMA;

#ifdef	DEBUG
	if (sw_debug & 2) {
		printf("sw_dma_start: started, flags=0x%x\n",
		    ncr_sc->sc_state);
	}
#endif
}


void
sw_dma_eop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{

	/* Not needed - DMA was stopped prior to examining sci_csr */
}

#if (defined(DEBUG) || defined(DIAGNOSTIC)) && !defined(COUNT_SW_LEFTOVERS)
#define COUNT_SW_LEFTOVERS
#endif
#ifdef COUNT_SW_LEFTOVERS
/*
 * Let's find out how often these occur.  Read these with DDB from time
 * to time.
 */
int	sw_3_leftover = 0;
int	sw_2_leftover = 0;
int	sw_1_leftover = 0;
int	sw_0_leftover = 0;
#endif

void
sw_dma_stop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sci_req *sr = ncr_sc->sc_current;
	struct sw_dma_handle *dh = sr->sr_dma_hand;
	int ntrans = 0, dva;
	u_int mode;
	u_int32_t csr;

	if ((ncr_sc->sc_state & NCR_DOINGDMA) == 0) {
#ifdef	DEBUG
		printf("sw_dma_stop: DMA not running\n");
#endif
		return;
	}
	ncr_sc->sc_state &= ~NCR_DOINGDMA;

	/* First, halt the DMA engine. */
	csr = SWREG_READ(ncr_sc, SWREG_CSR);
	csr &= ~SW_CSR_DMA_EN;
	SWREG_WRITE(ncr_sc, SWREG_CSR, csr);

	/*
	 * XXX HARDWARE BUG!
	 * Apparently, some early 4/100 SCSI controllers had a hardware
	 * bug that caused the controller to do illegal memory access.
	 * We see this as SW_CSR_DMA_BUS_ERR (makes sense).  To work around
	 * this, we simply need to clean up after ourselves ... there will
	 * be as many as 3 bytes left over.  Since we clean up "left-over"
	 * bytes on every read anyway, we just continue to chug along
	 * if SW_CSR_DMA_BUS_ERR is asserted.  (This was probably worked
	 * around in hardware later with the "left-over byte" indicator
	 * in the VME controller.)
	 */
#if 0
	if (csr & (SW_CSR_DMA_CONFLICT | SW_CSR_DMA_BUS_ERR)) {
#else
	if (csr & (SW_CSR_DMA_CONFLICT)) {
#endif
		printf("sw: DMA error, csr=0x%x, reset\n", csr);
		sr->sr_xs->error = XS_DRIVER_STUFFUP;
		ncr_sc->sc_state |= NCR_ABORTING;
		sw_reset_adapter(ncr_sc);
	}

	/* Note that timeout may have set the error flag. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		goto out;

	/*
	 * Now try to figure out how much actually transferred
	 *
	 * The "sw" doesn't have a FIFO or a bcr, so we've stored
	 * the starting PA of the transfer in the DMA handle,
	 * and subtract it from the ending PA left in the dma_addr
	 * register.
	 */
	dva = SWREG_READ(ncr_sc, SWREG_DMA_ADDR);
	ntrans = (dva - dh->dh_startingpa);

#ifdef	DEBUG
	if (sw_debug & 2) {
		printf("sw_dma_stop: ntrans=0x%x\n", ntrans);
	}
#endif

	if (ntrans > ncr_sc->sc_datalen)
		panic("sw_dma_stop: excess transfer");

	/* Adjust data pointer */
	ncr_sc->sc_dataptr += ntrans;
	ncr_sc->sc_datalen -= ntrans;

	/*
	 * After a read, we may need to clean-up
	 * "Left-over bytes"  (yuck!)  The "sw" doesn't
	 * have a "left-over" indicator, so we have to so
	 * this no matter what.  Ick.
	 */
	if ((dh->dh_flags & SIDH_OUT) == 0) {
		char *cp = ncr_sc->sc_dataptr;
		u_int32_t bpr;

		bpr = SWREG_READ(ncr_sc, SWREG_BPR);

		switch (dva & 3) {
		case 3:
			cp[0] = (bpr & 0xff000000) >> 24;
			cp[1] = (bpr & 0x00ff0000) >> 16;
			cp[2] = (bpr & 0x0000ff00) >> 8;
#ifdef COUNT_SW_LEFTOVERS
			++sw_3_leftover;
#endif
			break;

		case 2:
			cp[0] = (bpr & 0xff000000) >> 24;
			cp[1] = (bpr & 0x00ff0000) >> 16;
#ifdef COUNT_SW_LEFTOVERS
			++sw_2_leftover;
#endif
			break;

		case 1:
			cp[0] = (bpr & 0xff000000) >> 24;
#ifdef COUNT_SW_LEFTOVERS
			++sw_1_leftover;
#endif
			break;

#ifdef COUNT_SW_LEFTOVERS
		default:
			++sw_0_leftover;
			break;
#endif
		}
	}

 out:
	SWREG_WRITE(ncr_sc, SWREG_DMA_ADDR, 0);
	SWREG_WRITE(ncr_sc, SWREG_DMA_CNT, 0);

	/* Put SBIC back in PIO mode. */
	mode = NCR5380_READ(ncr_sc, sci_mode);
	mode &= ~(SCI_MODE_DMA | SCI_MODE_DMA_IE);
	NCR5380_WRITE(ncr_sc, sci_mode, mode);
	NCR5380_WRITE(ncr_sc, sci_icmd, 0);

#ifdef DEBUG
	if (sw_debug & 2) {
		printf("sw_dma_stop: ntrans=0x%x\n", ntrans);
	}
#endif
}
