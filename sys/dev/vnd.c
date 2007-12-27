/*	$NetBSD: vnd.c,v 1.170.4.2 2007/12/27 00:44:47 mjf Exp $	*/

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
 * NOTE 1: If the vnode supports the VOP_BMAP and VOP_STRATEGY operations,
 * this uses them to avoid distorting the local buffer cache.  If those
 * block-level operations are not available, this falls back to the regular
 * read and write calls.  Using these may distort the cache in some cases
 * but better have the driver working than preventing it to work on file
 * systems where the block-level operations are not implemented for
 * whatever reason.
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
__KERNEL_RCSID(0, "$NetBSD: vnd.c,v 1.170.4.2 2007/12/27 00:44:47 mjf Exp $");

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
#include <sys/kauth.h>

#include <net/zlib.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <dev/vndvar.h>

#include <prop/proplib.h>

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
	struct buf vx_buf;
	struct vnd_softc *vx_vnd;
};
#define	VND_BUFTOXFER(bp)	((struct vndxfer *)(void *)bp)

#define VND_GETXFER(vnd)	pool_get(&(vnd)->sc_vxpool, PR_WAITOK)
#define VND_PUTXFER(vnd, vx)	pool_put(&(vnd)->sc_vxpool, (vx))

#define VNDLABELDEV(dev) \
    (MAKEDISKDEV(major((dev)), vndunit((dev)), RAW_PART))

/* called by main() at boot time (XXX: and the LKM driver) */
void	vndattach(int);

static void	vndclear(struct vnd_softc *, int);
static int	vndsetcred(struct vnd_softc *, kauth_cred_t);
static void	vndthrottle(struct vnd_softc *, struct vnode *);
static void	vndiodone(struct buf *);
#if 0
static void	vndshutdown(void);
#endif

static void	vndgetdefaultlabel(struct vnd_softc *, struct disklabel *);
static void	vndgetdisklabel(dev_t, struct vnd_softc *);

static int	vndlock(struct vnd_softc *);
static void	vndunlock(struct vnd_softc *);
#ifdef VND_COMPRESSION
static void	compstrategy(struct buf *, off_t);
static void	*vnd_alloc(void *, u_int, u_int);
static void	vnd_free(void *, void *);
#endif /* VND_COMPRESSION */

static void	vndthread(void *);
static bool	vnode_has_op(const struct vnode *, int);
static void	handle_with_rdwr(struct vnd_softc *, const struct buf *,
		    struct buf *);
static void	handle_with_strategy(struct vnd_softc *, const struct buf *,
		    struct buf *);
static void	vnd_set_properties(struct vnd_softc *);

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

static int	vnd_match(struct device *, struct cfdata *, void *);
static void	vnd_attach(struct device *, struct device *, void *);
static int	vnd_detach(struct device *, int);

CFATTACH_DECL(vnd, sizeof(struct vnd_softc),
    vnd_match, vnd_attach, vnd_detach, NULL);
extern struct cfdriver vnd_cd;

static struct vnd_softc	*vnd_spawn(int);
int	vnd_destroy(struct device *);

void
vndattach(int num)
{
	int error;

	error = config_cfattach_attach(vnd_cd.cd_name, &vnd_ca);
	if (error)
		aprint_error("%s: unable to register cfattach\n",
		    vnd_cd.cd_name);
}

static int
vnd_match(struct device *self, struct cfdata *cfdata,
    void *aux)
{
	return 1;
}

