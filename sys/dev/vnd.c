/*	$NetBSD: vnd.c,v 1.117 2005/07/18 16:36:29 christos Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)vn.c	8.9 (Berkeley) 5/14/95
 */

/*
 * Copyright (c) 1988 University of Utah.
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
 *	@(#)vn.c	8.9 (Berkeley) 5/14/95
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vnd.c,v 1.117 2005/07/18 16:36:29 christos Exp $");

#if defined(_KERNEL_OPT)
#include "fs_nfs.h"
#include "opt_vnd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/bufq.h>
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
#include <net/zlib.h>

#include <miscfs/specfs/specdev.h>

#include <dev/vndvar.h>

#if defined(VNDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
int dovndcluster = 1;
#define VDB_FOLLOW	0x01
#define VDB_INIT	0x02
#define VDB_IO		0x04
#define VDB_LABEL	0x08
int vnddebug = 0x00;
#endif

#define vndunit(x)	DISKUNIT(x)

struct vndxfer {
	struct buf	*vx_bp;		/* Pointer to parent buffer */
	int		vx_error;
	int		vx_pending;	/* # of pending aux buffers */
	int		vx_flags;
#define VX_BUSY		1
};

struct vndbuf {
	struct buf	vb_buf;
	struct vndxfer	*vb_xfer;
};

#define VND_GETXFER(vnd)	pool_get(&(vnd)->sc_vxpool, PR_WAITOK)
#define VND_PUTXFER(vnd, vx)	pool_put(&(vnd)->sc_vxpool, (vx))

#define VND_GETBUF(vnd)		pool_get(&(vnd)->sc_vbpool, PR_WAITOK)
#define VND_PUTBUF(vnd, vb)	pool_put(&(vnd)->sc_vbpool, (vb))

struct vnd_softc *vnd_softc;
int numvnd = 0;

#define VNDLABELDEV(dev) \
    (MAKEDISKDEV(major((dev)), vndunit((dev)), RAW_PART))

/* called by main() at boot time (XXX: and the LKM driver) */
void	vndattach(int);
int	vnddetach(void);

static void	vndclear(struct vnd_softc *, int);
static int	vndsetcred(struct vnd_softc *, struct ucred *);
static void	vndthrottle(struct vnd_softc *, struct vnode *);
static void	vndiodone(struct buf *);
#if 0
static void	vndshutdown(void);
#endif

static void	vndgetdefaultlabel(struct vnd_softc *, struct disklabel *);
static void	vndgetdisklabel(dev_t);

static int	vndlock(struct vnd_softc *);
static void	vndunlock(struct vnd_softc *);
#ifdef VND_COMPRESSION
static void	compstrategy(struct buf *, off_t);
static void	*vnd_alloc(void *, u_int, u_int);
static void	vnd_free(void *, void *);
#endif /* VND_COMPRESSION */

void vndthread(void *);

static dev_type_open(vndopen);
static dev_type_close(vndclose);
static dev_type_read(vndread);
static dev_type_write(vndwrite);
static dev_type_ioctl(vndioctl);
static dev_type_strategy(vndstrategy);
static dev_type_dump(vnddump);
static dev_type_size(vndsize);

const struct bdevsw vnd_bdevsw = {
	vndopen, vndclose, vndstrategy, vndioctl, vnddump, vndsize, D_DISK
};

