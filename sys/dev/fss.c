/*	$NetBSD: fss.c,v 1.18.2.1 2006/01/15 10:02:47 yamt Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 * File system snapshot disk driver.
 *
 * Block/character interface to the snapshot of a mounted file system.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fss.c,v 1.18.2.1 2006/01/15 10:02:47 yamt Exp $");

#include "fss.h"

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
#include <sys/kthread.h>

#include <miscfs/specfs/specdev.h>

#include <dev/fssvar.h>

#include <machine/stdarg.h>

#ifdef DEBUG
#define FSS_STATISTICS
#endif

#ifdef FSS_STATISTICS
struct fss_stat {
	u_int64_t	cow_calls;
	u_int64_t	cow_copied;
	u_int64_t	cow_cache_full;
	u_int64_t	indir_read;
	u_int64_t	indir_write;
};

static struct fss_stat fss_stat[NFSS];

#define FSS_STAT_INC(sc, field)	\
			do { \
				fss_stat[sc->sc_unit].field++; \
			} while (0)
#define FSS_STAT_SET(sc, field, value) \
			do { \
				fss_stat[sc->sc_unit].field = value; \
			} while (0)
#define FSS_STAT_ADD(sc, field, value) \
			do { \
				fss_stat[sc->sc_unit].field += value; \
			} while (0)
#define FSS_STAT_VAL(sc, field) fss_stat[sc->sc_unit].field
#define FSS_STAT_CLEAR(sc) \
			do { \
				memset(&fss_stat[sc->sc_unit], 0, \
				    sizeof(struct fss_stat)); \
			} while (0)
#else /* FSS_STATISTICS */
#define FSS_STAT_INC(sc, field)
#define FSS_STAT_SET(sc, field, value)
#define FSS_STAT_ADD(sc, field, value)
#define FSS_STAT_CLEAR(sc)
#endif /* FSS_STATISTICS */

static struct fss_softc fss_softc[NFSS];

void fssattach(int);

dev_type_open(fss_open);
dev_type_close(fss_close);
dev_type_read(fss_read);
dev_type_write(fss_write);
dev_type_ioctl(fss_ioctl);
dev_type_strategy(fss_strategy);
dev_type_dump(fss_dump);
dev_type_size(fss_size);

static int fss_copy_on_write(void *, struct buf *);
static inline void fss_error(struct fss_softc *, const char *, ...);
static int fss_create_files(struct fss_softc *, struct fss_set *,
    off_t *, struct lwp *);
static int fss_create_snapshot(struct fss_softc *, struct fss_set *,
    struct lwp *);
static int fss_delete_snapshot(struct fss_softc *, struct lwp *);
static int fss_softc_alloc(struct fss_softc *);
static void fss_softc_free(struct fss_softc *);
static void fss_cluster_iodone(struct buf *);
static void fss_read_cluster(struct fss_softc *, u_int32_t);
static void fss_bs_thread(void *);
static int fss_bs_io(struct fss_softc *, fss_io_type,
    u_int32_t, off_t, int, caddr_t);
static u_int32_t *fss_bs_indir(struct fss_softc *, u_int32_t);

const struct bdevsw fss_bdevsw = {
	fss_open, fss_close, fss_strategy, fss_ioctl,
	fss_dump, fss_size, D_DISK
};

const struct cdevsw fss_cdevsw = {
	fss_open, fss_close, fss_read, fss_write, fss_ioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

void
fssattach(int num)
{
	int i;
	struct fss_softc *sc;

	for (i = 0; i < NFSS; i++) {
		sc = &fss_softc[i];
		sc->sc_unit = i;
		sc->sc_bdev = NODEV;
		simple_lock_init(&sc->sc_slock);
		bufq_alloc(&sc->sc_bufq, "fcfs", 0);
	}
}

int
fss_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	int s, mflag;
	struct fss_softc *sc;

	mflag = (mode == S_IFCHR ? FSS_CDEV_OPEN : FSS_BDEV_OPEN);

	if ((sc = FSS_DEV_TO_SOFTC(dev)) == NULL)
		return ENODEV;

	FSS_LOCK(sc, s);

	sc->sc_flags |= mflag;

	FSS_UNLOCK(sc, s);

	return 0;
}