static void
vnd_attach(struct device *parent, struct device *self,
    void *aux)
{
	struct vnd_softc *sc = (struct vnd_softc *)self;

	sc->sc_comp_offsets = NULL;
	sc->sc_comp_buff = NULL;
	sc->sc_comp_decombuf = NULL;
	bufq_alloc(&sc->sc_tab, "disksort", BUFQ_SORT_RAWBLOCK);
	disk_init(&sc->sc_dkdev, self->dv_xname, NULL);
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
vnd_detach(struct device *self, int flags)
{
	struct vnd_softc *sc = (struct vnd_softc *)self;
	if (sc->sc_flags & VNF_INITED)
		return EBUSY;

	pmf_device_deregister(self);
	bufq_free(sc->sc_tab);
	disk_destroy(&sc->sc_dkdev);

	return 0;
}

static struct vnd_softc *
vnd_spawn(int unit)
{
	struct cfdata *cf;

	cf = malloc(sizeof(*cf), M_DEVBUF, M_WAITOK);
	cf->cf_name = vnd_cd.cd_name;
	cf->cf_atname = vnd_cd.cd_name;
	cf->cf_unit = unit;
	cf->cf_fstate = FSTATE_STAR;

	return (struct vnd_softc *)config_attach_pseudo(cf);
}

int
vnd_destroy(struct device *dev)
{
	int error;
	struct cfdata *cf;

	cf = device_cfdata(dev);
	error = config_detach(dev, DETACH_QUIET);
	if (error)
		return error;
	free(cf, M_DEVBUF);
	return 0;
}

static int
vndopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;
	int error = 0, part, pmask;
	struct disklabel *lp;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndopen(0x%x, 0x%x, 0x%x, %p)\n", dev, flags, mode, l);
#endif
	sc = device_lookup(&vnd_cd, unit);
	if (sc == NULL) {
		sc = vnd_spawn(unit);
		if (sc == NULL)
			return ENOMEM;
	}

	if ((error = vndlock(sc)) != 0)
		return (error);

	lp = sc->sc_dkdev.dk_label;

	part = DISKPART(dev);
	pmask = (1 << part);

	/*
	 * If we're initialized, check to see if there are any other
	 * open partitions.  If not, then it's safe to update the
	 * in-core disklabel.  Only read the disklabel if it is
	 * not already valid.
	 */
	if ((sc->sc_flags & (VNF_INITED|VNF_VLABEL)) == VNF_INITED &&
	    sc->sc_dkdev.dk_openmask == 0)
		vndgetdisklabel(dev, sc);

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
vndclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = vndunit(dev);
	struct vnd_softc *sc;
	int error = 0, part;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndclose(0x%x, 0x%x, 0x%x, %p)\n", dev, flags, mode, l);
#endif
	sc = device_lookup(&vnd_cd, unit);
	if (sc == NULL)
		return ENXIO;

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

	if ((sc->sc_flags & VNF_INITED) == 0) {
		if ((error = vnd_destroy((struct device *)sc)) != 0) {
			aprint_error("%s: unable to detach instance\n",
			    sc->sc_dev.dv_xname);
			return error;
		}
	}

	return (0);
}

/*
 * Queue the request, and wakeup the kernel thread to handle it.
 */
static void
vndstrategy(struct buf *bp)
{
	int unit = vndunit(bp->b_dev);
	struct vnd_softc *vnd =
	    (struct vnd_softc *)device_lookup(&vnd_cd, unit);
	struct disklabel *lp = vnd->sc_dkdev.dk_label;
	daddr_t blkno;
	int s = splbio();

	if ((vnd->sc_flags & VNF_INITED) == 0) {
		bp->b_error = ENXIO;
		goto done;
	}

	/*
	 * The transfer must be a whole number of blocks.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0) {
		bp->b_error = EINVAL;
		goto done;
	}

	/*
	 * check if we're read-only.
	 */
	if ((vnd->sc_flags & VNF_READONLY) && !(bp->b_flags & B_READ)) {
		bp->b_error = EACCES;
		goto done;
	}

	/* If it's a nil transfer, wake up the top half now. */
	if (bp->b_bcount == 0) {
		goto done;
	}

	/*
	 * Do bounds checking and adjust transfer.  If there's an error,
	 * the bounds check will flag that for us.
	 */
	if (DISKPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    vnd->sc_size) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&vnd->sc_dkdev,
		    bp, vnd->sc_flags & (VNF_WLABEL|VNF_LABELLING)) <= 0)
			goto done;
	}

	/*
	 * Put the block number in terms of the logical blocksize
	 * of the "device".
	 */

	blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);

	/*
	 * Translate the partition-relative block number to an absolute.
	 */
	if (DISKPART(bp->b_dev) != RAW_PART) {
		struct partition *pp;

		pp = &vnd->sc_dkdev.dk_label->d_partitions[
		    DISKPART(bp->b_dev)];
		blkno += pp->p_offset;
	}
	bp->b_rawblkno = blkno;

