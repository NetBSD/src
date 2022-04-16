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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright 2013 Nexenta Systems, Inc.  All rights reserved.
 * Copyright (c) 2013 Joyent, Inc.  All rights reserved.
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
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/workqueue.h>

#ifdef __NetBSD__
static int
geterror(struct buf *bp)
{

	return (bp->b_error);
}
#endif

/*
 * Virtual device vector for disks.
 */

static void	vdev_disk_io_intr(buf_t *);

static void
vdev_disk_alloc(vdev_t *vd)
{
	vdev_disk_t *dvd;

	dvd = vd->vdev_tsd = kmem_zalloc(sizeof (vdev_disk_t), KM_SLEEP);

#ifdef illumos
	/*
	 * Create the LDI event callback list.
	 */
	list_create(&dvd->vd_ldi_cbs, sizeof (vdev_disk_ldi_cb_t),
	    offsetof(vdev_disk_ldi_cb_t, lcb_next));
#endif
}


static void
vdev_disk_free(vdev_t *vd)
{
	vdev_disk_t *dvd = vd->vdev_tsd;
#ifdef illumos
	vdev_disk_ldi_cb_t *lcb;
#endif

	if (dvd == NULL)
		return;

#ifdef illumos
	/*
	 * We have already closed the LDI handle. Clean up the LDI event
	 * callbacks and free vd->vdev_tsd.
	 */
	while ((lcb = list_head(&dvd->vd_ldi_cbs)) != NULL) {
		list_remove(&dvd->vd_ldi_cbs, lcb);
		(void) ldi_ev_remove_callbacks(lcb->lcb_id);
		kmem_free(lcb, sizeof (vdev_disk_ldi_cb_t));
	}
	list_destroy(&dvd->vd_ldi_cbs);
#endif
	kmem_free(dvd, sizeof (vdev_disk_t));
	vd->vdev_tsd = NULL;
}


/*
 * It's not clear what these hold/rele functions are supposed to do.
 */
static void
vdev_disk_hold(vdev_t *vd)
{

	ASSERT(spa_config_held(vd->vdev_spa, SCL_STATE, RW_WRITER));

}

static void
vdev_disk_rele(vdev_t *vd)
{

	ASSERT(spa_config_held(vd->vdev_spa, SCL_STATE, RW_WRITER));

}

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

	KASSERT(vp == dvd->vd_vp);

	cmd = 1;
	error = VOP_IOCTL(vp, DIOCCACHESYNC, &cmd, FREAD|FWRITE, kcred);
	bp->b_error = error;
	vdev_disk_io_intr(bp);
}