int
fss_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	int s, mflag, error;
	struct fss_softc *sc;

	mflag = (mode == S_IFCHR ? FSS_CDEV_OPEN : FSS_BDEV_OPEN);

	if ((sc = FSS_DEV_TO_SOFTC(dev)) == NULL)
		return ENODEV;

	FSS_LOCK(sc, s); 

	if ((sc->sc_flags & (FSS_CDEV_OPEN|FSS_BDEV_OPEN)) == mflag) {
		if ((sc->sc_uflags & FSS_UNCONFIG_ON_CLOSE) != 0 &&
		    (sc->sc_flags & FSS_ACTIVE) != 0) {
			FSS_UNLOCK(sc, s);
			error = fss_ioctl(dev, FSSIOCCLR, NULL, FWRITE, l);
			if (error)
				return error;
			FSS_LOCK(sc, s);
		}
		sc->sc_uflags &= ~FSS_UNCONFIG_ON_CLOSE;
	}

	sc->sc_flags &= ~mflag;

	FSS_UNLOCK(sc, s);

	return 0;
}

void
fss_strategy(struct buf *bp)
{
	int s;
	struct fss_softc *sc;

	sc = FSS_DEV_TO_SOFTC(bp->b_dev);

	FSS_LOCK(sc, s);

	if ((bp->b_flags & B_READ) != B_READ ||
	    sc == NULL || !FSS_ISVALID(sc)) {

		FSS_UNLOCK(sc, s);

		bp->b_error = (sc == NULL ? ENODEV : EROFS);
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	bp->b_rawblkno = bp->b_blkno;
	BUFQ_PUT(sc->sc_bufq, bp);
	wakeup(&sc->sc_bs_proc);

	FSS_UNLOCK(sc, s);
}

int
fss_read(dev_t dev, struct uio *uio, int flags)
{
	return physio(fss_strategy, NULL, dev, B_READ, minphys, uio);
}

int
fss_write(dev_t dev, struct uio *uio, int flags)
{
	return physio(fss_strategy, NULL, dev, B_WRITE, minphys, uio);
}

int
fss_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	int s, error;
	struct fss_softc *sc;
	struct fss_set *fss = (struct fss_set *)data;
	struct fss_get *fsg = (struct fss_get *)data;

	if ((sc = FSS_DEV_TO_SOFTC(dev)) == NULL)
		return ENODEV;

	FSS_LOCK(sc, s);
	while ((sc->sc_flags & FSS_EXCL) == FSS_EXCL) {
		error = ltsleep(sc, PRIBIO|PCATCH, "fsslock", 0, &sc->sc_slock);
		if (error) {
			FSS_UNLOCK(sc, s);
			return error;
		}
	}
	sc->sc_flags |= FSS_EXCL;
	FSS_UNLOCK(sc, s);

	switch (cmd) {
	case FSSIOCSET:
		if ((flag & FWRITE) == 0)
			error = EPERM;
		else if ((sc->sc_flags & FSS_ACTIVE) != 0)
			error = EBUSY;
		else
			error = fss_create_snapshot(sc, fss, l);
		break;

	case FSSIOCCLR:
		if ((flag & FWRITE) == 0)
			error = EPERM;
		else if ((sc->sc_flags & FSS_ACTIVE) == 0)
			error = ENXIO;
		else
			error = fss_delete_snapshot(sc, l);
		break;

	case FSSIOCGET:
		switch (sc->sc_flags & (FSS_PERSISTENT | FSS_ACTIVE)) {
		case FSS_ACTIVE:
			memcpy(fsg->fsg_mount, sc->sc_mntname, MNAMELEN);
			fsg->fsg_csize = FSS_CLSIZE(sc);
			fsg->fsg_time = sc->sc_time;
			fsg->fsg_mount_size = sc->sc_clcount;
			fsg->fsg_bs_size = sc->sc_clnext;
			error = 0;
			break;
		case FSS_PERSISTENT | FSS_ACTIVE:
			memcpy(fsg->fsg_mount, sc->sc_mntname, MNAMELEN);
			fsg->fsg_csize = 0;
			fsg->fsg_time = sc->sc_time;
			fsg->fsg_mount_size = 0;
			fsg->fsg_bs_size = 0;
			error = 0;
			break;
		default:
			error = ENXIO;
			break;
		}
		break;

	case FSSIOFSET:
		sc->sc_uflags = *(int *)data;
		error = 0;
		break;

	case FSSIOFGET:
		*(int *)data = sc->sc_uflags;
		error = 0;
		break;

	default:
		error = EINVAL;
		break;
	}

	FSS_LOCK(sc, s);
	sc->sc_flags &= ~FSS_EXCL;
	FSS_UNLOCK(sc, s);
	wakeup(sc);

	return error;
}

int
fss_size(dev_t dev)
{
	return -1;
}

int
fss_dump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	return EROFS;
}

/*
 * An error occurred reading or writing the snapshot or backing store.
 * If it is the first error log to console.
 * The caller holds the simplelock.
 */