const struct cdevsw vnd_cdevsw = {
	vndopen, vndclose, vndread, vndwrite, vndioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static int vndattached;

void
vndattach(int num)
{
	int i;
	char *mem;

	if (vndattached)
		return;
	vndattached = 1;
	if (num <= 0)
		return;
	i = num * sizeof(struct vnd_softc);
	mem = malloc(i, M_DEVBUF, M_NOWAIT|M_ZERO);
	if (mem == NULL) {
		printf("WARNING: no memory for vnode disks\n");
		return;
	}
	vnd_softc = (struct vnd_softc *)mem;
	numvnd = num;

	for (i = 0; i < numvnd; i++) {
		vnd_softc[i].sc_unit = i;
		vnd_softc[i].sc_comp_offsets = NULL;
		vnd_softc[i].sc_comp_buff = NULL;
		vnd_softc[i].sc_comp_decombuf = NULL;
		bufq_alloc(&vnd_softc[i].sc_tab,
		    BUFQ_DISKSORT|BUFQ_SORT_RAWBLOCK);
	}
}

int
vnddetach(void)
{
	int i;

	/* First check we aren't in use. */
	for (i = 0; i < numvnd; i++)
		if (vnd_softc[i].sc_flags & VNF_INITED)
			return (EBUSY);

	for (i = 0; i < numvnd; i++)
		bufq_free(&vnd_softc[i].sc_tab);

	free(vnd_softc, M_DEVBUF);
	vndattached = 0;

	return (0);
}

static int
vndopen(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;
	int error = 0, part, pmask;
	struct disklabel *lp;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndopen(0x%x, 0x%x, 0x%x, %p)\n", dev, flags, mode, p);
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
	 * in-core disklabel.  Only read the disklabel if it is
	 * not realdy valid.
	 */
	if ((sc->sc_flags & (VNF_INITED|VNF_VLABEL)) == VNF_INITED &&
	    sc->sc_dkdev.dk_openmask == 0)
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

static int
vndclose(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;
	int error = 0, part;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndclose(0x%x, 0x%x, 0x%x, %p)\n", dev, flags, mode, p);
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

	if (sc->sc_dkdev.dk_openmask == 0) {
		if ((sc->sc_flags & VNF_KLABEL) == 0)
			sc->sc_flags &= ~VNF_VLABEL;
	}

	vndunlock(sc);
	return (0);
}

/*
 * Qeue the request, and wakeup the kernel thread to handle it.
 */
static void
vndstrategy(struct buf *bp)
{
	int unit = vndunit(bp->b_dev);
	struct vnd_softc *vnd = &vnd_softc[unit];
	struct disklabel *lp = vnd->sc_dkdev.dk_label;
	int s = splbio();

	bp->b_resid = bp->b_bcount;

	if ((vnd->sc_flags & VNF_INITED) == 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	/*
	 * The transfer must be a whole number of blocks.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	/*
	 * check if we're read-only.
	 */
	if ((vnd->sc_flags & VNF_READONLY) && !(bp->b_flags & B_READ)) {
		bp->b_error = EACCES;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	/*
	 * Do bounds checking and adjust transfer.  If there's an error,
	 * the bounds check will flag that for us.
	 */
	if (DISKPART(bp->b_dev) != RAW_PART) {
		if (bounds_check_with_label(&vnd->sc_dkdev,
		    bp, vnd->sc_flags & (VNF_WLABEL|VNF_LABELLING)) <= 0)
			goto done;
	}

	/* If it's a nil transfer, wake up the top half now. */
	if (bp->b_bcount == 0)
		goto done;
#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndstrategy(%p): unit %d\n", bp, unit);
#endif
	BUFQ_PUT(&vnd->sc_tab, bp);
	wakeup(&vnd->sc_tab);
	splx(s);
	return;
done:
	biodone(bp);
	splx(s);
}

void
vndthread(void *arg)
{
	struct vnd_softc *vnd = arg;
	struct buf *bp;
	struct vndxfer *vnx;
	struct mount *mp;
	int s, bsize, resid;
	off_t bn;
	caddr_t addr;
	int sz, flags, error;
	struct disklabel *lp;
	struct partition *pp;

	s = splbio();
	vnd->sc_flags |= VNF_KTHREAD;
	wakeup(&vnd->sc_kthread);

	/*
	 * Dequeue requests, break them into bsize pieces and submit using
	 * VOP_BMAP/VOP_STRATEGY.
	 */
	while ((vnd->sc_flags & VNF_VUNCONF) == 0) {
		bp = BUFQ_GET(&vnd->sc_tab);
		if (bp == NULL) {
			tsleep(&vnd->sc_tab, PRIBIO, "vndbp", 0);
			continue;
		};
		splx(s);

#ifdef DEBUG
		if (vnddebug & VDB_FOLLOW)
			printf("vndthread(%p\n", bp);
#endif
		lp = vnd->sc_dkdev.dk_label;
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
			pp = &vnd->sc_dkdev.dk_label->d_partitions[
			    DISKPART(bp->b_dev)];
			bn += pp->p_offset;
		}

		/* ...and convert to a byte offset within the file. */
		bn *= lp->d_secsize;

		if (vnd->sc_vp->v_mount == NULL) {
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR;
			goto done;
		}
#ifdef VND_COMPRESSION
		/* handle a compressed read */
		if ((bp->b_flags & B_READ) && (vnd->sc_flags & VNF_COMP)) {
			compstrategy(bp, bn);
			goto done;
		}
#endif /* VND_COMPRESSION */
		
		bsize = vnd->sc_vp->v_mount->mnt_stat.f_iosize;
		addr = bp->b_data;
		flags = (bp->b_flags & (B_READ|B_ASYNC)) | B_CALL;

		/*
		 * Allocate a header for this transfer and link it to the
		 * buffer
		 */
		s = splbio();
		vnx = VND_GETXFER(vnd);
		splx(s);
		vnx->vx_flags = VX_BUSY;
		vnx->vx_error = 0;
		vnx->vx_pending = 0;
		vnx->vx_bp = bp;

		if ((flags & B_READ) == 0)
			vn_start_write(vnd->sc_vp, &mp, V_WAIT);

		/*
		 * Feed requests sequentially.
		 * We do it this way to keep from flooding NFS servers if we
		 * are connected to an NFS file.  This places the burden on
		 * the client rather than the server.
		 */
		for (resid = bp->b_resid; resid; resid -= sz) {
			struct vndbuf *nbp;
			struct vnode *vp;
			daddr_t nbn;
			int off, nra;

			nra = 0;
			vn_lock(vnd->sc_vp, LK_EXCLUSIVE | LK_RETRY | LK_CANRECURSE);
			error = VOP_BMAP(vnd->sc_vp, bn / bsize, &vp, &nbn, &nra);
			VOP_UNLOCK(vnd->sc_vp, 0);
	
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
				s = splbio();
				vnx->vx_error = error;
				goto out;
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
#ifdef	DEBUG
			if (vnddebug & VDB_IO)
				printf("vndstrategy: vp %p/%p bn 0x%qx/0x%" PRIx64
				       " sz 0x%x\n",
				    vnd->sc_vp, vp, (long long)bn, nbn, sz);
#endif

			s = splbio();
			while (vnd->sc_active >= vnd->sc_maxactive) {
				tsleep(&vnd->sc_tab, PRIBIO, "vndac", 0);
			}
			vnd->sc_active++;
			nbp = VND_GETBUF(vnd);
			splx(s);
			BUF_INIT(&nbp->vb_buf);
			nbp->vb_buf.b_flags = flags;
			nbp->vb_buf.b_bcount = sz;
			nbp->vb_buf.b_bufsize = round_page((ulong)addr + sz)
			    - trunc_page((ulong) addr);
			nbp->vb_buf.b_error = 0;
			nbp->vb_buf.b_data = addr;
			nbp->vb_buf.b_blkno = nbp->vb_buf.b_rawblkno = nbn + btodb(off);
			nbp->vb_buf.b_proc = bp->b_proc;
			nbp->vb_buf.b_iodone = vndiodone;
			nbp->vb_buf.b_vp = vp;

			nbp->vb_xfer = vnx;
	
			BIO_COPYPRIO(&nbp->vb_buf, bp);

			/*
			 * Just sort by block number
			 */
			s = splbio();
			if (vnx->vx_error != 0) {
				VND_PUTBUF(vnd, nbp);
				goto out;
			}
			vnx->vx_pending++;
#ifdef DEBUG
			if (vnddebug & VDB_IO)
				printf("vndstart(%ld): bp %p vp %p blkno "
				    "0x%" PRIx64 " flags %x addr %p cnt 0x%x\n",
				    (long) (vnd-vnd_softc), &nbp->vb_buf,
				    nbp->vb_buf.b_vp, nbp->vb_buf.b_blkno,
				    nbp->vb_buf.b_flags, nbp->vb_buf.b_data,
				    nbp->vb_buf.b_bcount);
#endif

			/* Instrumentation. */
			disk_busy(&vnd->sc_dkdev);

			if ((nbp->vb_buf.b_flags & B_READ) == 0)
				vp->v_numoutput++;
			VOP_STRATEGY(vp, &nbp->vb_buf);
	
			splx(s);
			bn += sz;
			addr += sz;
		}

		s = splbio();

out: /* Arrive here at splbio */
		if ((flags & B_READ) == 0)
			vn_finished_write(mp, 0);
		vnx->vx_flags &= ~VX_BUSY;
		if (vnx->vx_pending == 0) {
			if (vnx->vx_error != 0) {
				bp->b_error = vnx->vx_error;
				bp->b_flags |= B_ERROR;
			}
			VND_PUTXFER(vnd, vnx);
			biodone(bp);
		}
		continue;
done:
		biodone(bp);
		s = splbio();
	}

	vnd->sc_flags &= (~VNF_KTHREAD | VNF_VUNCONF);
	wakeup(&vnd->sc_kthread);
	splx(s);
	kthread_exit(0);
}


static void
vndiodone(struct buf *bp)
{
	struct vndbuf *vbp = (struct vndbuf *) bp;
	struct vndxfer *vnx = (struct vndxfer *)vbp->vb_xfer;
	struct buf *pbp = vnx->vx_bp;
	struct vnd_softc *vnd = &vnd_softc[vndunit(pbp->b_dev)];
	int s, resid;

	s = splbio();
#ifdef DEBUG
	if (vnddebug & VDB_IO)
		printf("vndiodone(%ld): vbp %p vp %p blkno 0x%" PRIx64
		       " addr %p cnt 0x%x\n",
		    (long) (vnd-vnd_softc), vbp, vbp->vb_buf.b_vp,
		    vbp->vb_buf.b_blkno, vbp->vb_buf.b_data,
		    vbp->vb_buf.b_bcount);
#endif

	resid = vbp->vb_buf.b_bcount - vbp->vb_buf.b_resid;
	pbp->b_resid -= resid;
	disk_unbusy(&vnd->sc_dkdev, resid, (pbp->b_flags & B_READ));
	vnx->vx_pending--;

	if (vbp->vb_buf.b_error) {
#ifdef DEBUG
		if (vnddebug & VDB_IO)
			printf("vndiodone: vbp %p error %d\n", vbp,
			    vbp->vb_buf.b_error);
#endif
		vnx->vx_error = vbp->vb_buf.b_error;
	}

	VND_PUTBUF(vnd, vbp);

	/*
	 * Wrap up this transaction if it has run to completion or, in
	 * case of an error, when all auxiliary buffers have returned.
	 */
	if (vnx->vx_error != 0) {
		pbp->b_flags |= B_ERROR;
		pbp->b_error = vnx->vx_error;
		if ((vnx->vx_flags & VX_BUSY) == 0 && vnx->vx_pending == 0) {

#ifdef DEBUG
			if (vnddebug & VDB_IO)
				printf("vndiodone: pbp %p iodone: error %d\n",
					pbp, vnx->vx_error);
#endif
			VND_PUTXFER(vnd, vnx);
			biodone(pbp);
		}
	} else if (pbp->b_resid == 0) {

#ifdef DIAGNOSTIC
		if (vnx->vx_pending != 0)
			panic("vndiodone: vnx pending: %d", vnx->vx_pending);
#endif

		if ((vnx->vx_flags & VX_BUSY) == 0) {
#ifdef DEBUG
			if (vnddebug & VDB_IO)
				printf("vndiodone: pbp %p iodone\n", pbp);
#endif
			VND_PUTXFER(vnd, vnx);
			biodone(pbp);
		}
	}

	vnd->sc_active--;
	wakeup(&vnd->sc_tab);
	splx(s);
}

/* ARGSUSED */
static int
vndread(dev_t dev, struct uio *uio, int flags)
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndread(0x%x, %p)\n", dev, uio);
#endif

	if (unit >= numvnd)
		return (ENXIO);
	sc = &vnd_softc[unit];

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (ENXIO);

	return (physio(vndstrategy, NULL, dev, B_READ, minphys, uio));
}

/* ARGSUSED */
static int
vndwrite(dev_t dev, struct uio *uio, int flags)
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndwrite(0x%x, %p)\n", dev, uio);
#endif

	if (unit >= numvnd)
		return (ENXIO);
	sc = &vnd_softc[unit];

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (ENXIO);

	return (physio(vndstrategy, NULL, dev, B_WRITE, minphys, uio));
}

/* ARGSUSED */
static int
vndioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int unit = vndunit(dev);
	struct vnd_softc *vnd;
	struct vnd_ioctl *vio;
	struct vattr vattr;
	struct nameidata nd;
	int error, part, pmask;
	size_t geomsize;
	int fflags;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndioctl(0x%x, 0x%lx, %p, 0x%x, %p): unit %d\n",
		    dev, cmd, data, flag, p, unit);