static int
vdev_disk_open(vdev_t *vd, uint64_t *psize, uint64_t *max_psize,
    uint64_t *ashift, uint64_t *pashift)
{
	spa_t *spa = vd->vdev_spa;
	vdev_disk_t *dvd;
	vnode_t *vp;
	int error, cmd;
	uint64_t numsecs;
	unsigned secsize;
	struct disk *pdk;
	struct dkwedge_info dkw;
	struct disk_sectoralign dsa;

	/*
	 * We must have a pathname, and it must be absolute.
	 */
	if (vd->vdev_path == NULL || vd->vdev_path[0] != '/') {
		vd->vdev_stat.vs_aux = VDEV_AUX_BAD_LABEL;
		return (SET_ERROR(EINVAL));
	}

	/*
	 * Reopen the device if it's not currently open. Otherwise,
	 * just update the physical size of the device.
	 */
	if (vd->vdev_tsd != NULL) {
		ASSERT(vd->vdev_reopening);
		dvd = vd->vdev_tsd;
		vp = dvd->vd_vp;
		KASSERT(vp != NULL);
		goto skip_open;
	}

	/*
	 * Create vd->vdev_tsd.
	 */
	vdev_disk_alloc(vd);
	dvd = vd->vdev_tsd;

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
	 */
	if (vd->vdev_devid != NULL) {
		/* XXXNETBSD wedges */
#ifdef illumos
		if (ddi_devid_str_decode(vd->vdev_devid, &dvd->vd_devid,
		    &dvd->vd_minor) != 0) {
			vd->vdev_stat.vs_aux = VDEV_AUX_BAD_LABEL;
			return (SET_ERROR(EINVAL));
		}
#endif
	}

	error = EINVAL;		/* presume failure */

	error = vn_open(vd->vdev_path, UIO_SYSSPACE, FREAD|FWRITE, 0,
	    &vp, CRCREAT, 0);
	if (error != 0) {
		vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
		return (SET_ERROR(error));
	}
	if (vp->v_type != VBLK) {
#ifdef __NetBSD__
		vn_close(vp, FREAD|FWRITE, kcred);
#else
		vrele(vp);
#endif
		vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
		return (SET_ERROR(EINVAL));
	}

	pdk = NULL;
	if (getdiskinfo(vp, &dkw) == 0)
		pdk = disk_find(dkw.dkw_devname);

	/* XXXNETBSD Once tls-maxphys gets merged this block becomes:
		dvd->vd_maxphys = (pdk ? disk_maxphys(pdk) : MACHINE_MAXPHYS);
	*/
	{
		struct buf buf = {
			.b_dev = vp->v_rdev,
			.b_bcount = MAXPHYS,
		};
		if (pdk && pdk->dk_driver && pdk->dk_driver->d_minphys)
			(*pdk->dk_driver->d_minphys)(&buf);
		dvd->vd_maxphys = buf.b_bcount;
	}

	/*
	 * XXXNETBSD Compare the devid to the stored value.
	 */

	/*
	 * Create a workqueue to process cache-flushes concurrently.
	 */
	error = workqueue_create(&dvd->vd_wq, "vdevsync",
	    vdev_disk_flush, dvd, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error != 0) {
#ifdef __NetBSD__
		vn_close(vp, FREAD|FWRITE, kcred);
#else
		vrele(vp);
#endif
		return (SET_ERROR(error));
	}

	dvd->vd_vp = vp;

skip_open:
	error = getdisksize(vp, &numsecs, &secsize);
	if (error != 0) {
		vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
		return (SET_ERROR(error));
	}

	*psize = numsecs * secsize;
	*max_psize = *psize;

	*ashift = highbit(MAX(secsize, SPA_MINBLOCKSIZE)) - 1;

	/*
	 * Try to determine whether the disk has a preferred physical
	 * sector size even if it can emulate a smaller logical sector
	 * size with r/m/w cycles, e.g. a disk with 4096-byte sectors
	 * that for compatibility claims to support 512-byte ones.
	 */
	if (VOP_IOCTL(vp, DIOCGSECTORALIGN, &dsa, FREAD, NOCRED) == 0) {
		*pashift = highbit(dsa.dsa_alignment * secsize) - 1;
		if (dsa.dsa_firstaligned % dsa.dsa_alignment)
			printf("ZFS WARNING: vdev %s: sectors are misaligned"
			    " (alignment=%"PRIu32", firstaligned=%"PRIu32")\n",
			    vd->vdev_path,
			    dsa.dsa_alignment, dsa.dsa_firstaligned);
	} else {
		*pashift = *ashift;
	}

	vd->vdev_wholedisk = 0;
	if (getdiskinfo(vp, &dkw) != 0 &&
	    dkw.dkw_offset == 0 && dkw.dkw_size == numsecs)
		vd->vdev_wholedisk = 1,

	/*
	 * Clear the nowritecache bit, so that on a vdev_reopen() we will
	 * try again.
	 */
	vd->vdev_nowritecache = B_FALSE;

	return (0);
}

static void
vdev_disk_close(vdev_t *vd)
{
	vdev_disk_t *dvd = vd->vdev_tsd;

	if (vd->vdev_reopening || dvd == NULL)
		return;

#ifdef illumos
	if (dvd->vd_minor != NULL) {
		ddi_devid_str_free(dvd->vd_minor);
		dvd->vd_minor = NULL;
	}

	if (dvd->vd_devid != NULL) {
		ddi_devid_free(dvd->vd_devid);
		dvd->vd_devid = NULL;
	}

	if (dvd->vd_lh != NULL) {
		(void) ldi_close(dvd->vd_lh, spa_mode(vd->vdev_spa), kcred);
		dvd->vd_lh = NULL;
	}
#endif

#ifdef __NetBSD__
	if (dvd->vd_vp != NULL) {
		vn_close(dvd->vd_vp, FREAD|FWRITE, kcred);
		dvd->vd_vp = NULL;
	}
	if (dvd->vd_wq != NULL) {
		workqueue_destroy(dvd->vd_wq);
		dvd->vd_wq = NULL;
	}
#endif

	vd->vdev_delayed_close = B_FALSE;
#ifdef illumos
	/*
	 * If we closed the LDI handle due to an offline notify from LDI,
	 * don't free vd->vdev_tsd or unregister the callbacks here;
	 * the offline finalize callback or a reopen will take care of it.
	 */
	if (dvd->vd_ldi_offline)
		return;
#endif

	vdev_disk_free(vd);
}

