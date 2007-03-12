/*	$NetBSD: si.c,v 1.19.2.2 2007/03/12 05:49:38 rmind Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: si.c,v 1.19.2.2 2007/03/12 05:49:38 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/cpu.h>
#include <m68k/cacheops.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/dmac_0266.h>

#include "ioconf.h"

#define MIN_DMA_LEN 128
#define DMAC_BASE	0xe0e80000 /* XXX */
#define SI_REGSIZE	8

struct si_softc {
	struct	ncr5380_softc	ncr_sc;
	int	sc_options;
	volatile struct dma_regs *sc_regs;
	int	sc_xlen;
};

static void si_attach(struct device *, struct device *, void *);
static int  si_match(struct device *, struct cfdata *, void *);
int  si_intr(int);

static void si_dma_alloc(struct ncr5380_softc *);
static void si_dma_free(struct ncr5380_softc *);
static void si_dma_start(struct ncr5380_softc *);
static void si_dma_poll(struct ncr5380_softc *);
static void si_dma_eop(struct ncr5380_softc *);
static void si_dma_stop(struct ncr5380_softc *);

CFATTACH_DECL(si, sizeof(struct si_softc),
    si_match, si_attach, NULL, NULL);

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

int si_options = 0x00;


static int
si_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hb_attach_args *ha = aux;
	int addr;

	if (strcmp(ha->ha_name, "si"))
		return 0;

	addr = IIOV(ha->ha_address);

	if (badaddr((void *)addr, 1))
		return 0;

	ha->ha_size = SI_REGSIZE;

	return 1;
}

/*
 * Card attach function
 */

static void
si_attach(struct device *parent, struct device *self, void *aux)
{
	struct si_softc *sc = (struct si_softc *)self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	struct cfdata *cf = device_cfdata(self);
	struct hb_attach_args *ha = aux;

	ncr_sc->sc_regt = ha->ha_bust;
	if (bus_space_map(ncr_sc->sc_regt, (bus_addr_t)ha->ha_address,
	    ha->ha_size, 0, &ncr_sc->sc_regh) != 0) {
		printf("can't map device space\n");
		return;
	}

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
	ncr_sc->sc_dma_start   = si_dma_start;
	ncr_sc->sc_dma_eop     = si_dma_eop;
	ncr_sc->sc_dma_stop    = si_dma_stop;

	if (sc->sc_options & SI_DISABLE_DMA)
		/* Override this function pointer. */
		ncr_sc->sc_dma_alloc = NULL;

	ncr_sc->sci_r0 = 0;
	ncr_sc->sci_r1 = 1;
	ncr_sc->sci_r2 = 2;
	ncr_sc->sci_r3 = 3;
	ncr_sc->sci_r4 = 4;
	ncr_sc->sci_r5 = 5;
	ncr_sc->sci_r6 = 6;
	ncr_sc->sci_r7 = 7;

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
si_intr(int unit)
{
	struct si_softc *sc;

	if (unit >= si_cd.cd_ndevs)
		return 0;

	sc = si_cd.cd_devs[unit];
	(void)ncr5380_intr(&sc->ncr_sc);

	return 0;
}

/*
 *  DMA routines for news1700 machines
 */
static void
si_dma_alloc(struct ncr5380_softc *ncr_sc)
{
	struct sci_req *sr = ncr_sc->sc_current;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("%s: DMA already in use", __func__);
#endif

	/*
	 * On news68k, SCSI has its own DMAC so no need allocate it.
	 * Just mark that DMA is available.
	 */
	sr->sr_dma_hand = (void *)-1;
}

static void
si_dma_free(struct ncr5380_softc *ncr_sc)
{
	struct sci_req *sr = ncr_sc->sc_current;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand == NULL)
		panic("%s: DMA not in use", __func__);
#endif

	sr->sr_dma_hand = NULL;
}


static void
si_dma_start(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	volatile struct dma_regs *dmac = sc->sc_regs;
	struct sci_req *sr = ncr_sc->sc_current;
	u_int addr, offset, rest;
	long len;
	int i;

	/* reset DMAC */
	dmac->ctl = DC_CTL_RST;
	dmac->ctl = 0;

	addr = (u_int)ncr_sc->sc_dataptr;
	offset = addr & DMAC_SEG_OFFSET;
	len = sc->sc_xlen = ncr_sc->sc_datalen;

	/* set DMA transfer length */
	dmac->tcnt = (uint32_t)len;

	/* set offset of first segment */
	dmac->offset = offset;

	/* set first DMA segment address */
	dmac->tag = 0;
	dmac->mapent = kvtop((void *)addr) >> DMAC_SEG_SHIFT;
	rest = DMAC_SEG_SIZE - offset;
	addr += rest;
	len -= rest;

	/* set all the rest segments */
	for (i = 1; len > 0; i++) {
		dmac->tag = i;
		dmac->mapent = kvtop((void *)addr) >> DMAC_SEG_SHIFT;
		len -= DMAC_SEG_SIZE;
		addr += DMAC_SEG_SIZE;
	}
	/* terminate TAG */
	dmac->tag = 0;

	if (sr->sr_xs->xs_control & XS_CTL_DATA_OUT) {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_OUT);
		NCR5380_WRITE(ncr_sc, sci_icmd, SCI_ICMD_DATA);
		NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode)
		    | SCI_MODE_DMA | SCI_MODE_DMA_IE);

		/* set Dir */
		dmac->ctl = 0;

		/* start DMA */
		NCR5380_WRITE(ncr_sc, sci_dma_send, 0);
		dmac->ctl = DC_CTL_ENB;
	} else {
		NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_DATA_IN);
		NCR5380_WRITE(ncr_sc, sci_icmd, 0);
		NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode)
		    | SCI_MODE_DMA | SCI_MODE_DMA_IE);

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
static void
si_dma_poll(struct ncr5380_softc *ncr_sc)
{

	printf("si_dma_poll\n");
}

/*
 * news68k (probably) does not use the EOP signal.
 */
static void
si_dma_eop(struct ncr5380_softc *ncr_sc)
{

	printf("si_dma_eop\n");
}

static void
si_dma_stop(struct ncr5380_softc *ncr_sc)
{
	struct si_softc *sc = (struct si_softc *)ncr_sc;
	volatile struct dma_regs *dmac = sc->sc_regs;
	struct sci_req *sr = ncr_sc->sc_current;
	int resid, ntrans;

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

	/* stop DMAC */
	resid = dmac->tcnt;
	dmac->ctl &= ~DC_CTL_ENB;

	/* OK, have either phase mis-match or end of DMA. */
	/* Set an impossible phase to prevent data movement? */
	NCR5380_WRITE(ncr_sc, sci_tcmd, PHASE_INVALID);

	/* Note that timeout may have set the error flag. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		goto out;

#ifdef DEBUG
	if (resid)
		printf("si_dma_stop: datalen = 0x%x, resid = 0x%x\n",
		    sc->sc_xlen, resid);
#endif

	ntrans = sc->sc_xlen - resid;

	ncr_sc->sc_dataptr += ntrans;
	ncr_sc->sc_datalen -= ntrans;

	if (sr->sr_xs->xs_control & XS_CTL_DATA_IN) {
		/* flush data cache */
		PCIA();
	}

 out:
	/* reset DMAC */
	dmac->ctl = DC_CTL_RST;
	dmac->ctl = 0;

	NCR5380_WRITE(ncr_sc, sci_mode, NCR5380_READ(ncr_sc, sci_mode) &
	    ~(SCI_MODE_DMA | SCI_MODE_DMA_IE));
	NCR5380_WRITE(ncr_sc, sci_icmd, 0);
}
