/*	$NetBSD: vnd.c,v 1.44 1997/06/26 16:28:37 kleink Exp $	*/

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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
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
 * from: Utah $Hdr: vn.c 1.13 94/04/02$
 *
 *	@(#)vn.c	8.6 (Berkeley) 4/1/94
 */

/*
 * Vnode disk driver.
 *
 * Block/character interface to a vnode.  Allows one to treat a file
 * as a disk (e.g. build a filesystem in it, mount it, etc.).
 *
 * NOTE 1: This uses the VOP_BMAP/VOP_STRATEGY interface to the vnode
 * instead of a simple VOP_RDWR.  We do this to avoid distorting the
 * local buffer cache.
 *
 * NOTE 2: There is a security issue involved with this driver.
 * Once mounted all access to the contents of the "mapped" file via
 * the special file is controlled by the permissions on the special
 * file, the protection of the mapped file is ignored (effectively,
 * by using root credentials in all transactions).
 *
 * NOTE 3: Doesn't interact with leases, should it?
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>

#include <miscfs/specfs/specdev.h>

#include <dev/vndvar.h>

#if defined(VNDDEBUG) && !defined(DEBUG)
#define	DEBUG
#endif

#ifdef DEBUG
int dovndcluster = 1;
#define	VDB_FOLLOW	0x01
#define	VDB_INIT	0x02
#define	VDB_IO		0x04
#define	VDB_LABEL	0x08
int vnddebug = 0x00;
#endif

#define b_cylin	b_resid

#define	vndunit(x)	DISKUNIT(x)

struct vndxfer {
	struct buf	*vx_bp;		/* Pointer to parent buffer */
	int		vx_error;
	int		vx_pending;	/* # of pending aux buffers */
};

struct vndbuf {
	struct buf	vb_buf;
	struct vndxfer	*vb_xfer;
};

#define	getvndxfer()	\
	((struct vndxfer *)malloc(sizeof(struct vndxfer), M_DEVBUF, M_WAITOK))
#define putvndxfer(vnx)	\
	free((caddr_t)(vnx), M_DEVBUF)
#define	getvndbuf()	\
	((struct vndbuf *)malloc(sizeof(struct vndbuf), M_DEVBUF, M_WAITOK))
#define putvndbuf(vbp)	\
	free((caddr_t)(vbp), M_DEVBUF)

struct vnd_softc *vnd_softc;
int numvnd = 0;

#define	VNDLABELDEV(dev) \
	(MAKEDISKDEV(major((dev)), vndunit((dev)), RAW_PART))

/* called by main() at boot time */
void	vndattach __P((int));

void	vndclear __P((struct vnd_softc *));
void	vndstart __P((struct vnd_softc *));
int	vndsetcred __P((struct vnd_softc *, struct ucred *));
void	vndthrottle __P((struct vnd_softc *, struct vnode *));
void	vndiodone __P((struct buf *));
void	vndshutdown __P((void));

void	vndgetdisklabel __P((dev_t));

static	int vndlock __P((struct vnd_softc *));
static	void vndunlock __P((struct vnd_softc *));

void
vndattach(num)
	int num;
{
	char *mem;
	register u_long size;

	if (num <= 0)
		return;
	size = num * sizeof(struct vnd_softc);
	mem = malloc(size, M_DEVBUF, M_NOWAIT);
	if (mem == NULL) {
		printf("WARNING: no memory for vnode disks\n");
		return;
	}
	bzero(mem, size);
	vnd_softc = (struct vnd_softc *)mem;
	numvnd = num;
}

int
vndopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;
	int error = 0, part, pmask;
	struct disklabel *lp;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndopen(%x, %x, %x, %p)\n", dev, flags, mode, p);
