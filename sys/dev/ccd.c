/*	$NetBSD: ccd.c,v 1.71.2.6 2002/06/20 03:43:23 nathanw Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 1999 The NetBSD Foundation, Inc.
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
 *
 * Dynamic configuration and disklabel support by:
 *	Jason R. Thorpe <thorpej@nas.nasa.gov>
 *	Numerical Aerodynamic Simulation Facility
 *	Mail Stop 258-6
 *	NASA Ames Research Center
 *	Moffett Field, CA 94035
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ccd.c,v 1.71.2.6 2002/06/20 03:43:23 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/queue.h>

#include <dev/ccdvar.h>

#if defined(CCDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
#define CCDB_FOLLOW	0x01
#define CCDB_INIT	0x02
#define CCDB_IO		0x04
#define CCDB_LABEL	0x08
#define CCDB_VNODE	0x10
int ccddebug = 0x00;
#endif

#define	ccdunit(x)	DISKUNIT(x)

struct ccdbuf {
	struct buf	cb_buf;		/* new I/O buf */
	struct buf	*cb_obp;	/* ptr. to original I/O buf */
	struct ccd_softc *cb_sc;	/* pointer to ccd softc */
	int		cb_comp;	/* target component */
	SIMPLEQ_ENTRY(ccdbuf) cb_q;	/* fifo of component buffers */
};

/* component buffer pool */
struct pool ccd_cbufpool;

#define	CCD_GETBUF()		pool_get(&ccd_cbufpool, PR_NOWAIT)
#define	CCD_PUTBUF(cbp)		pool_put(&ccd_cbufpool, cbp)

#define CCDLABELDEV(dev)	\
	(MAKEDISKDEV(major((dev)), ccdunit((dev)), RAW_PART))

/* called by main() at boot time */
void	ccdattach __P((int));

/* called by biodone() at interrupt time */
void	ccdiodone __P((struct buf *));
int	ccdsize __P((dev_t));

static	void ccdstart __P((struct ccd_softc *, struct buf *));
static	void ccdinterleave __P((struct ccd_softc *));
static	void ccdintr __P((struct ccd_softc *, struct buf *));
static	int ccdinit __P((struct ccd_softc *, char **, struct vnode **,
	    struct proc *));
static	int ccdlookup __P((char *, struct proc *p, struct vnode **));
static	struct ccdbuf *ccdbuffer __P((struct ccd_softc *, struct buf *,
		daddr_t, caddr_t, long));
static	void ccdgetdefaultlabel __P((struct ccd_softc *, struct disklabel *));
static	void ccdgetdisklabel __P((dev_t));
static	void ccdmakedisklabel __P((struct ccd_softc *));

#ifdef DEBUG
static	void printiinfo __P((struct ccdiinfo *));
#endif

/* Non-private for the benefit of libkvm. */
struct	ccd_softc *ccd_softc;
int	numccd = 0;

/*
 * Called by main() during pseudo-device attachment.  All we need
 * to do is allocate enough space for devices to be configured later.
 */
void
ccdattach(num)
	int num;
{
	struct ccd_softc *cs;
	int i;

	if (num <= 0) {
#ifdef DIAGNOSTIC
		panic("ccdattach: count <= 0");
#endif
		return;
	}

	ccd_softc = (struct ccd_softc *)malloc(num * sizeof(struct ccd_softc),
	    M_DEVBUF, M_NOWAIT|M_ZERO);
	if (ccd_softc == NULL) {
		printf("WARNING: no memory for concatenated disks\n");
		return;
	}
	numccd = num;

	/* Initialize the component buffer pool. */
	pool_init(&ccd_cbufpool, sizeof(struct ccdbuf), 0,
	    0, 0, "ccdpl", NULL);

	/* Initialize per-softc structures. */
	for (i = 0; i < num; i++) {
		cs = &ccd_softc[i];
		sprintf(cs->sc_xname, "ccd%d", i);	/* XXX */
		cs->sc_dkdev.dk_name = cs->sc_xname;	/* XXX */
		lockinit(&cs->sc_lock, PRIBIO, "ccdlk", 0, 0);
	}
}

static int
ccdinit(cs, cpaths, vpp, p)
	struct ccd_softc *cs;
	char **cpaths;
	struct vnode **vpp;
	struct proc *p;
{
	struct ccdcinfo *ci = NULL;
	size_t size;
	int ix;
	struct vattr va;
	size_t minsize;
	int maxsecsize;
	struct partinfo dpart;
	struct ccdgeom *ccg = &cs->sc_geom;
	char tmppath[MAXPATHLEN];
	int error, path_alloced;

#ifdef DEBUG
	if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
		printf("%s: ccdinit\n", cs->sc_xname);
#endif

