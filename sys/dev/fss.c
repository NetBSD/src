/*	$NetBSD: fss.c,v 1.3 2004/01/10 17:16:38 hannken Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fss.c,v 1.3 2004/01/10 17:16:38 hannken Exp $");

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

#include <machine/stdarg.h>

#include "fss.h"
#include <dev/fssvar.h>

#ifdef DEBUG
#include "opt_ddb.h"
#include <ddb/ddbvar.h>
#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#endif

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

typedef enum {
	FSS_READ,
	FSS_WRITE
} fss_io_type;

void fssattach(int);

dev_type_open(fss_open);
dev_type_close(fss_close);
dev_type_read(fss_read);
dev_type_write(fss_write);
dev_type_ioctl(fss_ioctl);
dev_type_strategy(fss_strategy);
dev_type_dump(fss_dump);
dev_type_size(fss_size);

static inline void fss_error(struct fss_softc *, const char *, ...);
static inline int fss_valid(struct fss_softc *);
static int fss_create_snapshot(struct fss_softc *, struct fss_set *,
    struct proc *);
static int fss_delete_snapshot(struct fss_softc *, struct proc *);
static int fss_softc_alloc(struct fss_softc *);
static void fss_softc_free(struct fss_softc *);
static void fss_cluster_iodone(struct buf *);
static void fss_read_cluster(struct fss_softc *, u_int32_t);
static int fss_write_cluster(struct fss_cache *, u_int32_t);
static void fss_bs_thread(void *);
static inline void block_to_cluster(struct fss_softc *, daddr_t, long,
    u_int32_t *, long *);
static int fss_bs_io(struct fss_softc *, fss_io_type,
    u_int32_t, long, int, caddr_t);
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
		lockinit(&sc->sc_lock, PRIBIO, "fsslock", 0, 0);
		bufq_alloc(&sc->sc_bufq, BUFQ_FCFS|BUFQ_SORT_RAWBLOCK);
	}
}

int
fss_open(dev_t dev, int flags, int mode, struct proc *p)
{
	struct fss_softc *sc;

	if (fss_dev_to_softc(dev, &sc) != 0)
		return ENXIO;

	return 0;
}

int
fss_close(dev_t dev, int flags, int mode, struct proc *p)
{
	struct fss_softc *sc;

	if (fss_dev_to_softc(dev, &sc) != 0)
		return ENXIO;

	return 0;
}

void
fss_strategy(struct buf *bp)
{
	struct fss_softc *sc;

	if ((bp->b_flags & B_READ) != B_READ ||
	    fss_dev_to_softc(bp->b_dev, &sc) != 0 ||
	    !fss_valid(sc)) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	bp->b_rawblkno = bp->b_blkno;
	BUFQ_PUT(&sc->sc_bufq, bp);
	wakeup(&sc->sc_bs_proc);
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
fss_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int error;
	struct fss_softc *sc;
	struct fss_set *fss = (struct fss_set *)data;
	struct fss_get *fsg = (struct fss_get *)data;

	if ((error = fss_dev_to_softc(dev, &sc)) != 0)
		return error;

	lockmgr(&sc->sc_lock, LK_EXCLUSIVE, NULL);

	error = EINVAL;

	switch (cmd) {
	case FSSIOCSET:
		if ((flag & FWRITE) == 0)
			error = EPERM;
		else if ((sc->sc_flags & FSS_ACTIVE) != 0)
			error = EBUSY;
		else
			error = fss_create_snapshot(sc, fss, p);
		break;

	case FSSIOCCLR:
		if ((flag & FWRITE) == 0)
			error = EPERM;
		else if ((sc->sc_flags & FSS_ACTIVE) == 0)
			error = ENXIO;
		else
			error = fss_delete_snapshot(sc, p);
		break;

	case FSSIOCGET:
		if ((sc->sc_flags & FSS_ACTIVE) == FSS_ACTIVE) {
			memcpy(fsg->fsg_mount, sc->sc_mntname, MNAMELEN);
			fsg->fsg_csize = sc->sc_clsize;
			fsg->fsg_time = sc->sc_time;
			fsg->fsg_mount_size = sc->sc_clcount;
			fsg->fsg_bs_size = sc->sc_clnext;
			error = 0;
		} else
			error = ENXIO;
		break;
	}

	lockmgr(&sc->sc_lock, LK_RELEASE, NULL);

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
	return ENXIO;
}

/*
 * An error occured reading or writing the snapshot or backing store.
 * If it is the first error log to console.
 */