#ifdef DEBUG
	if (vnddebug & VDB_FOLLOW)
		printf("vndstrategy(%p): unit %d\n", bp, unit);
#endif
	BUFQ_PUT(vnd->sc_tab, bp);
	wakeup(&vnd->sc_tab);
	splx(s);
	return;

done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	splx(s);
}

static void
vndthread(void *arg)
{
	struct vnd_softc *vnd = arg;
	bool usestrategy;
	int s;

	/* Determine whether we can use VOP_BMAP and VOP_STRATEGY to
	 * directly access the backing vnode.  If we can, use these two
	 * operations to avoid messing with the local buffer cache.
	 * Otherwise fall back to regular VOP_READ/VOP_WRITE operations
	 * which are guaranteed to work with any file system. */
	usestrategy = vnode_has_op(vnd->sc_vp, VOFFSET(vop_bmap)) &&
	    vnode_has_op(vnd->sc_vp, VOFFSET(vop_strategy));

#ifdef DEBUG
	if (vnddebug & VDB_INIT)
		printf("vndthread: vp %p, %s\n", vnd->sc_vp,
		    usestrategy ?
		    "using bmap/strategy operations" :
		    "using read/write operations");
#endif

	s = splbio();
	vnd->sc_flags |= VNF_KTHREAD;
	wakeup(&vnd->sc_kthread);

	/*
	 * Dequeue requests and serve them depending on the available
	 * vnode operations.
	 */
	while ((vnd->sc_flags & VNF_VUNCONF) == 0) {
		struct vndxfer *vnx;
		int flags;
		struct buf *obp;
		struct buf *bp;

		obp = BUFQ_GET(vnd->sc_tab);
		if (obp == NULL) {
			tsleep(&vnd->sc_tab, PRIBIO, "vndbp", 0);
			continue;
		};
		splx(s);
		flags = obp->b_flags;
#ifdef DEBUG
		if (vnddebug & VDB_FOLLOW)
			printf("vndthread(%p\n", obp);
#endif

		if (vnd->sc_vp->v_mount == NULL) {
			obp->b_error = ENXIO;
			goto done;
		}
#ifdef VND_COMPRESSION
		/* handle a compressed read */
		if ((flags & B_READ) != 0 && (vnd->sc_flags & VNF_COMP)) {
			off_t bn;
			
			/* Convert to a byte offset within the file. */
			bn = obp->b_rawblkno *
			    vnd->sc_dkdev.dk_label->d_secsize;

			compstrategy(obp, bn);
			goto done;
		}
#endif /* VND_COMPRESSION */
		
		/*
		 * Allocate a header for this transfer and link it to the
		 * buffer
		 */
		s = splbio();
		vnx = VND_GETXFER(vnd);
		splx(s);
		vnx->vx_vnd = vnd;

		s = splbio();
		while (vnd->sc_active >= vnd->sc_maxactive) {
			tsleep(&vnd->sc_tab, PRIBIO, "vndac", 0);
		}
		vnd->sc_active++;
		splx(s);

		/* Instrumentation. */
		disk_busy(&vnd->sc_dkdev);

		bp = &vnx->vx_buf;
		BUF_INIT(bp);
		bp->b_flags = (obp->b_flags & B_READ) | B_CALL;
		bp->b_iodone = vndiodone;
		bp->b_private = obp;
		bp->b_vp = vnd->sc_vp;
		bp->b_data = obp->b_data;
		bp->b_bcount = obp->b_bcount;
		BIO_COPYPRIO(bp, obp);

		/* Handle the request using the appropriate operations. */
		if (usestrategy)
			handle_with_strategy(vnd, obp, bp);
		else
			handle_with_rdwr(vnd, obp, bp);

		s = splbio();
		continue;

done:
		biodone(obp);
		s = splbio();
	}

	vnd->sc_flags &= (~VNF_KTHREAD | VNF_VUNCONF);
	wakeup(&vnd->sc_kthread);
	splx(s);
	kthread_exit(0);
}

