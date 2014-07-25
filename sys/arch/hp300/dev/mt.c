/*	$NetBSD: mt.c,v 1.54 2014/07/25 08:10:33 dholland Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 1992, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: mt.c 1.8 95/09/12$
 */
/*	@(#)mt.c	3.9	90/07/10	mt Xinu
 *
 * Magnetic tape driver (7974a, 7978a/b, 7979a, 7980a, 7980xc)
 * Original version contributed by Mt. Xinu.
 * Modified for 4.4BSD by Mark Davies and Andrew Vignaux, Department of
 * Computer Science, Victoria University of Wellington
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mt.c,v 1.54 2014/07/25 08:10:33 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/tty.h>
#include <sys/kernel.h>
#include <sys/tprintf.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <hp300/dev/hpibvar.h>

#include <hp300/dev/mtreg.h>

#include "ioconf.h"

static const struct mtinfo {
	u_short	hwid;
	const char *desc;
} mtinfo[] = {
	{ MT7978ID,	"7978"	},
	{ MT7979AID,	"7979A"	},
	{ MT7980ID,	"7980"	},
	{ MT7974AID,	"7974A"	},
};
static const int nmtinfo = sizeof(mtinfo) / sizeof(mtinfo[0]);

struct	mt_softc {
	device_t sc_dev;
	struct	callout sc_start_ch;
	struct	callout sc_intr_ch;
	int	sc_hpibno;	/* logical HPIB this slave it attached to */
	int	sc_slave;	/* HPIB slave address (0-6) */
	short	sc_flags;	/* see below */
	u_char	sc_lastdsj;	/* place for DSJ in mtreaddsj() */
	u_char	sc_lastecmd;	/* place for End Command in mtreaddsj() */
	short	sc_recvtimeo;	/* count of hpibsend timeouts to prevent hang */
	short	sc_statindex;	/* index for next sc_stat when MTF_STATTIMEO */
	struct	mt_stat sc_stat;/* status bytes last read from device */
	short	sc_density;	/* current density of tape (mtio.h format) */
	short	sc_type;	/* tape drive model (hardware IDs) */
	struct	hpibqueue sc_hq; /* HPIB device queue member */
	tpr_t	sc_ttyp;
	struct bufq_state *sc_tab;/* buf queue */
	int	sc_active;
};

#ifdef DEBUG
int	mtdebug = 0;
#define	dlog	if (mtdebug) log
#else
#define	dlog	if (0) log
#endif

#define	UNIT(x)		(minor(x) & 3)

#define B_CMD		B_DEVPRIVATE	/* command buf instead of data */
#define	b_cmd		b_blkno		/* blkno holds cmd when B_CMD */

static int	mtmatch(device_t, cfdata_t, void *);
static void	mtattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mt, sizeof(struct mt_softc),
    mtmatch, mtattach, NULL, NULL);

static dev_type_open(mtopen);
static dev_type_close(mtclose);
static dev_type_read(mtread);
static dev_type_write(mtwrite);
static dev_type_ioctl(mtioctl);
static dev_type_strategy(mtstrategy);

const struct bdevsw mt_bdevsw = {
	.d_open = mtopen,
	.d_close = mtclose,
	.d_strategy = mtstrategy,
	.d_ioctl = mtioctl,
	.d_dump = nodump,
	.d_psize = nosize,
	.d_discard = nodiscard,
	.d_flag = D_TAPE
};

const struct cdevsw mt_cdevsw = {
	.d_open = mtopen,
	.d_close = mtclose,
	.d_read = mtread,
	.d_write = mtwrite,
	.d_ioctl = mtioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TAPE
};

static int	mtident(struct mt_softc *, struct hpibbus_attach_args *);
static void	mtustart(struct mt_softc *);
static int	mtreaddsj(struct mt_softc *, int);
static int	mtcommand(dev_t, int, int);
static void	spl_mtintr(void *);
static void	spl_mtstart(void *);