#endif
	if (unit >= numvnd)
		return (ENXIO);
	sc = &vnd_softc[unit];

	if ((error = vndlock(sc)) != 0)
		return (error);

	lp = sc->sc_dkdev.dk_label;

	part = DISKPART(dev);
	pmask = (1 << part);

	/*
	 * If we're initialized, check to see if there are any other
	 * open partitions.  If not, then it's safe to update the
	 * in-core disklabel.
	 */
	if ((sc->sc_flags & VNF_INITED) && (sc->sc_dkdev.dk_openmask == 0))
		vndgetdisklabel(dev);

	/* Check that the partitions exists. */
	if (part != RAW_PART) {
		if (((sc->sc_flags & VNF_INITED) == 0) ||
		    ((part >= lp->d_npartitions) ||
		     (lp->d_partitions[part].p_fstype == FS_UNUSED))) {
			error = ENXIO;
			goto done;
		}
	}

	/* Prevent our unit from being unconfigured while open. */
	switch (mode) {
	case S_IFCHR:
		sc->sc_dkdev.dk_copenmask |= pmask;
		break;

	case S_IFBLK:
		sc->sc_dkdev.dk_bopenmask |= pmask;
		break;
	}
	sc->sc_dkdev.dk_openmask =
	    sc->sc_dkdev.dk_copenmask | sc->sc_dkdev.dk_bopenmask;

 done:
	vndunlock(sc);
	return (error);
}

int
vndclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;
	int error = 0, part;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndclose(%x, %x, %x, %p)\n", dev, flags, mode, p);
#endif

	if (unit >= numvnd)
		return (ENXIO);
	sc = &vnd_softc[unit];

	if ((error = vndlock(sc)) != 0)
		return (error);

	part = DISKPART(dev);

	/* ...that much closer to allowing unconfiguration... */
	switch (mode) {
	case S_IFCHR:
		sc->sc_dkdev.dk_copenmask &= ~(1 << part);
		break;

	case S_IFBLK:
		sc->sc_dkdev.dk_bopenmask &= ~(1 << part);
		break;
	}
	sc->sc_dkdev.dk_openmask =
	    sc->sc_dkdev.dk_copenmask | sc->sc_dkdev.dk_bopenmask;

	vndunlock(sc);
	return (0);
}

/*
 * Break the request into bsize pieces and submit using VOP_BMAP/VOP_STRATEGY.
 */
