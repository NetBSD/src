/*	$NetBSD: si.c,v 1.2.6.3 2001/09/21 22:36:21 nathanw Exp $	*/

/*-
 * Copyright (c) 1996,2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, Gordon W. Ross, Jason R. Thorpe and
 * Paul Kranenburg.
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
 * This file contains VME bus-dependent of the `si' SCSI adapter.
 * This hardware is frequently found on Sun 3 and Sun 4 machines.
 *
 * The SCSI machinery on this adapter is implemented by an NCR5380,
 * which is taken care of by the chipset driver in /sys/dev/ic/ncr5380sbc.c
 *
 * The logic has a bit to enable or disable the DMA engine,
 * but that bit also gates the interrupt line from the NCR5380!
 * Therefore, in order to get any interrupt from the 5380, (i.e.
 * for reselect) one must clear the DMA engine transfer count and
 * then enable DMA.  This has the further complication that you
 * CAN NOT touch the NCR5380 while the DMA enable bit is set, so
 * we have to turn DMA back off before we even look at the 5380.
 *
 * What wonderfully whacky hardware this is!
 *
 */

/*
 * This driver originated as an MD implementation for the sun3 and sun4
 * ports. The notes pertaining to that history are included below.
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

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

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

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <dev/vme/sireg.h>

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth DMA overhead)
 */
#define	MIN_DMA_LEN 128

#ifdef	DEBUG
int si_debug = 0;
#endif

/*
 * This structure is used to keep track of mapped DMA requests.
 */
struct si_dma_handle {
	int 		dh_flags;
#define	SIDH_BUSY	0x01		/* This DH is in use */
#define	SIDH_OUT	0x02		/* DMA does data out (write) */
	int 		dh_maplen;	/* Original data length */
	bus_dmamap_t	dh_dmamap;
#define dh_dvma	dh_dmamap->dm_segs[0].ds_addr /* VA of buffer in DVMA space */
};

/*
 * The first structure member has to be the ncr5380_softc
 * so we can just cast to go back and fourth between them.
 */
struct si_softc {
	struct ncr5380_softc	ncr_sc;
	bus_space_tag_t		sc_bustag;	/* bus tags */
	bus_dma_tag_t		sc_dmatag;
	vme_chipset_tag_t	sc_vctag;

	int		sc_adapter_iv_am; /* int. vec + address modifier */
	struct si_dma_handle *sc_dma;
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
 * the value in si_options.
 *
 * Note, there's a separate sw_options to make life easier.
 */
#define	SI_ENABLE_DMA	0x01	/* Use DMA (maybe polled) */
#define	SI_DMA_INTR	0x02	/* DMA completion interrupts */
#define	SI_DO_RESELECT	0x04	/* Allow disconnect/reselect */
#define	SI_OPTIONS_MASK	(SI_ENABLE_DMA|SI_DMA_INTR|SI_DO_RESELECT)
#define SI_OPTIONS_BITS	"\10\3RESELECT\2DMA_INTR\1DMA"
int si_options = SI_ENABLE_DMA|SI_DMA_INTR|SI_DO_RESELECT;

static int	si_match __P((struct device *, struct cfdata *, void *));
static void	si_attach __P((struct device *, struct device *, void *));
static int	si_intr __P((void *));
static void	si_reset_adapter __P((struct ncr5380_softc *));

void	si_dma_alloc __P((struct ncr5380_softc *));
void	si_dma_free __P((struct ncr5380_softc *));
void	si_dma_poll __P((struct ncr5380_softc *));

void	si_dma_setup __P((struct ncr5380_softc *));
void	si_dma_start __P((struct ncr5380_softc *));
void	si_dma_eop __P((struct ncr5380_softc *));
void	si_dma_stop __P((struct ncr5380_softc *));

void	si_intr_on  __P((struct ncr5380_softc *));
void	si_intr_off __P((struct ncr5380_softc *));

/*
 * Shorthand bus space access
 * XXX - must look into endian issues here.
 */
#define SIREG_READ(sc, index) \
	bus_space_read_2((sc)->sc_regt, (sc)->sc_regh, index)
#define SIREG_WRITE(sc, index, v) \
	bus_space_write_2((sc)->sc_regt, (sc)->sc_regh, index, v)


/* Auto-configuration glue. */
struct cfattach si_ca = {
	sizeof(struct si_softc), si_match, si_attach
};

static int
si_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
        vme_am_t		mod; 
        vme_addr_t		vme_addr;

	/* Make sure there is something there... */
	mod = VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA;
	vme_addr = va->r[0].offset;