int
vdev_disk_physio(vdev_t *vd, caddr_t data,
    size_t size, uint64_t offset, int flags, boolean_t isdump)
{
#ifdef illumos
	vdev_disk_t *dvd = vd->vdev_tsd;

	/*
	 * If the vdev is closed, it's likely in the REMOVED or FAULTED state.
	 * Nothing to be done here but return failure.
	 */
	if (dvd == NULL || (dvd->vd_ldi_offline && dvd->vd_lh == NULL))
		return (EIO);

	ASSERT(vd->vdev_ops == &vdev_disk_ops);

	/*
	 * If in the context of an active crash dump, use the ldi_dump(9F)
	 * call instead of ldi_strategy(9F) as usual.
	 */
	if (isdump) {
		ASSERT3P(dvd, !=, NULL);
		return (ldi_dump(dvd->vd_lh, data, lbtodb(offset),
		    lbtodb(size)));
	}

	return (vdev_disk_ldi_physio(dvd->vd_lh, data, size, offset, flags));
#endif
#ifdef __NetBSD__
	return (EIO);
#endif
}

static void
vdev_disk_io_intr(buf_t *bp)
{
	zio_t *zio = bp->b_private;

	/*
	 * The rest of the zio stack only deals with EIO, ECKSUM, and ENXIO.
	 * Rather than teach the rest of the stack about other error
	 * possibilities (EFAULT, etc), we normalize the error value here.
	 */
	zio->io_error = (geterror(bp) != 0 ? SET_ERROR(EIO) : 0);

	if (zio->io_error == 0 && bp->b_resid != 0)
		zio->io_error = SET_ERROR(EIO);

	putiobuf(bp);
	zio_delay_interrupt(zio);
}

static void
vdev_disk_ioctl_free(zio_t *zio)
{
	kmem_free(zio->io_vsd, sizeof (struct dk_callback));
}

static const zio_vsd_ops_t vdev_disk_vsd_ops = {
	vdev_disk_ioctl_free,
	zio_vsd_default_cksum_report
};

static void
vdev_disk_ioctl_done(void *zio_arg, int error)
{
	zio_t *zio = zio_arg;

	zio->io_error = error;

	zio_interrupt(zio);
}

static void
vdev_disk_io_start(zio_t *zio)
{
	vdev_t *vd = zio->io_vd;
	vdev_disk_t *dvd = vd->vdev_tsd;
	vnode_t *vp;
	buf_t *bp, *nbp;
	int error, size, off, resid;

	/*
	 * If the vdev is closed, it's likely in the REMOVED or FAULTED state.
	 * Nothing to be done here but return failure.
	 */
#ifdef illumos
	if (dvd == NULL || (dvd->vd_ldi_offline && dvd->vd_lh == NULL)) {
		zio->io_error = SET_ERROR(ENXIO);
		zio_interrupt(zio);
		return;
	}
#endif
#ifdef __NetBSD__
	if (dvd == NULL) {
		zio->io_error = SET_ERROR(ENXIO);
		zio_interrupt(zio);
		return;
	}
	ASSERT3U(dvd->vd_maxphys, >, 0);
	vp = dvd->vd_vp;
#endif

	if (zio->io_type == ZIO_TYPE_IOCTL) {
		/* XXPOLICY */
		if (!vdev_readable(vd)) {
			zio->io_error = SET_ERROR(ENXIO);
			zio_interrupt(zio);
			return;
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
			return;

		default:
			zio->io_error = SET_ERROR(ENOTSUP);
			break;
		}

		zio_execute(zio);
		return;
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
		mutex_enter(vp->v_interlock);
		vp->v_numoutput++;
		mutex_exit(vp->v_interlock);
	}

	if (bp->b_bcount <= dvd->vd_maxphys) {
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
			size = uimin(resid, dvd->vd_maxphys);
			nbp = getiobuf(vp, true);
			nbp->b_blkno = btodb(zio->io_offset + off);
			/* Below call increments v_numoutput. */
			nestiobuf_setup(bp, nbp, off, size);
			(void)VOP_STRATEGY(vp, nbp);
			resid -= size;
			off += size;
		}
	}
}