static inline void
fss_error(struct fss_softc *sc, const char *fmt, ...)
{
	va_list ap;

	if ((sc->sc_flags & (FSS_ACTIVE|FSS_ERROR)) == FSS_ACTIVE) {
		va_start(ap, fmt);
		printf("fss%d: snapshot invalid: ", sc->sc_unit);
		vprintf(fmt, ap);
		printf("\n");
		va_end(ap);
	}
	if ((sc->sc_flags & FSS_ACTIVE) == FSS_ACTIVE)
		sc->sc_flags |= FSS_ERROR;
}

/*
 * Allocate the variable sized parts of the softc and
 * fork the kernel thread.
 *
 * The fields sc_clcount, sc_clshift, sc_cache_size and sc_indir_size
 * must be initialized.
 */
static int
fss_softc_alloc(struct fss_softc *sc)
{
	int i, len, error;

	len = (sc->sc_clcount+NBBY-1)/NBBY;
	sc->sc_copied = malloc(len, M_TEMP, M_ZERO|M_WAITOK|M_CANFAIL);
	if (sc->sc_copied == NULL)
		return(ENOMEM);

	len = sc->sc_cache_size*sizeof(struct fss_cache);
	sc->sc_cache = malloc(len, M_TEMP, M_ZERO|M_WAITOK|M_CANFAIL);
	if (sc->sc_cache == NULL)
		return(ENOMEM);

	len = FSS_CLSIZE(sc);
	for (i = 0; i < sc->sc_cache_size; i++) {
		sc->sc_cache[i].fc_type = FSS_CACHE_FREE;
		sc->sc_cache[i].fc_softc = sc;
		sc->sc_cache[i].fc_xfercount = 0;
		sc->sc_cache[i].fc_data = malloc(len, M_TEMP,
		    M_WAITOK|M_CANFAIL);
		if (sc->sc_cache[i].fc_data == NULL)
			return(ENOMEM);
	}

	len = (sc->sc_indir_size+NBBY-1)/NBBY;
	sc->sc_indir_valid = malloc(len, M_TEMP, M_ZERO|M_WAITOK|M_CANFAIL);
	if (sc->sc_indir_valid == NULL)
		return(ENOMEM);

	len = FSS_CLSIZE(sc);
	sc->sc_indir_data = malloc(len, M_TEMP, M_ZERO|M_WAITOK|M_CANFAIL);
	if (sc->sc_indir_data == NULL)
		return(ENOMEM);

	if ((error = kthread_create1(fss_bs_thread, sc, &sc->sc_bs_proc,
	    "fssbs%d", sc->sc_unit)) != 0)
		return error;

	sc->sc_flags |= FSS_BS_THREAD;
	return 0;
}

/*
 * Free the variable sized parts of the softc.
 */
static void
fss_softc_free(struct fss_softc *sc)
{
	int s, i;

	if ((sc->sc_flags & FSS_BS_THREAD) != 0) {
		FSS_LOCK(sc, s);
		sc->sc_flags &= ~FSS_BS_THREAD;
		wakeup(&sc->sc_bs_proc);
		while (sc->sc_bs_proc != NULL)
			ltsleep(&sc->sc_bs_proc, PRIBIO, "fssthread", 0,
			    &sc->sc_slock);
		FSS_UNLOCK(sc, s);
	}

	if (sc->sc_copied != NULL)
		free(sc->sc_copied, M_TEMP);
	sc->sc_copied = NULL;

	if (sc->sc_cache != NULL) {
		for (i = 0; i < sc->sc_cache_size; i++)
			if (sc->sc_cache[i].fc_data != NULL)
				free(sc->sc_cache[i].fc_data, M_TEMP);
		free(sc->sc_cache, M_TEMP);
	}
	sc->sc_cache = NULL;

	if (sc->sc_indir_valid != NULL)
		free(sc->sc_indir_valid, M_TEMP);
	sc->sc_indir_valid = NULL;

	if (sc->sc_indir_data != NULL)
		free(sc->sc_indir_data, M_TEMP);
	sc->sc_indir_data = NULL;
}

/*
 * Check if an unmount is ok. If forced, set this snapshot into ERROR state.
 */
int
fss_umount_hook(struct mount *mp, int forced)
{
	int i, s;

	for (i = 0; i < NFSS; i++) {
		FSS_LOCK(&fss_softc[i], s);
		if ((fss_softc[i].sc_flags & FSS_ACTIVE) != 0 &&
		    fss_softc[i].sc_mount == mp) {
			if (forced)
				fss_error(&fss_softc[i], "forced unmount");
			else {
				FSS_UNLOCK(&fss_softc[i], s);
				return EBUSY;
			}
		}
		FSS_UNLOCK(&fss_softc[i], s);
	}

	return 0;
}

/*
 * A buffer is written to the snapshotted block device. Copy to
 * backing store if needed.
 */