	if (vme_probe(ct, vme_addr, 1, mod, VME_D8, NULL, 0) != 0)
		return (0);

	/*
	 * If this is a VME SCSI board, we have to determine whether
	 * it is an "sc" (Sun2) or "si" (Sun3) SCSI board.  This can
	 * be determined using the fact that the "sc" board occupies
	 * 4K bytes in VME space but the "si" board occupies 2K bytes.
	 */
	return (vme_probe(ct, vme_addr + 0x801, 1, mod, VME_D8, NULL, 0) != 0);
}

static void
si_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct si_softc		*sc = (struct si_softc *) self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	bus_space_tag_t		bt;
	bus_space_handle_t	bh;
	vme_mapresc_t resc;
	vme_intr_handle_t	ih;
	vme_am_t		mod;
	char bits[64];
	int i;

	sc->sc_dmatag = va->va_bdt;
	sc->sc_vctag = ct;

	mod = VME_AM_A24 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA;

	if (vme_space_map(ct, va->r[0].offset, SIREG_BANK_SZ,
			  mod, VME_D8, 0, &bt, &bh, &resc) != 0)
		panic("%s: vme_space_map", ncr_sc->sc_dev.dv_xname);

	ncr_sc->sc_regt = bt;
	ncr_sc->sc_regh = bh;

	sc->sc_options = si_options;

	ncr_sc->sc_dma_setup = si_dma_setup;
	ncr_sc->sc_dma_start = si_dma_start;
	ncr_sc->sc_dma_eop   = si_dma_stop;
	ncr_sc->sc_dma_stop  = si_dma_stop;

	vme_intr_map(ct, va->ilevel, va->ivector, &ih);
	vme_intr_establish(ct, ih, IPL_BIO, si_intr, sc);

	printf("\n");

	sc->sc_adapter_iv_am = (mod << 8) | (va->ivector & 0xFF);

	/*
	 * Pull in the options flags.  Allow the user to completely
	 * override the default values.
	 */
	if ((ncr_sc->sc_dev.dv_cfdata->cf_flags & SI_OPTIONS_MASK) != 0)
		sc->sc_options =
		    (ncr_sc->sc_dev.dv_cfdata->cf_flags & SI_OPTIONS_MASK);

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
	ncr_sc->sc_dma_alloc = si_dma_alloc;
	ncr_sc->sc_dma_free  = si_dma_free;
	ncr_sc->sc_dma_poll  = si_dma_poll;

	ncr_sc->sc_flags = 0;
	if ((sc->sc_options & SI_DO_RESELECT) == 0)
		ncr_sc->sc_no_disconnect = 0xFF;
	if ((sc->sc_options & SI_DMA_INTR) == 0)
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;
	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * Allocate DMA handles.
	 */
	i = SCI_OPENINGS * sizeof(struct si_dma_handle);
	sc->sc_dma = (struct si_dma_handle *)malloc(i, M_DEVBUF, M_NOWAIT);
	if (sc->sc_dma == NULL)
		panic("si: dma handle malloc failed\n");

	for (i = 0; i < SCI_OPENINGS; i++) {
		sc->sc_dma[i].dh_flags = 0;

		/* Allocate a DMA handle */
		if (vme_dmamap_create(
				sc->sc_vctag,	/* VME chip tag */
				MAXPHYS,	/* size */
				VME_AM_A24,	/* address modifier */
				VME_D16,	/* data size */
				0,		/* swap */
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
			bitmask_snprintf(sc->sc_options, SI_OPTIONS_BITS,
			    bits, sizeof(bits)));
	}

	ncr_sc->sc_channel.chan_id = 7;
	ncr_sc->sc_adapter.adapt_minphys = minphys;

	/*
	 *  Initialize si board itself.
	 */
	si_reset_adapter(ncr_sc);
	ncr5380_attach(ncr_sc);

	if (sc->sc_options & SI_DO_RESELECT) {
		/*
		 * Need to enable interrupts (and DMA!)
		 * on this H/W for reselect to work.
		 */
		ncr_sc->sc_intr_on   = si_intr_on;
		ncr_sc->sc_intr_off  = si_intr_off;
	}
}

#define CSR_WANT (SI_CSR_SBC_IP | SI_CSR_DMA_IP | \
	SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR )

static int
si_intr(void *arg)
{
	struct si_softc *sc = arg;
	struct ncr5380_softc *ncr_sc = (struct ncr5380_softc *)arg;
	int dma_error, claimed;
	u_short csr;

	claimed = 0;
	dma_error = 0;

	/* SBC interrupt? DMA interrupt? */
	csr = SIREG_READ(ncr_sc, SIREG_CSR);

	NCR_TRACE("si_intr: csr=0x%x\n", csr);

	if (csr & SI_CSR_DMA_CONFLICT) {
		dma_error |= SI_CSR_DMA_CONFLICT;
		printf("si_intr: DMA conflict\n");
	}
	if (csr & SI_CSR_DMA_BUS_ERR) {
		dma_error |= SI_CSR_DMA_BUS_ERR;
		printf("si_intr: DMA bus error\n");
	}
	if (dma_error) {
		if (sc->ncr_sc.sc_state & NCR_DOINGDMA)
			sc->ncr_sc.sc_state |= NCR_ABORTING;
		/* Make sure we will call the main isr. */
		csr |= SI_CSR_DMA_IP;
	}

	if (csr & (SI_CSR_SBC_IP | SI_CSR_DMA_IP)) {
		claimed = ncr5380_intr(&sc->ncr_sc);
#ifdef DEBUG
		if (!claimed) {
			printf("si_intr: spurious from SBC\n");
			if (si_debug & 4) {
				Debugger();	/* XXX */
			}
		}
#endif
	}

	return (claimed);
}


static void
si_reset_adapter(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;

#ifdef	DEBUG
	if (si_debug) {
		printf("si_reset_adapter\n");
	}
#endif

	/*
	 * The SCSI3 controller has an 8K FIFO to buffer data between the
	 * 5380 and the DMA.  Make sure it starts out empty.
	 *
	 * The reset bits in the CSR are active low.
	 */
	SIREG_WRITE(ncr_sc, SIREG_CSR, 0);
	delay(10);
	SIREG_WRITE(ncr_sc, SIREG_CSR,
			SI_CSR_FIFO_RES | SI_CSR_SCSI_RES | SI_CSR_INTR_EN);
	delay(10);

	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNT, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRL, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTL, 0);
	SIREG_WRITE(ncr_sc, SIREG_IV_AM, sc->sc_adapter_iv_am);
	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNTH, 0);

	SCI_CLR_INTR(ncr_sc);
}

/*****************************************************************
 * Common functions for DMA
 ****************************************************************/

/*
 * Allocate a DMA handle and put it in sc->sc_dma.  Prepare
 * for DMA transfer.
 */
void
si_dma_alloc(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsipi_xfer *xs = sr->sr_xs;
	struct si_dma_handle *dh;
	int i, xlen;
	u_long addr;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("si_dma_alloc: already have DMA handle");
#endif

#if 1	/* XXX - Temporary */
	/* XXX - In case we think DMA is completely broken... */
	if ((sc->sc_options & SI_ENABLE_DMA) == 0)
		return;
#endif

	addr = (u_long) ncr_sc->sc_dataptr;
	xlen = ncr_sc->sc_datalen;

	/* If the DMA start addr is misaligned then do PIO */
	if ((addr & 1) || (xlen & 1)) {
		printf("si_dma_alloc: misaligned.\n");
		return;
	}

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("si_dma_alloc: xlen=0x%x\n", xlen);

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
		printf("si_dma_alloc: can't remap 0x%lx/0x%x, doing PIO\n",
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
si_dma_free(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;

#ifdef DIAGNOSTIC
	if (dh == NULL)
		panic("si_dma_free: no DMA handle");
#endif

	if (ncr_sc->sc_state & NCR_DOINGDMA)
		panic("si_dma_free: free while in progress");

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
si_dma_poll(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sci_req *sr = ncr_sc->sc_current;
	int tmo, csr_mask, csr;

	/* Make sure DMA started successfully. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		return;

	csr_mask = SI_CSR_SBC_IP | SI_CSR_DMA_IP |
		SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR;

	tmo = 50000;	/* X100 = 5 sec. */
	for (;;) {
		csr = SIREG_READ(ncr_sc, SIREG_CSR);
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
	if (si_debug) {
		printf("si_dma_poll: done, csr=0x%x\n", csr);
	}
#endif
}


/*****************************************************************
 * VME functions for DMA
 ****************************************************************/


/*
 * This is called when the bus is going idle,
 * so we want to enable the SBC interrupts.
 * That is controlled by the DMA enable!
 * Who would have guessed!
 * What a NASTY trick!
 */
void
si_intr_on(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	u_int16_t csr;

	/* Clear DMA start address and counters */
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRL, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTL, 0);

	/* Enter receive mode (for safety) and enable DMA engine */
	csr = SIREG_READ(ncr_sc, SIREG_CSR);
	csr &= ~SI_CSR_SEND;
	csr |= SI_CSR_DMA_EN;
	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);
}

/*
 * This is called when the bus is idle and we are
 * about to start playing with the SBC chip.
 */
void
si_intr_off(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	u_int16_t csr;

	csr = SIREG_READ(ncr_sc, SIREG_CSR);
	csr &= ~SI_CSR_DMA_EN;
	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);
}

/*
 * This function is called during the COMMAND or MSG_IN phase
 * that precedes a DATA_IN or DATA_OUT phase, in case we need
 * to setup the DMA engine before the bus enters a DATA phase.
 *
 * XXX: The VME adapter appears to suppress SBC interrupts
 * when the FIFO is not empty or the FIFO count is non-zero!
 *
 * On the VME version we just clear the DMA count and address
 * here (to make sure it stays idle) and do the real setup
 * later, in dma_start.
 */
void
si_dma_setup(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;
	u_int16_t csr;
	u_long dva;
	int xlen;

	/*
	 * Set up the DMA controller.
	 * Note that (dh->dh_len < sc_datalen)
	 */

	csr = SIREG_READ(ncr_sc, SIREG_CSR);

	/* Disable DMA while we're setting up the transfer */
	csr &= ~SI_CSR_DMA_EN;

	/* Reset the FIFO */
	csr &= ~SI_CSR_FIFO_RES;		/* active low */
	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);
	csr |= SI_CSR_FIFO_RES;
	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);

	/*
	 * Get the DVMA mapping for this segment.
	 */
	dva = (u_long)(dh->dh_dvma);
	if (dva & 1)
		panic("si_dma_setup: bad dmaaddr=0x%lx", dva);
	xlen = ncr_sc->sc_datalen;
	xlen &= ~1;
	sc->sc_xlen = xlen;	/* XXX: or less... */

