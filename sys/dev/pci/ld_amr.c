/*	$NetBSD: ld_amr.c,v 1.1.4.2 2002/02/28 04:14:03 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ld_amr.c,v 1.1.4.2 2002/02/28 04:14:03 nathanw Exp $");

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

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/ldvar.h>

#include <dev/pci/amrreg.h>
#include <dev/pci/amrvar.h>

struct ld_amr_softc {
	struct	ld_softc sc_ld;
	int	sc_hwunit;
};

void	ld_amr_attach(struct device *, struct device *, void *);
int	ld_amr_dobio(struct ld_amr_softc *, void *, int, int, int,
		     struct buf *);
int	ld_amr_dump(struct ld_softc *, void *, int, int);
void	ld_amr_handler(struct amr_ccb *);
int	ld_amr_match(struct device *, struct cfdata *, void *);
int	ld_amr_start(struct ld_softc *, struct buf *);

struct cfattach ld_amr_ca = {
	sizeof(struct ld_amr_softc), ld_amr_match, ld_amr_attach
};

struct {
	const char	*ds_descr;
	int	ds_enable;
} static const ld_amr_dstate[] = {
	{ "offline",	0 },
	{ "degraded",	LDF_ENABLED },
	{ "optimal",	LDF_ENABLED },
	{ "online",	LDF_ENABLED },
	{ "failed",	0 },
	{ "rebuilding",	LDF_ENABLED },
	{ "hotspare",	0 },
};

int
ld_amr_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

void
ld_amr_attach(struct device *parent, struct device *self, void *aux)
{
	struct amr_attach_args *amra;
	struct ld_amr_softc *sc;
	struct ld_softc *ld;
	struct amr_softc *amr;
	const char *statestr;
	int state;

	sc = (struct ld_amr_softc *)self;
	ld = &sc->sc_ld;
	amr = (struct amr_softc *)parent;
	amra = aux;

	sc->sc_hwunit = amra->amra_unit;
	ld->sc_maxxfer = AMR_MAX_XFER;
	ld->sc_maxqueuecnt = amr->amr_maxqueuecnt / amr->amr_numdrives;
	ld->sc_secperunit = amr->amr_drive[sc->sc_hwunit].al_size;
	ld->sc_secsize = AMR_SECTOR_SIZE;
	ld->sc_start = ld_amr_start;
	ld->sc_dump = ld_amr_dump;

	if (ld->sc_maxqueuecnt > AMR_MAX_CMDS_PU)
		ld->sc_maxqueuecnt = AMR_MAX_CMDS_PU;

	/*
	 * Print status information and attach to the ld driver proper.
	 */
	state = AMR_DRV_CURSTATE(amr->amr_drive[sc->sc_hwunit].al_state);
	if (state >= sizeof(ld_amr_dstate) / sizeof(ld_amr_dstate[0])) {
		ld->sc_flags = LDF_ENABLED;
		statestr = "status unknown";
	} else {
		ld->sc_flags = ld_amr_dstate[state].ds_enable;
		statestr = ld_amr_dstate[state].ds_descr;
	}

	printf(": RAID %d, %s\n",
	    amr->amr_drive[sc->sc_hwunit].al_properties & AMR_DRV_RAID_MASK,
	    statestr);

	ldattach(ld);
}

int
ld_amr_dobio(struct ld_amr_softc *sc, void *data, int datasize,
	     int blkno, int dowrite, struct buf *bp)
{
	struct amr_ccb *ac;
	struct amr_softc *amr;
	struct amr_mailbox *mb;
	int s, rv;

	amr = (struct amr_softc *)sc->sc_ld.sc_dv.dv_parent;

	if ((rv = amr_ccb_alloc(amr, &ac)) != 0)
		return (rv);

	mb = &ac->ac_mbox;
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

int
ld_amr_start(struct ld_softc *ld, struct buf *bp)
{

	return (ld_amr_dobio((struct ld_amr_softc *)ld, bp->b_data,
	    bp->b_bcount, bp->b_rawblkno, (bp->b_flags & B_READ) == 0, bp));
}

void
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

int
ld_amr_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_amr_softc *sc;

	sc = (struct ld_amr_softc *)ld;

	return (ld_amr_dobio(sc, data, blkcnt * ld->sc_secsize, blkno, 1,
	    NULL));
}