static int
fss_copy_on_write(void *v, struct buf *bp)
{
	int s;
	u_int32_t cl, ch, c;
	struct fss_softc *sc = v;

	FSS_LOCK(sc, s);
	if (!FSS_ISVALID(sc)) {
		FSS_UNLOCK(sc, s);
		return 0;
	}

	FSS_UNLOCK(sc, s);

	FSS_STAT_INC(sc, cow_calls);

	cl = FSS_BTOCL(sc, dbtob(bp->b_blkno));
	ch = FSS_BTOCL(sc, dbtob(bp->b_blkno)+bp->b_bcount-1);

	for (c = cl; c <= ch; c++)
		fss_read_cluster(sc, c);

	return 0;
}

/*
 * Lookup and open needed files.
 *
 * For file system internal snapshot initializes sc_mntname, sc_mount,
 * sc_bs_vp and sc_time.
 *
 * Otherwise returns dev and size of the underlying block device.
 * Initializes sc_mntname, sc_mount_vp, sc_bdev, sc_bs_vp and sc_mount
 */
static int
fss_create_files(struct fss_softc *sc, struct fss_set *fss,
    off_t *bsize, struct lwp *l)
{
	int error, bits, fsbsize;
	struct timespec ts;
	struct partinfo dpart;
	struct vattr va;
	struct nameidata nd;

	/*
	 * Get the mounted file system.
	 */

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fss->fss_mount, l);
	if ((error = namei(&nd)) != 0)
		return error;

	if ((nd.ni_vp->v_flag & VROOT) != VROOT) {
		vrele(nd.ni_vp);
		return EINVAL;
	}

	sc->sc_mount = nd.ni_vp->v_mount;
	memcpy(sc->sc_mntname, sc->sc_mount->mnt_stat.f_mntonname, MNAMELEN);

	vrele(nd.ni_vp);

	/*
	 * Check for file system internal snapshot.
	 */

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fss->fss_bstore, l);
	if ((error = namei(&nd)) != 0)
		return error;

	if (nd.ni_vp->v_type == VREG && nd.ni_vp->v_mount == sc->sc_mount) {
		vrele(nd.ni_vp);
		sc->sc_flags |= FSS_PERSISTENT;

		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fss->fss_bstore, l);
		if ((error = vn_open(&nd, FREAD, 0)) != 0)
			return error;
		sc->sc_bs_vp = nd.ni_vp;

		fsbsize = sc->sc_bs_vp->v_mount->mnt_stat.f_iosize;
		bits = sizeof(sc->sc_bs_bshift)*NBBY;
		for (sc->sc_bs_bshift = 1; sc->sc_bs_bshift < bits;
		    sc->sc_bs_bshift++)
			if (FSS_FSBSIZE(sc) == fsbsize)
				break;
		if (sc->sc_bs_bshift >= bits) {
			VOP_UNLOCK(sc->sc_bs_vp, 0);
			return EINVAL;
		}

		sc->sc_bs_bmask = FSS_FSBSIZE(sc)-1;
		sc->sc_clshift = 0;

		error = VFS_SNAPSHOT(sc->sc_mount, sc->sc_bs_vp, &ts);
		TIMESPEC_TO_TIMEVAL(&sc->sc_time, &ts);

		VOP_UNLOCK(sc->sc_bs_vp, 0);

		return error;
	}
	vrele(nd.ni_vp);

	/*
	 * Get the block device it is mounted on.
	 */

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    sc->sc_mount->mnt_stat.f_mntfromname, l);
	if ((error = namei(&nd)) != 0)
		return error;

	if (nd.ni_vp->v_type != VBLK) {
		vrele(nd.ni_vp);
		return EINVAL;
	}

	error = VOP_IOCTL(nd.ni_vp, DIOCGPART, &dpart, FREAD,
	    l->l_proc->p_ucred, l);
	if (error) {
		vrele(nd.ni_vp);
		return error;
	}

	sc->sc_mount_vp = nd.ni_vp;
	sc->sc_bdev = nd.ni_vp->v_rdev;
	*bsize = (off_t)dpart.disklab->d_secsize*dpart.part->p_size;
	vrele(nd.ni_vp);

	/*
	 * Get the backing store
	 */

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fss->fss_bstore, l);
	if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0)
		return error;
	VOP_UNLOCK(nd.ni_vp, 0);

	sc->sc_bs_vp = nd.ni_vp;

	if (nd.ni_vp->v_type != VREG && nd.ni_vp->v_type != VCHR)
		return EINVAL;

	if (sc->sc_bs_vp->v_type == VREG) {
		error = VOP_GETATTR(sc->sc_bs_vp, &va, l->l_proc->p_ucred, l);
		if (error != 0)
			return error;
		sc->sc_bs_size = va.va_size;
		fsbsize = sc->sc_bs_vp->v_mount->mnt_stat.f_iosize;
		if (fsbsize & (fsbsize-1))	/* No power of two */
			return EINVAL;
		for (sc->sc_bs_bshift = 1; sc->sc_bs_bshift < 32;
		    sc->sc_bs_bshift++)
			if (FSS_FSBSIZE(sc) == fsbsize)
				break;
		if (sc->sc_bs_bshift >= 32)
			return EINVAL;
		sc->sc_bs_bmask = FSS_FSBSIZE(sc)-1;
	} else {
		sc->sc_bs_bshift = DEV_BSHIFT;
		sc->sc_bs_bmask = FSS_FSBSIZE(sc)-1;
	}

	/*
	 * As all IO to from/to the backing store goes through
	 * VOP_STRATEGY() clean the buffer cache to prevent
	 * cache incoherencies.
	 */
	if ((error = vinvalbuf(sc->sc_bs_vp, V_SAVE, l->l_proc->p_ucred, l, 0, 0)) != 0)
		return error;

	return 0;
}

