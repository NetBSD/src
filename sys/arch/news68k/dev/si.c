/*	$NetBSD: si.c,v 1.6 2001/04/25 17:53:18 bouyer Exp $	*/

/*
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
 * This file contains the machine-dependent parts of the Sony CXD1180
 * controller. The machine-independent parts are in ncr5380sbc.c.
 * Written by Izumi Tsutsui.
 * 
 * This code is based on arch/vax/vsa/ncr.c and sun3/dev/si.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/cpu.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/dmac_0266.h>

#define MIN_DMA_LEN 128
#define DMAC_BASE	0xe0e80000 /* XXX */

struct si_dma_handle {
	int	dh_flags;
#define SIDH_BUSY	0x01
#define SIDH_OUT	0x02
	caddr_t dh_addr;
	int	dh_len;
};

struct si_softc {
	struct	ncr5380_softc	ncr_sc;
	int	sc_options;
	volatile struct dma_regs *sc_regs;
	struct	si_dma_handle ncr_dma[SCI_OPENINGS];
};

void si_attach __P((struct device *, struct device *, void *));
int  si_match  __P((struct device *, struct cfdata *, void *));
int si_intr __P((int));

void si_dma_alloc __P((struct ncr5380_softc *));
void si_dma_free __P((struct ncr5380_softc *));
void si_dma_setup __P((struct ncr5380_softc *));
void si_dma_start __P((struct ncr5380_softc *));
void si_dma_poll __P((struct ncr5380_softc *));
void si_dma_eop __P((struct ncr5380_softc *));
void si_dma_stop __P((struct ncr5380_softc *));

struct cfattach si_ca = {
	sizeof(struct si_softc), si_match, si_attach
};

/*
 * Options for disconnect/reselect, DMA, and interrupts.
 * By default, allow disconnect/reselect on targets 4-6.
 * Those are normally tapes that really need it enabled.
 * The options are taken from the config file.
 */
#define SI_NO_DISCONNECT	0x000ff
#define SI_NO_PARITY_CHK	0x0ff00
#define SI_FORCE_POLLING	0x10000
#define SI_DISABLE_DMA		0x20000

int si_options = 0x0f;


int
si_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct hb_attach_args *ha = aux;
	int addr;

	if (strcmp(ha->ha_name, "si"))
		return 0;

	addr = IIOV(ha->ha_address);

	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

/*
 * Card attach function
 */

void
si_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct si_softc *sc = (struct si_softc *)self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	struct cfdata *cf = self->dv_cfdata;
	struct hb_attach_args *ha = aux;
	u_char *addr;

	/* Get options from config flags if specified. */
	if (cf->cf_flags)
		sc->sc_options = cf->cf_flags;
	else
		sc->sc_options = si_options;

	printf(": options=0x%x\n", sc->sc_options);

	ncr_sc->sc_no_disconnect = (sc->sc_options & SI_NO_DISCONNECT);
	ncr_sc->sc_parity_disable = (sc->sc_options & SI_NO_PARITY_CHK) >> 8;
	if (sc->sc_options & SI_FORCE_POLLING)
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;

	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;
	ncr_sc->sc_dma_alloc   = si_dma_alloc;
	ncr_sc->sc_dma_free    = si_dma_free;
	ncr_sc->sc_dma_poll    = si_dma_poll;
	ncr_sc->sc_dma_setup   = si_dma_setup;
	ncr_sc->sc_dma_start   = si_dma_start;
	ncr_sc->sc_dma_eop     = si_dma_eop;
	ncr_sc->sc_dma_stop    = si_dma_stop;

	if (sc->sc_options & SI_DISABLE_DMA)
		/* Override this function pointer. */
		ncr_sc->sc_dma_alloc = NULL;

	addr = (u_char *)IIOV(ha->ha_address);
	ncr_sc->sci_r0 = addr + 0;
	ncr_sc->sci_r1 = addr + 1;
	ncr_sc->sci_r2 = addr + 2;
	ncr_sc->sci_r3 = addr + 3;
	ncr_sc->sci_r4 = addr + 4;
	ncr_sc->sci_r5 = addr + 5;
	ncr_sc->sci_r6 = addr + 6;
	ncr_sc->sci_r7 = addr + 7;

	ncr_sc->sc_rev = NCR_VARIANT_CXD1180;

	ncr_sc->sc_pio_in  = ncr5380_pio_in;
	ncr_sc->sc_pio_out = ncr5380_pio_out;

	ncr_sc->sc_adapter.adapt_minphys = minphys;
	ncr_sc->sc_channel.chan_id = 7;

	/* soft reset DMAC */
	sc->sc_regs = (void *)IIOV(DMAC_BASE);
	sc->sc_regs->ctl = DC_CTL_RST;

	ncr5380_attach(ncr_sc);
}

int
si_intr(unit)
	int unit;
{
	struct si_softc *sc;
	extern struct cfdriver si_cd;

	if (unit >= si_cd.cd_ndevs)
		return 0;

	sc = si_cd.cd_devs[unit];
	(void)ncr5380_intr(&sc->ncr_sc);

	return 0;
}

/*
 *  DMA routines for news1700 machines
 */