	/* Allocate space for the component info. */
	cs->sc_cinfo = malloc(cs->sc_nccdisks * sizeof(struct ccdcinfo),
	    M_DEVBUF, M_WAITOK);

	cs->sc_size = 0;

	/*
	 * Verify that each component piece exists and record
	 * relevant information about it.
	 */
	maxsecsize = 0;
	minsize = 0;
	for (ix = 0, path_alloced = 0; ix < cs->sc_nccdisks; ix++) {
		ci = &cs->sc_cinfo[ix];
		ci->ci_vp = vpp[ix];

		/*
		 * Copy in the pathname of the component.
		 */
		memset(tmppath, 0, sizeof(tmppath));	/* sanity */
		error = copyinstr(cpaths[ix], tmppath,
		    MAXPATHLEN, &ci->ci_pathlen);
		if (error) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				printf("%s: can't copy path, error = %d\n",
				    cs->sc_xname, error);
#endif
			goto out;
		}
		ci->ci_path = malloc(ci->ci_pathlen, M_DEVBUF, M_WAITOK);
		memcpy(ci->ci_path, tmppath, ci->ci_pathlen);
		path_alloced++;

		/*
		 * XXX: Cache the component's dev_t.
		 */
		if ((error = VOP_GETATTR(vpp[ix], &va, p->p_ucred, p)) != 0) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				printf("%s: %s: getattr failed %s = %d\n",
				    cs->sc_xname, ci->ci_path,
				    "error", error);
#endif
			goto out;
		}
		ci->ci_dev = va.va_rdev;

		/*
		 * Get partition information for the component.
		 */
		error = VOP_IOCTL(vpp[ix], DIOCGPART, (caddr_t)&dpart,
		    FREAD, p->p_ucred, p);
		if (error) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				 printf("%s: %s: ioctl failed, error = %d\n",
				     cs->sc_xname, ci->ci_path, error);
#endif
			goto out;
		}

/*
 * This diagnostic test is disabled (for now?) since not all port supports
 * on-disk BSD disklabel.
 */
#if 0 /* def DIAGNOSTIC */
		/* Check fstype field of component. */
		if (dpart.part->p_fstype != FS_CCD)
			printf("%s: WARNING: %s: fstype %d != FS_CCD\n",
			    cs->sc_xname, ci->ci_path, dpart.part->p_fstype);
#endif

		/*
		 * Calculate the size, truncating to an interleave
		 * boundary if necessary.
		 */
		maxsecsize =
		    ((dpart.disklab->d_secsize > maxsecsize) ?
		    dpart.disklab->d_secsize : maxsecsize);
		size = dpart.part->p_size;
		if (cs->sc_ileave > 1)
			size -= size % cs->sc_ileave;

		if (size == 0) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				printf("%s: %s: size == 0\n",
				    cs->sc_xname, ci->ci_path);
#endif
			error = ENODEV;
			goto out;
		}

		if (minsize == 0 || size < minsize)
			minsize = size;
		ci->ci_size = size;
		cs->sc_size += size;
	}

	/*
	 * Don't allow the interleave to be smaller than
	 * the biggest component sector.
	 */
	if ((cs->sc_ileave > 0) &&
	    (cs->sc_ileave < (maxsecsize / DEV_BSIZE))) {
#ifdef DEBUG
		if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
			printf("%s: interleave must be at least %d\n",
			    cs->sc_xname, (maxsecsize / DEV_BSIZE));
#endif
		error = EINVAL;
		goto out;
	}

	/*
	 * If uniform interleave is desired set all sizes to that of
	 * the smallest component.
	 */
	if (cs->sc_flags & CCDF_UNIFORM) {
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_nccdisks]; ci++)
			ci->ci_size = minsize;

		cs->sc_size = cs->sc_nccdisks * minsize;
	}

	/*
	 * Construct the interleave table.
	 */
	ccdinterleave(cs);

	/*
	 * Create pseudo-geometry based on 1MB cylinders.  It's
	 * pretty close.
	 */
	ccg->ccg_secsize = DEV_BSIZE;
	ccg->ccg_ntracks = 1;
	ccg->ccg_nsectors = 1024 * (1024 / ccg->ccg_secsize);
	ccg->ccg_ncylinders = cs->sc_size / ccg->ccg_nsectors;

	cs->sc_flags |= CCDF_INITED;

	return (0);

 out:
	for (ix = 0; ix < path_alloced; ix++)
		free(cs->sc_cinfo[ix].ci_path, M_DEVBUF);
	free(cs->sc_cinfo, M_DEVBUF);
	return (error);
}