#endif
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
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCSDINFO:
	case ODIOCWDINFO:
#endif
	case DIOCKLABEL:
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
	case DIOCKLABEL:
	case DIOCWLABEL:
	case DIOCGDEFLABEL:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
	case ODIOCSDINFO:
	case ODIOCWDINFO:
	case ODIOCGDEFLABEL:
#endif
		if ((vnd->sc_flags & VNF_INITED) == 0)
			return (ENXIO);
	}

	switch (cmd) {
	case VNDIOCSET:
		if (vnd->sc_flags & VNF_INITED)
			return (EBUSY);

		if ((error = vndlock(vnd)) != 0)
			return (error);

		fflags = FREAD;
		if ((vio->vnd_flags & VNDIOF_READONLY) == 0)
			fflags |= FWRITE;
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, vio->vnd_file, p);
		if ((error = vn_open(&nd, fflags, 0)) != 0)
			goto unlock_and_exit;
		error = VOP_GETATTR(nd.ni_vp, &vattr, p->p_ucred, p);
		if (!error && nd.ni_vp->v_type != VREG)
			error = EOPNOTSUPP;
		if (error) {
			VOP_UNLOCK(nd.ni_vp, 0);
			goto close_and_exit;
		}

		/* If using a compressed file, initialize its info */
		/* (or abort with an error if kernel has no compression) */
		if (vio->vnd_flags & VNF_COMP) {
#ifdef VND_COMPRESSION
			struct vnd_comp_header *ch;
			int i;
			u_int32_t comp_size;
			u_int32_t comp_maxsize;
 
			/* allocate space for compresed file header */
			ch = malloc(sizeof(struct vnd_comp_header),
			M_TEMP, M_WAITOK);
 
			/* read compressed file header */
			error = vn_rdwr(UIO_READ, nd.ni_vp, (caddr_t)ch,
			  sizeof(struct vnd_comp_header), 0, UIO_SYSSPACE,
			  IO_UNIT|IO_NODELOCKED, p->p_ucred, NULL, NULL);
			if(error) {
				free(ch, M_TEMP);
				VOP_UNLOCK(nd.ni_vp, 0);
				goto close_and_exit;
			}
 
			/* save some header info */
			vnd->sc_comp_blksz = ntohl(ch->block_size);
			/* note last offset is the file byte size */
			vnd->sc_comp_numoffs = ntohl(ch->num_blocks)+1;
			free(ch, M_TEMP);
			if(vnd->sc_comp_blksz % DEV_BSIZE !=0) {
				VOP_UNLOCK(nd.ni_vp, 0);
				error = EINVAL;
				goto close_and_exit;
			}
			if(sizeof(struct vnd_comp_header) +
			  sizeof(u_int64_t) * vnd->sc_comp_numoffs >
			  vattr.va_size) {
				VOP_UNLOCK(nd.ni_vp, 0);
				error = EINVAL;
				goto close_and_exit;
			}
 
			/* set decompressed file size */
			vattr.va_size =
			  (vnd->sc_comp_numoffs - 1) * vnd->sc_comp_blksz;
 
			/* allocate space for all the compressed offsets */
			vnd->sc_comp_offsets =
			malloc(sizeof(u_int64_t) * vnd->sc_comp_numoffs,
			M_DEVBUF, M_WAITOK);
 
			/* read in the offsets */
			error = vn_rdwr(UIO_READ, nd.ni_vp,
			  (caddr_t)vnd->sc_comp_offsets,
			  sizeof(u_int64_t) * vnd->sc_comp_numoffs,
			  sizeof(struct vnd_comp_header), UIO_SYSSPACE,
			  IO_UNIT|IO_NODELOCKED, p->p_ucred, NULL, NULL);
			if(error) {
				VOP_UNLOCK(nd.ni_vp, 0);
				goto close_and_exit;
			}
			/*
			 * find largest block size (used for allocation limit).
			 * Also convert offset to native byte order.
			 */
			comp_maxsize = 0;
			for (i = 0; i < vnd->sc_comp_numoffs - 1; i++) {
				vnd->sc_comp_offsets[i] =
				  be64toh(vnd->sc_comp_offsets[i]);
				comp_size = be64toh(vnd->sc_comp_offsets[i + 1])
				  - vnd->sc_comp_offsets[i];
				if (comp_size > comp_maxsize)
					comp_maxsize = comp_size;
			}
			vnd->sc_comp_offsets[vnd->sc_comp_numoffs - 1] =
			  be64toh(vnd->sc_comp_offsets[vnd->sc_comp_numoffs - 1]);
 
			/* create compressed data buffer */
			vnd->sc_comp_buff = malloc(comp_maxsize,
			  M_DEVBUF, M_WAITOK);
 
			/* create decompressed buffer */
			vnd->sc_comp_decombuf = malloc(vnd->sc_comp_blksz,
			  M_DEVBUF, M_WAITOK);
			vnd->sc_comp_buffblk = -1;
 
			/* Initialize decompress stream */
			bzero(&vnd->sc_comp_stream, sizeof(z_stream));
			vnd->sc_comp_stream.zalloc = vnd_alloc;
			vnd->sc_comp_stream.zfree = vnd_free;
			error = inflateInit2(&vnd->sc_comp_stream, MAX_WBITS);
			if(error) {
				if(vnd->sc_comp_stream.msg)
					printf("vnd%d: compressed file, %s\n",
					  unit, vnd->sc_comp_stream.msg);
				VOP_UNLOCK(nd.ni_vp, 0);
				error = EINVAL;
				goto close_and_exit;
			}
 
			vnd->sc_flags |= VNF_COMP | VNF_READONLY;
#else /* !VND_COMPRESSION */
			error = EOPNOTSUPP;
			goto close_and_exit;
#endif /* VND_COMPRESSION */
		}
 
		VOP_UNLOCK(nd.ni_vp, 0);
		vnd->sc_vp = nd.ni_vp;
		vnd->sc_size = btodb(vattr.va_size);	/* note truncation */

		/*
		 * Use pseudo-geometry specified.  If none was provided,
		 * use "standard" Adaptec fictitious geometry.
		 */
		if (vio->vnd_flags & VNDIOF_HASGEOM) {

			memcpy(&vnd->sc_geom, &vio->vnd_geom,
			    sizeof(vio->vnd_geom));

			/*
			 * Sanity-check the sector size.
			 * XXX Don't allow secsize < DEV_BSIZE.	 Should
			 * XXX we?
			 */
			if (vnd->sc_geom.vng_secsize < DEV_BSIZE ||
			    (vnd->sc_geom.vng_secsize % DEV_BSIZE) != 0 ||
			    vnd->sc_geom.vng_ncylinders == 0 ||
			    (vnd->sc_geom.vng_ntracks *
			     vnd->sc_geom.vng_nsectors) == 0) {
				error = EINVAL;
				goto close_and_exit;
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
				error = EINVAL;
				goto close_and_exit;
			}
		} else {
			/*
			 * Size must be at least 2048 DEV_BSIZE blocks
			 * (1M) in order to use this geometry.
			 */
			if (vnd->sc_size < (32 * 64)) {
				error = EINVAL;
				goto close_and_exit;
			}

			vnd->sc_geom.vng_secsize = DEV_BSIZE;
			vnd->sc_geom.vng_nsectors = 32;
			vnd->sc_geom.vng_ntracks = 64;
			vnd->sc_geom.vng_ncylinders = vnd->sc_size / (64 * 32);
		}

		if (vio->vnd_flags & VNDIOF_READONLY) {
			vnd->sc_flags |= VNF_READONLY;
		}

		if ((error = vndsetcred(vnd, p->p_ucred)) != 0)
			goto close_and_exit;

		memset(vnd->sc_xname, 0, sizeof(vnd->sc_xname)); /* XXX */
		snprintf(vnd->sc_xname, sizeof(vnd->sc_xname), "vnd%d", unit);


		vndthrottle(vnd, vnd->sc_vp);
		vio->vnd_size = dbtob(vnd->sc_size);
		vnd->sc_flags |= VNF_INITED;

		/* create the kernel thread, wait for it to be up */
		error = kthread_create1(vndthread, vnd, &vnd->sc_kthread,
		    vnd->sc_xname);
		if (error)
			goto close_and_exit;
		while ((vnd->sc_flags & VNF_KTHREAD) == 0) {
			tsleep(&vnd->sc_kthread, PRIBIO, "vndthr", 0);
		}