/*
 * Create a snapshot.
 */
static int
fss_create_snapshot(struct fss_softc *sc, struct fss_set *fss, struct lwp *l)
{
	int len, error;
	u_int32_t csize;
	off_t bsize;

	/*
	 * Open needed files.
	 */
	if ((error = fss_create_files(sc, fss, &bsize, l)) != 0)
		goto bad;

	if (sc->sc_flags & FSS_PERSISTENT) {
		fss_softc_alloc(sc);
		sc->sc_flags |= FSS_ACTIVE;
		return 0;
	}

	/*
	 * Set cluster size. Must be a power of two and
	 * a multiple of backing store block size.
	 */
	if (fss->fss_csize <= 0)
		csize = MAXPHYS;
	else
		csize = fss->fss_csize;
	if (bsize/csize > FSS_CLUSTER_MAX)
		csize = bsize/FSS_CLUSTER_MAX+1;

	for (sc->sc_clshift = sc->sc_bs_bshift; sc->sc_clshift < 32;
	    sc->sc_clshift++)
		if (FSS_CLSIZE(sc) >= csize)
			break;
	if (sc->sc_clshift >= 32) {
		error = EINVAL;
		goto bad;
	}
	sc->sc_clmask = FSS_CLSIZE(sc)-1;

	/*
	 * Set number of cache slots.
	 */
	if (FSS_CLSIZE(sc) <= 8192)
		sc->sc_cache_size = 32;
	else if (FSS_CLSIZE(sc) <= 65536)
		sc->sc_cache_size = 8;
	else
		sc->sc_cache_size = 4;

	/*
	 * Set number of clusters and size of last cluster.
	 */
	sc->sc_clcount = FSS_BTOCL(sc, bsize-1)+1;
	sc->sc_clresid = FSS_CLOFF(sc, bsize-1)+1;

	/*
	 * Set size of indirect table.
	 */
	len = sc->sc_clcount*sizeof(u_int32_t);
	sc->sc_indir_size = FSS_BTOCL(sc, len)+1;
	sc->sc_clnext = sc->sc_indir_size;
	sc->sc_indir_cur = 0;

	if ((error = fss_softc_alloc(sc)) != 0)
		goto bad;

	/*
	 * Activate the snapshot.
	 */

	if ((error = vfs_write_suspend(sc->sc_mount, PUSER|PCATCH, 0)) != 0)
		goto bad;

	microtime(&sc->sc_time);

	if (error == 0)
		error = vn_cow_establish(sc->sc_mount_vp,
		    fss_copy_on_write, sc);
	if (error == 0)
		sc->sc_flags |= FSS_ACTIVE;

	vfs_write_resume(sc->sc_mount);

	if (error != 0)
		goto bad;

#ifdef DEBUG
	printf("fss%d: %s snapshot active\n", sc->sc_unit, sc->sc_mntname);
	printf("fss%d: %u clusters of %u, %u cache slots, %u indir clusters\n",
	    sc->sc_unit, sc->sc_clcount, FSS_CLSIZE(sc),
	    sc->sc_cache_size, sc->sc_indir_size);
#endif

	return 0;

bad:
	fss_softc_free(sc);
	if (sc->sc_bs_vp != NULL) {
		if (sc->sc_flags & FSS_PERSISTENT)
			vn_close(sc->sc_bs_vp, FREAD, l->l_proc->p_ucred, l);
		else
			vn_close(sc->sc_bs_vp, FREAD|FWRITE, l->l_proc->p_ucred, l);
	}
	sc->sc_bs_vp = NULL;

	return error;
}