static void
ccdinterleave(cs)
	struct ccd_softc *cs;
{
	struct ccdcinfo *ci, *smallci;
	struct ccdiinfo *ii;
	daddr_t bn, lbn;
	int ix;
	u_long size;

#ifdef DEBUG
	if (ccddebug & CCDB_INIT)
		printf("ccdinterleave(%p): ileave %d\n", cs, cs->sc_ileave);
#endif
	/*
	 * Allocate an interleave table.
	 * Chances are this is too big, but we don't care.
	 */
	size = (cs->sc_nccdisks + 1) * sizeof(struct ccdiinfo);
	cs->sc_itable = (struct ccdiinfo *)malloc(size, M_DEVBUF,
	    M_WAITOK|M_ZERO);

	/*
	 * Trivial case: no interleave (actually interleave of disk size).
	 * Each table entry represents a single component in its entirety.
	 */
	if (cs->sc_ileave == 0) {
		bn = 0;
		ii = cs->sc_itable;

		for (ix = 0; ix < cs->sc_nccdisks; ix++) {
			/* Allocate space for ii_index. */
			ii->ii_index = malloc(sizeof(int), M_DEVBUF, M_WAITOK);
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
		return;
	}

	/*
	 * The following isn't fast or pretty; it doesn't have to be.
	 */
	size = 0;
	bn = lbn = 0;
	for (ii = cs->sc_itable; ; ii++) {
		/* Allocate space for ii_index. */
		ii->ii_index = malloc((sizeof(int) * cs->sc_nccdisks),
		    M_DEVBUF, M_WAITOK);

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
}

/* ARGSUSED */
int
ccdopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	int unit = ccdunit(dev);
	struct ccd_softc *cs;
	struct disklabel *lp;
	int error = 0, part, pmask;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdopen(0x%x, 0x%x)\n", dev, flags);
#endif
	if (unit >= numccd)
		return (ENXIO);
	cs = &ccd_softc[unit];

	if ((error = lockmgr(&cs->sc_lock, LK_EXCLUSIVE, NULL)) != 0)
		return (error);

	lp = cs->sc_dkdev.dk_label;

	part = DISKPART(dev);
	pmask = (1 << part);

	/*
	 * If we're initialized, check to see if there are any other
	 * open partitions.  If not, then it's safe to update
	 * the in-core disklabel.
	 */
	if ((cs->sc_flags & CCDF_INITED) && (cs->sc_dkdev.dk_openmask == 0))
		ccdgetdisklabel(dev);

	/* Check that the partition exists. */
	if (part != RAW_PART) {
		if (((cs->sc_flags & CCDF_INITED) == 0) ||
		    ((part >= lp->d_npartitions) ||
		     (lp->d_partitions[part].p_fstype == FS_UNUSED))) {
			error = ENXIO;
			goto done;
		}
	}

	/* Prevent our unit from being unconfigured while open. */
	switch (fmt) {
	case S_IFCHR:
		cs->sc_dkdev.dk_copenmask |= pmask;
		break;

	case S_IFBLK:
		cs->sc_dkdev.dk_bopenmask |= pmask;
		break;
	}
	cs->sc_dkdev.dk_openmask =
	    cs->sc_dkdev.dk_copenmask | cs->sc_dkdev.dk_bopenmask;

 done:
	(void) lockmgr(&cs->sc_lock, LK_RELEASE, NULL);
	return (error);
}

/* ARGSUSED */
int
ccdclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	int unit = ccdunit(dev);
	struct ccd_softc *cs;
	int error = 0, part;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdclose(0x%x, 0x%x)\n", dev, flags);
#endif

	if (unit >= numccd)
		return (ENXIO);
	cs = &ccd_softc[unit];

	if ((error = lockmgr(&cs->sc_lock, LK_EXCLUSIVE, NULL)) != 0)
		return (error);

	part = DISKPART(dev);

	/* ...that much closer to allowing unconfiguration... */
	switch (fmt) {
	case S_IFCHR:
		cs->sc_dkdev.dk_copenmask &= ~(1 << part);
		break;

	case S_IFBLK:
		cs->sc_dkdev.dk_bopenmask &= ~(1 << part);
		break;
	}
	cs->sc_dkdev.dk_openmask =
	    cs->sc_dkdev.dk_copenmask | cs->sc_dkdev.dk_bopenmask;

	(void) lockmgr(&cs->sc_lock, LK_RELEASE, NULL);
	return (0);
}