void
vndstrategy(bp)
	register struct buf *bp;
{
	int unit = vndunit(bp->b_dev);
	struct vnd_softc *vnd = &vnd_softc[unit];
	struct vndbuf *nbp;
	struct vndxfer *vnx;
	int bn, bsize, resid;
	caddr_t addr;
	int sz, flags, error, wlabel;
	struct disklabel *lp;
	struct partition *pp;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndstrategy(%p): unit %d\n", bp, unit);
#endif
	if ((vnd->sc_flags & VNF_INITED) == 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	/* If it's a nil transfer, wake up the top half now. */
	if (bp->b_bcount == 0)
		goto done;

	lp = vnd->sc_dkdev.dk_label;

	/*
	 * The transfer must be a whole number of blocks.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	/*
	 * Do bounds checking and adjust transfer.  If there's an error,
	 * the bounds check will flag that for us.
	 */
	wlabel = vnd->sc_flags & (VNF_WLABEL|VNF_LABELLING);
	if (DISKPART(bp->b_dev) != RAW_PART)
		if (bounds_check_with_label(bp, lp, wlabel) <= 0)
			goto done;

	bp->b_resid = bp->b_bcount;

	/*
	 * Put the block number in terms of the logical blocksize
	 * of the "device".
	 */
	bn = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);

	/*
	 * Translate the partition-relative block number to an absolute.
	 */
	if (DISKPART(bp->b_dev) != RAW_PART) {
		pp = &vnd->sc_dkdev.dk_label->d_partitions[DISKPART(bp->b_dev)];
		bn += pp->p_offset;
	}

	/* ...and convert to a byte offset within the file. */
	bn *= lp->d_secsize;

 	bsize = vnd->sc_vp->v_mount->mnt_stat.f_iosize;
	addr = bp->b_data;
	flags = bp->b_flags | B_CALL;

	/* Allocate a header for this transfer and link it to the buffer */
	vnx = getvndxfer();
	vnx->vx_error = 0;
	vnx->vx_pending = 0;
	vnx->vx_bp = bp;

	for (resid = bp->b_resid; resid; resid -= sz) {
		struct vnode *vp;
		daddr_t nbn;
		int off, s, nra;

		nra = 0;
		VOP_LOCK(vnd->sc_vp);
		error = VOP_BMAP(vnd->sc_vp, bn / bsize, &vp, &nbn, &nra);
		VOP_UNLOCK(vnd->sc_vp);

		if (error == 0 && (long)nbn == -1)
			error = EIO;

		/*
		 * If there was an error or a hole in the file...punt.
		 * Note that we may have to wait for any operations
		 * that we have already fired off before releasing
		 * the buffer.
		 *
		 * XXX we could deal with holes here but it would be
		 * a hassle (in the write case).
		 */
		if (error) {
			vnx->vx_error = error;
			s = splbio();
			if (vnx->vx_pending == 0) {
				bp->b_error = error;
				bp->b_flags |= B_ERROR;
				putvndxfer(vnx);
				goto done;
			}
			splx(s);
			return;
		}

#ifdef DEBUG
		if (!dovndcluster)
			nra = 0;
#endif

		if ((off = bn % bsize) != 0)
			sz = bsize - off;
		else
			sz = (1 + nra) * bsize;
		if (resid < sz)
			sz = resid;
#ifdef DEBUG
		if (vnddebug & VDB_IO)
			printf("vndstrategy: vp %p/%p bn %x/%x sz %x\n",
			    vnd->sc_vp, vp, bn, nbn, sz);
#endif

		nbp = getvndbuf();
		nbp->vb_buf.b_flags = flags;
		nbp->vb_buf.b_bcount = sz;
		nbp->vb_buf.b_bufsize = bp->b_bufsize;
		nbp->vb_buf.b_error = 0;
		if (vp->v_type == VBLK || vp->v_type == VCHR)
			nbp->vb_buf.b_dev = vp->v_rdev;
		else
			nbp->vb_buf.b_dev = NODEV;
		nbp->vb_buf.b_data = addr;
		nbp->vb_buf.b_blkno = nbn + btodb(off);
		nbp->vb_buf.b_proc = bp->b_proc;
		nbp->vb_buf.b_iodone = vndiodone;
		nbp->vb_buf.b_vp = vp;
		nbp->vb_buf.b_rcred = vnd->sc_cred;	/* XXX crdup? */
		nbp->vb_buf.b_wcred = vnd->sc_cred;	/* XXX crdup? */
		if (bp->b_dirtyend == 0) {
			nbp->vb_buf.b_dirtyoff = 0;
			nbp->vb_buf.b_dirtyend = sz;
		} else {
			nbp->vb_buf.b_dirtyoff =
			    max(0, bp->b_dirtyoff - (bp->b_bcount - resid));
			nbp->vb_buf.b_dirtyend =
			    min(sz,
				max(0, bp->b_dirtyend - (bp->b_bcount-resid)));
		}
		if (bp->b_validend == 0) {
			nbp->vb_buf.b_validoff = 0;
			nbp->vb_buf.b_validend = sz;
		} else {
			nbp->vb_buf.b_validoff =
			    max(0, bp->b_validoff - (bp->b_bcount - resid));
			nbp->vb_buf.b_validend =
			    min(sz,
				max(0, bp->b_validend - (bp->b_bcount-resid)));
		}

		nbp->vb_xfer = vnx;

		/*
		 * Just sort by block number
		 */
		nbp->vb_buf.b_cylin = nbp->vb_buf.b_blkno;
		s = splbio();
		vnx->vx_pending++;
		disksort(&vnd->sc_tab, &nbp->vb_buf);
		if (vnd->sc_tab.b_active < vnd->sc_maxactive) {
			vnd->sc_tab.b_active++;
			vndstart(vnd);
		}
		splx(s);
		bn += sz;
		addr += sz;
	}
	return;

 done:
	biodone(bp);
}

/*
 * Feed requests sequentially.
 * We do it this way to keep from flooding NFS servers if we are connected
 * to an NFS file.  This places the burden on the client rather than the
 * server.
 */
void
vndstart(vnd)
	register struct vnd_softc *vnd;
{
	register struct buf *bp;

	/*
	 * Dequeue now since lower level strategy routine might
	 * queue using same links
	 */
	bp = vnd->sc_tab.b_actf;
	vnd->sc_tab.b_actf = bp->b_actf;
#ifdef DEBUG
	if (vnddebug & VDB_IO)
		printf("vndstart(%ld): bp %p vp %p blkno %x addr %p cnt %lx\n",
		    (long) (vnd-vnd_softc), bp, bp->b_vp, bp->b_blkno,
		    bp->b_data, bp->b_bcount);
#endif

	/* Instrumentation. */
	disk_busy(&vnd->sc_dkdev);

	if ((bp->b_flags & B_READ) == 0)
		bp->b_vp->v_numoutput++;
	VOP_STRATEGY(bp);
}