/*
 * Delete a snapshot.
 */
static int
fss_delete_snapshot(struct fss_softc *sc, struct lwp *l)
{
	int s;

	if ((sc->sc_flags & FSS_PERSISTENT) == 0)
		vn_cow_disestablish(sc->sc_mount_vp, fss_copy_on_write, sc);

	FSS_LOCK(sc, s);
	sc->sc_flags &= ~(FSS_ACTIVE|FSS_ERROR);
	sc->sc_mount = NULL;
	sc->sc_bdev = NODEV;
	FSS_UNLOCK(sc, s);

	fss_softc_free(sc);
	if (sc->sc_flags & FSS_PERSISTENT)
		vn_close(sc->sc_bs_vp, FREAD, l->l_proc->p_ucred, l);
	else
		vn_close(sc->sc_bs_vp, FREAD|FWRITE, l->l_proc->p_ucred, l);
	sc->sc_bs_vp = NULL;
	sc->sc_flags &= ~FSS_PERSISTENT;

	FSS_STAT_CLEAR(sc);

	return 0;
}

/*
 * A read from the snapshotted block device has completed.
 */
static void
fss_cluster_iodone(struct buf *bp)
{
	int s;
	struct fss_cache *scp = bp->b_private;

	KASSERT(bp->b_vp == NULL);

	FSS_LOCK(scp->fc_softc, s);

	if (bp->b_flags & B_ERROR)
		fss_error(scp->fc_softc, "fs read error %d", bp->b_error);

	if (--scp->fc_xfercount == 0)
		wakeup(&scp->fc_data);

	FSS_UNLOCK(scp->fc_softc, s);

	putiobuf(bp);
}

/*
 * Read a cluster from the snapshotted block device to the cache.
 */
static void
fss_read_cluster(struct fss_softc *sc, u_int32_t cl)
{
	int s, todo, len;
	caddr_t addr;
	daddr_t dblk;
	struct buf *bp;
	struct fss_cache *scp, *scl;

	/*
	 * Get a free cache slot.
	 */
	scl = sc->sc_cache+sc->sc_cache_size;

	FSS_LOCK(sc, s);

restart:
	if (isset(sc->sc_copied, cl) || !FSS_ISVALID(sc)) {
		FSS_UNLOCK(sc, s);
		return;
	}

	for (scp = sc->sc_cache; scp < scl; scp++)
		if (scp->fc_type != FSS_CACHE_FREE &&
		    scp->fc_cluster == cl) {
			ltsleep(&scp->fc_type, PRIBIO, "cowwait2", 0,
			    &sc->sc_slock);
			goto restart;
		}

	for (scp = sc->sc_cache; scp < scl; scp++)
		if (scp->fc_type == FSS_CACHE_FREE) {
			scp->fc_type = FSS_CACHE_BUSY;
			scp->fc_cluster = cl;
			break;
		}
	if (scp >= scl) {
		FSS_STAT_INC(sc, cow_cache_full);
		ltsleep(&sc->sc_cache, PRIBIO, "cowwait3", 0, &sc->sc_slock);
		goto restart;
	}

	FSS_UNLOCK(sc, s);

	/*
	 * Start the read.
	 */
	FSS_STAT_INC(sc, cow_copied);

	dblk = btodb(FSS_CLTOB(sc, cl));
	addr = scp->fc_data;
	if (cl == sc->sc_clcount-1) {
		todo = sc->sc_clresid;
		memset(addr+todo, 0, FSS_CLSIZE(sc)-todo);
	} else
		todo = FSS_CLSIZE(sc);
	while (todo > 0) {
		len = todo;
		if (len > MAXPHYS)
			len = MAXPHYS;

		bp = getiobuf();
		bp->b_flags = B_READ|B_CALL;
		bp->b_bcount = len;
		bp->b_bufsize = bp->b_bcount;
		bp->b_error = 0;
		bp->b_data = addr;
		bp->b_blkno = dblk;
		bp->b_proc = NULL;
		bp->b_dev = sc->sc_bdev;
		bp->b_vp = NULLVP;
		bp->b_private = scp;
		bp->b_iodone = fss_cluster_iodone;

		DEV_STRATEGY(bp);

		FSS_LOCK(sc, s);
		scp->fc_xfercount++;
		FSS_UNLOCK(sc, s);

		dblk += btodb(len);
		addr += len;
		todo -= len;
	}

	/*
	 * Wait for all read requests to complete.
	 */
	FSS_LOCK(sc, s);
	while (scp->fc_xfercount > 0)
		ltsleep(&scp->fc_data, PRIBIO, "cowwait", 0, &sc->sc_slock);

	scp->fc_type = FSS_CACHE_VALID;
	setbit(sc->sc_copied, scp->fc_cluster);
	FSS_UNLOCK(sc, s);

	wakeup(&sc->sc_bs_proc);
}