static inline void
fss_error(struct fss_softc *sc, const char *fmt, ...)
{
	int s;
	va_list ap;

	s = splbio();
	if ((sc->sc_flags & (FSS_ACTIVE|FSS_ERROR)) == FSS_ACTIVE) {
		va_start(ap, fmt);
		printf("fss%d: snapshot invalid: ", sc->sc_unit);
		vprintf(fmt, ap);
		printf("\n");
		va_end(ap);
	}
	if ((sc->sc_flags & FSS_ACTIVE) == FSS_ACTIVE)
		sc->sc_flags |= FSS_ERROR;
	splx(s);
}

/*
 * Check if this snapshot is still valid.
 */
static inline int
fss_valid(struct fss_softc *sc)
{
	int s, valid;

	s = splbio();
	valid = ((sc->sc_flags & (FSS_ACTIVE|FSS_ERROR)) == FSS_ACTIVE);
	splx(s);

	return valid;
}

/*
 * Allocate the variable sized parts of the softc and
 * fork the kernel thread.
 *
 * The fields sc_clcount, sc_clsize, sc_cache_size and sc_indir_size
 * must be initialized.
 */
static int
fss_softc_alloc(struct fss_softc *sc)
{
	int i, len, error;

	len = (sc->sc_clcount+NBBY-1)/NBBY;
	sc->sc_copied = malloc(len, M_TEMP, M_ZERO|M_WAITOK);

	len = sc->sc_cache_size*sizeof(struct fss_cache);
	sc->sc_cache = malloc(len, M_TEMP, M_WAITOK);

	len = sc->sc_clsize;
	for (i = 0; i < sc->sc_cache_size; i++) {
		sc->sc_cache[i].fc_type = FSS_CACHE_FREE;
		sc->sc_cache[i].fc_softc = sc;
		sc->sc_cache[i].fc_xfercount = 0;
		sc->sc_cache[i].fc_data = malloc(len, M_TEMP, M_WAITOK);
	}

	len = (sc->sc_indir_size+NBBY-1)/NBBY;
	sc->sc_indir_valid = malloc(len, M_TEMP, M_ZERO|M_WAITOK);

	len = sc->sc_clsize;
	sc->sc_indir_data = malloc(len, M_TEMP, M_ZERO|M_WAITOK);

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
	int i;

	if ((sc->sc_flags & FSS_BS_THREAD) != 0) {
		sc->sc_flags &= ~FSS_BS_THREAD;
		wakeup(&sc->sc_bs_proc);
		while (sc->sc_bs_proc != NULL)
			tsleep(sc, PRIBIO, "fssthread", 1);
	}

	if (sc->sc_copied != NULL)
		free(sc->sc_copied, M_TEMP);
	sc->sc_copied = NULL;

	if (sc->sc_cache != NULL) {
		for (i = 0; i < sc->sc_cache_size; i++)
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

	s = splbio();
	for (i = 0; i < NFSS; i++)
		if ((fss_softc[i].sc_flags & FSS_ACTIVE) != 0 &&
		    fss_softc[i].sc_mount == mp) {
			if (forced)
				fss_error(&fss_softc[i], "forced unmount");
			else {
				splx(s);
				return EBUSY;
			}
		}
	splx(s);
	return 0;
}

/*
 * A buffer is written to the snapshotted block device. Copy to
 * backing store if needed.
 */
void
fss_copy_on_write(struct fss_softc *sc, struct buf *bp)
{
	u_int32_t cl, ch, c;

#ifdef DIAGNOSTIC
	/*
	 * Buffer written on a suspended file system. This is always an error.
	 */
	if (sc->sc_mount &&
	    (sc->sc_mount->mnt_iflag & IMNT_SUSPENDED) == IMNT_SUSPENDED) {
		printf_nolog("fss%d: write while suspended, %lu@%" PRId64 "\n",
		    sc->sc_unit, bp->b_bcount, bp->b_blkno);
#ifdef DEBUG
		db_stack_trace_print((db_expr_t)__builtin_frame_address(0),
		    TRUE, 65535, "", printf_nolog);
#endif /* DEBUG */
	}
#endif /* DIAGNOSTIC */

	if (!fss_valid(sc))
		return;

	sc->sc_cowcount++;

	FSS_STAT_INC(sc, cow_calls);

	block_to_cluster(sc, bp->b_blkno, 0, &cl, NULL);
	block_to_cluster(sc, bp->b_blkno, bp->b_bcount-1, &ch, NULL);
	for (c = cl; c <= ch; c++) {
		if (!fss_valid(sc))
			break;

		fss_read_cluster(sc, c);
	}

	if (--sc->sc_cowcount == 0)
		wakeup(&sc->sc_cowcount);
}

/*
 * Lookup and open needed files.
 *
 * Returns dev and size of the underlying block device.
 * Initializes the fields sc_mntname, sc_bs_vp, sc_mount and sc_strategy
 */
static int
fss_create_files(struct fss_softc *sc, struct fss_set *fss,
    dev_t *bdev, off_t *bsize, struct proc *p)
{
	int error;
	struct partinfo dpart;
	struct statfs statfs;
	struct nameidata nd;
	const struct bdevsw *bdevsw;

	/*
	 * Get the mounted file system.
	 */

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fss->fss_mount, p);
	if ((error = namei(&nd)) != 0)
		return error;

	if ((nd.ni_vp->v_flag & VROOT) != VROOT) {
		vrele(nd.ni_vp);
		return EINVAL;
	}

	sc->sc_mount = nd.ni_vp->v_mount;

	/*
	 * Get the block device it is mounted on.
	 */

	error = VFS_STATFS(sc->sc_mount, &statfs, p);
	vrele(nd.ni_vp);
	if (error != 0)
		return error;

	memcpy(sc->sc_mntname, statfs.f_mntonname, MNAMELEN); 

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, statfs.f_mntfromname, p);
	if ((error = namei(&nd)) != 0)
		return error;

	if (nd.ni_vp->v_type != VBLK) {
		vrele(nd.ni_vp);
		return EINVAL;
	}

	error = VOP_IOCTL(nd.ni_vp, DIOCGPART, &dpart, FREAD, p->p_ucred, p);
	if (error) {
		vrele(nd.ni_vp);
		return error;
	}

	*bdev = nd.ni_vp->v_rdev;
	*bsize = (off_t)dpart.disklab->d_secsize*dpart.part->p_size;
	vrele(nd.ni_vp);

	if ((bdevsw = bdevsw_lookup(*bdev)) == NULL)
		return EINVAL;
	sc->sc_strategy = bdevsw->d_strategy;

	/*
	 * Get the backing store
	 */

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fss->fss_bstore, p);
	if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0)
		return error;
	VOP_UNLOCK(nd.ni_vp, 0);

	sc->sc_bs_vp = nd.ni_vp;

	if (nd.ni_vp->v_type != VREG && nd.ni_vp->v_type != VCHR)
		return EINVAL;

	if (nd.ni_vp->v_mount == sc->sc_mount)
		return EDEADLK;

	if (sc->sc_bs_vp->v_type == VREG) {
		error = VFS_STATFS(sc->sc_bs_vp->v_mount, &statfs, p);
		if (error != 0)
			return error;
		sc->sc_bs_bsize = statfs.f_iosize;
	} else
		sc->sc_bs_bsize = DEV_BSIZE;

	/*
	 * As all IO to from/to the backing store goes through
	 * VOP_STRATEGY() clean the buffer cache to prevent
	 * cache incoherencies.
	 */
	if ((error = vinvalbuf(sc->sc_bs_vp, V_SAVE, p->p_ucred, p, 0, 0)) != 0)
		return error;

	return 0;
}

