/*	$NetBSD: ccd.c,v 1.6 1995/03/02 06:38:11 cgd Exp $      */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cd.c 1.6 90/11/28$
 *
 *	@(#)cd.c	8.2 (Berkeley) 11/16/93
 */

/*
 * "Concatenated" disk driver.
 */
#include "ccd.h"
#if NCCD > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/dkstat.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/stat.h>
#ifdef COMPAT_NOLABEL
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#endif

#include <dev/ccdvar.h>

#ifdef DEBUG
int ccddebug = 0x00;
#define CCDB_FOLLOW	0x01
#define CCDB_INIT	0x02
#define CCDB_IO		0x04
#endif

#define	ccdunit(x)	DISKUNIT(x)

struct ccdbuf {
	struct buf	cb_buf;		/* new I/O buf */
	struct buf	*cb_obp;	/* ptr. to original I/O buf */
	int		cb_unit;	/* target unit */
	int		cb_comp;	/* target component */
};

#define	getccdbuf()	\
	((struct ccdbuf *)malloc(sizeof(struct ccdbuf), M_DEVBUF, M_WAITOK))
#define putccdbuf(cbp)	\
	free((caddr_t)(cbp), M_DEVBUF)

struct ccd_softc {
	int		 sc_flags;		/* flags */
	size_t		 sc_size;		/* size of ccd */
	int		 sc_ileave;		/* interleave */
	int		 sc_nccdisks;		/* number of components */
	struct ccdcinfo	 sc_cinfo[NCCDISKS];	/* component info */
	struct ccdiinfo	 *sc_itable;		/* interleave table */
	int		 sc_usecnt;		/* number of requests active */
	int		 sc_dk;			/* disk index */
};

struct ccdbuf	*ccdbuffer __P((struct ccd_softc *cs, struct buf *bp,
		    daddr_t bn, caddr_t addr, long bcount));
char		*ccddevtostr __P((dev_t));
void		ccdiodone __P((struct ccdbuf *cbp));

/* sc_flags */
#define	CCDF_ALIVE	0x01
#define CCDF_INITED	0x02

struct ccd_softc *ccd_softc;
int numccd;

/*
 * Since this is called after auto-configuration of devices,
 * we can handle the initialization here.
 *
 * XXX this will not work if you want to use a ccd as your primary
 * swap device since swapconf() has been called before now.
 */
void
ccdattach(num)
	int num;
{
	char *mem;
	register u_long size;
	register struct ccddevice *ccd;
	extern int dkn;

	if (num <= 0)
		return;
	size = num * sizeof(struct ccd_softc);
	mem = malloc(size, M_DEVBUF, M_NOWAIT);
	if (mem == NULL) {
		printf("WARNING: no memory for concatonated disks\n");
		return;
	}
	bzero(mem, size);
	ccd_softc = (struct ccd_softc *)mem;
	numccd = num;
	for (ccd = ccddevice; ccd->ccd_unit >= 0; ccd++) {
		/*
		 * XXX
		 * Assign disk index first so that init routine
		 * can use it (saves having the driver drag around
		 * the ccddevice pointer just to set up the dk_*
		 * info in the open routine).
		 */
		if (dkn < DK_NDRIVE)
			ccd->ccd_dk = dkn++;
		else
			ccd->ccd_dk = -1;
		if (ccdinit(ccd))
			printf("ccd%d configured\n", ccd->ccd_unit);
		else if (ccd->ccd_dk >= 0) {
			ccd->ccd_dk = -1;
			dkn--;
		}
	}
}

ccdinit(ccd)
	struct ccddevice *ccd;
{
	register struct ccd_softc *cs = &ccd_softc[ccd->ccd_unit];
	register struct ccdcinfo *ci;
	register size_t size;
	register int ix;
	size_t minsize;
	dev_t dev;
	struct bdevsw *bsw;
	int error;
	struct proc *p = curproc; /* XXX */

#ifdef DEBUG
	if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
		printf("ccdinit: unit %d\n", ccd->ccd_unit);
#endif
	cs->sc_dk = ccd->ccd_dk;
	cs->sc_size = 0;
	cs->sc_ileave = ccd->ccd_interleave;
	cs->sc_nccdisks = 0;
	/*
	 * Verify that each component piece exists and record
	 * relevant information about it.
	 */
	minsize = 0;
	for (ix = 0; ix < NCCDISKS; ix++) {
		if ((dev = ccd->ccd_dev[ix]) == NODEV)
			break;
		ci = &cs->sc_cinfo[ix];
		ci->ci_dev = dev;
		bsw = &bdevsw[major(dev)];
		/*
		 * Open the partition
		 */
		if (bsw->d_open &&
		    (error = (*bsw->d_open)(dev, 0, S_IFBLK, p))) {
			printf("ccd%d: component %s open failed, error = %d\n",
			       ccd->ccd_unit, ccddevtostr(dev), error);
			return(0);
		}
		/*
		 * Calculate size (truncated to interleave boundary
		 * if necessary.
		 */
		if (bsw->d_psize) {
			size = (size_t) (*bsw->d_psize)(dev);
			if ((int)size < 0)
				size = 0;
		} else
			size = 0;
		if (cs->sc_ileave > 1)
			size -= size % cs->sc_ileave;
		if (size == 0) {
			printf("ccd%d: not configured (component %s missing)\n",
			       ccd->ccd_unit, ccddevtostr(dev));
			return(0);
		}
#ifdef COMPAT_NOLABEL
		/*
		 * XXX if this is a 'c' partition then we need to mark the
		 * label area writeable since there cannot be a label.
		 */
		if ((minor(dev) & 7) == 2 && bsw->d_open) {
			int i, flag;

			for (i = 0; i < nchrdev; i++)
				if (cdevsw[i].d_open == bsw->d_open)
					break;
			if (i != nchrdev && cdevsw[i].d_ioctl) {
				flag = 1;
				(void)(*cdevsw[i].d_ioctl)(dev, DIOCWLABEL,
					(caddr_t)&flag, FWRITE, p);
			}
		}
#endif
		if (minsize == 0 || size < minsize)
			minsize = size;
		ci->ci_size = size;
		cs->sc_size += size;
		cs->sc_nccdisks++;
	}
	/*
	 * If uniform interleave is desired set all sizes to that of
	 * the smallest component.
	 */
	if (ccd->ccd_flags & CCDF_UNIFORM) {
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_nccdisks]; ci++)
			ci->ci_size = minsize;
		cs->sc_size = cs->sc_nccdisks * minsize;
	}
	/*
	 * Construct the interleave table
	 */
	if (!ccdinterleave(cs))
		return(0);
	if (ccd->ccd_dk >= 0)
		dk_wpms[ccd->ccd_dk] = 32 * (60 * DEV_BSIZE / 2);	/* XXX */
	printf("ccd%d: %d components ", ccd->ccd_unit, cs->sc_nccdisks);
	for (ix = 0; ix < cs->sc_nccdisks; ix++)
		printf("%c%s%c",
		       ix == 0 ? '(' : ' ',
		       ccddevtostr(cs->sc_cinfo[ix].ci_dev),
		       ix == cs->sc_nccdisks - 1 ? ')' : ',');
	printf(", %d blocks ", cs->sc_size);
	if (cs->sc_ileave)
		printf("interleaved at %d blocks\n", cs->sc_ileave);
	else
		printf("concatenated\n");
	cs->sc_flags = CCDF_ALIVE | CCDF_INITED;
	return(1);
}