#ifdef	DEBUG
	if (si_debug & 2) {
		printf("si_dma_start: dh=%p, dmaaddr=0x%lx, xlen=%d\n",
			   dh, dva, xlen);
	}
#endif
	/* Set direction (send/recv) */
	if (dh->dh_flags & SIDH_OUT) {
		csr |= SI_CSR_SEND;
	} else {
		csr &= ~SI_CSR_SEND;
	}

	/* Set byte-packing control */
	if (dva & 2) {
		csr |= SI_CSR_BPCON;
	} else {
		csr &= ~SI_CSR_BPCON;
	}

	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);

	/* Load start address */
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRH, (u_int16_t)(dva >> 16));
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRL, (u_int16_t)(dva & 0xFFFF));

	/* Clear DMA counters; these will be set in si_dma_start() */
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTL, 0);

	/* Clear FIFO counter. (also hits dma_count) */
	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNTH, 0);
	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNT, 0);
}


void
si_dma_start(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;
	int xlen;
	u_int mode;
	u_int16_t csr;

	xlen = sc->sc_xlen;

	/* Load transfer length */
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTH, (u_int16_t)(xlen >> 16));
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTL, (u_int16_t)(xlen & 0xFFFF));
	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNTH, (u_int16_t)(xlen >> 16));
	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNT, (u_int16_t)(xlen & 0xFFFF));

	/*
	 * Acknowledge the phase change.  (After DMA setup!)
	 * Put the SBIC into DMA mode, and start the transfer.
	 */
	if (dh->dh_flags & SIDH_OUT) {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_OUT);
		SCI_CLR_INTR(ncr_sc);
		NCR5380_WRITE(ncr_sc, sci_icmd, SCI_ICMD_DATA);

		mode = NCR5380_READ(ncr_sc, sci_mode);
		mode |= (SCI_MODE_DMA | SCI_MODE_DMA_IE);
		NCR5380_WRITE(ncr_sc, sci_mode, mode);

		NCR5380_WRITE(ncr_sc, sci_dma_send, 0); /* start it */
	} else {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_IN);
		SCI_CLR_INTR(ncr_sc);
		NCR5380_WRITE(ncr_sc, sci_icmd, 0);

		mode = NCR5380_READ(ncr_sc, sci_mode);
		mode |= (SCI_MODE_DMA | SCI_MODE_DMA_IE);
		NCR5380_WRITE(ncr_sc, sci_mode, mode);

		NCR5380_WRITE(ncr_sc, sci_irecv, 0); /* start it */
	}

	ncr_sc->sc_state |= NCR_DOINGDMA;

	/* Enable DMA engine */
	csr = SIREG_READ(ncr_sc, SIREG_CSR);
	csr |= SI_CSR_DMA_EN;
	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);

#ifdef	DEBUG
	if (si_debug & 2) {
		printf("si_dma_start: started, flags=0x%x\n",
			   ncr_sc->sc_state);
	}