/*
 * Create a snapshot.
 */
static int
fss_create_snapshot(struct fss_softc *sc, struct fss_set *fss, struct proc *p)
{
	int len, error;
	dev_t bdev;
	off_t bsize;

	/*
	 * Open needed files.
	 */
	if ((error = fss_create_files(sc, fss, &bdev, &bsize, p)) != 0)
		goto bad;

	/*
	 * Set parameters.
	 */
	if (fss->fss_csize == 0)
		sc->sc_clsize = MAXPHYS;
	else if (fss->fss_csize < 0 || (fss->fss_csize & (DEV_BSIZE-1)) != 0) {
		error = EINVAL;
		goto bad;
	} else if (bsize/fss->fss_csize > FSS_CLUSTER_MAX)
		sc->sc_clsize = (bsize/FSS_CLUSTER_MAX+DEV_BSIZE-1) &
		    ~(DEV_BSIZE-1);
	else
		sc->sc_clsize = fss->fss_csize;

	if (sc->sc_clsize <= 8192)
		sc->sc_cache_size = 32;
	else if (sc->sc_clsize <= 65536)
		sc->sc_cache_size = 8;
	else
		sc->sc_cache_size = 4;

	block_to_cluster(sc, btodb(bsize)-1, DEV_BSIZE-1,
	    &sc->sc_clcount, &sc->sc_cllast);
	sc->sc_clcount += 1;
	sc->sc_cllast += 1;

	len = sc->sc_clcount*sizeof(u_int32_t);
	sc->sc_indir_size = (len+sc->sc_clsize-1)/sc->sc_clsize;
	sc->sc_clnext = sc->sc_indir_size;
	sc->sc_indir_cur = 0;

	if ((error = fss_softc_alloc(sc)) != 0)
		goto bad;

	/* XXX FSSNAP_PREPARE */

	/*
	 * Activate the snapshot.
	 */

	if ((error = vfs_write_suspend(sc->sc_mount, PUSER|PCATCH, 0)) != 0)
		goto bad;

	microtime(&sc->sc_time);

	/* XXX FSSNAP_CREATE */

	if (error == 0) {
		sc->sc_flags |= FSS_ACTIVE;
		sc->sc_bdev = bdev;
	}

	vfs_write_resume(sc->sc_mount);

	if (error != 0)
		goto bad;

#ifdef DEBUG
	printf("fss%d: %s snapshot active\n", sc->sc_unit, sc->sc_mntname);
	printf("fss%d: %u clusters of %u, %u cache slots, %u indir clusters\n",
	    sc->sc_unit, sc->sc_clcount, sc->sc_clsize,
	    sc->sc_cache_size, sc->sc_indir_size);
#endif

	return 0;

bad:
	fss_softc_free(sc);
	if (sc->sc_bs_vp != NULL)
		vn_close(sc->sc_bs_vp, FREAD|FWRITE, p->p_ucred, p);
	sc->sc_bs_vp = NULL;

	return error;
}

