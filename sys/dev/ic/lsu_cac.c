/*	$NetBSD$	*/

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
 * Compaq array controller front-end for lsu(4) driver.
 */

#include "rnd.h"

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

#include <dev/lsuvar.h>

#include <dev/ic/cacreg.h>
#include <dev/ic/cacvar.h>

struct lsu_cac_softc {
	struct	lsu_softc sc_lsu;
	int	sc_hwunit;
};

static void	lsu_cac_attach(struct device *, struct device *, void *);
static void	lsu_cac_done(struct device *, void *, int);
static int	lsu_cac_dump(struct lsu_softc *, void *, int, int);
static int	lsu_cac_match(struct device *, struct cfdata *, void *);
static int	lsu_cac_start(struct lsu_softc *, struct buf *);

struct cfattach lsu_cac_ca = {
	sizeof(struct lsu_cac_softc), lsu_cac_match, lsu_cac_attach
};

static int
lsu_cac_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
lsu_cac_attach(struct device *parent, struct device *self, void *aux)
{
	struct cac_drive_info dinfo;
	struct cac_attach_args *caca;
	struct lsu_softc *lsu;
	struct lsu_cac_softc *sc;
	struct cac_softc *cac;
	const char *type;

	sc = (struct lsu_cac_softc *)self;
	lsu = &sc->sc_lsu;
	caca = (struct cac_attach_args *)aux;
	sc->sc_hwunit = caca->caca_unit;
	cac = (struct cac_softc *)parent;

	if (cac_cmd(cac, CAC_CMD_GET_LOG_DRV_INFO, &dinfo, sizeof(dinfo),
	    sc->sc_hwunit, 0, CAC_CCB_DATA_IN, NULL)) {
		printf("%s: CMD_GET_LOG_DRV_INFO failed\n", self->dv_xname);
		return;
	}

	lsu->sc_ncylinders = CAC_GET2(dinfo.ncylinders);
	lsu->sc_nheads = CAC_GET1(dinfo.nheads);
	lsu->sc_nsectors = CAC_GET1(dinfo.nsectors);
	lsu->sc_secsize = CAC_GET2(dinfo.secsize);
	lsu->sc_maxxfer = CAC_MAX_XFER;
	lsu->sc_maxqueuecnt = CAC_MAX_CCBS / cac->sc_nunits;	/* XXX */
	lsu->sc_secperunit = lsu->sc_ncylinders * lsu->sc_nheads *
	    lsu->sc_nsectors;
	lsu->sc_start = lsu_cac_start;
	lsu->sc_dump = lsu_cac_dump;

	switch (CAC_GET1(dinfo.mirror)) {
	case 0:
		type = "standalone disk or RAID0";
		break;
	case 1:
		type = "RAID4";
		break;
	case 2:
		type = "RAID1";
		break;
	case 3:
		type = "RAID5";
		break;
	default:
		type = "unknown type of";
		break;
	}

	printf(": %s array\n", type);

	/* XXX We should verify this... */
	lsu->sc_flags = LSUF_ENABLED;
	lsuattach(lsu);
}

static int
lsu_cac_start(struct lsu_softc *lsu, struct buf *bp)
{
	int flags, cmd;
	struct cac_softc *cac;
	struct lsu_cac_softc *sc;
	struct cac_context cc;
	
	sc = (struct lsu_cac_softc *)lsu;
	cac = (struct cac_softc *)lsu->sc_dv.dv_parent;

	cc.cc_handler = lsu_cac_done;
	cc.cc_context = bp;
	cc.cc_dv = &lsu->sc_dv;

	if ((bp->b_flags & B_READ) == 0) {
		cmd = CAC_CMD_WRITE;
		flags = CAC_CCB_DATA_OUT;
	} else {
		cmd = CAC_CMD_READ;
		flags = CAC_CCB_DATA_IN;
	}

	return (cac_cmd(cac, cmd, bp->b_data, bp->b_bcount, sc->sc_hwunit,
	    bp->b_rawblkno, flags, &cc));
}

static int
lsu_cac_dump(struct lsu_softc *lsu, void *data, int blkno, int blkcnt)
{
	struct lsu_cac_softc *sc;

	sc = (struct lsu_cac_softc *)lsu;

	return (cac_cmd((struct cac_softc *)lsu->sc_dv.dv_parent,
	    CAC_CMD_WRITE_MEDIA, data, blkcnt * lsu->sc_secsize,
	    sc->sc_hwunit, blkno, CAC_CCB_DATA_OUT, NULL));
}

static void
lsu_cac_done(struct device *dv, void *context, int error)
{
	struct buf *bp;

	bp = context;

	if (error) {
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
		bp->b_resid = bp->b_bcount;
	} else
		bp->b_resid = 0;

	lsudone((struct lsu_softc *)dv, bp);
}
