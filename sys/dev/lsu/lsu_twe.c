/*	$NetBSD: lsu_twe.c,v 1.4.2.1 2000/11/20 11:41:27 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * 3ware "Escalade" RAID controller front-end for lsu(4) driver.
 */

#include "rnd.h"
#include "opt_twe.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>

#include <dev/lsu/lsuvar.h>

#include <dev/pci/twereg.h>
#include <dev/pci/twevar.h>

struct lsu_twe_softc {
	struct	lsu_softc sc_lsu;
	struct	buf_queue sc_bufq;
	int	sc_hwunit;
	int	sc_queuecnt;
};

static void	lsu_twe_attach(struct device *, struct device *, void *);
static int	lsu_twe_dobio(struct lsu_twe_softc *, int, void *, int, int,
			      int, struct twe_context *);
static int	lsu_twe_dump(struct lsu_softc *, void *, int, int);
static void	lsu_twe_handler(struct twe_ccb *, int);
static int	lsu_twe_match(struct device *, struct cfdata *, void *);
static int	lsu_twe_start(struct lsu_softc *, struct buf *);

struct cfattach lsu_twe_ca = {
	sizeof(struct lsu_twe_softc), lsu_twe_match, lsu_twe_attach
};

static int
lsu_twe_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
lsu_twe_attach(struct device *parent, struct device *self, void *aux)
{
	struct twe_attach_args *twea;
	struct lsu_twe_softc *sc;
	struct lsu_softc *lsu;
	struct twe_softc *twe;

	sc = (struct lsu_twe_softc *)self;
	lsu = &sc->sc_lsu;
	twe = (struct twe_softc *)parent;
	twea = aux;

	sc->sc_hwunit = twea->twea_unit;
	lsu->sc_flags = LSUF_ENABLED;
	lsu->sc_maxxfer = TWE_MAX_XFER;
	lsu->sc_secperunit = twe->sc_dsize[twea->twea_unit];
	lsu->sc_secsize = TWE_SECTOR_SIZE;
	lsu->sc_start = lsu_twe_start;
	lsu->sc_dump = lsu_twe_dump;

	/* Build synthetic geometry as per controller internal rules. */
	if (lsu->sc_secperunit > 0x200000) {
		lsu->sc_nheads = 255;
		lsu->sc_nsectors = 63;
	} else {
		lsu->sc_nheads = 64;
		lsu->sc_nsectors = 32;
	}
	lsu->sc_ncylinders = lsu->sc_secperunit /
	    (lsu->sc_nheads * lsu->sc_nsectors);

	printf("\n");
	BUFQ_INIT(&sc->sc_bufq);
	lsuattach(lsu);
}

static int
lsu_twe_dobio(struct lsu_twe_softc *sc, int unit, void *data, int datasize,
	      int blkno, int dowrite, struct twe_context *tx)
{
	struct twe_ccb *ccb;
	struct twe_cmd *tc;
	struct twe_softc *twe;
	int s, rv;

	twe = (struct twe_softc *)sc->sc_lsu.sc_dv.dv_parent;

	if ((rv = twe_ccb_alloc(twe, &ccb, tx == NULL)) != 0)
		return (rv);

	ccb->ccb_data = data;
	ccb->ccb_datasize = datasize;
	tc = ccb->ccb_cmd;

	/* Build the command. */
	tc->tc_size = 3;
	tc->tc_unit = unit;
	tc->tc_count = htole16(datasize / TWE_SECTOR_SIZE);
	tc->tc_args.io.lba = htole32(blkno);

	if (dowrite) {
		ccb->ccb_flags |= TWE_CCB_DATA_OUT;
		tc->tc_opcode = TWE_OP_WRITE | (tc->tc_size << 5);
	} else {
		ccb->ccb_flags |= TWE_CCB_DATA_IN;
		tc->tc_opcode = TWE_OP_READ | (tc->tc_size << 5);
	}

	/* Map the data transfer. */
	if ((rv = twe_ccb_map(twe, ccb)) != 0) {
		twe_ccb_free(twe, ccb);
		return (rv);
	}

	if (tx == NULL) {
		/*
		 * Polled commands must not sit on the software queue.  Wait
		 * up to 2 seconds for the command to complete.
		 */
		s = splbio();
		if ((rv = twe_ccb_submit(twe, ccb)) == 0)
			rv = twe_ccb_poll(twe, ccb, 2000);
		twe_ccb_unmap(twe, ccb);
		twe_ccb_free(twe, ccb);
		splx(s);
	} else {
		memcpy(&ccb->ccb_tx, tx, sizeof(struct twe_context));
		twe_ccb_enqueue(twe, ccb);
		rv = 0;
	}

	return (rv);
}

static int
lsu_twe_start(struct lsu_softc *lsu, struct buf *bp)
{
	struct twe_context tx;
	struct lsu_twe_softc *sc;
	int s, rv;

	sc = (struct lsu_twe_softc *)lsu;

	s = splbio();
	if (bp != NULL) {
		if (sc->sc_queuecnt == TWE_MAX_PU_QUEUECNT) {
			BUFQ_INSERT_TAIL(&sc->sc_bufq, bp);
			splx(s);
			return (0);
		}
	} else {
		bp = BUFQ_FIRST(&sc->sc_bufq);
		BUFQ_REMOVE(&sc->sc_bufq, bp);
	}
	sc->sc_queuecnt++;
	splx(s);

	tx.tx_handler = lsu_twe_handler;
	tx.tx_context = bp;
	tx.tx_dv = &lsu->sc_dv;

	if ((rv = lsu_twe_dobio(sc, sc->sc_hwunit, bp->b_data, bp->b_bcount,
	    bp->b_rawblkno, (bp->b_flags & B_READ) == 0, &tx)) != 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = rv;
		bp->b_resid = bp->b_bcount;
	    	s = splbio();
	    	sc->sc_queuecnt--;
		lsudone(lsu, bp);
		splx(s);
	}

	return (0);
}

static void
lsu_twe_handler(struct twe_ccb *ccb, int error)
{
	struct buf *bp;
	struct twe_context *tx;
	struct lsu_twe_softc *sc;
	struct twe_softc *twe;

	tx = &ccb->ccb_tx;
	bp = tx->tx_context;
	sc = (struct lsu_twe_softc *)tx->tx_dv;
	twe = (struct twe_softc *)sc->sc_lsu.sc_dv.dv_parent;

	twe_ccb_unmap(twe, ccb);
	twe_ccb_free(twe, ccb);

	if (--sc->sc_queuecnt < TWE_MAX_PU_QUEUECNT &&
	    BUFQ_FIRST(&sc->sc_bufq) != NULL)
		lsu_twe_start(&sc->sc_lsu, NULL);

	if (error) {
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
		bp->b_resid = bp->b_bcount;
	} else
		bp->b_resid = 0;

	lsudone(&sc->sc_lsu, bp);
}

static int
lsu_twe_dump(struct lsu_softc *lsu, void *data, int blkno, int blkcnt)
{
	struct lsu_twe_softc *sc;

	sc = (struct lsu_twe_softc *)lsu;

	return (lsu_twe_dobio(sc, sc->sc_hwunit, data, blkcnt * lsu->sc_secsize,
	    blkno, 1, NULL));
}