#ifdef DEBUG
		if (vnddebug & VDB_INIT)
			printf("vndioctl: SET vp %p size 0x%lx %d/%d/%d/%d\n",
			    vnd->sc_vp, (unsigned long) vnd->sc_size,
			    vnd->sc_geom.vng_secsize,
			    vnd->sc_geom.vng_nsectors,
			    vnd->sc_geom.vng_ntracks,
			    vnd->sc_geom.vng_ncylinders);
#endif

		/* Attach the disk. */
		vnd->sc_dkdev.dk_name = vnd->sc_xname;
		disk_attach(&vnd->sc_dkdev);

		/* Initialize the xfer and buffer pools. */
		pool_init(&vnd->sc_vxpool, sizeof(struct vndxfer), 0,
		    0, 0, "vndxpl", NULL);
		pool_init(&vnd->sc_vbpool, sizeof(struct vndbuf), 0,
		    0, 0, "vndbpl", NULL);

		/* Try and read the disklabel. */
		vndgetdisklabel(dev);

		vndunlock(vnd);

		break;

close_and_exit:
		(void) vn_close(nd.ni_vp, fflags, p->p_ucred, p);
unlock_and_exit:
#ifdef VND_COMPRESSION
		/* free any allocated memory (for compressed file) */
		if(vnd->sc_comp_offsets) {
			free(vnd->sc_comp_offsets, M_DEVBUF);
			vnd->sc_comp_offsets = NULL;
		}
		if(vnd->sc_comp_buff) {
			free(vnd->sc_comp_buff, M_DEVBUF);
			vnd->sc_comp_buff = NULL;
		}
		if(vnd->sc_comp_decombuf) {
			free(vnd->sc_comp_decombuf, M_DEVBUF);
			vnd->sc_comp_decombuf = NULL;
		}