/*
 * XXX not really ccd specific.
 * Could be called something like bdevtostr in machine/conf.c.
 */
char *
ccddevtostr(dev)
	dev_t dev;
{
	static char dbuf[5];

	switch (major(dev)) {
#ifdef hp300
	case 2:
		dbuf[0] = 'r'; dbuf[1] = 'd';
		break;
	case 4:
		dbuf[0] = 's'; dbuf[1] = 'd';
		break;
	case 5:
		dbuf[0] = 'c'; dbuf[1] = 'd';
		break;
	case 6:
		dbuf[0] = 'v'; dbuf[1] = 'n';
		break;
#endif
	default:
		dbuf[0] = dbuf[1] = '?';
		break;
	}
	dbuf[2] = (minor(dev) >> 3) + '0';
	dbuf[3] = (minor(dev) & 7) + 'a';
	dbuf[4] = '\0';
	return (dbuf);
}

ccdinterleave(cs)
	register struct ccd_softc *cs;
{
	register struct ccdcinfo *ci, *smallci;
	register struct ccdiinfo *ii;
	register daddr_t bn, lbn;
	register int ix;
	u_long size;

#ifdef DEBUG
	if (ccddebug & CCDB_INIT)
		printf("ccdinterleave(%x): ileave %d\n", cs, cs->sc_ileave);
#endif
	/*
	 * Allocate an interleave table.
	 * Chances are this is too big, but we don't care.
	 */
	size = (cs->sc_nccdisks + 1) * sizeof(struct ccdiinfo);
	cs->sc_itable = (struct ccdiinfo *)malloc(size, M_DEVBUF, M_WAITOK);
	bzero((caddr_t)cs->sc_itable, size);
	/*
	 * Trivial case: no interleave (actually interleave of disk size).
	 * Each table entry represent a single component in its entirety.
	 */
	if (cs->sc_ileave == 0) {
		bn = 0;
		ii = cs->sc_itable;
		for (ix = 0; ix < cs->sc_nccdisks; ix++) {
			ii->ii_ndisk = 1;
			ii->ii_startblk = bn;
			ii->ii_startoff = 0;
			ii->ii_index[0] = ix;
			bn += cs->sc_cinfo[ix].ci_size;
			ii++;
		}
		ii->ii_ndisk = 0;
#ifdef DEBUG
		if (ccddebug & CCDB_INIT)
			printiinfo(cs->sc_itable);
#endif
		return(1);
	}
	/*
	 * The following isn't fast or pretty; it doesn't have to be.
	 */
	size = 0;
	bn = lbn = 0;
	for (ii = cs->sc_itable; ; ii++) {
		/*
		 * Locate the smallest of the remaining components
		 */
		smallci = NULL;
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_nccdisks]; ci++)
			if (ci->ci_size > size &&
			    (smallci == NULL ||
			     ci->ci_size < smallci->ci_size))
				smallci = ci;
		/*
		 * Nobody left, all done
		 */
		if (smallci == NULL) {
			ii->ii_ndisk = 0;
			break;
		}
		/*
		 * Record starting logical block and component offset
		 */
		ii->ii_startblk = bn / cs->sc_ileave;
		ii->ii_startoff = lbn;
		/*
		 * Determine how many disks take part in this interleave
		 * and record their indices.
		 */
		ix = 0;
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_nccdisks]; ci++)
			if (ci->ci_size >= smallci->ci_size)
				ii->ii_index[ix++] = ci - cs->sc_cinfo;
		ii->ii_ndisk = ix;
		bn += ix * (smallci->ci_size - size);
		lbn = smallci->ci_size / cs->sc_ileave;
		size = smallci->ci_size;
	}