/*
 * Checks if the given vnode supports the requested operation.
 * The operation is specified the offset returned by VOFFSET.
 *
 * XXX The test below used to determine this is quite fragile
 * because it relies on the file system to use genfs to specify
 * unimplemented operations.  There might be another way to do
 * it more cleanly.
 */
static bool
vnode_has_op(const struct vnode *vp, int opoffset)
{
	int (*defaultp)(void *);
	int (*opp)(void *);

	defaultp = vp->v_op[VOFFSET(vop_default)];
	opp = vp->v_op[opoffset];

	return opp != defaultp && opp != genfs_eopnotsupp &&
	    opp != genfs_badop && opp != genfs_nullop;
}

/*
 * Handes the read/write request given in 'bp' using the vnode's VOP_READ
 * and VOP_WRITE operations.
 *
 * 'obp' is a pointer to the original request fed to the vnd device.
 */
static void
handle_with_rdwr(struct vnd_softc *vnd, const struct buf *obp, struct buf *bp)
{
	bool doread;
	off_t offset;
	size_t resid;
	struct vnode *vp;

	doread = bp->b_flags & B_READ;
	offset = obp->b_rawblkno * vnd->sc_dkdev.dk_label->d_secsize;
	vp = vnd->sc_vp;

#if defined(DEBUG)
	if (vnddebug & VDB_IO)
		printf("vnd (rdwr): vp %p, %s, rawblkno 0x%" PRIx64
		    ", secsize %d, offset %" PRIu64
		    ", bcount %d\n",
		    vp, doread ? "read" : "write", obp->b_rawblkno,
		    vnd->sc_dkdev.dk_label->d_secsize, offset,
		    bp->b_bcount);
#endif

	/* Issue the read or write operation. */
	bp->b_error =
	    vn_rdwr(doread ? UIO_READ : UIO_WRITE,
	    vp, bp->b_data, bp->b_bcount, offset,
	    UIO_SYSSPACE, 0, vnd->sc_cred, &resid, NULL);
	bp->b_resid = resid;

	/* We need to increase the number of outputs on the vnode if
	 * there was any write to it. */
	if (!doread)
		V_INCR_NUMOUTPUT(vp);

	biodone(bp);
}

/*
 * Handes the read/write request given in 'bp' using the vnode's VOP_BMAP
 * and VOP_STRATEGY operations.
 *
 * 'obp' is a pointer to the original request fed to the vnd device.
 */
static void
handle_with_strategy(struct vnd_softc *vnd, const struct buf *obp,
    struct buf *bp)
{
	int bsize, error, flags, skipped;
	size_t resid, sz;
	off_t bn, offset;

	flags = obp->b_flags;

	if (!(flags & B_READ)) {
		int s;
		
		s = splbio();
		V_INCR_NUMOUTPUT(bp->b_vp);
		splx(s);
	}

	/* convert to a byte offset within the file. */
	bn = obp->b_rawblkno * vnd->sc_dkdev.dk_label->d_secsize;

	bsize = vnd->sc_vp->v_mount->mnt_stat.f_iosize;
	skipped = 0;

	/*
	 * Break the request into bsize pieces and feed them
	 * sequentially using VOP_BMAP/VOP_STRATEGY.
	 * We do it this way to keep from flooding NFS servers if we
	 * are connected to an NFS file.  This places the burden on
	 * the client rather than the server.
	 */
	error = 0;
	bp->b_resid = bp->b_bcount;
	for (offset = 0, resid = bp->b_resid; resid;
	    resid -= sz, offset += sz) {
		struct buf *nbp;
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
			skipped += resid;
			break;
		}

#ifdef DEBUG
		if (!dovndcluster)
			nra = 0;
#endif

		off = bn % bsize;
		sz = MIN(((off_t)1 + nra) * bsize - off, resid);
#ifdef	DEBUG
		if (vnddebug & VDB_IO)
			printf("vndstrategy: vp %p/%p bn 0x%qx/0x%" PRIx64
			       " sz 0x%zx\n",
			    vnd->sc_vp, vp, (long long)bn, nbn, sz);
#endif

