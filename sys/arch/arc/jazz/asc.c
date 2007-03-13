/*	$NetBSD: asc.c,v 1.20.2.1 2007/03/13 16:49:55 ad Exp $	*/

/*
 * Copyright (c) 2003 Izumi Tsutsui.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: asc.c,v 1.20.2.1 2007/03/13 16:49:55 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <arc/jazz/jazziovar.h>
#include <arc/jazz/dma.h>
#include <arc/jazz/pica.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#define ASC_NPORTS	0x10
#define ASC_ID_53CF94	0xa2	/* XXX should be in MI ncr53c9xreg.h? */
#define ASC_ID_FAS216	0x12	/* XXX should be in MI ncr53c9xreg.h? */

struct asc_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* glue to MI code */

	bus_space_tag_t sc_iot;		/* bus space tag */
	bus_space_handle_t sc_ioh;	/* bus space handle */
	bus_space_handle_t sc_dmaioh;	/* bus space handle for DMAC */

	bus_dma_tag_t sc_dmat;		/* DMA tag */
	bus_dmamap_t sc_dmamap;		/* DMA map for transfers */

	int     sc_active;              /* DMA state */
	int     sc_datain;              /* DMA Data Direction */
	size_t  sc_dmasize;             /* DMA size */
	char    **sc_dmaaddr;           /* DMA address */
	size_t  *sc_dmalen;             /* DMA length */
};

/*
 * Autoconfiguration data for config.
 */
int asc_match(struct device *, struct cfdata *, void *);
void asc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(asc, sizeof(struct asc_softc),
    asc_match, asc_attach, NULL, NULL);

static void asc_minphys(struct buf *);

/*
 *  Functions and the switch for the MI code.
 */
u_char asc_read_reg(struct ncr53c9x_softc *, int);
void asc_write_reg(struct ncr53c9x_softc *, int, u_char);
int asc_dma_isintr(struct ncr53c9x_softc *);
void asc_dma_reset(struct ncr53c9x_softc *);
int asc_dma_intr(struct ncr53c9x_softc *);
int asc_dma_setup(struct ncr53c9x_softc *, void **, size_t *, int, size_t *);
void asc_dma_go(struct ncr53c9x_softc *);
void asc_dma_stop(struct ncr53c9x_softc *);
int asc_dma_isactive(struct ncr53c9x_softc *);

struct ncr53c9x_glue asc_glue = {
	asc_read_reg,
	asc_write_reg,
	asc_dma_isintr,
	asc_dma_reset,
	asc_dma_intr,
	asc_dma_setup,
	asc_dma_go,
	asc_dma_stop,
	asc_dma_isactive,
	NULL			/* gl_clear_latched_intr */
};

/*
 * Match driver based on name
 */
int
asc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct jazzio_attach_args *ja = aux;

	if (strcmp(ja->ja_name, "ESP216") != 0)
		return 0;
	return 1;
}