static void	mtstart(void *);
static void	mtgo(void *);
static void	mtintr(void *);

static int
mtmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct hpibbus_attach_args *ha = aux;

	return mtident(NULL, ha);
}

static void
mtattach(device_t parent, device_t self, void *aux)
{
	struct mt_softc *sc = device_private(self);
	struct hpibbus_attach_args *ha = aux;
	int hpibno, slave;

	sc->sc_dev = self;
	if (mtident(sc, ha) == 0) {
		aprint_error(": impossible!\n");
		return;
	}

	hpibno = device_unit(parent);
	slave = ha->ha_slave;

	bufq_alloc(&sc->sc_tab, "fcfs", 0);
	callout_init(&sc->sc_start_ch, 0);
	callout_init(&sc->sc_intr_ch, 0);

	sc->sc_hpibno = hpibno;
	sc->sc_slave = slave;
	sc->sc_flags = MTF_EXISTS;

	/* Initialize hpib job queue entry. */
	sc->sc_hq.hq_softc = sc;
	sc->sc_hq.hq_slave = sc->sc_slave;
	sc->sc_hq.hq_start = mtstart;
	sc->sc_hq.hq_go = mtgo;
	sc->sc_hq.hq_intr = mtintr;
}

static int
mtident(struct mt_softc *sc, struct hpibbus_attach_args *ha)
{
	int i;

	for (i = 0; i < nmtinfo; i++) {
		if (ha->ha_id == mtinfo[i].hwid) {
			if (sc != NULL) {
				sc->sc_type = mtinfo[i].hwid;
				aprint_normal(": %s tape\n", mtinfo[i].desc);
			}
			return 1;
		}
	}
	return 0;
}

/*
 * Perform a read of "Device Status Jump" register and update the
 * status if necessary.  If status is read, the given "ecmd" is also
 * performed, unless "ecmd" is zero.  Returns DSJ value, -1 on failure
 * and -2 on "temporary" failure.
 */
static int
mtreaddsj(struct mt_softc *sc, int ecmd)
{
	int retval;

	if (sc->sc_flags & MTF_STATTIMEO)
		goto getstats;
	retval = hpibrecv(sc->sc_hpibno,
	    (sc->sc_flags & MTF_DSJTIMEO) ? -1 : sc->sc_slave,
	    MTT_DSJ, &(sc->sc_lastdsj), 1);
	sc->sc_flags &= ~MTF_DSJTIMEO;
	if (retval != 1) {
		dlog(LOG_DEBUG, "%s can't hpibrecv DSJ",
		    device_xname(sc->sc_dev));
		if (sc->sc_recvtimeo == 0)
			sc->sc_recvtimeo = hz;
		if (--sc->sc_recvtimeo == 0)
			return -1;
		if (retval == 0)
			sc->sc_flags |= MTF_DSJTIMEO;
		return -2;
	}
	sc->sc_recvtimeo = 0;
	sc->sc_statindex = 0;
	dlog(LOG_DEBUG, "%s readdsj: 0x%x", device_xname(sc->sc_dev),
	    sc->sc_lastdsj);
	sc->sc_lastecmd = ecmd;
	switch (sc->sc_lastdsj) {
	    case 0:
		if (ecmd & MTE_DSJ_FORCE)
			break;
		return 0;

	    case 2:
		sc->sc_lastecmd = MTE_COMPLETE;
	    case 1:
		break;

	    default:
		log(LOG_ERR, "%s readdsj: DSJ 0x%x\n", device_xname(sc->sc_dev),
		    sc->sc_lastdsj);
		return -1;
	}
 getstats:
	retval = hpibrecv(sc->sc_hpibno,
	    (sc->sc_flags & MTF_STATCONT) ? -1 : sc->sc_slave,
	    MTT_STAT, ((char *)&(sc->sc_stat)) + sc->sc_statindex,
	    sizeof(sc->sc_stat) - sc->sc_statindex);
	sc->sc_flags &= ~(MTF_STATTIMEO | MTF_STATCONT);
	if (retval != sizeof(sc->sc_stat) - sc->sc_statindex) {
		if (sc->sc_recvtimeo == 0)
			sc->sc_recvtimeo = hz;
		if (--sc->sc_recvtimeo != 0) {
			if (retval >= 0) {
				sc->sc_statindex += retval;
				sc->sc_flags |= MTF_STATCONT;
			}
			sc->sc_flags |= MTF_STATTIMEO;
			return -2;
		}
		log(LOG_ERR, "%s readdsj: can't read status",
		    device_xname(sc->sc_dev));
		return -1;
	}
	sc->sc_recvtimeo = 0;
	sc->sc_statindex = 0;
	dlog(LOG_DEBUG, "%s readdsj: status is %x %x %x %x %x %x",
	    device_xname(sc->sc_dev),
	    sc->sc_stat1, sc->sc_stat2, sc->sc_stat3,
	    sc->sc_stat4, sc->sc_stat5, sc->sc_stat6);
	if (sc->sc_lastecmd)
		(void) hpibsend(sc->sc_hpibno, sc->sc_slave,
		    MTL_ECMD, &(sc->sc_lastecmd), 1);
	return (int)sc->sc_lastdsj;
}