void
ccdstrategy(bp)
	struct buf *bp;
{
	int unit = ccdunit(bp->b_dev);
	struct ccd_softc *cs = &ccd_softc[unit];
	int s;
	int wlabel;
	struct disklabel *lp;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdstrategy(%p): unit %d\n", bp, unit);
#endif
	if ((cs->sc_flags & CCDF_INITED) == 0) {
#ifdef DEBUG
		if (ccddebug & CCDB_FOLLOW)
			printf("ccdstrategy: unit %d: not inited\n", unit);
#endif
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	/* If it's a nil transfer, wake up the top half now. */
	if (bp->b_bcount == 0)
		goto done;

	lp = cs->sc_dkdev.dk_label;

	/*
	 * Do bounds checking and adjust transfer.  If there's an
	 * error, the bounds check will flag that for us.
	 */
	wlabel = cs->sc_flags & (CCDF_WLABEL|CCDF_LABELLING);
	if (DISKPART(bp->b_dev) != RAW_PART)
		if (bounds_check_with_label(bp, lp, wlabel) <= 0)
			goto done;

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

static void
ccdstart(cs, bp)
	struct ccd_softc *cs;
	struct buf *bp;
{
	long bcount, rcount;
	struct ccdbuf *cbp;
	caddr_t addr;
	daddr_t bn;
	struct partition *pp;
	SIMPLEQ_HEAD(, ccdbuf) cbufq;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdstart(%p, %p)\n", cs, bp);
#endif

	/* Instrumentation. */
	disk_busy(&cs->sc_dkdev);

	/*
	 * Translate the partition-relative block number to an absolute.
	 */
	bn = bp->b_blkno;
	if (DISKPART(bp->b_dev) != RAW_PART) {
		pp = &cs->sc_dkdev.dk_label->d_partitions[DISKPART(bp->b_dev)];
		bn += pp->p_offset;
	}

	/*
	 * Allocate the component buffers.
	 */
	SIMPLEQ_INIT(&cbufq);
	addr = bp->b_data;
	for (bcount = bp->b_bcount; bcount > 0; bcount -= rcount) {
		cbp = ccdbuffer(cs, bp, bn, addr, bcount);
		if (cbp == NULL) {
			/* Free the already allocated component buffers. */
			while ((cbp = SIMPLEQ_FIRST(&cbufq)) != NULL) {
				SIMPLEQ_REMOVE_HEAD(&cbufq, cb_q);
				CCD_PUTBUF(cbp);
			}

			/* Notify the upper layer we are out of memory. */
			bp->b_error = ENOMEM;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			disk_unbusy(&cs->sc_dkdev, 0);
			return;
		}
		SIMPLEQ_INSERT_TAIL(&cbufq, cbp, cb_q);
		rcount = cbp->cb_buf.b_bcount;
		bn += btodb(rcount);
		addr += rcount;
	}

	/* Now fire off the requests. */
	while ((cbp = SIMPLEQ_FIRST(&cbufq)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&cbufq, cb_q);
		if ((cbp->cb_buf.b_flags & B_READ) == 0)
			cbp->cb_buf.b_vp->v_numoutput++;
		VOP_STRATEGY(&cbp->cb_buf);
	}
}

/*
 * Build a component buffer header.
 */
static struct ccdbuf *
ccdbuffer(cs, bp, bn, addr, bcount)
	struct ccd_softc *cs;
	struct buf *bp;
	daddr_t bn;
	caddr_t addr;
	long bcount;
{
	struct ccdcinfo *ci;
	struct ccdbuf *cbp;
	daddr_t cbn, cboff;
	u_int64_t cbc;
	int ccdisk;

#ifdef DEBUG
	if (ccddebug & CCDB_IO)
		printf("ccdbuffer(%p, %p, %d, %p, %ld)\n",
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
		daddr_t sblk;

		sblk = 0;
		for (ccdisk = 0, ci = &cs->sc_cinfo[ccdisk];
		    cbn >= sblk + ci->ci_size;
		    ccdisk++, ci = &cs->sc_cinfo[ccdisk])
			sblk += ci->ci_size;
		cbn -= sblk;
	}
	/*
	 * Interleaved
	 */
	else {
		struct ccdiinfo *ii;
		int off;

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
	cbp = CCD_GETBUF();
	if (cbp == NULL)
		return (NULL);
	cbp->cb_buf.b_flags = bp->b_flags | B_CALL;
	cbp->cb_buf.b_iodone = ccdiodone;
	cbp->cb_buf.b_proc = bp->b_proc;
	cbp->cb_buf.b_dev = ci->ci_dev;		/* XXX */
	cbp->cb_buf.b_blkno = cbn + cboff;
	cbp->cb_buf.b_data = addr;
	cbp->cb_buf.b_vp = ci->ci_vp;
	LIST_INIT(&cbp->cb_buf.b_dep);
	if (cs->sc_ileave == 0)
		cbc = dbtob((u_int64_t)(ci->ci_size - cbn));
	else
		cbc = dbtob((u_int64_t)(cs->sc_ileave - cboff));
	cbp->cb_buf.b_bcount = cbc < bcount ? cbc : bcount;

	/*
	 * context for ccdiodone
	 */
	cbp->cb_obp = bp;
	cbp->cb_sc = cs;
	cbp->cb_comp = ccdisk;

#ifdef DEBUG
	if (ccddebug & CCDB_IO)
		printf(" dev 0x%x(u%lu): cbp %p bn %d addr %p bcnt %ld\n",
		    ci->ci_dev, (unsigned long) (ci-cs->sc_cinfo), cbp,
		    cbp->cb_buf.b_blkno, cbp->cb_buf.b_data,
		    cbp->cb_buf.b_bcount);
#endif

	return (cbp);
}

static void
ccdintr(cs, bp)
	struct ccd_softc *cs;
	struct buf *bp;
{

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdintr(%p, %p)\n", cs, bp);
#endif
	/*
	 * Request is done for better or worse, wakeup the top half.
	 */
	if (bp->b_flags & B_ERROR)
		bp->b_resid = bp->b_bcount;
	disk_unbusy(&cs->sc_dkdev, (bp->b_bcount - bp->b_resid));
	biodone(bp);
}

/*
 * Called at interrupt time.
 * Mark the component as done and if all components are done,
 * take a ccd interrupt.
 */
void
ccdiodone(vbp)
	struct buf *vbp;
{
	struct ccdbuf *cbp = (struct ccdbuf *) vbp;
	struct buf *bp = cbp->cb_obp;
	struct ccd_softc *cs = cbp->cb_sc;
	int count, s;

	s = splbio();
#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdiodone(%p)\n", cbp);
	if (ccddebug & CCDB_IO) {
		printf("ccdiodone: bp %p bcount %ld resid %ld\n",
		       bp, bp->b_bcount, bp->b_resid);
		printf(" dev 0x%x(u%d), cbp %p bn %d addr %p bcnt %ld\n",
		       cbp->cb_buf.b_dev, cbp->cb_comp, cbp,
		       cbp->cb_buf.b_blkno, cbp->cb_buf.b_data,
		       cbp->cb_buf.b_bcount);
	}
#endif

	if (cbp->cb_buf.b_flags & B_ERROR) {
		bp->b_flags |= B_ERROR;
		bp->b_error = cbp->cb_buf.b_error ?
		    cbp->cb_buf.b_error : EIO;

		printf("%s: error %d on component %d\n",
		       cs->sc_xname, bp->b_error, cbp->cb_comp);
	}
	count = cbp->cb_buf.b_bcount;
	CCD_PUTBUF(cbp);

	/*
	 * If all done, "interrupt".
	 */
	bp->b_resid -= count;
	if (bp->b_resid < 0)
		panic("ccdiodone: count");
	if (bp->b_resid == 0)
		ccdintr(cs, bp);
	splx(s);
}

/* ARGSUSED */
int
ccdread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	int unit = ccdunit(dev);
	struct ccd_softc *cs;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdread(0x%x, %p)\n", dev, uio);
#endif
	if (unit >= numccd)
		return (ENXIO);
	cs = &ccd_softc[unit];

	if ((cs->sc_flags & CCDF_INITED) == 0)
		return (ENXIO);

	/*
	 * XXX: It's not clear that using minphys() is completely safe,
	 * in particular, for raw I/O.  Underlying devices might have some
	 * non-obvious limits, because of the copy to user-space.
	 */
	return (physio(ccdstrategy, NULL, dev, B_READ, minphys, uio));
}

/* ARGSUSED */
int
ccdwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	int unit = ccdunit(dev);
	struct ccd_softc *cs;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdwrite(0x%x, %p)\n", dev, uio);
#endif
	if (unit >= numccd)
		return (ENXIO);
	cs = &ccd_softc[unit];

	if ((cs->sc_flags & CCDF_INITED) == 0)
		return (ENXIO);

	/*
	 * XXX: It's not clear that using minphys() is completely safe,
	 * in particular, for raw I/O.  Underlying devices might have some
	 * non-obvious limits, because of the copy to user-space.
	 */
	return (physio(ccdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

int
ccdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = ccdunit(dev);
	int i, j, lookedup = 0, error;
	int part, pmask;
	struct ccd_softc *cs;
	struct ccd_ioctl *ccio = (struct ccd_ioctl *)data;
	char **cpp;
	struct vnode **vpp;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif

	if (unit >= numccd)
		return (ENXIO);
	cs = &ccd_softc[unit];

	/* Must be open for writes for these commands... */
	switch (cmd) {
	case CCDIOCSET:
	case CCDIOCCLR:
	case DIOCSDINFO:
	case DIOCWDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCSDINFO:
	case ODIOCWDINFO:
#endif
	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
	}

	if ((error = lockmgr(&cs->sc_lock, LK_EXCLUSIVE, NULL)) != 0)
		return (error);

	/* Must be initialized for these... */
	switch (cmd) {
	case CCDIOCCLR:
	case DIOCGDINFO:
	case DIOCSDINFO:
	case DIOCWDINFO:
	case DIOCGPART:
	case DIOCWLABEL:
	case DIOCGDEFLABEL:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
	case ODIOCSDINFO:
	case ODIOCWDINFO:
	case ODIOCGDEFLABEL:
#endif
		if ((cs->sc_flags & CCDF_INITED) == 0) {
			error = ENXIO;
			goto out;
		}
	}

	switch (cmd) {
	case CCDIOCSET:
		if (cs->sc_flags & CCDF_INITED) {
			error = EBUSY;
			goto out;
		}

		/* Validate the flags. */
		if ((ccio->ccio_flags & CCDF_USERMASK) != ccio->ccio_flags) {
			error = EINVAL;
			goto out;
		}

		if (ccio->ccio_ndisks > CCD_MAXNDISKS) {
			error = EINVAL;
			goto out;
		}
			
		/* Fill in some important bits. */
		cs->sc_ileave = ccio->ccio_ileave;
		cs->sc_nccdisks = ccio->ccio_ndisks;
		cs->sc_flags = ccio->ccio_flags & CCDF_USERMASK;

		/*
		 * Allocate space for and copy in the array of
		 * componet pathnames and device numbers.
		 */
		cpp = malloc(ccio->ccio_ndisks * sizeof(char *),
		    M_DEVBUF, M_WAITOK);
		vpp = malloc(ccio->ccio_ndisks * sizeof(struct vnode *),
		    M_DEVBUF, M_WAITOK);

		error = copyin((caddr_t)ccio->ccio_disks, (caddr_t)cpp,
		    ccio->ccio_ndisks * sizeof(char **));
		if (error) {
			free(vpp, M_DEVBUF);
			free(cpp, M_DEVBUF);
			goto out;
		}

#ifdef DEBUG
		if (ccddebug & CCDB_INIT)
			for (i = 0; i < ccio->ccio_ndisks; ++i)
				printf("ccdioctl: component %d: 0x%p\n",
				    i, cpp[i]);
#endif

		for (i = 0; i < ccio->ccio_ndisks; ++i) {
#ifdef DEBUG
			if (ccddebug & CCDB_INIT)
				printf("ccdioctl: lookedup = %d\n", lookedup);
#endif
			if ((error = ccdlookup(cpp[i], p, &vpp[i])) != 0) {
				for (j = 0; j < lookedup; ++j)
					(void)vn_close(vpp[j], FREAD|FWRITE,
					    p->p_ucred, p);
				free(vpp, M_DEVBUF);
				free(cpp, M_DEVBUF);
				goto out;
			}
			++lookedup;
		}

		/*
		 * Initialize the ccd.  Fills in the softc for us.
		 */
		if ((error = ccdinit(cs, cpp, vpp, p)) != 0) {
			for (j = 0; j < lookedup; ++j)
				(void)vn_close(vpp[j], FREAD|FWRITE,
				    p->p_ucred, p);
			free(vpp, M_DEVBUF);
			free(cpp, M_DEVBUF);
			goto out;
		}

		/* We can free the temporary variables now. */
		free(vpp, M_DEVBUF);
		free(cpp, M_DEVBUF);

		/*
		 * The ccd has been successfully initialized, so
		 * we can place it into the array.  Don't try to
		 * read the disklabel until the disk has been attached,
		 * because space for the disklabel is allocated
		 * in disk_attach();
		 */
		ccio->ccio_unit = unit;
		ccio->ccio_size = cs->sc_size;

		/* Attach the disk. */
		disk_attach(&cs->sc_dkdev);

		/* Try and read the disklabel. */
		ccdgetdisklabel(dev);
		break;

	case CCDIOCCLR:
		/*
		 * Don't unconfigure if any other partitions are open
		 * or if both the character and block flavors of this
		 * partition are open.
		 */
		part = DISKPART(dev);
		pmask = (1 << part);
		if ((cs->sc_dkdev.dk_openmask & ~pmask) ||
		    ((cs->sc_dkdev.dk_bopenmask & pmask) &&
		    (cs->sc_dkdev.dk_copenmask & pmask))) {
			error = EBUSY;
			goto out;
		}

		/*
		 * Free ccd_softc information and clear entry.
		 */

		/* Close the components and free their pathnames. */
		for (i = 0; i < cs->sc_nccdisks; ++i) {
			/*
			 * XXX: this close could potentially fail and
			 * cause Bad Things.  Maybe we need to force
			 * the close to happen?
			 */
#ifdef DEBUG
			if (ccddebug & CCDB_VNODE)
				vprint("CCDIOCCLR: vnode info",
				    cs->sc_cinfo[i].ci_vp);
#endif
			(void)vn_close(cs->sc_cinfo[i].ci_vp, FREAD|FWRITE,
			    p->p_ucred, p);
			free(cs->sc_cinfo[i].ci_path, M_DEVBUF);
		}

		/* Free interleave index. */
		for (i = 0; cs->sc_itable[i].ii_ndisk; ++i)
			free(cs->sc_itable[i].ii_index, M_DEVBUF);

		/* Free component info and interleave table. */
		free(cs->sc_cinfo, M_DEVBUF);
		free(cs->sc_itable, M_DEVBUF);
		cs->sc_flags &= ~CCDF_INITED;

		/* Detatch the disk. */
		disk_detach(&cs->sc_dkdev);
		break;

	case DIOCGDINFO:
		*(struct disklabel *)data = *(cs->sc_dkdev.dk_label);
		break;
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *(cs->sc_dkdev.dk_label);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(data, &newlabel, sizeof (struct olddisklabel));
		break;
#endif

	case DIOCGPART:
		((struct partinfo *)data)->disklab = cs->sc_dkdev.dk_label;
		((struct partinfo *)data)->part =
		    &cs->sc_dkdev.dk_label->d_partitions[DISKPART(dev)];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:
#endif
	{
		struct disklabel *lp;
#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, data, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)data;

		cs->sc_flags |= CCDF_LABELLING;

		error = setdisklabel(cs->sc_dkdev.dk_label,
		    lp, 0, cs->sc_dkdev.dk_cpulabel);
		if (error == 0) {
			if (cmd == DIOCWDINFO
#ifdef __HAVE_OLD_DISKLABEL
			    || cmd == ODIOCWDINFO
#endif
			   )
				error = writedisklabel(CCDLABELDEV(dev),
				    ccdstrategy, cs->sc_dkdev.dk_label,
				    cs->sc_dkdev.dk_cpulabel);
		}

		cs->sc_flags &= ~CCDF_LABELLING;
		break;
	}

	case DIOCWLABEL:
		if (*(int *)data != 0)
			cs->sc_flags |= CCDF_WLABEL;
		else
			cs->sc_flags &= ~CCDF_WLABEL;
		break;

	case DIOCGDEFLABEL:
		ccdgetdefaultlabel(cs, (struct disklabel *)data);
		break;

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		ccdgetdefaultlabel(cs, &newlabel);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(data, &newlabel, sizeof (struct olddisklabel));
		break;
#endif

	default:
		error = ENOTTY;
	}

 out:
	(void) lockmgr(&cs->sc_lock, LK_RELEASE, NULL);
	return (error);
}

int
ccdsize(dev)
	dev_t dev;
{
	struct ccd_softc *cs;
	struct disklabel *lp;
	int part, unit, omask, size;

	unit = ccdunit(dev);
	if (unit >= numccd)
		return (-1);
	cs = &ccd_softc[unit];

	if ((cs->sc_flags & CCDF_INITED) == 0)
		return (-1);

	part = DISKPART(dev);
	omask = cs->sc_dkdev.dk_openmask & (1 << part);
	lp = cs->sc_dkdev.dk_label;

	if (omask == 0 && ccdopen(dev, 0, S_IFBLK, curproc->l_proc))
		return (-1);

	if (lp->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = lp->d_partitions[part].p_size *
		    (lp->d_secsize / DEV_BSIZE);

	if (omask == 0 && ccdclose(dev, 0, S_IFBLK, curproc->l_proc))
		return (-1);

	return (size);
}

int
ccddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return ENXIO;
}

/*
 * Lookup the provided name in the filesystem.  If the file exists,
 * is a valid block device, and isn't being used by anyone else,
 * set *vpp to the file's vnode.
 */
static int
ccdlookup(path, p, vpp)
	char *path;
	struct proc *p;
	struct vnode **vpp;	/* result */
{
	struct nameidata nd;
	struct vnode *vp;
	struct vattr va;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, path, p);
	if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0) {
#ifdef DEBUG
		if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
			printf("ccdlookup: vn_open error = %d\n", error);
#endif
		return (error);
	}
	vp = nd.ni_vp;

	if (vp->v_usecount > 1) {
		VOP_UNLOCK(vp, 0);
		(void)vn_close(vp, FREAD|FWRITE, p->p_ucred, p);
		return (EBUSY);
	}

	if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)) != 0) {
#ifdef DEBUG
		if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
			printf("ccdlookup: getattr error = %d\n", error);
#endif
		VOP_UNLOCK(vp, 0);
		(void)vn_close(vp, FREAD|FWRITE, p->p_ucred, p);
		return (error);
	}

	/* XXX: eventually we should handle VREG, too. */
	if (va.va_type != VBLK) {
		VOP_UNLOCK(vp, 0);
		(void)vn_close(vp, FREAD|FWRITE, p->p_ucred, p);
		return (ENOTBLK);
	}