void
vndiodone(bp)
	struct buf *bp;
{
	register struct vndbuf *vbp = (struct vndbuf *) bp;
	register struct vndxfer *vnx = (struct vndxfer *)vbp->vb_xfer;
	register struct buf *pbp = vnx->vx_bp;
	register struct vnd_softc *vnd = &vnd_softc[vndunit(pbp->b_dev)];
	int s, resid;

	s = splbio();
#ifdef DEBUG
	if (vnddebug & VDB_IO)
		printf("vndiodone(%ld): vbp %p vp %p blkno %x addr %p cnt %lx\n",
		    (long) (vnd-vnd_softc), vbp, vbp->vb_buf.b_vp,
		    vbp->vb_buf.b_blkno, vbp->vb_buf.b_data,
		    vbp->vb_buf.b_bcount);
#endif

	resid = vbp->vb_buf.b_bcount - vbp->vb_buf.b_resid;
	pbp->b_resid -= resid;
	disk_unbusy(&vnd->sc_dkdev, resid);
	vnx->vx_pending--;

	if (vbp->vb_buf.b_error) {
#ifdef DEBUG
		if (vnddebug & VDB_IO)
			printf("vndiodone: vbp %p error %d\n", vbp,
			    vbp->vb_buf.b_error);
#endif
		vnx->vx_error = vbp->vb_buf.b_error;
	}
	putvndbuf(vbp);

	/*
	 * Wrap up this transaction if it has run to completion or, in
	 * case of an error, when all auxiliary buffers have returned.
	 */
	if (pbp->b_resid == 0 || (vnx->vx_error && vnx->vx_pending == 0)) {

		if (vnx->vx_error != 0) {
			pbp->b_flags |= B_ERROR;
			pbp->b_error = vnx->vx_error;
		}
		putvndxfer(vnx);
#ifdef DEBUG
		if (vnddebug & VDB_IO)
			printf("vndiodone: pbp %p iodone\n", pbp);
#endif
		biodone(pbp);
	}

	if (vnd->sc_tab.b_actf)
		vndstart(vnd);
	else
		vnd->sc_tab.b_active--;
	splx(s);
}

/* ARGSUSED */
int
vndread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndread(%x, %p)\n", dev, uio);
#endif

	if (unit >= numvnd)
		return (ENXIO);
	sc = &vnd_softc[unit];

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (ENXIO);

	return (physio(vndstrategy, NULL, dev, B_READ, minphys, uio));
}

/* ARGSUSED */
int
vndwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndwrite(%x, %p)\n", dev, uio);
#endif

	if (unit >= numvnd)
		return (ENXIO);
	sc = &vnd_softc[unit];

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (ENXIO);

	return (physio(vndstrategy, NULL, dev, B_WRITE, minphys, uio));
}

/* ARGSUSED */
int
vndioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = vndunit(dev);
	register struct vnd_softc *vnd;
	struct vnd_ioctl *vio;
	struct vattr vattr;
	struct nameidata nd;
	int error, part, pmask;
	size_t geomsize;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndioctl(%x, %lx, %p, %x, %p): unit %d\n",
		    dev, cmd, data, flag, p, unit);