static int
mtopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct mt_softc *sc;
	int req_den;
	int error;

	sc = device_lookup_private(&mt_cd, UNIT(dev));
	if (sc == NULL)
		return ENXIO;

	if ((sc->sc_flags & MTF_EXISTS) == 0)
		return ENXIO;

	dlog(LOG_DEBUG, "%s open: flags 0x%x", device_xname(sc->sc_dev),
	    sc->sc_flags);
	if (sc->sc_flags & MTF_OPEN)
		return EBUSY;
	sc->sc_flags |= MTF_OPEN;
	sc->sc_ttyp = tprintf_open(l->l_proc);
	if ((sc->sc_flags & MTF_ALIVE) == 0) {
		error = mtcommand(dev, MTRESET, 0);
		if (error != 0 || (sc->sc_flags & MTF_ALIVE) == 0)
			goto errout;
		if ((sc->sc_stat1 & (SR1_BOT | SR1_ONLINE)) == SR1_ONLINE)
			(void) mtcommand(dev, MTREW, 0);
	}
	for (;;) {
		if ((error = mtcommand(dev, MTNOP, 0)) != 0)
			goto errout;
		if (!(sc->sc_flags & MTF_REW))
			break;
		error = kpause("mt", true, hz, NULL);
		if (error != 0 && error != EWOULDBLOCK) {
			error = EINTR;
			goto errout;
		}
	}
	if ((flag & FWRITE) && (sc->sc_stat1 & SR1_RO)) {
		error = EROFS;
		goto errout;
	}
	if (!(sc->sc_stat1 & SR1_ONLINE)) {
		uprintf("%s: not online\n", device_xname(sc->sc_dev));
		error = EIO;
		goto errout;
	}
	/*
	 * Select density:
	 *  - find out what density the drive is set to
	 *	(i.e. the density of the current tape)
	 *  - if we are going to write
	 *    - if we're not at the beginning of the tape
	 *      - complain if we want to change densities
	 *    - otherwise, select the mtcommand to set the density
	 *
	 * If the drive doesn't support it then don't change the recorded
	 * density.
	 *
	 * The original MOREbsd code had these additional conditions
	 * for the mid-tape change
	 *
	 *	req_den != T_BADBPI &&
	 *	sc->sc_density != T_6250BPI
	 *
	 * which suggests that it would be possible to write multiple
	 * densities if req_den == T_BAD_BPI or the current tape
	 * density was 6250.  Testing of our 7980 suggests that the
	 * device cannot change densities mid-tape.
	 *
	 * ajv@comp.vuw.ac.nz
	 */
	sc->sc_density = (sc->sc_stat2 & SR2_6250) ? T_6250BPI : (
			 (sc->sc_stat3 & SR3_1600) ? T_1600BPI : (
			 (sc->sc_stat3 & SR3_800) ? T_800BPI : -1));
	req_den = (dev & T_DENSEL);

	if (flag & FWRITE) {
		if (!(sc->sc_stat1 & SR1_BOT)) {
			if (sc->sc_density != req_den) {
				uprintf("%s: can't change density mid-tape\n",
				    device_xname(sc->sc_dev));
				error = EIO;
				goto errout;
			}
		}
		else {
			int mtset_density =
			    (req_den == T_800BPI  ? MTSET800BPI : (
			     req_den == T_1600BPI ? MTSET1600BPI : (
			     req_den == T_6250BPI ? MTSET6250BPI : (
			     sc->sc_type == MT7980ID
						  ? MTSET6250DC
						  : MTSET6250BPI))));
			if (mtcommand(dev, mtset_density, 0) == 0)
				sc->sc_density = req_den;
		}
	}
	return 0;