#ifdef DEBUG
	if (ccddebug & CCDB_VNODE)
		vprint("ccdlookup: vnode info", vp);
#endif

	VOP_UNLOCK(vp, 0);
	*vpp = vp;
	return (0);
}

static void
ccdgetdefaultlabel(cs, lp)
	struct ccd_softc *cs;
	struct disklabel *lp;
{
	struct ccdgeom *ccg = &cs->sc_geom;

	memset(lp, 0, sizeof(*lp));

	lp->d_secperunit = cs->sc_size;
	lp->d_secsize = ccg->ccg_secsize;
	lp->d_nsectors = ccg->ccg_nsectors;
	lp->d_ntracks = ccg->ccg_ntracks;
	lp->d_ncylinders = ccg->ccg_ncylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	strncpy(lp->d_typename, "ccd", sizeof(lp->d_typename));
	lp->d_type = DTYPE_CCD;
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = cs->sc_size;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(cs->sc_dkdev.dk_label);
}

/*
 * Read the disklabel from the ccd.  If one is not present, fake one
 * up.
 */
static void
ccdgetdisklabel(dev)
	dev_t dev;
{
	int unit = ccdunit(dev);
	struct ccd_softc *cs = &ccd_softc[unit];
	char *errstring;
	struct disklabel *lp = cs->sc_dkdev.dk_label;
	struct cpu_disklabel *clp = cs->sc_dkdev.dk_cpulabel;

	memset(clp, 0, sizeof(*clp));

	ccdgetdefaultlabel(cs, lp);

	/*
	 * Call the generic disklabel extraction routine.
	 */
	errstring = readdisklabel(CCDLABELDEV(dev), ccdstrategy,
	    cs->sc_dkdev.dk_label, cs->sc_dkdev.dk_cpulabel);
	if (errstring)
		ccdmakedisklabel(cs);
	else {
		int i;
		struct partition *pp;

		/*
		 * Sanity check whether the found disklabel is valid.
		 *
		 * This is necessary since total size of ccd may vary
		 * when an interleave is changed even though exactly
		 * same componets are used, and old disklabel may used
		 * if that is found.
		 */
		if (lp->d_secperunit != cs->sc_size)
			printf("WARNING: %s: "
			    "total sector size in disklabel (%d) != "
			    "the size of ccd (%lu)\n", cs->sc_xname,
			    lp->d_secperunit, (u_long)cs->sc_size);
		for (i = 0; i < lp->d_npartitions; i++) {
			pp = &lp->d_partitions[i];
			if (pp->p_offset + pp->p_size > cs->sc_size)
				printf("WARNING: %s: end of partition `%c' "
				    "exceeds the size of ccd (%lu)\n",
				    cs->sc_xname, 'a' + i, (u_long)cs->sc_size);
		}
	}

#ifdef DEBUG
	/* It's actually extremely common to have unlabeled ccds. */
	if (ccddebug & CCDB_LABEL)
		if (errstring != NULL)
			printf("%s: %s\n", cs->sc_xname, errstring);
#endif
}

/*
 * Take care of things one might want to take care of in the event
 * that a disklabel isn't present.
 */
static void
ccdmakedisklabel(cs)
	struct ccd_softc *cs;
{
	struct disklabel *lp = cs->sc_dkdev.dk_label;

	/*
	 * For historical reasons, if there's no disklabel present
	 * the raw partition must be marked FS_BSDFFS.
	 */
	lp->d_partitions[RAW_PART].p_fstype = FS_BSDFFS;

	strncpy(lp->d_packname, "default label", sizeof(lp->d_packname));

	lp->d_checksum = dkcksum(lp);
}

#ifdef DEBUG
static void
printiinfo(ii)
	struct ccdiinfo *ii;
{
	int ix, i;

	for (ix = 0; ii->ii_ndisk; ix++, ii++) {
		printf(" itab[%d]: #dk %d sblk %d soff %d",
		    ix, ii->ii_ndisk, ii->ii_startblk, ii->ii_startoff);
		for (i = 0; i < ii->ii_ndisk; i++)
			printf(" %d", ii->ii_index[i]);
		printf("\n");
	}
}
#endif