#ifdef DEBUG
	if (ccddebug & CCDB_INIT)
		printiinfo(cs->sc_itable);
#endif
	return(1);
}

#ifdef DEBUG
printiinfo(ii)
	struct ccdiinfo *ii;
{
	register int ix, i;

	for (ix = 0; ii->ii_ndisk; ix++, ii++) {
		printf(" itab[%d]: #dk %d sblk %d soff %d",
		       ix, ii->ii_ndisk, ii->ii_startblk, ii->ii_startoff);
		for (i = 0; i < ii->ii_ndisk; i++)
			printf(" %d", ii->ii_index[i]);
		printf("\n");
	}
}
#endif

ccdopen(dev, flags)
	dev_t dev;
{
	int unit = ccdunit(dev);
	register struct ccd_softc *cs = &ccd_softc[unit];

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdopen(%x, %x)\n", dev, flags);
#endif
	if (unit >= numccd || (cs->sc_flags & CCDF_ALIVE) == 0)
		return(ENXIO);
	return(0);
}

ccdstrategy(bp)
	register struct buf *bp;
{
	register int unit = ccdunit(bp->b_dev);
	register struct ccd_softc *cs = &ccd_softc[unit];
	register daddr_t bn;
	register int sz, s;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdstrategy(%x): unit %d\n", bp, unit);
#endif
	if ((cs->sc_flags & CCDF_INITED) == 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}
	bn = bp->b_blkno;
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bn < 0 || bn + sz > cs->sc_size) {
		sz = cs->sc_size - bn;
		if (sz == 0) {
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			goto done;
		}
		bp->b_bcount = dbtob(sz);
	}
	bp->b_resid = bp->b_bcount;
	/*
	 * "Start" the unit.
	 */
	s = splbio();
	ccdstart(cs, bp);
	splx(s);
	return;