errout:
	sc->sc_flags &= ~MTF_OPEN;
	return error;
}

static int
mtclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct mt_softc *sc = device_lookup_private(&mt_cd,UNIT(dev));

	if (sc->sc_flags & MTF_WRT) {
		(void) mtcommand(dev, MTWEOF, 2);
		(void) mtcommand(dev, MTBSF, 0);
	}
	if ((minor(dev) & T_NOREWIND) == 0)
		(void) mtcommand(dev, MTREW, 0);
	sc->sc_flags &= ~MTF_OPEN;
	tprintf_close(sc->sc_ttyp);
	return 0;
}

static int
mtcommand(dev_t dev, int cmd, int cnt)
{
	int error = 0;
	buf_t *bp;

	bp = getiobuf(NULL, true);
	bp->b_cmd = cmd;
	bp->b_dev = dev;
	do {
		bp->b_cflags = BC_BUSY;
		bp->b_flags = B_CMD;
		bp->b_oflags = 0;
		mtstrategy(bp);
		biowait(bp);
		if (bp->b_error != 0) {
			error = bp->b_error;
			break;
		}
	} while (--cnt > 0);
	putiobuf(bp);

	return error;
}

/*
 * Only thing to check here is for legal record lengths (writes only).
 */
static void
mtstrategy(struct buf *bp)
{
	struct mt_softc *sc;
	int s;

	sc = device_lookup_private(&mt_cd,UNIT(bp->b_dev));
	dlog(LOG_DEBUG, "%s strategy", device_xname(sc->sc_dev));
	if ((bp->b_flags & (B_CMD | B_READ)) == 0) {
#define WRITE_BITS_IGNORED	8
#if 0
		if (bp->b_bcount & ((1 << WRITE_BITS_IGNORED) - 1)) {
			tprintf(sc->sc_ttyp,
			    "%s: write record must be multiple of %d\n",
			    device_xname(sc->sc_dev), 1 << WRITE_BITS_IGNORED);
			goto error;
		}
#endif
		s = 16 * 1024;
		if (sc->sc_stat2 & SR2_LONGREC) {
			switch (sc->sc_density) {
			    case T_1600BPI:
				s = 32 * 1024;
				break;

			    case T_6250BPI:
			    case T_BADBPI:
				s = 60 * 1024;
				break;
			}
		}
		if (bp->b_bcount > s) {
			tprintf(sc->sc_ttyp,
			    "%s: write record (%d) too big: limit (%d)\n",
			    device_xname(sc->sc_dev), bp->b_bcount, s);
#if 0 /* XXX see above */
	    error:
#endif
			bp->b_error = EIO;
			biodone(bp);
			return;
		}
	}
	s = splbio();
	bufq_put(sc->sc_tab, bp);
	if (sc->sc_active == 0) {
		sc->sc_active = 1;
		mtustart(sc);
	}
	splx(s);
}