#endif
	error = suser(p->p_ucred, &p->p_acflag);
	if (error)
		return (error);
	if (unit >= numvnd)
		return (ENXIO);

	vnd = &vnd_softc[unit];
	vio = (struct vnd_ioctl *)data;

	/* Must be open for writes for these commands... */
	switch (cmd) {
	case VNDIOCSET:
	case VNDIOCCLR:
	case DIOCSDINFO:
	case DIOCWDINFO:
	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
	}

	/* Must be initialized for these... */
	switch (cmd) {
	case VNDIOCCLR:
	case DIOCGDINFO:
	case DIOCSDINFO:
	case DIOCWDINFO:
	case DIOCGPART:
	case DIOCWLABEL:
		if ((vnd->sc_flags & VNF_INITED) == 0)
			return (ENXIO);
	}

	switch (cmd) {
	case VNDIOCSET:
		if (vnd->sc_flags & VNF_INITED)
			return (EBUSY);

		if ((error = vndlock(vnd)) != 0)
			return (error);

		/*
		 * Always open for read and write.
		 * This is probably bogus, but it lets vn_open()
		 * weed out directories, sockets, etc. so we don't
		 * have to worry about them.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, vio->vnd_file, p);
		if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0) {
			vndunlock(vnd);
			return(error);
		}
		error = VOP_GETATTR(nd.ni_vp, &vattr, p->p_ucred, p);
		if (error) {
			VOP_UNLOCK(nd.ni_vp);
			(void) vn_close(nd.ni_vp, FREAD|FWRITE, p->p_ucred, p);
			vndunlock(vnd);
			return(error);
		}
		VOP_UNLOCK(nd.ni_vp);
		vnd->sc_vp = nd.ni_vp;
		vnd->sc_size = btodb(vattr.va_size);	/* note truncation */

		/*
		 * Use pseudo-geometry specified.  If none was provided,
		 * use "standard" Adaptec fictitious geometry.
		 */
		if (vio->vnd_flags & VNDIOF_HASGEOM) {

			bcopy(&vio->vnd_geom, &vnd->sc_geom,
			    sizeof(vio->vnd_geom));

			/*
			 * Sanity-check the sector size.
			 * XXX Don't allow secsize < DEV_BSIZE.  Should
			 * XXX we?
			 */
			if (vnd->sc_geom.vng_secsize < DEV_BSIZE ||
			    (vnd->sc_geom.vng_secsize % DEV_BSIZE) != 0) {
				(void) vn_close(nd.ni_vp, FREAD|FWRITE,
				    p->p_ucred, p);
				vndunlock(vnd);
				return (EINVAL);
			}

			/*
			 * Compute the size (in DEV_BSIZE blocks) specified
			 * by the geometry.
			 */
			geomsize = (vnd->sc_geom.vng_nsectors *
			    vnd->sc_geom.vng_ntracks *
			    vnd->sc_geom.vng_ncylinders) *
			    (vnd->sc_geom.vng_secsize / DEV_BSIZE);

			/*
			 * Sanity-check the size against the specified
			 * geometry.
			 */
			if (vnd->sc_size < geomsize) {
				(void) vn_close(nd.ni_vp, FREAD|FWRITE,
				    p->p_ucred, p);
				vndunlock(vnd);
				return (EINVAL);
			}
		} else {
			/*
			 * Size must be at least 2048 DEV_BSIZE blocks
			 * (1M) in order to use this geometry.
			 */
			if (vnd->sc_size < (32 * 64))
				return (EINVAL);

			vnd->sc_geom.vng_secsize = DEV_BSIZE;
			vnd->sc_geom.vng_nsectors = 32;
			vnd->sc_geom.vng_ntracks = 64;
			vnd->sc_geom.vng_ncylinders = vnd->sc_size / (64 * 32);

			/*
			 * Compute the actual size allowed by this geometry.
			 */
			geomsize = 32 * 64 * vnd->sc_geom.vng_ncylinders;
		}

		/*
		 * Truncate the size to that specified by
		 * the geometry.
		 * XXX Should we even bother with this?
		 */
		vnd->sc_size = geomsize;

		if ((error = vndsetcred(vnd, p->p_ucred)) != 0) {
			(void) vn_close(nd.ni_vp, FREAD|FWRITE, p->p_ucred, p);
			vndunlock(vnd);
			return(error);
		}
		vndthrottle(vnd, vnd->sc_vp);
		vio->vnd_size = dbtob(vnd->sc_size);
		vnd->sc_flags |= VNF_INITED;
#ifdef DEBUG
		if (vnddebug & VDB_INIT)
			printf("vndioctl: SET vp %p size %lx %d/%d/%d/%d\n",
			    vnd->sc_vp, (unsigned long) vnd->sc_size,
			    vnd->sc_geom.vng_secsize,
			    vnd->sc_geom.vng_nsectors,
			    vnd->sc_geom.vng_ntracks,
			    vnd->sc_geom.vng_ncylinders);
#endif

		/* Attach the disk. */
		bzero(vnd->sc_xname, sizeof(vnd->sc_xname));	/* XXX */
		sprintf(vnd->sc_xname, "vnd%d", unit);		/* XXX */
		vnd->sc_dkdev.dk_name = vnd->sc_xname;
		disk_attach(&vnd->sc_dkdev);

		/* Try and read the disklabel. */
		vndgetdisklabel(dev);

		vndunlock(vnd);

		break;

	case VNDIOCCLR:
		if ((error = vndlock(vnd)) != 0)
			return (error);

		/*
		 * Don't unconfigure if any other partitions are open
		 * or if both the character and block flavors of this
		 * partition are open.
		 */
		part = DISKPART(dev);
		pmask = (1 << part);
		if ((vnd->sc_dkdev.dk_openmask & ~pmask) ||
		    ((vnd->sc_dkdev.dk_bopenmask & pmask) &&
		    (vnd->sc_dkdev.dk_copenmask & pmask))) {
			vndunlock(vnd);
			return (EBUSY);
		}

		vndclear(vnd);
#ifdef DEBUG
		if (vnddebug & VDB_INIT)
			printf("vndioctl: CLRed\n");
#endif

		/* Detatch the disk. */
		disk_detach(&vnd->sc_dkdev);

		vndunlock(vnd);

		break;

	case DIOCGDINFO:
		*(struct disklabel *)data = *(vnd->sc_dkdev.dk_label);
		break;

	case DIOCGPART:
		((struct partinfo *)data)->disklab = vnd->sc_dkdev.dk_label;
		((struct partinfo *)data)->part =
		    &vnd->sc_dkdev.dk_label->d_partitions[DISKPART(dev)];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((error = vndlock(vnd)) != 0)
			return (error);

		vnd->sc_flags |= VNF_LABELLING;

		error = setdisklabel(vnd->sc_dkdev.dk_label,
		    (struct disklabel *)data, 0, vnd->sc_dkdev.dk_cpulabel);
		if (error == 0) {
			if (cmd == DIOCWDINFO)
				error = writedisklabel(VNDLABELDEV(dev),
				    vndstrategy, vnd->sc_dkdev.dk_label,
				    vnd->sc_dkdev.dk_cpulabel);
		}

		vnd->sc_flags &= ~VNF_LABELLING;

		vndunlock(vnd);

		if (error)
			return (error);
		break;

	case DIOCWLABEL:
		if (*(int *)data != 0)
			vnd->sc_flags |= VNF_WLABEL;
		else
			vnd->sc_flags &= ~VNF_WLABEL;
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

/*
 * Duplicate the current processes' credentials.  Since we are called only
 * as the result of a SET ioctl and only root can do that, any future access
 * to this "disk" is essentially as root.  Note that credentials may change
 * if some other uid can write directly to the mapped file (NFS).
 */
int
vndsetcred(vnd, cred)
	register struct vnd_softc *vnd;
	struct ucred *cred;
{
	struct uio auio;
	struct iovec aiov;
	char *tmpbuf;
	int error;

	vnd->sc_cred = crdup(cred);
	tmpbuf = malloc(DEV_BSIZE, M_TEMP, M_WAITOK);

	/* XXX: Horrible kludge to establish credentials for NFS */
	aiov.iov_base = tmpbuf;
	aiov.iov_len = min(DEV_BSIZE, dbtob(vnd->sc_size));
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = aiov.iov_len;
	VOP_LOCK(vnd->sc_vp);
	error = VOP_READ(vnd->sc_vp, &auio, 0, vnd->sc_cred);
	VOP_UNLOCK(vnd->sc_vp);

	free(tmpbuf, M_TEMP);
	return (error);
}

/*
 * Set maxactive based on FS type
 */
void
vndthrottle(vnd, vp)
	register struct vnd_softc *vnd;
	struct vnode *vp;
{
#ifdef NFS
	extern int (**nfsv2_vnodeop_p) __P((void *));

	if (vp->v_op == nfsv2_vnodeop_p)
		vnd->sc_maxactive = 2;
	else
#endif
		vnd->sc_maxactive = 8;

	if (vnd->sc_maxactive < 1)
		vnd->sc_maxactive = 1;
}

void
vndshutdown()
{
	register struct vnd_softc *vnd;

	for (vnd = &vnd_softc[0]; vnd < &vnd_softc[numvnd]; vnd++)
		if (vnd->sc_flags & VNF_INITED)
			vndclear(vnd);
}

void
vndclear(vnd)
	register struct vnd_softc *vnd;
{
	register struct vnode *vp = vnd->sc_vp;
	struct proc *p = curproc;		/* XXX */

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndclear(%p): vp %p\n", vnd, vp);
#endif
	vnd->sc_flags &= ~VNF_INITED;
	if (vp == (struct vnode *)0)
		panic("vndioctl: null vp");
	(void) vn_close(vp, FREAD|FWRITE, vnd->sc_cred, p);
	crfree(vnd->sc_cred);
	vnd->sc_vp = (struct vnode *)0;
	vnd->sc_cred = (struct ucred *)0;
	vnd->sc_size = 0;
}

int
vndsize(dev)
	dev_t dev;
{
	struct vnd_softc *sc;
	struct disklabel *lp;
	int part, unit, omask;
	int size;

	unit = vndunit(dev);
	if (unit >= numvnd)
		return (-1);
	sc = &vnd_softc[unit];

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (-1);

	part = DISKPART(dev);
	omask = sc->sc_dkdev.dk_openmask & (1 << part);
	lp = sc->sc_dkdev.dk_label;

	if (omask == 0 && vndopen(dev, 0, S_IFBLK, curproc))
		return (-1);

	if (lp->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = lp->d_partitions[part].p_size *
		    (lp->d_secsize / DEV_BSIZE);

	if (omask == 0 && vndclose(dev, 0, S_IFBLK, curproc))
		return (-1);

	return (size);
}

int
vnddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return ENXIO;
}

/*
 * Read the disklabel from a vnd.  If one is not present, create a fake one.
 */
void
vndgetdisklabel(dev)
	dev_t dev;
{
	struct vnd_softc *sc = &vnd_softc[vndunit(dev)];
	char *errstring;
	struct disklabel *lp = sc->sc_dkdev.dk_label;
	struct cpu_disklabel *clp = sc->sc_dkdev.dk_cpulabel;
	struct vndgeom *vng = &sc->sc_geom;
	struct partition *pp;
	int i;

	bzero(lp, sizeof(*lp));
	bzero(clp, sizeof(*clp));

	lp->d_secperunit = sc->sc_size / (vng->vng_secsize / DEV_BSIZE);
	lp->d_secsize = vng->vng_secsize;
	lp->d_nsectors = vng->vng_nsectors;
	lp->d_ntracks = vng->vng_ntracks;
	lp->d_ncylinders = vng->vng_ncylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	strncpy(lp->d_typename, "vnd", sizeof(lp->d_typename));
	lp->d_type = DTYPE_VND;
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	pp = &lp->d_partitions[RAW_PART];
	pp->p_offset = 0;
	pp->p_size = lp->d_secperunit;
	pp->p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);

	/*
	 * Call the generic disklabel extraction routine.
	 */
	errstring = readdisklabel(VNDLABELDEV(dev), vndstrategy, lp, clp);
	if (errstring) {
		/*
		 * Lack of disklabel is common, but we print the warning
		 * anyway, since it might contain other useful information.
		 */
		printf("%s: %s\n", sc->sc_xname, errstring);

		/*
		 * For historical reasons, if there's no disklabel
		 * present, all partitions must be FS_BSDFFS and
		 * occupy the entire disk.
		 */
		for (i = 0; i < MAXPARTITIONS; i++) {
			lp->d_partitions[i].p_size = lp->d_secperunit;
			lp->d_partitions[i].p_offset = 0;
			lp->d_partitions[i].p_fstype = FS_BSDFFS;
		}

		strncpy(lp->d_packname, "default label",
		    sizeof(lp->d_packname));
	}
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
static int
vndlock(sc)
	struct vnd_softc *sc;
{
	int error;

	while ((sc->sc_flags & VNF_LOCKED) != 0) {
		sc->sc_flags |= VNF_WANTED;
		if ((error = tsleep(sc, PRIBIO | PCATCH, "vndlck", 0)) != 0)
			return (error);
	}
	sc->sc_flags |= VNF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
static void
vndunlock(sc)
	struct vnd_softc *sc;
{

	sc->sc_flags &= ~VNF_LOCKED;
	if ((sc->sc_flags & VNF_WANTED) != 0) {
		sc->sc_flags &= ~VNF_WANTED;
		wakeup(sc);
	}
}