/*
 * Delete a snapshot.
 */
static int
fss_delete_snapshot(struct fss_softc *sc, struct proc *p)
{
	int s;

	s = splbio();
	sc->sc_flags &= ~(FSS_ACTIVE|FSS_ERROR);
	splx(s);

	while (sc->sc_cowcount > 0) {
		tsleep(&sc->sc_cowcount, PRIBIO, "cowwait1", 0);
	}

	sc->sc_mount = NULL;
	sc->sc_bdev = NODEV;
	fss_softc_free(sc);
	vn_close(sc->sc_bs_vp, FREAD|FWRITE, p->p_ucred, p);
	sc->sc_bs_vp = NULL;

	/* XXX FSSNAP_DESTROY */

	FSS_STAT_CLEAR(sc);

	return 0;
}

/*
 * Convert disk block with offset to backing store cluster with offset.
 */
static inline void
block_to_cluster(struct fss_softc *sc, daddr_t blkno, long off,
                 u_int32_t *cblk, long *coff)
{
	blkno = dbtob(blkno)+off;
	if (cblk != NULL)
		*cblk = blkno/sc->sc_clsize;
	if (coff != NULL)
		*coff = blkno%sc->sc_clsize;
}

/*
 * A read from the snapshotted block device has completed.
 */
static void
fss_cluster_iodone(struct buf *bp)
{
	int s;
	struct fss_cache *scp = bp->b_private;

	s = splbio();

	if (bp->b_flags & B_EINTR)
		fss_error(scp->fc_softc, "fs read interrupted");
	if (bp->b_flags & B_ERROR)
		fss_error(scp->fc_softc, "fs read error %d", bp->b_error);

	if (bp->b_vp != NULL)
		brelvp(bp);

	if (--scp->fc_xfercount == 0)
		wakeup(&scp->fc_data);

	pool_put(&bufpool, bp);

	splx(s);
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

restart:
	if (isset(sc->sc_copied, cl)) {
		return;
	}

	for (scp = sc->sc_cache; scp < scl; scp++)
		if (scp->fc_type != FSS_CACHE_FREE &&
		    scp->fc_cluster == cl) {
			tsleep(&scp->fc_type, PRIBIO, "cowwait2", 0);
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
		tsleep(&sc->sc_cache, PRIBIO, "cowwait3", 0);
		goto restart;
	}

	/*
	 * Start the read.
	 */
	FSS_STAT_INC(sc, cow_copied);

	dblk = cl*btodb(sc->sc_clsize);
	addr = scp->fc_data;
	if (cl == sc->sc_clcount-1) {
		todo = sc->sc_cllast;
		memset(addr+todo, 0, sc->sc_clsize-todo);
	} else
		todo = sc->sc_clsize;
	while (todo > 0) {
		len = todo;
		if (len > MAXPHYS)
			len = MAXPHYS;

		s = splbio();
		bp = pool_get(&bufpool, PR_WAITOK);
		splx(s);

		BUF_INIT(bp);
		bp->b_flags = B_READ|B_CALL;
		bp->b_bcount = len;
		bp->b_bufsize = bp->b_bcount;
		bp->b_error = 0;
		bp->b_data = addr;
		bp->b_blkno = bp->b_rawblkno = dblk;
		bp->b_proc = NULL;
		bp->b_dev = sc->sc_bdev;
		bp->b_vp = NULLVP;
		bp->b_private = scp;
		bp->b_iodone = fss_cluster_iodone;

		(*sc->sc_strategy)(bp);

		s = splbio();
		scp->fc_xfercount++;
		splx(s);

		dblk += btodb(len);
		addr += len;
		todo -= len;
	}

	/*
	 * Wait for all read requests to complete.
	 */
	s = splbio();
	while (scp->fc_xfercount > 0)
		tsleep(&scp->fc_data, PRIBIO, "cowwait", 0);
	splx(s);

	scp->fc_type = FSS_CACHE_VALID;
	setbit(sc->sc_copied, scp->fc_cluster);
	wakeup(&sc->sc_bs_proc);
}