/*
 * Read/write clusters from/to backing store.
 * For persistent snapshots must be called with cl == 0. off is the
 * offset into the snapshot.
 */
static int
fss_bs_io(struct fss_softc *sc, fss_io_type rw,
    u_int32_t cl, off_t off, int len, caddr_t data)
{
	int error;

	off += FSS_CLTOB(sc, cl);

	vn_lock(sc->sc_bs_vp, LK_EXCLUSIVE|LK_RETRY);

	error = vn_rdwr((rw == FSS_READ ? UIO_READ : UIO_WRITE), sc->sc_bs_vp,
	    data, len, off, UIO_SYSSPACE, IO_UNIT|IO_NODELOCKED,
	    sc->sc_bs_proc->p_ucred, NULL, NULL);
	if (error == 0) {
		simple_lock(&sc->sc_bs_vp->v_interlock);
		error = VOP_PUTPAGES(sc->sc_bs_vp, trunc_page(off),
		    round_page(off+len), PGO_CLEANIT|PGO_SYNCIO|PGO_FREE);
	}

	VOP_UNLOCK(sc->sc_bs_vp, 0);

	return error;
}

/*
 * Get a pointer to the indirect slot for this cluster.
 */
static u_int32_t *
fss_bs_indir(struct fss_softc *sc, u_int32_t cl)
{
	u_int32_t icl;
	int ioff;

	icl = cl/(FSS_CLSIZE(sc)/sizeof(u_int32_t));
	ioff = cl%(FSS_CLSIZE(sc)/sizeof(u_int32_t));

	if (sc->sc_indir_cur == icl)
		return &sc->sc_indir_data[ioff];

	if (sc->sc_indir_dirty) {
		FSS_STAT_INC(sc, indir_write);
		if (fss_bs_io(sc, FSS_WRITE, sc->sc_indir_cur, 0,
		    FSS_CLSIZE(sc), (caddr_t)sc->sc_indir_data) != 0)
			return NULL;
		setbit(sc->sc_indir_valid, sc->sc_indir_cur);
	}

	sc->sc_indir_dirty = 0;
	sc->sc_indir_cur = icl;

	if (isset(sc->sc_indir_valid, sc->sc_indir_cur)) {
		FSS_STAT_INC(sc, indir_read);
		if (fss_bs_io(sc, FSS_READ, sc->sc_indir_cur, 0,
		    FSS_CLSIZE(sc), (caddr_t)sc->sc_indir_data) != 0)
			return NULL;
	} else
		memset(sc->sc_indir_data, 0, FSS_CLSIZE(sc));

	return &sc->sc_indir_data[ioff];
}

/*
 * The kernel thread (one for every active snapshot).
 *
 * After wakeup it cleans the cache and runs the I/O requests.
 */
