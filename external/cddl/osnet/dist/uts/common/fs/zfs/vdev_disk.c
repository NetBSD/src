
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/zfs_context.h>
#include <sys/spa.h>
#include <sys/refcount.h>
#include <sys/vdev_disk.h>
#include <sys/vdev_impl.h>
#include <sys/fs/zfs.h>
#include <sys/zio.h>
#include <sys/sunldi.h>
#include <sys/fm/fs/zfs.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/workqueue.h>

/*
 * Virtual device vector for disks.
 */

static void	vdev_disk_io_intr(buf_t *);

static void
vdev_disk_flush(struct work *work, void *cookie)
{
	vdev_disk_t *dvd;
	int error, cmd;
	buf_t *bp;
	vnode_t *vp;

	bp = (struct buf *)work;
	vp = bp->b_vp;
	dvd = cookie;
	
	KASSERT(vp == dvd->vd_vn);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	cmd = 1;
	error = VOP_IOCTL(vp, DIOCCACHESYNC, &cmd, FREAD|FWRITE,
	    kauth_cred_get());
	VOP_UNLOCK(vp, 0);
	bp->b_error = error;
	vdev_disk_io_intr(bp);
}

static int
vdev_disk_open(vdev_t *vd, uint64_t *psize, uint64_t *ashift)
{
	struct partinfo pinfo;
	vdev_disk_t *dvd;
	vnode_t *vp;
	int error, cmd;

	/*
	 * We must have a pathname, and it must be absolute.
	 */
	if (vd->vdev_path == NULL || vd->vdev_path[0] != '/') {
		vd->vdev_stat.vs_aux = VDEV_AUX_BAD_LABEL;
		return (EINVAL);
	}

	dvd = vd->vdev_tsd = kmem_zalloc(sizeof (vdev_disk_t), KM_SLEEP);

	/*
	 * When opening a disk device, we want to preserve the user's original
	 * intent.  We always want to open the device by the path the user gave
	 * us, even if it is one of multiple paths to the save device.  But we
	 * also want to be able to survive disks being removed/recabled.
	 * Therefore the sequence of opening devices is:
	 *
	 * 1. Try opening the device by path.  For legacy pools without the
	 *    'whole_disk' property, attempt to fix the path by appending 's0'.
	 *
	 * 2. If the devid of the device matches the stored value, return
	 *    success.
	 *
	 * 3. Otherwise, the device may have moved.  Try opening the device
	 *    by the devid instead.
	 *
	 */
	if (vd->vdev_devid != NULL) {
		/* XXXNETBSD wedges */
	}

	error = EINVAL;		/* presume failure */

	error = vn_open(vd->vdev_path, UIO_SYSSPACE, FREAD|FWRITE, 0,
	    &vp, CRCREAT, 0);
	if (error != 0) {
		vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
		return error;
	}
	if (vp->v_type != VBLK) {
		vrele(vp);
		vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
		return EINVAL;
	}

	/*
	 * XXXNETBSD Compare the devid to the stored value.
	 */

	/*
	 * Determine the actual size of the device.
	 * XXXNETBSD wedges.
	 */
	error = VOP_IOCTL(vp, DIOCGPART, &pinfo, FREAD|FWRITE,
	    kauth_cred_get());
	if (error != 0) {
		vrele(vp);
		vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
		return error;
	}
	*psize = (uint64_t)pinfo.part->p_size * pinfo.disklab->d_secsize;
	*ashift = highbit(MAX(pinfo.disklab->d_secsize, SPA_MINBLOCKSIZE)) - 1;
	vd->vdev_wholedisk = (pinfo.part->p_offset == 0); /* XXXNETBSD */

	/*
	 * Create a workqueue to process cache-flushes concurrently.
	 */
	error = workqueue_create(&dvd->vd_wq, "vdevsync",
	    vdev_disk_flush, dvd, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error != 0) {
		vrele(vp);
		return error;
	}

	/*
	 * Clear the nowritecache bit, so that on a vdev_reopen() we will
	 * try again.
	 */
	vd->vdev_nowritecache = B_FALSE;

	dvd->vd_vn = vp;
	return 0;
}