#endif /* VND_COMPRESSION */
		vndunlock(vnd);
		return (error);

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
		if (((vnd->sc_dkdev.dk_openmask & ~pmask) ||
		    ((vnd->sc_dkdev.dk_bopenmask & pmask) &&
		    (vnd->sc_dkdev.dk_copenmask & pmask))) &&
			!(vio->vnd_flags & VNDIOF_FORCE)) {
			vndunlock(vnd);
			return (EBUSY);
		}

		/*
		 * XXX vndclear() might call vndclose() implicitely;
		 * release lock to avoid recursion
		 */
		vndunlock(vnd);
		vndclear(vnd, minor(dev));
#ifdef DEBUG
		if (vnddebug & VDB_INIT)
			printf("vndioctl: CLRed\n");
#endif

		/* Destroy the xfer and buffer pools. */
		pool_destroy(&vnd->sc_vxpool);
		pool_destroy(&vnd->sc_vbpool);

		/* Detatch the disk. */
		disk_detach(&vnd->sc_dkdev);

		break;

	case VNDIOCGET: {
		struct vnd_user *vnu;
		struct vattr va;

		vnu = (struct vnd_user *)data;

		if (vnu->vnu_unit == -1)
			vnu->vnu_unit = unit;
		if (vnu->vnu_unit >= numvnd)
			return (ENXIO);
		if (vnu->vnu_unit < 0)
			return (EINVAL);

		vnd = &vnd_softc[vnu->vnu_unit];

		if (vnd->sc_flags & VNF_INITED) {
			error = VOP_GETATTR(vnd->sc_vp, &va, p->p_ucred, p);
			if (error)
				return (error);
			vnu->vnu_dev = va.va_fsid;
			vnu->vnu_ino = va.va_fileid;
		}
		else {
			/* unused is not an error */
			vnu->vnu_dev = 0;
			vnu->vnu_ino = 0;
		}

		break;
	}

	case DIOCGDINFO:
		*(struct disklabel *)data = *(vnd->sc_dkdev.dk_label);
		break;

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *(vnd->sc_dkdev.dk_label);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(data, &newlabel, sizeof (struct olddisklabel));
		break;