static void
mtustart(struct mt_softc *sc)
{

	dlog(LOG_DEBUG, "%s ustart", device_xname(sc->sc_dev));
	if (hpibreq(device_parent(sc->sc_dev), &sc->sc_hq))
		mtstart(sc);
}

static void
spl_mtintr(void *arg)
{
	struct mt_softc *sc = arg;
	int s = splbio();

	hpibppclear(sc->sc_hpibno);
	mtintr(sc);
	splx(s);
}

static void
spl_mtstart(void *arg)
{
	int s = splbio();

	mtstart(arg);
	splx(s);
}

static void
mtstart(void *arg)
{
	struct mt_softc *sc = arg;
	struct buf *bp;
	short	cmdcount = 1;
	u_char	cmdbuf[2];

	dlog(LOG_DEBUG, "%s start", device_xname(sc->sc_dev));
	sc->sc_flags &= ~MTF_WRT;
	bp = bufq_peek(sc->sc_tab);
	if ((sc->sc_flags & MTF_ALIVE) == 0 &&
	    ((bp->b_flags & B_CMD) == 0 || bp->b_cmd != MTRESET))
		goto fatalerror;

	if (sc->sc_flags & MTF_REW) {
		if (!hpibpptest(sc->sc_hpibno, sc->sc_slave))
			goto stillrew;
		switch (mtreaddsj(sc, MTE_DSJ_FORCE|MTE_COMPLETE|MTE_IDLE)) {
		    case 0:
		    case 1:
		stillrew:
			if ((sc->sc_stat1 & SR1_BOT) ||
			    !(sc->sc_stat1 & SR1_ONLINE)) {
				sc->sc_flags &= ~MTF_REW;
				break;
			}
		    case -2:
			/*
			 * -2 means "timeout" reading DSJ, which is probably
			 * temporary.  This is considered OK when doing a NOP,
			 * but not otherwise.
			 */
			if (sc->sc_flags & (MTF_DSJTIMEO | MTF_STATTIMEO)) {
				callout_reset(&sc->sc_start_ch, hz >> 5,
				    spl_mtstart, sc);
				return;
			}
		    case 2:
			if (bp->b_cmd != MTNOP || !(bp->b_flags & B_CMD)) {
				bp->b_error = EBUSY;
				goto done;
			}
			goto done;

		    default:
			goto fatalerror;
		}
	}
	if (bp->b_flags & B_CMD) {
		if (sc->sc_flags & MTF_PASTEOT) {
			switch(bp->b_cmd) {
			    case MTFSF:
			    case MTWEOF:
			    case MTFSR:
				bp->b_error = ENOSPC;
				goto done;

			    case MTBSF:
			    case MTOFFL:
			    case MTBSR:
			    case MTREW:
				sc->sc_flags &= ~(MTF_PASTEOT | MTF_ATEOT);
				break;
			}
		}
		switch(bp->b_cmd) {
		    case MTFSF:
			if (sc->sc_flags & MTF_HITEOF)
				goto done;
			cmdbuf[0] = MTTC_FSF;
			break;

		    case MTBSF:
			if (sc->sc_flags & MTF_HITBOF)
				goto done;
			cmdbuf[0] = MTTC_BSF;
			break;

		    case MTOFFL:
			sc->sc_flags |= MTF_REW;
			cmdbuf[0] = MTTC_REWOFF;
			break;

		    case MTWEOF:
			cmdbuf[0] = MTTC_WFM;
			break;

		    case MTBSR:
			cmdbuf[0] = MTTC_BSR;
			break;

		    case MTFSR:
			cmdbuf[0] = MTTC_FSR;
			break;

		    case MTREW:
			sc->sc_flags |= MTF_REW;
			cmdbuf[0] = MTTC_REW;
			break;

		    case MTNOP:
			/*
			 * NOP is supposed to set status bits.
			 * Force readdsj to do it.
			 */
			switch (mtreaddsj(sc,
			  MTE_DSJ_FORCE | MTE_COMPLETE | MTE_IDLE)) {
			    default:
				goto done;

			    case -1:
				/*
				 * If this fails, perform a device clear
				 * to fix any protocol problems and (most
				 * likely) get the status.
				 */
				bp->b_cmd = MTRESET;
				break;

			    case -2:
				callout_reset(&sc->sc_start_ch, hz >> 5,
				    spl_mtstart, sc);
				return;
			}

		    case MTRESET:
			/*
			 * 1) selected device clear (send with "-2" secondary)
			 * 2) set timeout, then wait for "service request"
			 * 3) interrupt will read DSJ (and END COMPLETE-IDLE)
			 */
			if (hpibsend(sc->sc_hpibno, sc->sc_slave, -2, NULL, 0)){
				log(LOG_ERR, "%s can't reset",
				    device_xname(sc->sc_dev));
				goto fatalerror;
			}
			callout_reset(&sc->sc_intr_ch, 4 * hz, spl_mtintr, sc);
			hpibawait(sc->sc_hpibno);
			return;

		    case MTSET800BPI:
			cmdbuf[0] = MTTC_800;
			break;

		    case MTSET1600BPI:
			cmdbuf[0] = MTTC_1600;
			break;

		    case MTSET6250BPI:
			cmdbuf[0] = MTTC_6250;
			break;

		    case MTSET6250DC:
			cmdbuf[0] = MTTC_DC6250;
			break;
		}
	} else {
		if (sc->sc_flags & MTF_PASTEOT) {
			bp->b_error = ENOSPC;
			goto done;
		}
		if (bp->b_flags & B_READ) {
			sc->sc_flags |= MTF_IO;
			cmdbuf[0] = MTTC_READ;
		} else {
			sc->sc_flags |= MTF_WRT | MTF_IO;
			cmdbuf[0] = MTTC_WRITE;
			cmdbuf[1] = (bp->b_bcount + ((1 << WRITE_BITS_IGNORED) - 1)) >> WRITE_BITS_IGNORED;
			cmdcount = 2;
		}
	}
	if (hpibsend(sc->sc_hpibno, sc->sc_slave, MTL_TCMD, cmdbuf, cmdcount)
	    == cmdcount) {
		if (sc->sc_flags & MTF_REW)
			goto done;
		hpibawait(sc->sc_hpibno);
		return;
	}
fatalerror:
	/*
	 * If anything fails, the drive is probably hosed, so mark it not
	 * "ALIVE" (but it EXISTS and is OPEN or we wouldn't be here, and
	 * if, last we heard, it was REWinding, remember that).
	 */
	sc->sc_flags &= MTF_EXISTS | MTF_OPEN | MTF_REW;
	bp->b_error = EIO;
done:
	sc->sc_flags &= ~(MTF_HITEOF | MTF_HITBOF);
	(void)bufq_get(sc->sc_tab);
	biodone(bp);
	hpibfree(device_parent(sc->sc_dev), &sc->sc_hq);
	if ((bp = bufq_peek(sc->sc_tab)) == NULL)
		sc->sc_active = 0;
	else
		mtustart(sc);
}