static void
vdev_disk_io_done(zio_t *zio)
{
#ifdef illumos
	vdev_t *vd = zio->io_vd;

	/*
	 * If the device returned EIO, then attempt a DKIOCSTATE ioctl to see if
	 * the device has been removed.  If this is the case, then we trigger an
	 * asynchronous removal of the device. Otherwise, probe the device and
	 * make sure it's still accessible.
	 */
	if (zio->io_error == EIO && !vd->vdev_remove_wanted) {
		vdev_disk_t *dvd = vd->vdev_tsd;
		int state = DKIO_NONE;

		if (ldi_ioctl(dvd->vd_lh, DKIOCSTATE, (intptr_t)&state,
		    FKIOCTL, kcred, NULL) == 0 && state != DKIO_INSERTED) {
			/*
			 * We post the resource as soon as possible, instead of
			 * when the async removal actually happens, because the
			 * DE is using this information to discard previous I/O
			 * errors.
			 */
			zfs_post_remove(zio->io_spa, vd);
			vd->vdev_remove_wanted = B_TRUE;
			spa_async_request(zio->io_spa, SPA_ASYNC_REMOVE);
		} else if (!vd->vdev_delayed_close) {
			vd->vdev_delayed_close = B_TRUE;
		}
	}
#endif
}

vdev_ops_t vdev_disk_ops = {
	vdev_disk_open,
	vdev_disk_close,
	vdev_default_asize,
	vdev_disk_io_start,
	vdev_disk_io_done,
	NULL,
	vdev_disk_hold,
	vdev_disk_rele,
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
#ifdef __NetBSD__
	return (ENOTSUP);
#else
	ldi_handle_t vd_lh;
	vdev_label_t *label;
	uint64_t s, size;
	int l;
	ddi_devid_t tmpdevid;
	int error = -1;
	char *minor_name;

	/*
	 * Read the device label and build the nvlist.
	 */
	if (devid != NULL && ddi_devid_str_decode(devid, &tmpdevid,
	    &minor_name) == 0) {
		error = ldi_open_by_devid(tmpdevid, minor_name,
		    FREAD, kcred, &vd_lh, zfs_li);
		ddi_devid_free(tmpdevid);
		ddi_devid_str_free(minor_name);
	}

	if (error && (error = ldi_open_by_name(devpath, FREAD, kcred, &vd_lh,
	    zfs_li)))
		return (error);

	if (ldi_get_size(vd_lh, &s)) {
		(void) ldi_close(vd_lh, FREAD, kcred);
		return (SET_ERROR(EIO));
	}

	size = P2ALIGN_TYPED(s, sizeof (vdev_label_t), uint64_t);
	label = kmem_alloc(sizeof (vdev_label_t), KM_SLEEP);

	*config = NULL;
	for (l = 0; l < VDEV_LABELS; l++) {
		uint64_t offset, state, txg = 0;

		/* read vdev label */
		offset = vdev_label_offset(size, l, 0);
		if (vdev_disk_ldi_physio(vd_lh, (caddr_t)label,
		    VDEV_SKIP_SIZE + VDEV_PHYS_SIZE, offset, B_READ) != 0)
			continue;

		if (nvlist_unpack(label->vl_vdev_phys.vp_nvlist,
		    sizeof (label->vl_vdev_phys.vp_nvlist), config, 0) != 0) {
			*config = NULL;
			continue;
		}

		if (nvlist_lookup_uint64(*config, ZPOOL_CONFIG_POOL_STATE,
		    &state) != 0 || state >= POOL_STATE_DESTROYED) {
			nvlist_free(*config);
			*config = NULL;
			continue;
		}

		if (nvlist_lookup_uint64(*config, ZPOOL_CONFIG_POOL_TXG,
		    &txg) != 0 || txg == 0) {
			nvlist_free(*config);
			*config = NULL;
			continue;
		}

		break;
	}

	kmem_free(label, sizeof (vdev_label_t));
	(void) ldi_close(vd_lh, FREAD, kcred);
	if (*config == NULL)
		error = SET_ERROR(EIDRM);

	return (error);
#endif
}