static void
vdev_disk_close(vdev_t *vd)
{
	vdev_disk_t *dvd = vd->vdev_tsd;
	vnode_t *vp;

	if (dvd == NULL)
		return;

	dprintf("removing disk %s, devid %s\n",
	    vd->vdev_path ? vd->vdev_path : "<none>",
	    vd->vdev_devid ? vd->vdev_devid : "<none>");

	if ((vp = dvd->vd_vn) != NULL) {
/* XXX NetBSD Sometimes we deadlock on this why ? */
//		vprint("vnode close info", vp);
		vn_close(vp, FREAD|FWRITE, kauth_cred_get());
//		vprint("vnode close info", vp);
/* XXX is this needed ?		vrele(vp); */
		workqueue_destroy(dvd->vd_wq);
	}
	kmem_free(dvd, sizeof (vdev_disk_t));
	vd->vdev_tsd = NULL;
}

static void
vdev_disk_io_intr(buf_t *bp)
{
	zio_t *zio = bp->b_private;

	dprintf("vdev_disk_io_intr bp=%p\n", bp);
	/*
	 * The rest of the zio stack only deals with EIO, ECKSUM, and ENXIO.
	 * Rather than teach the rest of the stack about other error
	 * possibilities (EFAULT, etc), we normalize the error value here.
	 */
	if (bp->b_error == 0) {
		if (bp->b_resid != 0) {
			zio->io_error = EIO;
		} else {
			zio->io_error = 0;
		}
	} else {
		zio->io_error = EIO;
	}

	putiobuf(bp);
	zio_interrupt(zio);
}

static int
vdev_disk_io_start(zio_t *zio)
{
	vdev_t *vd = zio->io_vd;
	vdev_disk_t *dvd = vd->vdev_tsd;
	vnode_t *vp;
	buf_t *bp, *nbp;
	int error, size, off, resid;

	vp = dvd->vd_vn;
	if (zio->io_type == ZIO_TYPE_IOCTL) {
		/* XXPOLICY */
		if (!vdev_readable(vd)) {
			zio->io_error = ENXIO;
			return (ZIO_PIPELINE_CONTINUE);
		}

		switch (zio->io_cmd) {
		case DKIOCFLUSHWRITECACHE:

			if (zfs_nocacheflush)
				break;

			if (vd->vdev_nowritecache) {
				zio->io_error = ENOTSUP;
				break;
			}

			bp = getiobuf(vp, true);
			bp->b_private = zio;
			workqueue_enqueue(dvd->vd_wq, &bp->b_work, NULL);
			return (ZIO_PIPELINE_STOP);

		default:
			zio->io_error = ENOTSUP;
			break;
		}

		return (ZIO_PIPELINE_CONTINUE);
	}

	bp = getiobuf(vp, true);
	bp->b_flags = (zio->io_type == ZIO_TYPE_READ ? B_READ : B_WRITE);
	bp->b_cflags = BC_BUSY | BC_NOCACHE;
	bp->b_data = zio->io_data;
	bp->b_blkno = btodb(zio->io_offset);
	bp->b_bcount = zio->io_size;
	bp->b_resid = zio->io_size;
	bp->b_iodone = vdev_disk_io_intr;
	bp->b_private = zio;

	if (!(bp->b_flags & B_READ)) {
		mutex_enter(&vp->v_interlock);
		vp->v_numoutput++;
		mutex_exit(&vp->v_interlock);
	}
	
	if (bp->b_bcount <= MAXPHYS) {
		/* We can do this I/O in one pass. */
		(void)VOP_STRATEGY(vp, bp);
	} else {
		/*
		 * The I/O is larger than we can process in one pass.
		 * Split it into smaller pieces.
		 */
		resid = zio->io_size;
		off = 0;
		while (resid != 0) {
			size = min(resid, MAXPHYS);
			nbp = getiobuf(vp, true);
			nbp->b_blkno = btodb(zio->io_offset + off);
			/* Below call increments v_numoutput. */
			nestiobuf_setup(bp, nbp, off, size);
			(void)VOP_STRATEGY(vp, nbp);
			resid -= size;
			off += size;
		}
	}
	
	return (ZIO_PIPELINE_STOP);
}

static void
vdev_disk_io_done(zio_t *zio)
{

	/* NetBSD: nothing */
}

vdev_ops_t vdev_disk_ops = {
	vdev_disk_open,
	vdev_disk_close,
	vdev_default_asize,
	vdev_disk_io_start,
	vdev_disk_io_done,
	NULL,
	VDEV_TYPE_DISK,		/* name of this vdev type */
	B_TRUE			/* leaf vdev */
};

/*
 * Given the root disk device devid or pathname, read the label from
 * the device, and construct a configuration nvlist.
 */
int
vdev_disk_read_rootlabel(char *devpath, char *devid, nvlist_t **config)
{

	return EOPNOTSUPP;
}