/*
 * The Utah code had a bug which meant that the driver was unable to read.
 * "rw" was initialized to bp->b_flags & B_READ before "bp" was initialized.
 *   -- ajv@comp.vuw.ac.nz
 */
static void
mtgo(void *arg)
{
	struct mt_softc *sc = arg;
	struct buf *bp;
	int rw;

	dlog(LOG_DEBUG, "%s go", device_xname(sc->sc_dev));
	bp = bufq_peek(sc->sc_tab);
	rw = bp->b_flags & B_READ;
	hpibgo(sc->sc_hpibno, sc->sc_slave, rw ? MTT_READ : MTL_WRITE,
	    bp->b_data, bp->b_bcount, rw, rw != 0);
}

static void
mtintr(void *arg)
{
	struct mt_softc *sc = arg;
	struct buf *bp;
	int i;
	u_char cmdbuf[4];

	bp = bufq_peek(sc->sc_tab);
	if (bp == NULL) {
		log(LOG_ERR, "%s intr: bp == NULL", device_xname(sc->sc_dev));
		return;
	}

	dlog(LOG_DEBUG, "%s intr", device_xname(sc->sc_dev));

	/*
	 * Some operation completed.  Read status bytes and report errors.
	 * Clear EOF flags here `cause they're set once on specific conditions
	 * below when a command succeeds.
	 * A DSJ of 2 always means keep waiting.  If the command was READ
	 * (and we're in data DMA phase) stop data transfer first.
	 */
	sc->sc_flags &= ~(MTF_HITEOF | MTF_HITBOF);
	if ((bp->b_flags & (B_CMD|B_READ)) == B_READ &&
	    !(sc->sc_flags & (MTF_IO | MTF_STATTIMEO | MTF_DSJTIMEO))){
		cmdbuf[0] = MTE_STOP;
		(void) hpibsend(sc->sc_hpibno, sc->sc_slave, MTL_ECMD,cmdbuf,1);
	}
	switch (mtreaddsj(sc, 0)) {
	    case 0:
		break;

	    case 1:
		/*
		 * If we're in the middle of a READ/WRITE and have yet to
		 * start the data transfer, a DSJ of one should terminate it.
		 */
		sc->sc_flags &= ~MTF_IO;
		break;

	    case 2:
		(void) hpibawait(sc->sc_hpibno);
		return;

	    case -2:
		/*
		 * -2 means that the drive failed to respond quickly enough
		 * to the request for DSJ.  It's probably just "busy" figuring
		 * it out and will know in a little bit...
		 */
		callout_reset(&sc->sc_intr_ch, hz >> 5, spl_mtintr, sc);
		return;

	    default:
		log(LOG_ERR, "%s intr: can't get drive stat",
		    device_xname(sc->sc_dev));
		goto error;
	}
	if (sc->sc_stat1 & (SR1_ERR | SR1_REJECT)) {
		i = sc->sc_stat4 & SR4_ERCLMASK;
		log(LOG_ERR, "%s: %s error, retry %d, SR2/3 %x/%x, code %d",
			device_xname(sc->sc_dev), i == SR4_DEVICE ? "device" :
			(i == SR4_PROTOCOL ? "protocol" :
			(i == SR4_SELFTEST ? "selftest" : "unknown")),
			sc->sc_stat4 & SR4_RETRYMASK, sc->sc_stat2,
			sc->sc_stat3, sc->sc_stat5);

		if ((bp->b_flags & B_CMD) && bp->b_cmd == MTRESET)
			callout_stop(&sc->sc_intr_ch);
		if (sc->sc_stat3 & SR3_POWERUP)
			sc->sc_flags &= MTF_OPEN | MTF_EXISTS;
		goto error;
	}
	/*
	 * Report and clear any soft errors.
	 */
	if (sc->sc_stat1 & SR1_SOFTERR) {
		log(LOG_WARNING, "%s: soft error, retry %d\n",
		    device_xname(sc->sc_dev), sc->sc_stat4 & SR4_RETRYMASK);
		sc->sc_stat1 &= ~SR1_SOFTERR;
	}
	/*
	 * We've initiated a read or write, but haven't actually started to
	 * DMA the data yet.  At this point, the drive's ready.
	 */
	if (sc->sc_flags & MTF_IO) {
		sc->sc_flags &= ~MTF_IO;
		if (hpibustart(sc->sc_hpibno))
			mtgo(sc);
		return;
	}
	/*
	 * Check for End Of Tape - we're allowed to hit EOT and then write (or
	 * read) one more record.  If we get here and have not already hit EOT,
	 * return ENOSPC to inform the process that it's hit it.  If we get
	 * here and HAVE already hit EOT, don't allow any more operations that
	 * move the tape forward.
	 */
	if (sc->sc_stat1 & SR1_EOT) {
		if (sc->sc_flags & MTF_ATEOT)
			sc->sc_flags |= MTF_PASTEOT;
		else {
			bp->b_error = ENOSPC;
			sc->sc_flags |= MTF_ATEOT;
		}
	}
	/*
	 * If a motion command was being executed, check for Tape Marks.
	 * If we were doing data, make sure we got the right amount, and
	 * check for hitting tape marks on reads.
	 */
	if (bp->b_flags & B_CMD) {
		if (sc->sc_stat1 & SR1_EOF) {
			if (bp->b_cmd == MTFSR)
				sc->sc_flags |= MTF_HITEOF;
			if (bp->b_cmd == MTBSR)
				sc->sc_flags |= MTF_HITBOF;
		}
		if (bp->b_cmd == MTRESET) {
			callout_stop(&sc->sc_intr_ch);
			sc->sc_flags |= MTF_ALIVE;
		}
	} else {
		i = hpibrecv(sc->sc_hpibno, sc->sc_slave, MTT_BCNT, cmdbuf, 2);
		if (i != 2) {
			log(LOG_ERR, "%s intr: can't get xfer length\n",
			    device_xname(sc->sc_dev));
			goto error;
		}
		i = (int) *((u_short *) cmdbuf);
		if (i <= bp->b_bcount) {
			if (i == 0)
				sc->sc_flags |= MTF_HITEOF;
			bp->b_resid = bp->b_bcount - i;
			dlog(LOG_DEBUG, "%s intr: bcount %d, resid %d",
			    device_xname(sc->sc_dev), bp->b_bcount,
			    bp->b_resid);
		} else {
			tprintf(sc->sc_ttyp,
				"%s: record (%d) larger than wanted (%d)\n",
				device_xname(sc->sc_dev), i, bp->b_bcount);
 error:
			sc->sc_flags &= ~MTF_IO;
			bp->b_error = EIO;
		}
	}
	/*
	 * The operation is completely done.
	 * Let the drive know with an END command.
	 */
	cmdbuf[0] = MTE_COMPLETE | MTE_IDLE;
	(void) hpibsend(sc->sc_hpibno, sc->sc_slave, MTL_ECMD, cmdbuf, 1);
	bp->b_flags &= ~B_CMD;
	(void)bufq_get(sc->sc_tab);
	biodone(bp);
	hpibfree(device_parent(sc->sc_dev), &sc->sc_hq);
	if (bufq_peek(sc->sc_tab) == NULL)
		sc->sc_active = 0;
	else
		mtustart(sc);
}

static int
mtread(dev_t dev, struct uio *uio, int flags)
{

	return physio(mtstrategy, NULL, dev, B_READ, minphys, uio);
}

static int
mtwrite(dev_t dev, struct uio *uio, int flags)
{

	return physio(mtstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static int
mtioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct mtop *op;
	int cnt;

	switch (cmd) {
	    case MTIOCTOP:
		op = (struct mtop *)data;
		switch(op->mt_op) {
		    case MTWEOF:
		    case MTFSF:
		    case MTBSR:
		    case MTBSF:
		    case MTFSR:
			cnt = op->mt_count;
			break;

		    case MTOFFL:
		    case MTREW:
		    case MTNOP:
			cnt = 0;
			break;

		    default:
			return EINVAL;
		}
		return mtcommand(dev, op->mt_op, cnt);

	    case MTIOCGET:
		break;

	    default:
		return EINVAL;
	}
	return 0;
}