/*
 * Write a cluster from the cache to the backing store.
 */
static int
fss_write_cluster(struct fss_cache *scp, u_int32_t cl)
{
	int s, error, todo, len, nra;
	daddr_t nbn;
	caddr_t addr;
	off_t pos;
	struct buf *bp;
	struct vnode *vp;
	struct fss_softc *sc;

	error = 0;
	sc = scp->fc_softc;

	pos = (off_t)cl*sc->sc_clsize;
	addr = scp->fc_data;
	todo = sc->sc_clsize;

	while (todo > 0) {
		vn_lock(sc->sc_bs_vp, LK_EXCLUSIVE|LK_RETRY);
		error = VOP_BMAP(sc->sc_bs_vp, pos/sc->sc_bs_bsize,
		    &vp, &nbn, &nra);
		VOP_UNLOCK(sc->sc_bs_vp, 0);
		if (error == 0 && nbn == (daddr_t)-1)
			error = EIO;
		if (error)
			break;

		len = (nra+1)*sc->sc_bs_bsize-pos%sc->sc_bs_bsize;
		if (len > todo)
			len = todo;

		s = splbio();
		bp = pool_get(&bufpool, PR_WAITOK);
		splx(s);

		BUF_INIT(bp);
		bp->b_flags = B_CALL;
		bp->b_bcount = len;
		bp->b_bufsize = bp->b_bcount;
		bp->b_error = 0;
		bp->b_data = addr;
		bp->b_blkno = bp->b_rawblkno = nbn+btodb(pos%sc->sc_bs_bsize);
		bp->b_proc = NULL;
		bp->b_vp = NULLVP;
		bp->b_private = scp;
		bp->b_iodone = fss_cluster_iodone;
		bgetvp(vp, bp);
		bp->b_vp->v_numoutput++;

		VOP_STRATEGY(bp);

		s = splbio();
		scp->fc_xfercount++;
		splx(s);

		pos += len;
		addr += len;
		todo -= len;
	}

	/*
	 * Wait for all write requests to complete.
	 */
	s = splbio();
	while (scp->fc_xfercount > 0)
		tsleep(&scp->fc_data, PRIBIO, "bswwait", 0);
	splx(s);

	return error;
}

