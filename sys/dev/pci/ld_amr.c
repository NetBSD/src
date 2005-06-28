/*	$NetBSD: ld_amr.c,v 1.7 2005/06/28 00:28:42 thorpej Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
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
 * AMI RAID controller front-end for ld(4) driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_amr.c,v 1.7 2005/06/28 00:28:42 thorpej Exp $");

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

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/ldvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/amrreg.h>
#include <dev/pci/amrvar.h>

struct ld_amr_softc {
	struct	ld_softc sc_ld;
	int	sc_hwunit;
};

static int	ld_amr_dobio(struct ld_amr_softc *, void *, int, int, int,
			     struct buf *);
static int	ld_amr_dump(struct ld_softc *, void *, int, int);
static void	ld_amr_handler(struct amr_ccb *);
static int	ld_amr_start(struct ld_softc *, struct buf *);

static int
ld_amr_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
ld_amr_attach(struct device *parent, struct device *self, void *aux)
{
	struct amr_attach_args *amra;
	struct ld_amr_softc *sc;
	struct ld_softc *ld;
	struct amr_softc *amr;
	const char *statestr;
	int happy;

	sc = (struct ld_amr_softc *)self;
	ld = &sc->sc_ld;
	amr = (struct amr_softc *)parent;
	amra = aux;

	sc->sc_hwunit = amra->amra_unit;
	ld->sc_maxxfer = amr_max_xfer;
	ld->sc_maxqueuecnt = (amr->amr_maxqueuecnt - AMR_NCCB_RESV)
	    / amr->amr_numdrives;
	ld->sc_secperunit = amr->amr_drive[sc->sc_hwunit].al_size;
	ld->sc_secsize = AMR_SECTOR_SIZE;
	ld->sc_start = ld_amr_start;
	ld->sc_dump = ld_amr_dump;

	if (ld->sc_maxqueuecnt > AMR_MAX_CMDS_PU)
		ld->sc_maxqueuecnt = AMR_MAX_CMDS_PU;

	/*
	 * Print status information and attach to the ld driver proper.
	 */
	statestr = amr_drive_state(amr->amr_drive[sc->sc_hwunit].al_state,
	    &happy);
	if (happy)
		ld->sc_flags = LDF_ENABLED;
	aprint_normal(": RAID %d, %s\n",
	    amr->amr_drive[sc->sc_hwunit].al_properties & AMR_DRV_RAID_MASK,
	    statestr);

	ldattach(ld);
}

CFATTACH_DECL(ld_amr, sizeof(struct ld_amr_softc),
    ld_amr_match, ld_amr_attach, NULL, NULL);

static int
ld_amr_dobio(struct ld_amr_softc *sc, void *data, int datasize,
	     int blkno, int dowrite, struct buf *bp)
{
	struct amr_ccb *ac;
	struct amr_softc *amr;
	struct amr_mailbox_cmd *mb;
	int s, rv;

	amr = (struct amr_softc *)sc->sc_ld.sc_dv.dv_parent;

	if ((rv = amr_ccb_alloc(amr, &ac)) != 0)
		return (rv);

	mb = &ac->ac_cmd;
	mb->mb_command = (dowrite ? AMR_CMD_LWRITE : AMR_CMD_LREAD);
	mb->mb_drive = sc->sc_hwunit;
	mb->mb_blkcount = htole16(datasize / AMR_SECTOR_SIZE);
	mb->mb_lba = htole32(blkno);

	if ((rv = amr_ccb_map(amr, ac, data, datasize, dowrite)) != 0) {
		amr_ccb_free(amr, ac);
		return (rv);
	}

	if (bp == NULL) {
		/*
		 * Polled commands must not sit on the software queue.  Wait
		 * up to 30 seconds for the command to complete.
		 */
		s = splbio();
		rv = amr_ccb_poll(amr, ac, 30000);
		splx(s);
		amr_ccb_unmap(amr, ac);
		amr_ccb_free(amr, ac);
	} else {
		ac->ac_handler = ld_amr_handler;
		ac->ac_context = bp;
		ac->ac_dv = (struct device *)sc;
		amr_ccb_enqueue(amr, ac);
		rv = 0;
	}

	return (rv);
}

static int
ld_amr_start(struct ld_softc *ld, struct buf *bp)
{

	return (ld_amr_dobio((struct ld_amr_softc *)ld, bp->b_data,
	    bp->b_bcount, bp->b_rawblkno, (bp->b_flags & B_READ) == 0, bp));
}

static void
ld_amr_handler(struct amr_ccb *ac)
{
	struct buf *bp;
	struct ld_amr_softc *sc;
	struct amr_softc *amr;

	bp = ac->ac_context;
	sc = (struct ld_amr_softc *)ac->ac_dv;
	amr = (struct amr_softc *)sc->sc_ld.sc_dv.dv_parent;

	if (ac->ac_status != AMR_STATUS_SUCCESS) {
		printf("%s: cmd status 0x%02x\n", sc->sc_ld.sc_dv.dv_xname,
		    ac->ac_status);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
	} else
		bp->b_resid = 0;

	amr_ccb_unmap(amr, ac);
	amr_ccb_free(amr, ac);
	lddone(&sc->sc_ld, bp);
}

static int
ld_amr_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_amr_softc *sc;

	sc = (struct ld_amr_softc *)ld;

	return (ld_amr_dobio(sc, data, blkcnt * ld->sc_secsize, blkno, 1,
	    NULL));
}