		nbp = getiobuf();
		nestiobuf_setup(bp, nbp, offset, sz);
		nbp->b_blkno = nbn + btodb(off);

#if 0 /* XXX #ifdef DEBUG */
		if (vnddebug & VDB_IO)
			printf("vndstart(%ld): bp %p vp %p blkno "
			    "0x%" PRIx64 " flags %x addr %p cnt 0x%x\n",
			    (long) (vnd-vnd_softc), &nbp->vb_buf,
			    nbp->vb_buf.b_vp, nbp->vb_buf.b_blkno,
			    nbp->vb_buf.b_flags, nbp->vb_buf.b_data,
			    nbp->vb_buf.b_bcount);
#endif
		VOP_STRATEGY(vp, nbp);
		bn += sz;
	}
	nestiobuf_done(bp, skipped, error);
}

static void
vndiodone(struct buf *bp)
{
	struct vndxfer *vnx = VND_BUFTOXFER(bp);
	struct vnd_softc *vnd = vnx->vx_vnd;
	struct buf *obp = bp->b_private;

	KASSERT(&vnx->vx_buf == bp);
	KASSERT(vnd->sc_active > 0);
#ifdef DEBUG
	if (vnddebug & VDB_IO) {
		printf("vndiodone1: bp %p iodone: error %d\n",
		    bp, bp->b_error);
	}
#endif
	disk_unbusy(&vnd->sc_dkdev, bp->b_bcount - bp->b_resid,
	    (bp->b_flags & B_READ));
	vnd->sc_active--;
	if (vnd->sc_active == 0) {
		wakeup(&vnd->sc_tab);
	}
	obp->b_error = bp->b_error;
	obp->b_resid = bp->b_resid;
	VND_PUTXFER(vnd, vnx);
	biodone(obp);
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

	sc = device_lookup(&vnd_cd, unit);
	if (sc == NULL)
		return ENXIO;

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

	sc = device_lookup(&vnd_cd, unit);
	if (sc == NULL)
		return ENXIO;

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (ENXIO);

	return (physio(vndstrategy, NULL, dev, B_WRITE, minphys, uio));
}

static int
vnd_cget(struct lwp *l, int unit, int *un, struct vattr *va)
{
	struct vnd_softc *vnd;

	if (*un == -1)
		*un = unit;
	if (*un < 0)
		return EINVAL;

	vnd = device_lookup(&vnd_cd, *un);
	if (vnd == NULL)
		return (*un >= vnd_cd.cd_ndevs) ? ENXIO : -1;

	if ((vnd->sc_flags & VNF_INITED) == 0)
		return -1;

	return VOP_GETATTR(vnd->sc_vp, va, l->l_cred);
}

/* ARGSUSED */
static int
vndioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
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
		    dev, cmd, data, flag, l->l_proc, unit);