/*
 * Read/write clusters from/to backing store.
 */
static int
fss_bs_io(struct fss_softc *sc, fss_io_type rw,
    u_int32_t cl, long off, int len, caddr_t data)
{
	int s, error, todo, count, nra;
	off_t pos;
	daddr_t nbn;
	struct buf *bp;
	struct vnode *vp;

	todo = len;
	pos = (off_t)cl*sc->sc_clsize+off;
	error = 0;

	while (todo > 0) {
		vn_lock(sc->sc_bs_vp, LK_EXCLUSIVE|LK_RETRY);
		error = VOP_BMAP(sc->sc_bs_vp, pos/sc->sc_bs_bsize,
		    &vp, &nbn, &nra);
		VOP_UNLOCK(sc->sc_bs_vp, 0);
		if (error == 0 && nbn == (daddr_t)-1)
			error = EIO;
		if (error)
			break;

		count = (nra+1)*sc->sc_bs_bsize-pos%sc->sc_bs_bsize;
		if (count > todo)
			count = todo;

		s = splbio();
		bp = pool_get(&bufpool, PR_WAITOK);
		splx(s);

		BUF_INIT(bp);
		bp->b_flags = (rw == FSS_READ ? B_READ : 0);
		bp->b_bcount = count;
		bp->b_bufsize = bp->b_bcount;
		bp->b_error = 0;
		bp->b_data = data;
		bp->b_blkno = bp->b_rawblkno = nbn+btodb(pos%sc->sc_bs_bsize);
		bp->b_proc = NULL;
		bp->b_vp = NULLVP;
		bgetvp(vp, bp);
		if ((bp->b_flags & B_READ) == 0)
			bp->b_vp->v_numoutput++;
		VOP_STRATEGY(bp);

		error = biowait(bp);

		if (bp->b_vp != NULL)
			brelvp(bp);

		s = splbio();
		pool_put(&bufpool, bp);
		splx(s);

		if (error)
			break;

		todo -= count;
		data += count;
		pos += count;
	}

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

	icl = cl/(sc->sc_clsize/sizeof(u_int32_t));
	ioff = cl%(sc->sc_clsize/sizeof(u_int32_t));

	if (sc->sc_indir_cur == icl)
		return &sc->sc_indir_data[ioff];

	if (sc->sc_indir_dirty) {
		FSS_STAT_INC(sc, indir_write);
		if (fss_bs_io(sc, FSS_WRITE, sc->sc_indir_cur, 0,
		    sc->sc_clsize, (caddr_t)sc->sc_indir_data) != 0)
			return NULL;
		setbit(sc->sc_indir_valid, sc->sc_indir_cur);
	}

	sc->sc_indir_dirty = 0;
	sc->sc_indir_cur = icl;

	if (isset(sc->sc_indir_valid, sc->sc_indir_cur)) {
		FSS_STAT_INC(sc, indir_read);
		if (fss_bs_io(sc, FSS_READ, sc->sc_indir_cur, 0,
		    sc->sc_clsize, (caddr_t)sc->sc_indir_data) != 0)
			return NULL;
	} else
		memset(sc->sc_indir_data, 0, sc->sc_clsize);

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

	s = splbio();
	nbp = pool_get(&bufpool, PR_WAITOK);
	splx(s);

	nfreed = nio = 1;		/* Dont sleep the first time */

	for (;;) {
		if (nfreed == 0 && nio == 0)
			tsleep(&sc->sc_bs_proc, PVM, "fssbs", 0);
		if ((sc->sc_flags & FSS_BS_THREAD) == 0) {
			s = splbio();
			pool_put(&bufpool, nbp);
			splx(s);
#ifdef FSS_STATISTICS
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
#endif /* FSS_STATISTICS */
			sc->sc_bs_proc = NULL;
			kthread_exit(0);
		}

		/*
		 * Clean the cache
		 */
		nfreed = 0;
		for (scp = sc->sc_cache; scp < scl; scp++) {
			if (scp->fc_type != FSS_CACHE_VALID)
				continue;
			indirp = fss_bs_indir(sc, scp->fc_cluster);
			if (indirp != NULL) {
				error = fss_write_cluster(scp, sc->sc_clnext);
			} else
				error = EIO;

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

		if ((bp = BUFQ_GET(&sc->sc_bufq)) == NULL)
			continue;

		nio++;

		if (!fss_valid(sc)) {
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
		BUF_INIT(nbp);
		nbp->b_flags = B_READ;
		nbp->b_bcount = bp->b_bcount;
		nbp->b_bufsize = bp->b_bcount;
		nbp->b_error = 0;
		nbp->b_data = bp->b_data;
		nbp->b_blkno = nbp->b_rawblkno = bp->b_blkno;
		nbp->b_proc = bp->b_proc;
		nbp->b_dev = sc->sc_bdev;
		nbp->b_vp = NULLVP;

		(*sc->sc_strategy)(nbp);

		if (biowait(nbp) != 0) {
			bp->b_resid = bp->b_bcount;
			bp->b_error = nbp->b_error;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			continue;
		}

		block_to_cluster(sc, bp->b_blkno, 0, &cl, &off);
		block_to_cluster(sc, bp->b_blkno, bp->b_bcount-1, &ch, NULL);
		bp->b_resid = bp->b_bcount;
		addr = bp->b_data;

		/*
		 * Replace those parts that have been saved to backing store.
		 */
		for (c = cl; c <= ch;
		    c++, off = 0, bp->b_resid -= len, addr += len) {
			len = sc->sc_clsize-off;
			if (len > bp->b_resid)
				len = bp->b_resid;

			if (isclr(sc->sc_copied, c))
				continue;

			indirp = fss_bs_indir(sc, c);
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
			if ((error = fss_bs_io(sc, FSS_READ, *indirp,
			    off, len, addr)) != 0) {
				bp->b_resid = bp->b_bcount;
				bp->b_error = error;
				bp->b_flags |= B_ERROR;
				break;
			}
		}

		biodone(bp);
	}
}
