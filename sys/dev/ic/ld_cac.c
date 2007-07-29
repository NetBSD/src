/*	$NetBSD: ld_cac.c,v 1.18 2007/07/29 12:50:20 ad Exp $	*/

/*-
 * Copyright (c) 2000, 2006 The NetBSD Foundation, Inc.
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
 * Compaq array controller front-end for ld(4) driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_cac.c,v 1.18 2007/07/29 12:50:20 ad Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>

#include <dev/ldvar.h>

#include <dev/ic/cacreg.h>
#include <dev/ic/cacvar.h>

struct ld_cac_softc {
	struct	ld_softc sc_ld;
	kmutex_t *sc_mutex;
	int	sc_hwunit;
	int	sc_serrcnt;
	struct	timeval sc_serrtm;
};

void	ld_cac_attach(struct device *, struct device *, void *);
void	ld_cac_done(struct device *, void *, int);
int	ld_cac_dump(struct ld_softc *, void *, int, int);
int	ld_cac_match(struct device *, struct cfdata *, void *);
int	ld_cac_start(struct ld_softc *, struct buf *);

static const struct	timeval ld_cac_serrintvl = { 60, 0 };

CFATTACH_DECL(ld_cac, sizeof(struct ld_cac_softc),
    ld_cac_match, ld_cac_attach, NULL, NULL);

int
ld_cac_match(struct device *parent, struct cfdata *match,
    void *aux)
{

	return (1);
}

void
ld_cac_attach(struct device *parent, struct device *self, void *aux)
{
	struct cac_drive_info dinfo;
	struct cac_attach_args *caca;
	struct ld_softc *ld;
	struct ld_cac_softc *sc;
	struct cac_softc *cac;
	const char *type;

	sc = (struct ld_cac_softc *)self;
	ld = &sc->sc_ld;
	caca = (struct cac_attach_args *)aux;
	sc->sc_hwunit = caca->caca_unit;
	cac = (struct cac_softc *)parent;
	sc->sc_mutex = &cac->sc_mutex;

	if (cac_cmd(cac, CAC_CMD_GET_LOG_DRV_INFO, &dinfo, sizeof(dinfo),
	    sc->sc_hwunit, 0, CAC_CCB_DATA_IN, NULL)) {
		aprint_error(": CMD_GET_LOG_DRV_INFO failed\n");
		return;
	}

	ld->sc_secsize = CAC_GET2(dinfo.secsize);
	ld->sc_maxxfer = CAC_MAX_XFER;
	ld->sc_maxqueuecnt = CAC_MAX_CCBS / cac->sc_nunits;	/* XXX */
	ld->sc_secperunit = CAC_GET2(dinfo.ncylinders) *
	    CAC_GET1(dinfo.nheads) * CAC_GET1(dinfo.nsectors);
	ld->sc_start = ld_cac_start;
	ld->sc_dump = ld_cac_dump;

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

	aprint_normal(": %s array\n", type);

	/* XXX We should verify this... */
	ld->sc_flags = LDF_ENABLED;
	ldattach(ld);
}

int
ld_cac_start(struct ld_softc *ld, struct buf *bp)
{
	int flags, cmd;
	struct cac_softc *cac;
	struct ld_cac_softc *sc;
	struct cac_context cc;

	sc = (struct ld_cac_softc *)ld;
	cac = (struct cac_softc *)device_parent(&ld->sc_dv);

	cc.cc_handler = ld_cac_done;
	cc.cc_context = bp;
	cc.cc_dv = &ld->sc_dv;

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

int
ld_cac_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_cac_softc *sc;

	sc = (struct ld_cac_softc *)ld;

	return (cac_cmd((struct cac_softc *)device_parent(&ld->sc_dv),
	    CAC_CMD_WRITE_MEDIA, data, blkcnt * ld->sc_secsize,
	    sc->sc_hwunit, blkno, CAC_CCB_DATA_OUT, NULL));
}

void
ld_cac_done(struct device *dv, void *context, int error)
{
	struct buf *bp;
	struct ld_cac_softc *sc;
	int rv;

	bp = context;
	rv = 0;
	sc = device_private(dv);

	if ((error & CAC_RET_CMD_REJECTED) == CAC_RET_CMD_REJECTED) {
		printf("%s: command rejected\n", dv->dv_xname);
		rv = EIO;
	}
	if (rv == 0 && (error & CAC_RET_INVAL_BLOCK) != 0) {
		printf("%s: invalid request block\n", dv->dv_xname);
		rv = EIO;
	}
	if (rv == 0 && (error & CAC_RET_HARD_ERROR) != 0) {
		printf("%s: hard error\n", dv->dv_xname);
		rv = EIO;
	}
	if (rv == 0 && (error & CAC_RET_SOFT_ERROR) != 0) {
		sc->sc_serrcnt++;
		if (ratecheck(&sc->sc_serrtm, &ld_cac_serrintvl)) {
			sc->sc_serrcnt = 0;
			printf("%s: %d soft errors; array may be degraded\n",
			    dv->dv_xname, sc->sc_serrcnt);
		}
	}

	if (rv) {
		bp->b_error = rv;
		bp->b_resid = bp->b_bcount;
	} else
		bp->b_resid = 0;

	mutex_exit(sc->sc_mutex);
	lddone((struct ld_softc *)dv, bp);
	mutex_enter(sc->sc_mutex);
}