void
asc_attach(struct device *parent, struct device *self, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	struct asc_softc *asc = (void *)self;
	struct ncr53c9x_softc *sc = &asc->sc_ncr53c9x;
	bus_space_tag_t iot;
	uint8_t asc_id;

#if 0
	/* Need info from platform dependent config?? */
	if (asc_conf == NULL)
		panic("asc_conf isn't initialized");
#endif

	sc->sc_glue = &asc_glue;

	asc->sc_iot = iot = ja->ja_bust;
	asc->sc_dmat = ja->ja_dmat;

	if (bus_space_map(iot, ja->ja_addr, ASC_NPORTS, 0, &asc->sc_ioh)) {
		printf(": unable to map I/O space\n");
		return;
	}

	if (bus_space_map(iot, R4030_SYS_DMA0_REGS, R4030_DMA_RANGE,
	    0, &asc->sc_dmaioh)) {
		printf(": unable to map DMA I/O space\n");
		goto out1;
	}

	if (bus_dmamap_create(asc->sc_dmat, MAXPHYS, 1, MAXPHYS, 0,
	    BUS_DMA_ALLOCNOW|BUS_DMA_NOWAIT, &asc->sc_dmamap)) {
		printf(": unable to create DMA map\n");
		goto out2;
	}

	/*
	 * XXX More of this should be in ncr53c9x_attach(), but
	 * XXX should we really poke around the chip that much in
	 * XXX the MI code?  Think about this more...
	 */

	/*
	 * Set up static configuration info.
	 */
	sc->sc_id = 7; /* XXX should be taken from ARC BIOS */
	sc->sc_cfg1 = sc->sc_id | NCRCFG1_PARENB;

	/* identify 53CF9x-2 or not */
	asc_write_reg(sc, NCR_CMD, NCRCMD_RSTCHIP);
	DELAY(25);
	asc_write_reg(sc, NCR_CMD, NCRCMD_DMA | NCRCMD_NOP);
	DELAY(25);
	asc_write_reg(sc, NCR_CFG2, NCRCFG2_FE);
	DELAY(25);
	asc_write_reg(sc, NCR_CMD, NCRCMD_DMA | NCRCMD_NOP);
	DELAY(25);
	asc_id = asc_read_reg(sc, NCR_TCH);
	if (asc_id == ASC_ID_53CF94 || asc_id == ASC_ID_FAS216) {
		/* XXX should be have NCR_VARIANT_NCR53CF94? */
		sc->sc_rev = NCR_VARIANT_NCR53C94;
		sc->sc_cfg2 = NCRCFG2_SCSI2 | NCRCFG2_FE;
		sc->sc_cfg3 = NCRF9XCFG3_IDM | NCRF9XCFG3_FCLK;
		sc->sc_features = NCR_F_FASTSCSI;
		sc->sc_cfg3_fscsi = NCRF9XCFG3_FSCSI;
		sc->sc_freq = 40; /* MHz */
		sc->sc_maxxfer = 16 * 1024 * 1024;
	} else {
		sc->sc_rev = NCR_VARIANT_NCR53C94;
		sc->sc_freq = 25; /* MHz */
		sc->sc_maxxfer = 64 * 1024;
	}

	/*
	 * XXX minsync and maxxfer _should_ be set up in MI code,
	 * XXX but it appears to have some dependency on what sort
	 * XXX of DMA we're hooked up to, etc.
	 */

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	sc->sc_minsync = 1000 / sc->sc_freq;

	/* establish interrupt */
	jazzio_intr_establish(ja->ja_intr, ncr53c9x_intr, asc);

	/* Do the common parts of attachment. */
	sc->sc_adapter.adapt_minphys = asc_minphys;
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	ncr53c9x_attach(sc);

#if 0
	/* Turn on target selection using the `DMA' method */
	sc->sc_features |= NCR_F_DMASELECT;
#endif
	return;

 out2:
	bus_space_unmap(iot, asc->sc_dmaioh, R4030_DMA_RANGE);
 out1:
	bus_space_unmap(iot, asc->sc_ioh, ASC_NPORTS);
}


static void
asc_minphys(struct buf *bp)
{

#define ASC_MAX_XFER	(32 * 1024)	/* XXX can't xfer 64kbytes? */

	if (bp->b_bcount > ASC_MAX_XFER)
		bp->b_bcount = ASC_MAX_XFER;
	minphys(bp);
}

/*
 * Glue functions.
 */

u_char
asc_read_reg(struct ncr53c9x_softc *sc, int reg)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return bus_space_read_1(asc->sc_iot, asc->sc_ioh, reg);
}

void
asc_write_reg(struct ncr53c9x_softc *sc, int reg, u_char val)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	bus_space_write_1(asc->sc_iot, asc->sc_ioh, reg, val);
}

int
asc_dma_isintr(struct ncr53c9x_softc *sc)
{

	return asc_read_reg(sc, NCR_STAT) & NCRSTAT_INT;
}

void
asc_dma_reset(struct ncr53c9x_softc *sc)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	/* halt DMA */
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_ENAB, 0);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_MODE, 0);
}

int
asc_dma_intr(struct ncr53c9x_softc *sc)
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	int datain, resid, trans;

	datain = asc->sc_datain;

#ifdef DIAGNOSTIC
	/* This is an "assertion" :) */
	if (asc->sc_active == 0)
		panic("asc_dma_intr: DMA wasn't active");