#endif
	vnd = device_lookup(&vnd_cd, unit);
	if (vnd == NULL &&
#ifdef COMPAT_30
	    cmd != VNDIOOCGET &&
#endif
	    cmd != VNDIOCGET)
		return ENXIO;
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
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, vio->vnd_file);
		if ((error = vn_open(&nd, fflags, 0)) != 0)
			goto unlock_and_exit;
		KASSERT(l);
		error = VOP_GETATTR(nd.ni_vp, &vattr, l->l_cred);
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
			error = vn_rdwr(UIO_READ, nd.ni_vp, (void *)ch,
			  sizeof(struct vnd_comp_header), 0, UIO_SYSSPACE,
			  IO_UNIT|IO_NODELOCKED, l->l_cred, NULL, NULL);
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
			if (vnd->sc_comp_blksz == 0 ||
			    vnd->sc_comp_blksz % DEV_BSIZE !=0) {
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
			    ((u_quad_t)vnd->sc_comp_numoffs - 1) *
			     (u_quad_t)vnd->sc_comp_blksz;
 
			/* allocate space for all the compressed offsets */
			vnd->sc_comp_offsets =
			malloc(sizeof(u_int64_t) * vnd->sc_comp_numoffs,
			M_DEVBUF, M_WAITOK);
 
			/* read in the offsets */
			error = vn_rdwr(UIO_READ, nd.ni_vp,
			  (void *)vnd->sc_comp_offsets,
			  sizeof(u_int64_t) * vnd->sc_comp_numoffs,
			  sizeof(struct vnd_comp_header), UIO_SYSSPACE,
			  IO_UNIT|IO_NODELOCKED, l->l_cred, NULL, NULL);
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
			VOP_UNLOCK(nd.ni_vp, 0);
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
		} else if (vnd->sc_size >= (32 * 64)) {
			/*
			 * Size must be at least 2048 DEV_BSIZE blocks
			 * (1M) in order to use this geometry.
			 */
			vnd->sc_geom.vng_secsize = DEV_BSIZE;
			vnd->sc_geom.vng_nsectors = 32;
			vnd->sc_geom.vng_ntracks = 64;
			vnd->sc_geom.vng_ncylinders = vnd->sc_size / (64 * 32);
		} else {
			vnd->sc_geom.vng_secsize = DEV_BSIZE;
			vnd->sc_geom.vng_nsectors = 1;
			vnd->sc_geom.vng_ntracks = 1;
			vnd->sc_geom.vng_ncylinders = vnd->sc_size;
		}

		vnd_set_properties(vnd);

		if (vio->vnd_flags & VNDIOF_READONLY) {
			vnd->sc_flags |= VNF_READONLY;
		}

		if ((error = vndsetcred(vnd, l->l_cred)) != 0)
			goto close_and_exit;

		vndthrottle(vnd, vnd->sc_vp);
		vio->vnd_size = dbtob(vnd->sc_size);
		vnd->sc_flags |= VNF_INITED;

		/* create the kernel thread, wait for it to be up */
		error = kthread_create(PRI_NONE, 0, NULL, vndthread, vnd,
		    &vnd->sc_kthread, vnd->sc_dev.dv_xname);
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
		disk_attach(&vnd->sc_dkdev);

		/* Initialize the xfer and buffer pools. */
		pool_init(&vnd->sc_vxpool, sizeof(struct vndxfer), 0,
		    0, 0, "vndxpl", NULL, IPL_BIO);

		/* Try and read the disklabel. */
		vndgetdisklabel(dev, vnd);

		vndunlock(vnd);

		break;

close_and_exit:
		(void) vn_close(nd.ni_vp, fflags, l->l_cred, l);
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

		/* Detatch the disk. */
		disk_detach(&vnd->sc_dkdev);
		break;

#ifdef COMPAT_30
	case VNDIOOCGET: {
		struct vnd_ouser *vnu;
		struct vattr va;
		vnu = (struct vnd_ouser *)data;
		KASSERT(l);
		switch (error = vnd_cget(l, unit, &vnu->vnu_unit, &va)) {
		case 0:
			vnu->vnu_dev = va.va_fsid;
			vnu->vnu_ino = va.va_fileid;
			break;
		case -1:
			/* unused is not an error */
			vnu->vnu_dev = 0;
			vnu->vnu_ino = 0;
			break;
		default:
			return error;
		}
		break;
	}