#endif

	case DIOCGPART:
		((struct partinfo *)data)->disklab = vnd->sc_dkdev.dk_label;
		((struct partinfo *)data)->part =
		    &vnd->sc_dkdev.dk_label->d_partitions[DISKPART(dev)];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:
#endif
	{
		struct disklabel *lp;

		if ((error = vndlock(vnd)) != 0)
			return (error);

		vnd->sc_flags |= VNF_LABELLING;

#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, data, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)data;

		error = setdisklabel(vnd->sc_dkdev.dk_label,
		    lp, 0, vnd->sc_dkdev.dk_cpulabel);
		if (error == 0) {
			if (cmd == DIOCWDINFO
#ifdef __HAVE_OLD_DISKLABEL
			    || cmd == ODIOCWDINFO
#endif
			   )
				error = writedisklabel(VNDLABELDEV(dev),
				    vndstrategy, vnd->sc_dkdev.dk_label,
				    vnd->sc_dkdev.dk_cpulabel);
		}

		vnd->sc_flags &= ~VNF_LABELLING;

		vndunlock(vnd);

		if (error)
			return (error);
		break;
	}

	case DIOCKLABEL:
		if (*(int *)data != 0)
			vnd->sc_flags |= VNF_KLABEL;
		else
			vnd->sc_flags &= ~VNF_KLABEL;
		break;

	case DIOCWLABEL:
		if (*(int *)data != 0)
			vnd->sc_flags |= VNF_WLABEL;
		else
			vnd->sc_flags &= ~VNF_WLABEL;
		break;

	case DIOCGDEFLABEL:
		vndgetdefaultlabel(vnd, (struct disklabel *)data);
		break;

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		vndgetdefaultlabel(vnd, &newlabel);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(data, &newlabel, sizeof (struct olddisklabel));
		break;