done:
	biodone(bp);
}

ccdstart(cs, bp)
	register struct ccd_softc *cs;
	register struct buf *bp;
{
	register long bcount, rcount;
	struct ccdbuf *cbp;
	caddr_t addr;
	daddr_t bn;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdstart(%x, %x)\n", cs, bp);
#endif
	/*
	 * Instumentation (not real meaningful)
	 */
	cs->sc_usecnt++;
	if (cs->sc_dk >= 0) {
		dk_busy |= 1 << cs->sc_dk;
		dk_xfer[cs->sc_dk]++;
		dk_wds[cs->sc_dk] += bp->b_bcount >> 6;
	}
	/*
	 * Allocate component buffers and fire off the requests
	 */
	bn = bp->b_blkno;
	addr = bp->b_data;
	for (bcount = bp->b_bcount; bcount > 0; bcount -= rcount) {
		cbp = ccdbuffer(cs, bp, bn, addr, bcount);
		rcount = cbp->cb_buf.b_bcount;
		(*bdevsw[major(cbp->cb_buf.b_dev)].d_strategy)(&cbp->cb_buf);
		bn += btodb(rcount);
		addr += rcount;
	}
}

/*
 * Build a component buffer header.
 */
struct ccdbuf *
ccdbuffer(cs, bp, bn, addr, bcount)
	register struct ccd_softc *cs;
	struct buf *bp;
	daddr_t bn;
	caddr_t addr;
	long bcount;
{
	register struct ccdcinfo *ci;
	register struct ccdbuf *cbp;
	register daddr_t cbn, cboff;

#ifdef DEBUG
	if (ccddebug & CCDB_IO)
		printf("ccdbuffer(%x, %x, %d, %x, %d)\n",
		       cs, bp, bn, addr, bcount);
#endif
	/*
	 * Determine which component bn falls in.
	 */
	cbn = bn;
	cboff = 0;
	/*
	 * Serially concatenated
	 */
	if (cs->sc_ileave == 0) {
		register daddr_t sblk;

		sblk = 0;
		for (ci = cs->sc_cinfo; cbn >= sblk + ci->ci_size; ci++)
			sblk += ci->ci_size;
		cbn -= sblk;
	}
	/*
	 * Interleaved
	 */
	else {
		register struct ccdiinfo *ii;
		int ccdisk, off;

		cboff = cbn % cs->sc_ileave;
		cbn /= cs->sc_ileave;
		for (ii = cs->sc_itable; ii->ii_ndisk; ii++)
			if (ii->ii_startblk > cbn)
				break;
		ii--;
		off = cbn - ii->ii_startblk;
		if (ii->ii_ndisk == 1) {
			ccdisk = ii->ii_index[0];
			cbn = ii->ii_startoff + off;
		} else {
			ccdisk = ii->ii_index[off % ii->ii_ndisk];
			cbn = ii->ii_startoff + off / ii->ii_ndisk;
		}
		cbn *= cs->sc_ileave;
		ci = &cs->sc_cinfo[ccdisk];
	}
	/*
	 * Fill in the component buf structure.
	 */
	cbp = getccdbuf();
	cbp->cb_buf.b_flags = bp->b_flags | B_CALL;
	cbp->cb_buf.b_iodone = (void (*)())ccdiodone;
	cbp->cb_buf.b_proc = bp->b_proc;
	cbp->cb_buf.b_dev = ci->ci_dev;
	cbp->cb_buf.b_blkno = cbn + cboff;
	cbp->cb_buf.b_data = addr;
	cbp->cb_buf.b_vp = 0;
	if (cs->sc_ileave == 0)
		cbp->cb_buf.b_bcount = dbtob(ci->ci_size - cbn);
	else
		cbp->cb_buf.b_bcount = dbtob(cs->sc_ileave - cboff);
	if (cbp->cb_buf.b_bcount > bcount)
		cbp->cb_buf.b_bcount = bcount;

	/*
	 * context for ccdiodone
	 */
	cbp->cb_obp = bp;
	cbp->cb_unit = cs - ccd_softc;
	cbp->cb_comp = ci - cs->sc_cinfo;

#ifdef DEBUG
	if (ccddebug & CCDB_IO)
		printf(" dev %x(u%d): cbp %x bn %d addr %x bcnt %d\n",
		       ci->ci_dev, ci-cs->sc_cinfo, cbp, cbp->cb_buf.b_blkno,
		       cbp->cb_buf.b_data, cbp->cb_buf.b_bcount);
#endif
	return (cbp);
}