#endif
}


void
si_dma_eop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{

	/* Not needed - DMA was stopped prior to examining sci_csr */
}


void
si_dma_stop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;
	int resid, ntrans;
	u_int16_t csr;
	u_int mode;

	if ((ncr_sc->sc_state & NCR_DOINGDMA) == 0) {
#ifdef	DEBUG
		printf("si_dma_stop: dma not running\n");
#endif
		return;
	}

	ncr_sc->sc_state &= ~NCR_DOINGDMA;

	csr = SIREG_READ(ncr_sc, SIREG_CSR);

	/* First, halt the DMA engine. */
	csr &= ~SI_CSR_DMA_EN;
	SIREG_WRITE(ncr_sc, SIREG_CSR, csr);

	if (csr & (SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR)) {
		printf("si: DMA error, csr=0x%x, reset\n", csr);
		sr->sr_xs->error = XS_DRIVER_STUFFUP;
		ncr_sc->sc_state |= NCR_ABORTING;
		si_reset_adapter(ncr_sc);
	}

	/* Note that timeout may have set the error flag. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		goto out;

	/*
	 * Now try to figure out how much actually transferred
	 *
	 * The fifo_count does not reflect how many bytes were
	 * actually transferred for VME.
	 *
	 * SCSI-3 VME interface is a little funny on writes:
	 * if we have a disconnect, the dma has overshot by
	 * one byte and the resid needs to be incremented.
	 * Only happens for partial transfers.
	 * (Thanks to Matt Jacob)
	 */

	resid = SIREG_READ(ncr_sc, SIREG_FIFO_CNTH) << 16;
	resid |= SIREG_READ(ncr_sc, SIREG_FIFO_CNT) & 0xFFFF;
	if (dh->dh_flags & SIDH_OUT)
		if ((resid > 0) && (resid < sc->sc_xlen))
			resid++;
	ntrans = sc->sc_xlen - resid;

#ifdef	DEBUG
	if (si_debug & 2) {
		printf("si_dma_stop: resid=0x%x ntrans=0x%x\n",
		    resid, ntrans);
	}
#endif

	if (ntrans > ncr_sc->sc_datalen)
		panic("si_dma_stop: excess transfer");

	/* Adjust data pointer */
	ncr_sc->sc_dataptr += ntrans;
	ncr_sc->sc_datalen -= ntrans;

#ifdef	DEBUG
	if (si_debug & 2) {
		printf("si_dma_stop: ntrans=0x%x\n", ntrans);
	}
#endif

	/*
	 * After a read, we may need to clean-up
	 * "Left-over bytes" (yuck!)
	 */
	if (((dh->dh_flags & SIDH_OUT) == 0) &&
		((csr & SI_CSR_LOB) != 0))
	{
		char *cp = ncr_sc->sc_dataptr;
		u_int16_t bprh, bprl;

		bprh = SIREG_READ(ncr_sc, SIREG_BPRH);
		bprl = SIREG_READ(ncr_sc, SIREG_BPRL);

#ifdef DEBUG
		printf("si: got left-over bytes: bprh=%x, bprl=%x, csr=%x\n",
			bprh, bprl, csr);
#endif

		if (csr & SI_CSR_BPCON) {
			/* have SI_CSR_BPCON */
			cp[-1] = (bprl & 0xff00) >> 8;
		} else {
			switch (csr & SI_CSR_LOB) {
			case SI_CSR_LOB_THREE:
				cp[-3] = (bprh & 0xff00) >> 8;
				cp[-2] = (bprh & 0x00ff);
				cp[-1] = (bprl & 0xff00) >> 8;
				break;
			case SI_CSR_LOB_TWO:
				cp[-2] = (bprh & 0xff00) >> 8;
				cp[-1] = (bprh & 0x00ff);
				break;
			case SI_CSR_LOB_ONE:
				cp[-1] = (bprh & 0xff00) >> 8;
				break;
			}
		}
	}

out:
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_ADDRL, 0);

	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTH, 0);
	SIREG_WRITE(ncr_sc, SIREG_DMA_CNTL, 0);

	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNTH, 0);
	SIREG_WRITE(ncr_sc, SIREG_FIFO_CNT, 0);

	mode = NCR5380_READ(ncr_sc, sci_mode);
	/* Put SBIC back in PIO mode. */
	mode &= ~(SCI_MODE_DMA | SCI_MODE_DMA_IE);
	NCR5380_WRITE(ncr_sc, sci_mode, mode);
	NCR5380_WRITE(ncr_sc, sci_icmd, 0);
}