#endif

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
static int
vndsetcred(struct vnd_softc *vnd, struct ucred *cred)
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
	vn_lock(vnd->sc_vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_READ(vnd->sc_vp, &auio, 0, vnd->sc_cred);
	if (error == 0) {
		/*
		 * Because vnd does all IO directly through the vnode
		 * we need to flush (at least) the buffer from the above
		 * VOP_READ from the buffer cache to prevent cache
		 * incoherencies.  Also, be careful to write dirty
		 * buffers back to stable storage.
		 */
		error = vinvalbuf(vnd->sc_vp, V_SAVE, vnd->sc_cred,
			    curproc, 0, 0);
	}
	VOP_UNLOCK(vnd->sc_vp, 0);

	free(tmpbuf, M_TEMP);
	return (error);
}

/*
 * Set maxactive based on FS type
 */
static void
vndthrottle(struct vnd_softc *vnd, struct vnode *vp)
{
#ifdef NFS
	extern int (**nfsv2_vnodeop_p)(void *);

	if (vp->v_op == nfsv2_vnodeop_p)
		vnd->sc_maxactive = 2;
	else
#endif
		vnd->sc_maxactive = 8;

	if (vnd->sc_maxactive < 1)
		vnd->sc_maxactive = 1;
}

#if 0
static void
vndshutdown(void)
{
	struct vnd_softc *vnd;

	for (vnd = &vnd_softc[0]; vnd < &vnd_softc[numvnd]; vnd++)
		if (vnd->sc_flags & VNF_INITED)
			vndclear(vnd);
}
#endif

static void
vndclear(struct vnd_softc *vnd, int myminor)
{
	struct vnode *vp = vnd->sc_vp;
	struct proc *p = curproc;		/* XXX */
	int fflags = FREAD;
	int bmaj, cmaj, i, mn;
	int s;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndclear(%p): vp %p\n", vnd, vp);
#endif
	/* locate the major number */
	bmaj = bdevsw_lookup_major(&vnd_bdevsw);
	cmaj = cdevsw_lookup_major(&vnd_cdevsw);

	/* Nuke the vnodes for any open instances */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = DISKMINOR(vnd->sc_unit, i);
		vdevgone(bmaj, mn, mn, VBLK);
		if (mn != myminor) /* XXX avoid to kill own vnode */
			vdevgone(cmaj, mn, mn, VCHR);
	}

	if ((vnd->sc_flags & VNF_READONLY) == 0)
		fflags |= FWRITE;

	s = splbio();
	bufq_drain(&vnd->sc_tab);
	splx(s);

	vnd->sc_flags |= VNF_VUNCONF;
	wakeup(&vnd->sc_tab);
	while (vnd->sc_flags & VNF_KTHREAD)
		tsleep(&vnd->sc_kthread, PRIBIO, "vnthr", 0);

#ifdef VND_COMPRESSION
	/* free the compressed file buffers */
	if(vnd->sc_flags & VNF_COMP) {
		if(vnd->sc_comp_offsets) {
			free(vnd->sc_comp_offsets, M_DEVBUF);
			vnd->sc_comp_offsets = NULL;
		}
		if(vnd->sc_comp_buff) {
			free(vnd->sc_comp_buff, M_DEVBUF);
			vnd->sc_comp_buff = NULL;
		}
		if(vnd->sc_comp_decombuf) {
			free(vnd->sc_comp_decombuf, M_DEVBUF);
			vnd->sc_comp_decombuf = NULL;
		}
	}
#endif /* VND_COMPRESSION */
	vnd->sc_flags &=
	    ~(VNF_INITED | VNF_READONLY | VNF_VLABEL
	      | VNF_VUNCONF | VNF_COMP);
	if (vp == (struct vnode *)0)
		panic("vndclear: null vp");
	(void) vn_close(vp, fflags, vnd->sc_cred, p);
	crfree(vnd->sc_cred);
	vnd->sc_vp = (struct vnode *)0;
	vnd->sc_cred = (struct ucred *)0;
	vnd->sc_size = 0;
}

static int
vndsize(dev_t dev)
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

static int
vnddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{

	/* Not implemented. */
	return ENXIO;
}

static void
vndgetdefaultlabel(struct vnd_softc *sc, struct disklabel *lp)
{
	struct vndgeom *vng = &sc->sc_geom;
	struct partition *pp;

	memset(lp, 0, sizeof(*lp));

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
}

/*
 * Read the disklabel from a vnd.  If one is not present, create a fake one.
 */