void
si_dma_alloc(ncr_sc)
	struct ncr5380_softc *ncr_sc;
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
		panic("si_dma_alloc: len=0x%x\n", xlen);

	/*
	 * Find free DMA handle.  Guaranteed to find one since we
	 * have as many DMA handles as the driver has processes.
	 * (instances?)
	 */
	 for (i = 0; i < SCI_OPENINGS; i++) {
		if ((sc->ncr_dma[i].dh_flags & SIDH_BUSY) == 0)
			goto found;
	}
	panic("si_dma_alloc(): no free DMA handles");
found:
	dh = &sc->ncr_dma[i];
	dh->dh_flags = SIDH_BUSY;
	dh->dh_addr = ncr_sc->sc_dataptr;
	dh->dh_len = xlen;

	/* Remember dest buffer parameters */
	if (xs->xs_control & XS_CTL_DATA_OUT)
		dh->dh_flags |= SIDH_OUT;

	sr->sr_dma_hand = dh;
}

void
si_dma_free(ncr_sc)
	struct ncr5380_softc *ncr_sc;
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
si_dma_setup(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	/* Do nothing here */
}

void
si_dma_start(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	volatile struct dma_regs *dmac = sc->sc_regs;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;
	u_int addr, offset, rest;
	long len;
	int i;

	/*
	 * Set the news68k-specific registers.
	 */

	/* reset DMAC */
	dmac->ctl = DC_CTL_RST;
	dmac->ctl = 0;

	addr = (u_int)dh->dh_addr;
	offset = addr & DMAC_SEG_OFFSET;
	len = (u_int)dh->dh_len;

	/* set DMA transfer length and offset of first segment */
	dmac->tcnt = len;
	dmac->offset = offset;

	/* set first DMA segment address */
	dmac->tag = 0;
	dmac->mapent = kvtop((caddr_t)addr) >> DMAC_SEG_SHIFT;
	rest = DMAC_SEG_SIZE - offset;
	addr += rest;
	len -= rest;

	/* set all the rest segments */
	for (i = 1; len > 0; i++) {
		dmac->tag = i;
		dmac->mapent = kvtop((caddr_t)addr) >> DMAC_SEG_SHIFT;
		len -= DMAC_SEG_SIZE;
		addr += DMAC_SEG_SIZE;
	}
	/* terminate TAG */
	dmac->tag = 0;

	/*
	 * Now from the 5380-internal DMA registers.
	 */
	if (dh->dh_flags & SIDH_OUT) {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_OUT);
		NCR5380_WRITE(ncr_sc, sci_icmd, SCI_ICMD_DATA);
		NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode)
		    | SCI_MODE_DMA);

		/* set Dir */
		dmac->ctl = 0;

		/* start DMA */
		NCR5380_WRITE(ncr_sc, sci_dma_send, 0);
		dmac->ctl = DC_CTL_ENB;
	} else {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_IN);
		NCR5380_WRITE(ncr_sc, sci_icmd, 0);
		NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode)
		    | SCI_MODE_DMA);

		/* set Dir */
		dmac->ctl = DC_CTL_MOD;

		/* start DMA */
		NCR5380_WRITE(ncr_sc, sci_irecv, 0);
		dmac->ctl = DC_CTL_MOD | DC_CTL_ENB;
	}
	ncr_sc->sc_state |= NCR_DOINGDMA;
}

/*
 * When?
 */
void
si_dma_poll(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	printf("si_dma_poll\n");
}

/*
 * news68k (probabry) does not use the EOP signal.
 */
void
si_dma_eop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	printf("si_dma_eop\n");
}

void
si_dma_stop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	volatile struct dma_regs *dmac = sc->sc_regs;
	struct sci_req *sr = ncr_sc->sc_current;
	struct si_dma_handle *dh = sr->sr_dma_hand;
	int resid, ntrans, i;

	/* check DMAC interrupt status */
	if ((dmac->stat & DC_ST_INT) == 0) {
#ifdef DEBUG
		printf("si_dma_stop: no DMA interrupt");
#endif
		return; /* XXX */
	}

	if ((ncr_sc->sc_state & NCR_DOINGDMA) == 0) {
#ifdef DEBUG
		printf("si_dma_stop: dma not running\n");
#endif
		return;
	}
	ncr_sc->sc_state &= ~NCR_DOINGDMA;

	/* OK, have either phase mis-match or end of DMA. */
	/* Set an impossible phase to prevent data movement? */
	NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_INVALID);

	/* Note that timeout may have set the error flag. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		goto out;

	/*
	 * Sometimes the FIFO buffer isn't drained when the
	 * interrupt is posted. Just loop here and hope that
	 * it will drain soon.
	 */
	for (i = 0; i < 200000; i++) { /* 2 sec */
		resid = dmac->tcnt;
		if (resid == 0)
			break;
		DELAY(10);
	}

	if (resid)
		printf("si_dma_stop: resid=0x%x\n", resid);

	ntrans = dh->dh_len - resid;

	ncr_sc->sc_dataptr += ntrans;
	ncr_sc->sc_datalen -= ntrans;

	if ((dh->dh_flags & SIDH_OUT) == 0) {
		PCIA();
	}

out:
	NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode) &
	    ~(SCI_MODE_DMA));
	NCR5380_WRITE(ncr_sc, sci_icmd, 0);
}