ccdintr(cs, bp)
	register struct ccd_softc *cs;
	register struct buf *bp;
{

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdintr(%x, %x)\n", cs, bp);
#endif
	/*
	 * Request is done for better or worse, wakeup the top half.
	 */
	if (--cs->sc_usecnt == 0 && cs->sc_dk >= 0)
		dk_busy &= ~(1 << cs->sc_dk);
	if (bp->b_flags & B_ERROR)
		bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * Called by biodone at interrupt time.
 * Mark the component as done and if all components are done,
 * take a ccd interrupt.
 */
void
ccdiodone(cbp)
	register struct ccdbuf *cbp;
{
	register struct buf *bp = cbp->cb_obp;
	register int unit = cbp->cb_unit;
	int count, s;

	s = splbio();
#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdiodone(%x)\n", cbp);
	if (ccddebug & CCDB_IO) {
		printf("ccdiodone: bp %x bcount %d resid %d\n",
		       bp, bp->b_bcount, bp->b_resid);
		printf(" dev %x(u%d), cbp %x bn %d addr %x bcnt %d\n",
		       cbp->cb_buf.b_dev, cbp->cb_comp, cbp,
		       cbp->cb_buf.b_blkno, cbp->cb_buf.b_data,
		       cbp->cb_buf.b_bcount);
	}
#endif

	if (cbp->cb_buf.b_flags & B_ERROR) {
		bp->b_flags |= B_ERROR;
		bp->b_error = cbp->cb_buf.b_error ? cbp->cb_buf.b_error : EIO;
#ifdef DEBUG
		printf("ccd%d: error %d on component %d\n",
		       unit, bp->b_error, cbp->cb_comp);
#endif
	}
	count = cbp->cb_buf.b_bcount;
	putccdbuf(cbp);

	/*
	 * If all done, "interrupt".
	 */
	bp->b_resid -= count;
	if (bp->b_resid < 0)
		panic("ccdiodone: count");
	if (bp->b_resid == 0)
		ccdintr(&ccd_softc[unit], bp);
	splx(s);
}

ccdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = ccdunit(dev);

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdread(%x, %x)\n", dev, uio);
#endif
	return(physio(ccdstrategy, NULL, dev, B_READ, minphys, uio));
}

ccdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = ccdunit(dev);

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdwrite(%x, %x)\n", dev, uio);
#endif
	return(physio(ccdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

ccdioctl(dev, cmd, data, flag)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
{
	return(EINVAL);
}

ccdsize(dev)
	dev_t dev;
{
	int unit = ccdunit(dev);
	register struct ccd_softc *cs = &ccd_softc[unit];

	if (unit >= numccd || (cs->sc_flags & CCDF_INITED) == 0)
		return(-1);
	return(cs->sc_size);
}

ccddump(dev)
{
	return(ENXIO);
}
#endif