#endif

	/* DMA has stopped */

	asc->sc_active = 0;

	if (asc->sc_dmasize == 0) {
		/* A "Transfer Pad" operation complete */
		NCR_DMA(("dmaintr: discarded %d bytes (tcl=%d, tcm=%d)\n",
		    NCR_READ_REG(sc, NCR_TCL) |
		    (NCR_READ_REG(sc, NCR_TCM) << 8),
		    NCR_READ_REG(sc, NCR_TCL),
		    NCR_READ_REG(sc, NCR_TCM)));

		return 0;
	}

	resid = 0;

	/*
	 * If a transfer onto the SCSI bus gets interrupted by the device
	 * (e.g. for a SAVEPOINTER message), the data in the FIFO counts
	 * as residual since the ESP counter registers get decremented as
	 * bytes are clocked into the FIFO.
	 */
	if (!datain &&
	    (resid = (asc_read_reg(sc, NCR_FFLAG) & NCRFIFO_FF)) != 0) {
		NCR_DMA(("asc_dma_intr: empty asc FIFO of %d ", resid));
	}

	if ((sc->sc_espstat & NCRSTAT_TC) == 0) {
		/*
		 * `Terminal count' is off, so read the residue
		 * out of the ASC counter registers.
		 */
		resid += (NCR_READ_REG(sc, NCR_TCL) |
		    (NCR_READ_REG(sc, NCR_TCM) << 8) |
		    ((sc->sc_cfg2 & NCRCFG2_FE)
		    ? (NCR_READ_REG(sc, NCR_TCH) << 16) : 0));

		if (resid == 0 && asc->sc_dmasize == 65536 &&
		    (sc->sc_cfg2 & NCRCFG2_FE) == 0)
			/* A transfer of 64K is encoded as `TCL=TCM=0' */
			resid = 65536;
	}

	/* halt DMA */
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_COUNT, 0);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_ENAB, 0);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_MODE, 0);

	bus_dmamap_sync(asc->sc_dmat, asc->sc_dmamap,
	    0, asc->sc_dmamap->dm_mapsize,
	    datain ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(asc->sc_dmat, asc->sc_dmamap);

	trans = asc->sc_dmasize - resid;

	if (trans < 0) {		/* transfered < 0 ? */
#if 0
		/*
		 * This situation can happen in perfectly normal operation
		 * if the ESP is reselected while using DMA to select
		 * another target.  As such, don't print the warning.
		 */
		printf("%s: xfer (%d) > req (%d)\n",
		    sc->sc_dev.dv_xname, trans, asc->sc_dmasize);
#endif
		trans = asc->sc_dmasize;
	}
	NCR_DMA(("dmaintr: tcl=%d, tcm=%d, tch=%d; trans=%d, resid=%d\n",
	    NCR_READ_REG(sc, NCR_TCL),
	    NCR_READ_REG(sc, NCR_TCM),
	    (sc->sc_cfg2 & NCRCFG2_FE) ? NCR_READ_REG(sc, NCR_TCH) : 0,
	    trans, resid));

	*asc->sc_dmalen -= trans;
	*asc->sc_dmaaddr += trans;

	return 0;
}

int
asc_dma_setup(struct ncr53c9x_softc *sc, void **addr, size_t *len,
    int datain, size_t *dmasize)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	/* halt DMA */
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_ENAB, 0);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_MODE, 0);

	asc->sc_dmaaddr = (char **)addr;
	asc->sc_dmalen = len;
	asc->sc_dmasize = *dmasize;
	asc->sc_datain = datain;

	/*
	 * No need to set up DMA in `Transfer Pad' operation.
	 */
	if (*dmasize == 0)
		return 0;

	bus_dmamap_load(asc->sc_dmat, asc->sc_dmamap, *addr, *len, NULL,
	    ((sc->sc_nexus->xs->xs_control & XS_CTL_NOSLEEP) ?
	    BUS_DMA_NOWAIT : BUS_DMA_WAITOK) | BUS_DMA_STREAMING |
	    (datain ? BUS_DMA_READ : BUS_DMA_WRITE));
	bus_dmamap_sync(asc->sc_dmat, asc->sc_dmamap,
	    0, asc->sc_dmamap->dm_mapsize,
	    datain ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	/* load transfer parameters */
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh,
	    R4030_DMA_ADDR, asc->sc_dmamap->dm_segs[0].ds_addr);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh,
	    R4030_DMA_COUNT, asc->sc_dmamap->dm_segs[0].ds_len);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh,
	    R4030_DMA_MODE, R4030_DMA_MODE_160NS | R4030_DMA_MODE_16);

	/* start DMA */
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh,
	    R4030_DMA_ENAB, R4030_DMA_ENAB_RUN |
	    (asc->sc_datain ? R4030_DMA_ENAB_READ : R4030_DMA_ENAB_WRITE));

	return 0;
}

void
asc_dma_go(struct ncr53c9x_softc *sc)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	/* No DMA transfer in Transfer Pad operation */
	if (asc->sc_dmasize == 0)
		return;

	asc->sc_active = 1;
}

void
asc_dma_stop(struct ncr53c9x_softc *sc)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	/* halt DMA */
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_ENAB, 0);
	bus_space_write_4(asc->sc_iot, asc->sc_dmaioh, R4030_DMA_MODE, 0);

	bus_dmamap_unload(asc->sc_dmat, asc->sc_dmamap);

	asc->sc_active = 0;
}

int
asc_dma_isactive(struct ncr53c9x_softc *sc)
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return asc->sc_active;
}