static void
vndgetdisklabel(dev_t dev)
{
	struct vnd_softc *sc = &vnd_softc[vndunit(dev)];
	const char *errstring;
	struct disklabel *lp = sc->sc_dkdev.dk_label;
	struct cpu_disklabel *clp = sc->sc_dkdev.dk_cpulabel;
	int i;

	memset(clp, 0, sizeof(*clp));

	vndgetdefaultlabel(sc, lp);

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
			/*
			 * Don't wipe out port specific hack (such as
			 * dos partition hack of i386 port).
			 */
			if (lp->d_partitions[i].p_size != 0)
				continue;

			lp->d_partitions[i].p_size = lp->d_secperunit;
			lp->d_partitions[i].p_offset = 0;
			lp->d_partitions[i].p_fstype = FS_BSDFFS;
		}

		strncpy(lp->d_packname, "default label",
		    sizeof(lp->d_packname));

		lp->d_npartitions = MAXPARTITIONS;
		lp->d_checksum = dkcksum(lp);
	}

	/* In-core label now valid. */
	sc->sc_flags |= VNF_VLABEL;
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
static int
vndlock(struct vnd_softc *sc)
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
vndunlock(struct vnd_softc *sc)
{

	sc->sc_flags &= ~VNF_LOCKED;
	if ((sc->sc_flags & VNF_WANTED) != 0) {
		sc->sc_flags &= ~VNF_WANTED;
		wakeup(sc);
	}
}

#ifdef VND_COMPRESSION
/* compressed file read */
static void
compstrategy(struct buf *bp, off_t bn)
{
	int error;
	int unit = vndunit(bp->b_dev);
	struct vnd_softc *vnd = &vnd_softc[unit];
	u_int32_t comp_block;
	struct uio auio;
	caddr_t addr;
	int s;

	/* set up constants for data move */
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = bp->b_flags & B_PHYS ? UIO_USERSPACE : UIO_SYSSPACE;
	auio.uio_procp = bp->b_proc;

	/* read, and transfer the data */
	addr = bp->b_data;
	s = splbio();
	while (bp->b_resid > 0) {
		unsigned length;
		size_t length_in_buffer;
		u_int32_t offset_in_buffer;
		struct iovec aiov;

		/* calculate the compressed block number */
		comp_block = bn / (off_t)vnd->sc_comp_blksz;

		/* check for good block number */
		if (comp_block >= vnd->sc_comp_numoffs) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			splx(s);
			return;
		}

		/* read in the compressed block, if not in buffer */
		if (comp_block != vnd->sc_comp_buffblk) {
			length = vnd->sc_comp_offsets[comp_block + 1] -
			    vnd->sc_comp_offsets[comp_block];
			vn_lock(vnd->sc_vp, LK_EXCLUSIVE | LK_RETRY);
			error = vn_rdwr(UIO_READ, vnd->sc_vp, vnd->sc_comp_buff,
			    length, vnd->sc_comp_offsets[comp_block],
			    UIO_SYSSPACE, IO_UNIT, vnd->sc_cred, NULL, NULL);
			if (error) {
				bp->b_error = error;
				bp->b_flags |= B_ERROR;
				VOP_UNLOCK(vnd->sc_vp, 0);
				splx(s);
				return;
			}
			/* uncompress the buffer */
			vnd->sc_comp_stream.next_in = vnd->sc_comp_buff;
			vnd->sc_comp_stream.avail_in = length;
			vnd->sc_comp_stream.next_out = vnd->sc_comp_decombuf;
			vnd->sc_comp_stream.avail_out = vnd->sc_comp_blksz;
			inflateReset(&vnd->sc_comp_stream);
			error = inflate(&vnd->sc_comp_stream, Z_FINISH);
			if (error != Z_STREAM_END) {
				if (vnd->sc_comp_stream.msg)
					printf("%s: compressed file, %s\n",
					    vnd->sc_xname,
					    vnd->sc_comp_stream.msg);
				bp->b_error = EBADMSG;
				bp->b_flags |= B_ERROR;
				VOP_UNLOCK(vnd->sc_vp, 0);
				splx(s);
				return;
			}
			vnd->sc_comp_buffblk = comp_block;
			VOP_UNLOCK(vnd->sc_vp, 0);
		}

		/* transfer the usable uncompressed data */
		offset_in_buffer = bn % (off_t)vnd->sc_comp_blksz;
		length_in_buffer = vnd->sc_comp_blksz - offset_in_buffer;
		if (length_in_buffer > bp->b_resid)
			length_in_buffer = bp->b_resid;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		aiov.iov_base = addr;
		aiov.iov_len = length_in_buffer;
		auio.uio_resid = aiov.iov_len;
		auio.uio_offset = 0;
		error = uiomove(vnd->sc_comp_decombuf + offset_in_buffer,
		    length_in_buffer, &auio);
		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
			splx(s);
			return;
		}

		bn += length_in_buffer;
		addr += length_in_buffer;
		bp->b_resid -= length_in_buffer;
	}
	splx(s);
}

/* compression memory allocation routines */
static void *
vnd_alloc(void *aux, u_int items, u_int siz)
{
	return malloc(items * siz, M_TEMP, M_NOWAIT);
}

static void
vnd_free(void *aux, void *ptr)
{
	free(ptr, M_TEMP);
}
#endif /* VND_COMPRESSION */