static void
fss_bs_thread(void *arg)
{
	int error, len, nfreed, nio, s;
	long off;
	caddr_t addr;
	u_int32_t c, cl, ch, *indirp;
	struct buf *bp, *nbp;
	struct fss_softc *sc;
	struct fss_cache *scp, *scl;

	sc = arg;

	scl = sc->sc_cache+sc->sc_cache_size;

	nbp = getiobuf();

	nfreed = nio = 1;		/* Dont sleep the first time */

	FSS_LOCK(sc, s);

	for (;;) {
		if (nfreed == 0 && nio == 0)
			ltsleep(&sc->sc_bs_proc, PVM-1, "fssbs", 0,
			    &sc->sc_slock);

		if ((sc->sc_flags & FSS_BS_THREAD) == 0) {
			sc->sc_bs_proc = NULL;
			wakeup(&sc->sc_bs_proc);

			FSS_UNLOCK(sc, s);

			putiobuf(nbp);
#ifdef FSS_STATISTICS
			if ((sc->sc_flags & FSS_PERSISTENT) == 0) {
				printf("fss%d: cow called %" PRId64 " times,"
				    " copied %" PRId64 " clusters,"
				    " cache full %" PRId64 " times\n",
				    sc->sc_unit,
				    FSS_STAT_VAL(sc, cow_calls),
				    FSS_STAT_VAL(sc, cow_copied),
				    FSS_STAT_VAL(sc, cow_cache_full));
				printf("fss%d: %" PRId64 " indir reads,"
				    " %" PRId64 " indir writes\n",
				    sc->sc_unit,
				    FSS_STAT_VAL(sc, indir_read),
				    FSS_STAT_VAL(sc, indir_write));
			}
#endif /* FSS_STATISTICS */
			kthread_exit(0);
		}

		/*
		 * Process I/O requests (persistent)
		 */

		if (sc->sc_flags & FSS_PERSISTENT) {
			nfreed = nio = 0;

			if ((bp = BUFQ_GET(sc->sc_bufq)) == NULL)
				continue;

			nio++;

			if (FSS_ISVALID(sc)) {
				FSS_UNLOCK(sc, s);

				error = fss_bs_io(sc, FSS_READ, 0,
				    dbtob(bp->b_blkno), bp->b_bcount,
				    bp->b_data);

				FSS_LOCK(sc, s);
			} else
				error = ENXIO;

			if (error) {
				bp->b_error = error;
				bp->b_flags |= B_ERROR;
				bp->b_resid = bp->b_bcount;
			}
			biodone(bp);

			continue;
		}

		/*
		 * Clean the cache
		 */
		nfreed = 0;
		for (scp = sc->sc_cache; scp < scl; scp++) {
			if (scp->fc_type != FSS_CACHE_VALID)
				continue;

			FSS_UNLOCK(sc, s);

			indirp = fss_bs_indir(sc, scp->fc_cluster);
			if (indirp != NULL) {
				error = fss_bs_io(sc, FSS_WRITE, sc->sc_clnext,
				    0, FSS_CLSIZE(sc), scp->fc_data);
			} else
				error = EIO;

			FSS_LOCK(sc, s);

			if (error == 0) {
				*indirp = sc->sc_clnext++;
				sc->sc_indir_dirty = 1;
			} else
				fss_error(sc, "write bs error %d", error);

			scp->fc_type = FSS_CACHE_FREE;
			nfreed++;
			wakeup(&scp->fc_type);
		}

		if (nfreed)
			wakeup(&sc->sc_cache);

		/*
		 * Process I/O requests
		 */
		nio = 0;

		if ((bp = BUFQ_GET(sc->sc_bufq)) == NULL)
			continue;

		nio++;

		if (!FSS_ISVALID(sc)) {
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}

		/*
		 * First read from the snapshotted block device.
		 * XXX Split to only read those parts that have not
		 * been saved to backing store?
		 */

		FSS_UNLOCK(sc, s);

		BUF_INIT(nbp);
		nbp->b_flags = B_READ;
		nbp->b_bcount = bp->b_bcount;
		nbp->b_bufsize = bp->b_bcount;
		nbp->b_error = 0;
		nbp->b_data = bp->b_data;
		nbp->b_blkno = bp->b_blkno;
		nbp->b_proc = bp->b_proc;
		nbp->b_dev = sc->sc_bdev;
		nbp->b_vp = NULLVP;

		DEV_STRATEGY(nbp);

		if (biowait(nbp) != 0) {
			bp->b_resid = bp->b_bcount;
			bp->b_error = nbp->b_error;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			continue;
		}

		cl = FSS_BTOCL(sc, dbtob(bp->b_blkno));
		off = FSS_CLOFF(sc, dbtob(bp->b_blkno));
		ch = FSS_BTOCL(sc, dbtob(bp->b_blkno)+bp->b_bcount-1);
		bp->b_resid = bp->b_bcount;
		addr = bp->b_data;

		FSS_LOCK(sc, s);

		/*
		 * Replace those parts that have been saved to backing store.
		 */

		for (c = cl; c <= ch;
		    c++, off = 0, bp->b_resid -= len, addr += len) {
			len = FSS_CLSIZE(sc)-off;
			if (len > bp->b_resid)
				len = bp->b_resid;

			if (isclr(sc->sc_copied, c))
				continue;

			FSS_UNLOCK(sc, s);

			indirp = fss_bs_indir(sc, c);

			FSS_LOCK(sc, s);

			if (indirp == NULL || *indirp == 0) {
				/*
				 * Not on backing store. Either in cache
				 * or hole in the snapshotted block device.
				 */
				for (scp = sc->sc_cache; scp < scl; scp++)
					if (scp->fc_type == FSS_CACHE_VALID &&
					    scp->fc_cluster == c)
						break;
				if (scp < scl)
					memcpy(addr, scp->fc_data+off, len);
				else
					memset(addr, 0, len);
				continue;
			}
			/*
			 * Read from backing store.
			 */

			FSS_UNLOCK(sc, s);

			if ((error = fss_bs_io(sc, FSS_READ, *indirp,
			    off, len, addr)) != 0) {
				bp->b_resid = bp->b_bcount;
				bp->b_error = error;
				bp->b_flags |= B_ERROR;
				break;
			}

			FSS_LOCK(sc, s);

		}

		biodone(bp);
	}
}