#endif
	case VNDIOCGET: {
		struct vnd_user *vnu;
		struct vattr va;
		vnu = (struct vnd_user *)data;
		KASSERT(l);
		switch (error = vnd_cget(l, unit, &vnu->vnu_unit, &va)) {
		case 0:
			vnu->vnu_dev = va.va_fsid;
			vnu->vnu_ino = va.va_fileid;
			break;
		case -1:
			/* unused is not an error */
			vnu->vnu_dev = 0;
			vnu->vnu_ino = 0;
			break;
		default:
			return error;
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
vndsetcred(struct vnd_softc *vnd, kauth_cred_t cred)
{
	struct uio auio;
	struct iovec aiov;
	char *tmpbuf;
	int error;

	vnd->sc_cred = kauth_cred_dup(cred);
	tmpbuf = malloc(DEV_BSIZE, M_TEMP, M_WAITOK);

	/* XXX: Horrible kludge to establish credentials for NFS */
	aiov.iov_base = tmpbuf;
	aiov.iov_len = min(DEV_BSIZE, dbtob(vnd->sc_size));
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = aiov.iov_len;
	UIO_SETUP_SYSSPACE(&auio);
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
			    curlwp, 0, 0);
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
	struct lwp *l = curlwp;
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
		mn = DISKMINOR(device_unit(&vnd->sc_dev), i);
		vdevgone(bmaj, mn, mn, VBLK);
		if (mn != myminor) /* XXX avoid to kill own vnode */
			vdevgone(cmaj, mn, mn, VCHR);
	}

	if ((vnd->sc_flags & VNF_READONLY) == 0)
		fflags |= FWRITE;

	s = splbio();
	bufq_drain(vnd->sc_tab);
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
	(void) vn_close(vp, fflags, vnd->sc_cred, l);
	kauth_cred_free(vnd->sc_cred);
	vnd->sc_vp = (struct vnode *)0;
	vnd->sc_cred = (kauth_cred_t)0;
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
	sc = (struct vnd_softc *)device_lookup(&vnd_cd, unit);
	if (sc == NULL)
		return -1;

	if ((sc->sc_flags & VNF_INITED) == 0)
		return (-1);

	part = DISKPART(dev);
	omask = sc->sc_dkdev.dk_openmask & (1 << part);
	lp = sc->sc_dkdev.dk_label;

	if (omask == 0 && vndopen(dev, 0, S_IFBLK, curlwp))	/* XXX */
		return (-1);

	if (lp->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = lp->d_partitions[part].p_size *
		    (lp->d_secsize / DEV_BSIZE);

	if (omask == 0 && vndclose(dev, 0, S_IFBLK, curlwp))	/* XXX */
		return (-1);

	return (size);
}

static int
vnddump(dev_t dev, daddr_t blkno, void *va,
    size_t size)
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
vndgetdisklabel(dev_t dev, struct vnd_softc *sc)
{
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
		printf("%s: %s\n", sc->sc_dev.dv_xname, errstring);

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
	struct vnd_softc *vnd =
	    (struct vnd_softc *)device_lookup(&vnd_cd, unit);
	u_int32_t comp_block;
	struct uio auio;
	char *addr;
	int s;

	/* set up constants for data move */
	auio.uio_rw = UIO_READ;
	UIO_SETUP_SYSSPACE(&auio);

	/* read, and transfer the data */
	addr = bp->b_data;
	bp->b_resid = bp->b_bcount;
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
					    vnd->sc_dev.dv_xname,
					    vnd->sc_comp_stream.msg);
				bp->b_error = EBADMSG;
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

static void
vnd_set_properties(struct vnd_softc *vnd)
{
	prop_dictionary_t disk_info, odisk_info, geom;

	disk_info = prop_dictionary_create();

	geom = prop_dictionary_create();

	prop_dictionary_set_uint64(geom, "sectors-per-unit",
	    vnd->sc_geom.vng_nsectors * vnd->sc_geom.vng_ntracks *
	    vnd->sc_geom.vng_ncylinders);

	prop_dictionary_set_uint32(geom, "sector-size",
	    vnd->sc_geom.vng_secsize);

	prop_dictionary_set_uint16(geom, "sectors-per-track",
	    vnd->sc_geom.vng_nsectors);

	prop_dictionary_set_uint16(geom, "tracks-per-cylinder",
	    vnd->sc_geom.vng_ntracks);

	prop_dictionary_set_uint64(geom, "cylinders-per-unit",
	    vnd->sc_geom.vng_ncylinders);

	prop_dictionary_set(disk_info, "geometry", geom);
	prop_object_release(geom);

	prop_dictionary_set(device_properties(&vnd->sc_dev),
	    "disk-info", disk_info);

	/*
	 * Don't release disk_info here; we keep a reference to it.
	 * disk_detach() will release it when we go away.
	 */

	odisk_info = vnd->sc_dkdev.dk_info;
	vnd->sc_dkdev.dk_info = disk_info;
	if (odisk_info)
		prop_object_release(odisk_info);
}
