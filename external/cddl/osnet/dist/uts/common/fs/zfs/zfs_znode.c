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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Portions Copyright 2007 Jeremy Teo */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/mntent.h>
#include <sys/u8_textprep.h>
#include <sys/dsl_dataset.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/atomic.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_acl.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_rlock.h>
#include <sys/zfs_fuid.h>
#include <sys/fs/zfs.h>
#include <sys/kidmap.h>
#endif /* _KERNEL */

#include <sys/dmu.h>
#include <sys/refcount.h>
#include <sys/stat.h>
#include <sys/zap.h>
#include <sys/zfs_znode.h>

#include "zfs_prop.h"

#if defined(_KERNEL) && defined(__NetBSD__)
#include <miscfs/specfs/specdev.h>
static const struct genfs_ops zfs_genfsops = {
	.gop_write = genfs_compat_gop_write,
};

#endif

extern int (**zfs_vnodeop_p)(void *);
extern int (**zfs_fifoop_p)(void *);
extern int (**zfs_specop_p)(void *);

/*
 * Define ZNODE_STATS to turn on statistic gathering. By default, it is only
 * turned on when DEBUG is also defined.
 */
#ifdef	DEBUG
#define	ZNODE_STATS
#endif	/* DEBUG */

#ifdef	ZNODE_STATS
#define	ZNODE_STAT_ADD(stat)			((stat)++)
#else
#define	ZNODE_STAT_ADD(stat)			/* nothing */
#endif	/* ZNODE_STATS */

#define	POINTER_IS_VALID(p)	(!((uintptr_t)(p) & 0x3))
#define	POINTER_INVALIDATE(pp)	(*(pp) = (void *)((uintptr_t)(*(pp)) | 0x1))

/*
 * Functions needed for userland (ie: libzpool) are not put under
 * #ifdef_KERNEL; the rest of the functions have dependencies
 * (such as VFS logic) that will not compile easily in userland.
 */
#ifdef _KERNEL
/*
 * Needed to close a small window in zfs_znode_move() that allows the zfsvfs to
 * be freed before it can be safely accessed.
 */
krwlock_t zfsvfs_lock;

static kmem_cache_t *znode_cache = NULL;

/*ARGSUSED*/
static void
znode_evict_error(dmu_buf_t *dbuf, void *user_ptr)
{
	/*
	 * We should never drop all dbuf refs without first clearing
	 * the eviction callback.
	 */
	panic("evicting znode %p\n", user_ptr);
}

/*ARGSUSED*/
static int
zfs_znode_cache_constructor(void *buf, void *arg, int kmflags)
{
	znode_t *zp = arg;

	ASSERT(!POINTER_IS_VALID(zp->z_zfsvfs));

	list_link_init(&zp->z_link_node);

	mutex_init(&zp->z_lock, NULL, MUTEX_DEFAULT, NULL);
	rw_init(&zp->z_parent_lock, NULL, RW_DEFAULT, NULL);
	rw_init(&zp->z_name_lock, NULL, RW_DEFAULT, NULL);
	mutex_init(&zp->z_acl_lock, NULL, MUTEX_DEFAULT, NULL);

	mutex_init(&zp->z_range_lock, NULL, MUTEX_DEFAULT, NULL);
	avl_create(&zp->z_range_avl, zfs_range_compare,
	    sizeof (rl_t), offsetof(rl_t, r_node));

	zp->z_dbuf = NULL;
	zp->z_dirlocks = NULL;
	zp->z_acl_cached = NULL;
	return (0);
}

/*ARGSUSED*/
static void
zfs_znode_cache_destructor(void *buf, void *arg)
{
	znode_t *zp = arg;

	ASSERT(!POINTER_IS_VALID(zp->z_zfsvfs));
	ASSERT(ZTOV(zp) == NULL);
		
	ASSERT(!list_link_active(&zp->z_link_node));
	mutex_destroy(&zp->z_lock);
	rw_destroy(&zp->z_parent_lock);
	rw_destroy(&zp->z_name_lock);
	mutex_destroy(&zp->z_acl_lock);
	avl_destroy(&zp->z_range_avl);
	mutex_destroy(&zp->z_range_lock);

	ASSERT(zp->z_dbuf == NULL);
	ASSERT(zp->z_dirlocks == NULL);
	ASSERT(zp->z_acl_cached == NULL);
}

#ifdef	ZNODE_STATS
static struct {
	uint64_t zms_zfsvfs_invalid;
	uint64_t zms_zfsvfs_recheck1;
	uint64_t zms_zfsvfs_unmounted;
	uint64_t zms_zfsvfs_recheck2;
	uint64_t zms_obj_held;
	uint64_t zms_vnode_locked;
	uint64_t zms_not_only_dnlc;
} znode_move_stats;
#endif	/* ZNODE_STATS */

static void
zfs_znode_move_impl(znode_t *ozp, znode_t *nzp)
{
	vnode_t *vp;

	/* Copy fields. */
	nzp->z_zfsvfs = ozp->z_zfsvfs;

	/* Swap vnodes. */
	vp = nzp->z_vnode;
	nzp->z_vnode = ozp->z_vnode;
	ozp->z_vnode = vp; /* let destructor free the overwritten vnode */
	ZTOV(ozp)->v_data = ozp;
	ZTOV(nzp)->v_data = nzp;

	nzp->z_id = ozp->z_id;
	ASSERT(ozp->z_dirlocks == NULL); /* znode not in use */
	ASSERT(avl_numnodes(&ozp->z_range_avl) == 0);
	nzp->z_unlinked = ozp->z_unlinked;
	nzp->z_atime_dirty = ozp->z_atime_dirty;
	nzp->z_zn_prefetch = ozp->z_zn_prefetch;
	nzp->z_blksz = ozp->z_blksz;
	nzp->z_seq = ozp->z_seq;
	nzp->z_mapcnt = ozp->z_mapcnt;
	nzp->z_last_itx = ozp->z_last_itx;
	nzp->z_gen = ozp->z_gen;
	nzp->z_sync_cnt = ozp->z_sync_cnt;
	nzp->z_phys = ozp->z_phys;
	nzp->z_dbuf = ozp->z_dbuf;

	/*
	 * Since this is just an idle znode and kmem is already dealing with
	 * memory pressure, release any cached ACL.
	 */
	if (ozp->z_acl_cached) {
		zfs_acl_free(ozp->z_acl_cached);
		ozp->z_acl_cached = NULL;
	}

	/* Update back pointers. */
	(void) dmu_buf_update_user(nzp->z_dbuf, ozp, nzp, &nzp->z_phys,
	    znode_evict_error);

	/*
	 * Invalidate the original znode by clearing fields that provide a
	 * pointer back to the znode. Set the low bit of the vfs pointer to
	 * ensure that zfs_znode_move() recognizes the znode as invalid in any
	 * subsequent callback.
	 */
	ozp->z_dbuf = NULL;
	POINTER_INVALIDATE(&ozp->z_zfsvfs);
}

#ifndef __NetBSD__
/*ARGSUSED*/
static kmem_cbrc_t
zfs_znode_move(void *buf, void *newbuf, size_t size, void *arg)
{
	znode_t *ozp = buf, *nzp = newbuf;
	zfsvfs_t *zfsvfs;
	vnode_t *vp;

	/*
	 * The znode is on the file system's list of known znodes if the vfs
	 * pointer is valid. We set the low bit of the vfs pointer when freeing
	 * the znode to invalidate it, and the memory patterns written by kmem
	 * (baddcafe and deadbeef) set at least one of the two low bits. A newly
	 * created znode sets the vfs pointer last of all to indicate that the
	 * znode is known and in a valid state to be moved by this function.
	 */
	zfsvfs = ozp->z_zfsvfs;
	if (!POINTER_IS_VALID(zfsvfs)) {
		ZNODE_STAT_ADD(znode_move_stats.zms_zfsvfs_invalid);
		return (KMEM_CBRC_DONT_KNOW);
	}

	/*
	 * Close a small window in which it's possible that the filesystem could
	 * be unmounted and freed, and zfsvfs, though valid in the previous
	 * statement, could point to unrelated memory by the time we try to
	 * prevent the filesystem from being unmounted.
	 */
	rw_enter(&zfsvfs_lock, RW_WRITER);
	if (zfsvfs != ozp->z_zfsvfs) {
		rw_exit(&zfsvfs_lock);
		ZNODE_STAT_ADD(znode_move_stats.zms_zfsvfs_recheck1);
		return (KMEM_CBRC_DONT_KNOW);
	}

	/*
	 * If the znode is still valid, then so is the file system. We know that
	 * no valid file system can be freed while we hold zfsvfs_lock, so we
	 * can safely ensure that the filesystem is not and will not be
	 * unmounted. The next statement is equivalent to ZFS_ENTER().
	 */
	rrw_enter(&zfsvfs->z_teardown_lock, RW_READER, FTAG);
	if (zfsvfs->z_unmounted) {
		ZFS_EXIT(zfsvfs);
		rw_exit(&zfsvfs_lock);
		ZNODE_STAT_ADD(znode_move_stats.zms_zfsvfs_unmounted);
		return (KMEM_CBRC_DONT_KNOW);
	}
	rw_exit(&zfsvfs_lock);

	mutex_enter(&zfsvfs->z_znodes_lock);
	/*
	 * Recheck the vfs pointer in case the znode was removed just before
	 * acquiring the lock.
	 */
	if (zfsvfs != ozp->z_zfsvfs) {
		mutex_exit(&zfsvfs->z_znodes_lock);
		ZFS_EXIT(zfsvfs);
		ZNODE_STAT_ADD(znode_move_stats.zms_zfsvfs_recheck2);
		return (KMEM_CBRC_DONT_KNOW);
	}

	/*
	 * At this point we know that as long as we hold z_znodes_lock, the
	 * znode cannot be freed and fields within the znode can be safely
	 * accessed. Now, prevent a race with zfs_zget().
	 */
	if (ZFS_OBJ_HOLD_TRYENTER(zfsvfs, ozp->z_id) == 0) {
		mutex_exit(&zfsvfs->z_znodes_lock);
		ZFS_EXIT(zfsvfs);
		ZNODE_STAT_ADD(znode_move_stats.zms_obj_held);
		return (KMEM_CBRC_LATER);
	}

	vp = ZTOV(ozp);
	if (mutex_tryenter(&vp->v_lock) == 0) {
		ZFS_OBJ_HOLD_EXIT(zfsvfs, ozp->z_id);
		mutex_exit(&zfsvfs->z_znodes_lock);
		ZFS_EXIT(zfsvfs);
		ZNODE_STAT_ADD(znode_move_stats.zms_vnode_locked);
		return (KMEM_CBRC_LATER);
	}

	/* Only move znodes that are referenced _only_ by the DNLC. */
	if (vp->v_count != 1 || !vn_in_dnlc(vp)) {
		mutex_exit(&vp->v_lock);
		ZFS_OBJ_HOLD_EXIT(zfsvfs, ozp->z_id);
		mutex_exit(&zfsvfs->z_znodes_lock);
		ZFS_EXIT(zfsvfs);
		ZNODE_STAT_ADD(znode_move_stats.zms_not_only_dnlc);
		return (KMEM_CBRC_LATER);
	}

	/*
	 * The znode is known and in a valid state to move. We're holding the
	 * locks needed to execute the critical section.
	 */
	zfs_znode_move_impl(ozp, nzp);
	mutex_exit(&vp->v_lock);
	ZFS_OBJ_HOLD_EXIT(zfsvfs, ozp->z_id);

	list_link_replace(&ozp->z_link_node, &nzp->z_link_node);
	mutex_exit(&zfsvfs->z_znodes_lock);
	ZFS_EXIT(zfsvfs);

	return (KMEM_CBRC_YES);
}
#endif	/* !__NetBSD__ */

void
zfs_znode_init(void)
{
	/*
	 * Initialize zcache
	 */
	rw_init(&zfsvfs_lock, NULL, RW_DEFAULT, NULL);
	ASSERT(znode_cache == NULL);
	znode_cache = kmem_cache_create("zfs_znode_cache",
	    sizeof (znode_t), 0, zfs_znode_cache_constructor,
	    zfs_znode_cache_destructor, NULL, NULL, NULL, 0);
}

void
zfs_znode_fini(void)
{

	/*
	 * Cleanup zcache
	 */
	if (znode_cache)
		kmem_cache_destroy(znode_cache);
	znode_cache = NULL;
	rw_destroy(&zfsvfs_lock);
}

#ifndef __NetBSD__
struct vnodeops *zfs_dvnodeops;
struct vnodeops *zfs_fvnodeops;
struct vnodeops *zfs_symvnodeops;
struct vnodeops *zfs_xdvnodeops;
struct vnodeops *zfs_evnodeops;
struct vnodeops *zfs_sharevnodeops;
#endif

void
zfs_remove_op_tables()
{
#ifndef __NetBSD__
	/*
	 * Remove vfs ops
	 */
	ASSERT(zfsfstype);
	(void) vfs_freevfsops_by_type(zfsfstype);
	zfsfstype = 0;

	/*
	 * Remove vnode ops
	 */
	if (zfs_dvnodeops)
		vn_freevnodeops(zfs_dvnodeops);
	if (zfs_fvnodeops)
		vn_freevnodeops(zfs_fvnodeops);
	if (zfs_symvnodeops)
		vn_freevnodeops(zfs_symvnodeops);
	if (zfs_xdvnodeops)
		vn_freevnodeops(zfs_xdvnodeops);
	if (zfs_evnodeops)
		vn_freevnodeops(zfs_evnodeops);
	if (zfs_sharevnodeops)
		vn_freevnodeops(zfs_sharevnodeops);

	zfs_dvnodeops = NULL;
	zfs_fvnodeops = NULL;
	zfs_symvnodeops = NULL;
	zfs_xdvnodeops = NULL;
	zfs_evnodeops = NULL;
	zfs_sharevnodeops = NULL;
#endif
}

#ifndef __NetBSD__
extern const fs_operation_def_t zfs_dvnodeops_template[];
extern const fs_operation_def_t zfs_fvnodeops_template[];
extern const fs_operation_def_t zfs_xdvnodeops_template[];
extern const fs_operation_def_t zfs_symvnodeops_template[];
extern const fs_operation_def_t zfs_evnodeops_template[];
extern const fs_operation_def_t zfs_sharevnodeops_template[];
#endif

int
zfs_create_op_tables()
{
#ifndef __NetBSD__
	int error;

	/*
	 * zfs_dvnodeops can be set if mod_remove() calls mod_installfs()
	 * due to a failure to remove the the 2nd modlinkage (zfs_modldrv).
	 * In this case we just return as the ops vectors are already set up.
	 */
	if (zfs_dvnodeops)
		return (0);

	error = vn_make_ops(MNTTYPE_ZFS, zfs_dvnodeops_template,
	    &zfs_dvnodeops);
	if (error)
		return (error);

	error = vn_make_ops(MNTTYPE_ZFS, zfs_fvnodeops_template,
	    &zfs_fvnodeops);
	if (error)
		return (error);

	error = vn_make_ops(MNTTYPE_ZFS, zfs_symvnodeops_template,
	    &zfs_symvnodeops);
	if (error)
		return (error);

	error = vn_make_ops(MNTTYPE_ZFS, zfs_xdvnodeops_template,
	    &zfs_xdvnodeops);
	if (error)
		return (error);

	error = vn_make_ops(MNTTYPE_ZFS, zfs_evnodeops_template,
	    &zfs_evnodeops);
	if (error)
		return (error);

	error = vn_make_ops(MNTTYPE_ZFS, zfs_sharevnodeops_template,
	    &zfs_sharevnodeops);

	return (error);
#endif
	return 0;
}

int
zfs_create_share_dir(zfsvfs_t *zfsvfs, dmu_tx_t *tx)
{
	zfs_acl_ids_t acl_ids;
	vattr_t vattr;
	znode_t *sharezp;
	vnode_t *vp;
	znode_t *zp;
	int error;

	vattr.va_mask = AT_MODE|AT_UID|AT_GID|AT_TYPE;
	vattr.va_type = VDIR;
	vattr.va_mode = S_IFDIR|0555;
	vattr.va_uid = crgetuid(kcred);
	vattr.va_gid = crgetgid(kcred);

	sharezp = kmem_cache_alloc(znode_cache, KM_SLEEP);
	sharezp->z_unlinked = 0;
	sharezp->z_atime_dirty = 0;
	sharezp->z_zfsvfs = zfsvfs;

	vp = ZTOV(sharezp);
	error = getnewvnode(VT_ZFS, zfsvfs->z_parent->z_vfs,
	    zfs_vnodeop_p, NULL, &sharezp->z_vnode);
	if (error) {
		kmem_cache_free(znode_cache, sharezp);
		return error;
	}
	vp->v_type = VDIR;

	VERIFY(0 == zfs_acl_ids_create(sharezp, IS_ROOT_NODE, &vattr,
	    kcred, NULL, &acl_ids));
	zfs_mknode(sharezp, &vattr, tx, kcred, IS_ROOT_NODE,
	    &zp, 0, &acl_ids);
	ASSERT3P(zp, ==, sharezp);
	ASSERT(!vn_in_dnlc(ZTOV(sharezp))); /* not valid to move */
	POINTER_INVALIDATE(&sharezp->z_zfsvfs);
	error = zap_add(zfsvfs->z_os, MASTER_NODE_OBJ,
	    ZFS_SHARES_DIR, 8, 1, &sharezp->z_id, tx);
	zfsvfs->z_shares_dir = sharezp->z_id;

	zfs_acl_ids_free(&acl_ids);
	ZTOV(sharezp)->v_count = 0;
	dmu_buf_rele(sharezp->z_dbuf, NULL);
	sharezp->z_dbuf = NULL;
	kmem_cache_free(znode_cache, sharezp);

	return (error);
}

/*
 * define a couple of values we need available
 * for both 64 and 32 bit environments.
 */
#ifndef NBITSMINOR64
#define	NBITSMINOR64	32
#endif
#ifndef MAXMAJ64
#define	MAXMAJ64	0xffffffffUL
#endif
#ifndef	MAXMIN64
#define	MAXMIN64	0xffffffffUL
#endif

/*
 * Create special expldev for ZFS private use.
 * Can't use standard expldev since it doesn't do
 * what we want.  The standard expldev() takes a
 * dev32_t in LP64 and expands it to a long dev_t.
 * We need an interface that takes a dev32_t in ILP32
 * and expands it to a long dev_t.
 */
static uint64_t
zfs_expldev(dev_t dev)
{
	return ((uint64_t)major(dev) << NBITSMINOR64) |
	    (minor_t)minor(dev);
}

/*
 * Special cmpldev for ZFS private use.
 * Can't use standard cmpldev since it takes
 * a long dev_t and compresses it to dev32_t in
 * LP64.  We need to do a compaction of a long dev_t
 * to a dev32_t in ILP32.
 */
dev_t
zfs_cmpldev(uint64_t dev)
{
	minor_t minor = (minor_t)dev & MAXMIN64;
	major_t major = (major_t)(dev >> NBITSMINOR64) & MAXMAJ64;

	return makedev(minor, major);
}

static void
zfs_znode_dmu_init(zfsvfs_t *zfsvfs, znode_t *zp, dmu_buf_t *db)
{
	znode_t		*nzp;

	ASSERT(!POINTER_IS_VALID(zp->z_zfsvfs) || (zfsvfs == zp->z_zfsvfs));
	ASSERT(MUTEX_HELD(ZFS_OBJ_MUTEX(zfsvfs, zp->z_id)));

	mutex_enter(&zp->z_lock);

	ASSERT(zp->z_dbuf == NULL);
	ASSERT(zp->z_acl_cached == NULL);
	zp->z_dbuf = db;
	nzp = dmu_buf_set_user_ie(db, zp, &zp->z_phys, znode_evict_error);

	/*
	 * there should be no
	 * concurrent zgets on this object.
	 */
	if (nzp != NULL)
		panic("existing znode %p for dbuf %p", (void *)nzp, (void *)db);

	/*
	 * Slap on VROOT if we are the root znode
	 */
	if (zp->z_id == zfsvfs->z_root)
		ZTOV(zp)->v_flag |= VROOT;

	mutex_exit(&zp->z_lock);
	vn_exists(ZTOV(zp));
}

void
zfs_znode_dmu_fini(znode_t *zp)
{
	dmu_buf_t *db = zp->z_dbuf;
	ASSERT(MUTEX_HELD(ZFS_OBJ_MUTEX(zp->z_zfsvfs, zp->z_id)) ||
	    zp->z_unlinked ||
	    RW_WRITE_HELD(&zp->z_zfsvfs->z_teardown_inactive_lock));
	ASSERT(zp->z_dbuf != NULL);
	zp->z_dbuf = NULL;
	VERIFY(zp == dmu_buf_update_user(db, zp, NULL, NULL, NULL));
	dmu_buf_rele(db, NULL);
}

/*
 * Construct a new znode/vnode and intialize.
 *
 * This does not do a call to dmu_set_user() that is
 * up to the caller to do, in case you don't want to
 * return the znode
 */

static znode_t *
zfs_znode_alloc(zfsvfs_t *zfsvfs, dmu_buf_t *db, int blksz)
{
	znode_t	*zp;
	vnode_t *vp;
	int error;

	zp = kmem_cache_alloc(znode_cache, KM_SLEEP);
	for (;;) {

		error = getnewvnode(VT_ZFS, zfsvfs->z_parent->z_vfs,
		    zfs_vnodeop_p, NULL, &zp->z_vnode);
		if (__predict_true(error == 0))
			break;
		printf("WARNING: zfs_znode_alloc: unable to get vnode, "
		    "error=%d\n", error);
		(void)kpause("zfsnewvn", false, hz, NULL);
	}
	
	ASSERT(zp->z_dirlocks == NULL);
	ASSERT(zp->z_dbuf == NULL);
	ASSERT(!POINTER_IS_VALID(zp->z_zfsvfs));

	/*
	 * Defer setting z_zfsvfs until the znode is ready to be a candidate for
	 * the zfs_znode_move() callback.
	 */
	zp->z_phys = NULL;
	zp->z_unlinked = 0;
	zp->z_atime_dirty = 0;
	zp->z_mapcnt = 0;
	zp->z_last_itx = 0;
	zp->z_id = db->db_object;
	zp->z_blksz = blksz;
	zp->z_seq = 0x7A4653;
	zp->z_sync_cnt = 0;

	vp = ZTOV(zp);

	zfs_znode_dmu_init(zfsvfs, zp, db);

	zp->z_gen = zp->z_phys->zp_gen;

	vp->v_vfsp = zfsvfs->z_parent->z_vfs;
	vp->v_type = IFTOVT((mode_t)zp->z_phys->zp_mode);
	vp->v_data = zp;
	switch (vp->v_type) {
	case VDIR:
		zp->z_zn_prefetch = B_TRUE; /* z_prefetch default is enabled */
		break;
	case VBLK:
	case VCHR:
	/* XXX NetBSD	vp->v_op = zfs_specop_p; */
		spec_node_init(vp, zfs_cmpldev(zp->z_phys->zp_rdev));
		break;
	case VFIFO:
		/* XXX NetBSD vp->v_op = zfs_fifoop_p; */
		break;
	}

	dprintf("zfs_znode_alloc znode %p -- vnode %p\n", zp, vp);
	dprintf("zfs_znode_alloc z_id %ld\n", zp->z_id);
	//cpu_Debugger();
	
	uvm_vnp_setsize(vp, zp->z_phys->zp_size);

	mutex_enter(&zfsvfs->z_znodes_lock);
	list_insert_tail(&zfsvfs->z_all_znodes, zp);
	membar_producer();
	/*
	 * Everything else must be valid before assigning z_zfsvfs makes the
	 * znode eligible for zfs_znode_move().
	 */
	zp->z_zfsvfs = zfsvfs;
	mutex_exit(&zfsvfs->z_znodes_lock);

	return (zp);
}

/*
 * Create a new DMU object to hold a zfs znode.
 *
 *	IN:	dzp	- parent directory for new znode
 *		vap	- file attributes for new znode
 *		tx	- dmu transaction id for zap operations
 *		cr	- credentials of caller
 *		flag	- flags:
 *			  IS_ROOT_NODE	- new object will be root
 *			  IS_XATTR	- new object is an attribute
 *		bonuslen - length of bonus buffer
 *		setaclp  - File/Dir initial ACL
 *		fuidp	 - Tracks fuid allocation.
 *
 *	OUT:	zpp	- allocated znode
 *
 */
void
zfs_mknode(znode_t *dzp, vattr_t *vap, dmu_tx_t *tx, cred_t *cr,
    uint_t flag, znode_t **zpp, int bonuslen, zfs_acl_ids_t *acl_ids)
{
	dmu_buf_t	*db;
	znode_phys_t	*pzp;
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	timestruc_t	now;
	uint64_t	gen, obj;
	int		err;

	ASSERT(vap && (vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	if (zfsvfs->z_replay) {
		obj = vap->va_nodeid;
		now = vap->va_ctime;		/* see zfs_replay_create() */
		gen = vap->va_nblocks;		/* ditto */
	} else {
		obj = 0;
		gethrestime(&now);
		gen = dmu_tx_get_txg(tx);
	}

	/*
	 * Create a new DMU object.
	 */
	/*
	 * There's currently no mechanism for pre-reading the blocks that will
	 * be to needed allocate a new object, so we accept the small chance
	 * that there will be an i/o error and we will fail one of the
	 * assertions below.
	 */
	if (vap->va_type == VDIR) {
		if (zfsvfs->z_replay) {
			err = zap_create_claim_norm(zfsvfs->z_os, obj,
			    zfsvfs->z_norm, DMU_OT_DIRECTORY_CONTENTS,
			    DMU_OT_ZNODE, sizeof (znode_phys_t) + bonuslen, tx);
			ASSERT3U(err, ==, 0);
		} else {
			obj = zap_create_norm(zfsvfs->z_os,
			    zfsvfs->z_norm, DMU_OT_DIRECTORY_CONTENTS,
			    DMU_OT_ZNODE, sizeof (znode_phys_t) + bonuslen, tx);
		}
	} else {
		if (zfsvfs->z_replay) {
			err = dmu_object_claim(zfsvfs->z_os, obj,
			    DMU_OT_PLAIN_FILE_CONTENTS, 0,
			    DMU_OT_ZNODE, sizeof (znode_phys_t) + bonuslen, tx);
			ASSERT3U(err, ==, 0);
		} else {
			obj = dmu_object_alloc(zfsvfs->z_os,
			    DMU_OT_PLAIN_FILE_CONTENTS, 0,
			    DMU_OT_ZNODE, sizeof (znode_phys_t) + bonuslen, tx);
		}
	}

	ZFS_OBJ_HOLD_ENTER(zfsvfs, obj);
	VERIFY(0 == dmu_bonus_hold(zfsvfs->z_os, obj, NULL, &db));
	dmu_buf_will_dirty(db, tx);

	/*
	 * Initialize the znode physical data to zero.
	 */
	ASSERT(db->db_size >= sizeof (znode_phys_t));
	bzero(db->db_data, db->db_size);
	pzp = db->db_data;

	/*
	 * If this is the root, fix up the half-initialized parent pointer
	 * to reference the just-allocated physical data area.
	 */
	if (flag & IS_ROOT_NODE) {
		dzp->z_dbuf = db;
		dzp->z_phys = pzp;
		dzp->z_id = obj;
	}

	/*
	 * If parent is an xattr, so am I.
	 */
	if (dzp->z_phys->zp_flags & ZFS_XATTR)
		flag |= IS_XATTR;

	if (vap->va_type == VBLK || vap->va_type == VCHR) {
		pzp->zp_rdev = zfs_expldev(vap->va_rdev);
	}

	if (zfsvfs->z_use_fuids)
		pzp->zp_flags = ZFS_ARCHIVE | ZFS_AV_MODIFIED;

	if (vap->va_type == VDIR) {
		pzp->zp_size = 2;		/* contents ("." and "..") */
		pzp->zp_links = (flag & (IS_ROOT_NODE | IS_XATTR)) ? 2 : 1;
	}

	pzp->zp_parent = dzp->z_id;
	if (flag & IS_XATTR)
		pzp->zp_flags |= ZFS_XATTR;

	pzp->zp_gen = gen;

	ZFS_TIME_ENCODE(&now, pzp->zp_crtime);
	ZFS_TIME_ENCODE(&now, pzp->zp_ctime);

	if (vap->va_mask & AT_ATIME) {
		ZFS_TIME_ENCODE(&vap->va_atime, pzp->zp_atime);
	} else {
		ZFS_TIME_ENCODE(&now, pzp->zp_atime);
	}

	if (vap->va_mask & AT_MTIME) {
		ZFS_TIME_ENCODE(&vap->va_mtime, pzp->zp_mtime);
	} else {
		ZFS_TIME_ENCODE(&now, pzp->zp_mtime);
	}
	pzp->zp_uid = acl_ids->z_fuid;
	pzp->zp_gid = acl_ids->z_fgid;
	pzp->zp_mode = acl_ids->z_mode;
	if (!(flag & IS_ROOT_NODE)) {
		*zpp = zfs_znode_alloc(zfsvfs, db, 0);
	} else {
		/*
		 * If we are creating the root node, the "parent" we
		 * passed in is the znode for the root.
		 */
		*zpp = dzp;
	}
	VERIFY(0 == zfs_aclset_common(*zpp, acl_ids->z_aclp, cr, tx));
	if (vap->va_mask & AT_XVATTR)
		zfs_xvattr_set(*zpp, (xvattr_t *)vap);

	ZFS_OBJ_HOLD_EXIT(zfsvfs, obj);
}

void
zfs_xvattr_set(znode_t *zp, xvattr_t *xvap)
{
	xoptattr_t *xoap;

	xoap = xva_getxoptattr(xvap);
	ASSERT(xoap);

	if (XVA_ISSET_REQ(xvap, XAT_CREATETIME)) {
		ZFS_TIME_ENCODE(&xoap->xoa_createtime, zp->z_phys->zp_crtime);
		XVA_SET_RTN(xvap, XAT_CREATETIME);
	}
	if (XVA_ISSET_REQ(xvap, XAT_READONLY)) {
		ZFS_ATTR_SET(zp, ZFS_READONLY, xoap->xoa_readonly);
		XVA_SET_RTN(xvap, XAT_READONLY);
	}
	if (XVA_ISSET_REQ(xvap, XAT_HIDDEN)) {
		ZFS_ATTR_SET(zp, ZFS_HIDDEN, xoap->xoa_hidden);
		XVA_SET_RTN(xvap, XAT_HIDDEN);
	}
	if (XVA_ISSET_REQ(xvap, XAT_SYSTEM)) {
		ZFS_ATTR_SET(zp, ZFS_SYSTEM, xoap->xoa_system);
		XVA_SET_RTN(xvap, XAT_SYSTEM);
	}
	if (XVA_ISSET_REQ(xvap, XAT_ARCHIVE)) {
		ZFS_ATTR_SET(zp, ZFS_ARCHIVE, xoap->xoa_archive);
		XVA_SET_RTN(xvap, XAT_ARCHIVE);
	}
	if (XVA_ISSET_REQ(xvap, XAT_IMMUTABLE)) {
		ZFS_ATTR_SET(zp, ZFS_IMMUTABLE, xoap->xoa_immutable);
		XVA_SET_RTN(xvap, XAT_IMMUTABLE);
	}
	if (XVA_ISSET_REQ(xvap, XAT_NOUNLINK)) {
		ZFS_ATTR_SET(zp, ZFS_NOUNLINK, xoap->xoa_nounlink);
		XVA_SET_RTN(xvap, XAT_NOUNLINK);
	}
	if (XVA_ISSET_REQ(xvap, XAT_APPENDONLY)) {
		ZFS_ATTR_SET(zp, ZFS_APPENDONLY, xoap->xoa_appendonly);
		XVA_SET_RTN(xvap, XAT_APPENDONLY);
	}
	if (XVA_ISSET_REQ(xvap, XAT_NODUMP)) {
		ZFS_ATTR_SET(zp, ZFS_NODUMP, xoap->xoa_nodump);
		XVA_SET_RTN(xvap, XAT_NODUMP);
	}
	if (XVA_ISSET_REQ(xvap, XAT_OPAQUE)) {
		ZFS_ATTR_SET(zp, ZFS_OPAQUE, xoap->xoa_opaque);
		XVA_SET_RTN(xvap, XAT_OPAQUE);
	}
	if (XVA_ISSET_REQ(xvap, XAT_AV_QUARANTINED)) {
		ZFS_ATTR_SET(zp, ZFS_AV_QUARANTINED,
		    xoap->xoa_av_quarantined);
		XVA_SET_RTN(xvap, XAT_AV_QUARANTINED);
	}
	if (XVA_ISSET_REQ(xvap, XAT_AV_MODIFIED)) {
		ZFS_ATTR_SET(zp, ZFS_AV_MODIFIED, xoap->xoa_av_modified);
		XVA_SET_RTN(xvap, XAT_AV_MODIFIED);
	}
	if (XVA_ISSET_REQ(xvap, XAT_AV_SCANSTAMP)) {
		(void) memcpy(zp->z_phys + 1, xoap->xoa_av_scanstamp,
		    sizeof (xoap->xoa_av_scanstamp));
		zp->z_phys->zp_flags |= ZFS_BONUS_SCANSTAMP;
		XVA_SET_RTN(xvap, XAT_AV_SCANSTAMP);
	}
	if (XVA_ISSET_REQ(xvap, XAT_REPARSE)) {
		ZFS_ATTR_SET(zp, ZFS_REPARSE, xoap->xoa_reparse);
		XVA_SET_RTN(xvap, XAT_REPARSE);
	}
}

int
zfs_zget(zfsvfs_t *zfsvfs, uint64_t obj_num, znode_t **zpp)
{
	dmu_object_info_t doi;
	dmu_buf_t   *db;
	znode_t     *zp;
	vnode_t     *vp;
	int err, first = 1;

	*zpp = NULL;
again:
	ZFS_OBJ_HOLD_ENTER(zfsvfs, obj_num);

	err = dmu_bonus_hold(zfsvfs->z_os, obj_num, NULL, &db);
	if (err) {
		ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
		return (err);
	}

	dmu_object_info_from_db(db, &doi);
	if (doi.doi_bonus_type != DMU_OT_ZNODE ||
	    doi.doi_bonus_size < sizeof (znode_phys_t)) {
		dmu_buf_rele(db, NULL);
		ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
		return (EINVAL);
	}

	zp = dmu_buf_get_user(db);
	if (zp != NULL) {
		mutex_enter(&zp->z_lock);

		/*
		 * Since we do immediate eviction of the z_dbuf, we
		 * should never find a dbuf with a znode that doesn't
		 * know about the dbuf.
		 */
		ASSERT3P(zp->z_dbuf, ==, db);
		ASSERT3U(zp->z_id, ==, obj_num);
		if (zp->z_unlinked) {
			err = ENOENT;
		} else {
			if ((vp = ZTOV(zp)) != NULL) {
				mutex_enter(vp->v_interlock);
				mutex_exit(&zp->z_lock);
				if (vget(vp, 0) != 0) {
					dmu_buf_rele(db, NULL);
					mutex_exit(vp->v_interlock);
					goto again;
				}
				mutex_enter(&zp->z_lock);
			} else {
				if (first) {
					ZFS_LOG(1, "dying znode detected (zp=%p)", zp);
					first = 0;
				}
				/*
				 * znode is dying so we can't reuse it, we must
				 * wait until destruction is completed.
				 */
				dmu_buf_rele(db, NULL);
				mutex_exit(&zp->z_lock);
				ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
				kpause("zcollide", 0, 1, NULL);
				goto again;
			}
			*zpp = zp;
			err = 0;
		}
		
		dmu_buf_rele(db, NULL);
		mutex_exit(&zp->z_lock);
		ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
		return (err);
	}

	/*
	 * Not found create new znode/vnode
	 * but only if file exists.
	 *
	 * There is a small window where zfs_vget() could
	 * find this object while a file create is still in
	 * progress.  Since a gen number can never be zero
	 * we will check that to determine if its an allocated
	 * file.
	 */

	if (((znode_phys_t *)db->db_data)->zp_gen != 0) {
		zp = zfs_znode_alloc(zfsvfs, db, doi.doi_data_block_size);
		*zpp = zp;

		vp = ZTOV(zp);
		genfs_node_init(vp, &zfs_genfsops);
		VOP_UNLOCK(vp);

		err = 0;
	} else {
		dmu_buf_rele(db, NULL);
		err = ENOENT;
	}
	ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
	return (err);
}

int
zfs_rezget(znode_t *zp)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	dmu_object_info_t doi;
	dmu_buf_t *db;
	uint64_t obj_num = zp->z_id;
	int err;

	ZFS_OBJ_HOLD_ENTER(zfsvfs, obj_num);

	err = dmu_bonus_hold(zfsvfs->z_os, obj_num, NULL, &db);
	if (err) {
		ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
		return (err);
	}

	dmu_object_info_from_db(db, &doi);
	if (doi.doi_bonus_type != DMU_OT_ZNODE ||
	    doi.doi_bonus_size < sizeof (znode_phys_t)) {
		dmu_buf_rele(db, NULL);
		ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
		return (EINVAL);
	}

	if (((znode_phys_t *)db->db_data)->zp_gen != zp->z_gen) {
		dmu_buf_rele(db, NULL);
		ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);
		return (EIO);
	}

	mutex_enter(&zp->z_acl_lock);
	if (zp->z_acl_cached) {
		zfs_acl_free(zp->z_acl_cached);
		zp->z_acl_cached = NULL;
	}
	mutex_exit(&zp->z_acl_lock);

	zfs_znode_dmu_init(zfsvfs, zp, db);
	zp->z_unlinked = (zp->z_phys->zp_links == 0);
	zp->z_blksz = doi.doi_data_block_size;

	ZFS_OBJ_HOLD_EXIT(zfsvfs, obj_num);

	return (0);
}

void
zfs_znode_delete(znode_t *zp, dmu_tx_t *tx)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	objset_t *os = zfsvfs->z_os;
	uint64_t obj = zp->z_id;
	uint64_t acl_obj = zp->z_phys->zp_acl.z_acl_extern_obj;

	ZFS_OBJ_HOLD_ENTER(zfsvfs, obj);
	if (acl_obj)
		VERIFY(0 == dmu_object_free(os, acl_obj, tx));
	VERIFY(0 == dmu_object_free(os, obj, tx));
	zfs_znode_dmu_fini(zp);
	ZFS_OBJ_HOLD_EXIT(zfsvfs, obj);
	zfs_znode_free(zp);
}

/*
 * zfs_zinactive must be called with ZFS_OBJ_HOLD_ENTER held. And this lock
 * will be released in zfs_zinactive.
 */
void
zfs_zinactive(znode_t *zp)
{
	vnode_t	*vp = ZTOV(zp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	uint64_t z_id = zp->z_id;
	
	ASSERT(zp->z_dbuf && zp->z_phys);

	/*
	 * Don't allow a zfs_zget() while were trying to release this znode
	 */
	ZFS_OBJ_HOLD_ENTER(zfsvfs, z_id);

	mutex_enter(&zp->z_lock);
	/*
	 * If this was the last reference to a file with no links,
	 * remove the file from the file system.
	 */
	if (zp->z_unlinked) {
		mutex_exit(&zp->z_lock);
		ZFS_OBJ_HOLD_EXIT(zfsvfs, z_id);
		zfs_rmnode(zp);
		return;
	}

	mutex_exit(&zp->z_lock);
	/* XXX why disabled zfs_znode_dmu_fini(zp); */
	ZFS_OBJ_HOLD_EXIT(zfsvfs, z_id);
	zfs_znode_free(zp);
}

void
zfs_znode_free(znode_t *zp)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	ASSERT(ZTOV(zp) == NULL);

	dprintf("destroying znode %p\n", zp);
	//cpu_Debugger();
	mutex_enter(&zfsvfs->z_znodes_lock);
	POINTER_INVALIDATE(&zp->z_zfsvfs);
	list_remove(&zfsvfs->z_all_znodes, zp);
	mutex_exit(&zfsvfs->z_znodes_lock);

	if (zp->z_acl_cached) {
		zfs_acl_free(zp->z_acl_cached);
		zp->z_acl_cached = NULL;
	}

	kmem_cache_free(znode_cache, zp);

	VFS_RELE(zfsvfs->z_vfs);
}

void
zfs_time_stamper_locked(znode_t *zp, uint_t flag, dmu_tx_t *tx)
{
	timestruc_t	now;

	ASSERT(MUTEX_HELD(&zp->z_lock));

	gethrestime(&now);

	if (tx) {
		dmu_buf_will_dirty(zp->z_dbuf, tx);
		zp->z_atime_dirty = 0;
		zp->z_seq++;
	} else {
		zp->z_atime_dirty = 1;
	}

	if (flag & AT_ATIME)
		ZFS_TIME_ENCODE(&now, zp->z_phys->zp_atime);

	if (flag & AT_MTIME) {
		ZFS_TIME_ENCODE(&now, zp->z_phys->zp_mtime);
		if (zp->z_zfsvfs->z_use_fuids)
			zp->z_phys->zp_flags |= (ZFS_ARCHIVE | ZFS_AV_MODIFIED);
	}

	if (flag & AT_CTIME) {
		ZFS_TIME_ENCODE(&now, zp->z_phys->zp_ctime);
		if (zp->z_zfsvfs->z_use_fuids)
			zp->z_phys->zp_flags |= ZFS_ARCHIVE;
	}
}

/*
 * Update the requested znode timestamps with the current time.
 * If we are in a transaction, then go ahead and mark the znode
 * dirty in the transaction so the timestamps will go to disk.
 * Otherwise, we will get pushed next time the znode is updated
 * in a transaction, or when this znode eventually goes inactive.
 *
 * Why is this OK?
 *  1 - Only the ACCESS time is ever updated outside of a transaction.
 *  2 - Multiple consecutive updates will be collapsed into a single
 *	znode update by the transaction grouping semantics of the DMU.
 */
void
zfs_time_stamper(znode_t *zp, uint_t flag, dmu_tx_t *tx)
{
	mutex_enter(&zp->z_lock);
	zfs_time_stamper_locked(zp, flag, tx);
	mutex_exit(&zp->z_lock);
}

/*
 * Grow the block size for a file.
 *
 *	IN:	zp	- znode of file to free data in.
 *		size	- requested block size
 *		tx	- open transaction.
 *
 * NOTE: this function assumes that the znode is write locked.
 */
void
zfs_grow_blocksize(znode_t *zp, uint64_t size, dmu_tx_t *tx)
{
	int		error;
	u_longlong_t	dummy;

	if (size <= zp->z_blksz)
		return;
	/*
	 * If the file size is already greater than the current blocksize,
	 * we will not grow.  If there is more than one block in a file,
	 * the blocksize cannot change.
	 */
	if (zp->z_blksz && zp->z_phys->zp_size > zp->z_blksz)
		return;

	error = dmu_object_set_blocksize(zp->z_zfsvfs->z_os, zp->z_id,
	    size, 0, tx);
	if (error == ENOTSUP)
		return;
	ASSERT3U(error, ==, 0);

	/* What blocksize did we actually get? */
	dmu_object_size_from_db(zp->z_dbuf, &zp->z_blksz, &dummy);
}

/*
 * Increase the file length
 *
 *	IN:	zp	- znode of file to free data in.
 *		end	- new end-of-file
 *
 * 	RETURN:	0 if success
 *		error code if failure
 */
static int
zfs_extend(znode_t *zp, uint64_t end)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	dmu_tx_t *tx;
	rl_t *rl;
	uint64_t newblksz;
	int error;

	/*
	 * We will change zp_size, lock the whole file.
	 */
	rl = zfs_range_lock(zp, 0, UINT64_MAX, RL_WRITER);

	/*
	 * Nothing to do if file already at desired length.
	 */
	if (end <= zp->z_phys->zp_size) {
		zfs_range_unlock(rl);
		return (0);
	}
top:
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_bonus(tx, zp->z_id);
	if (end > zp->z_blksz &&
	    (!ISP2(zp->z_blksz) || zp->z_blksz < zfsvfs->z_max_blksz)) {
		/*
		 * We are growing the file past the current block size.
		 */
		if (zp->z_blksz > zp->z_zfsvfs->z_max_blksz) {
			ASSERT(!ISP2(zp->z_blksz));
			newblksz = MIN(end, SPA_MAXBLOCKSIZE);
		} else {
			newblksz = MIN(end, zp->z_zfsvfs->z_max_blksz);
		}
		dmu_tx_hold_write(tx, zp->z_id, 0, newblksz);
	} else {
		newblksz = 0;
	}

	error = dmu_tx_assign(tx, TXG_NOWAIT);
	if (error) {
		if (error == ERESTART) {
			dmu_tx_wait(tx);
			dmu_tx_abort(tx);
			goto top;
		}
		dmu_tx_abort(tx);
		zfs_range_unlock(rl);
		return (error);
	}
	dmu_buf_will_dirty(zp->z_dbuf, tx);

	if (newblksz)
		zfs_grow_blocksize(zp, newblksz, tx);

	zp->z_phys->zp_size = end;

	zfs_range_unlock(rl);

	dmu_tx_commit(tx);

	uvm_vnp_setsize(ZTOV(zp), end);

	return (0);
}

/*
 * Free space in a file.
 *
 *	IN:	zp	- znode of file to free data in.
 *		off	- start of section to free.
 *		len	- length of section to free.
 *
 * 	RETURN:	0 if success
 *		error code if failure
 */
static int
zfs_free_range(znode_t *zp, uint64_t off, uint64_t len)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	rl_t *rl;
	int error;

	/*
	 * Lock the range being freed.
	 */
	rl = zfs_range_lock(zp, off, len, RL_WRITER);

	/*
	 * Nothing to do if file already at desired length.
	 */
	if (off >= zp->z_phys->zp_size) {
		zfs_range_unlock(rl);
		return (0);
	}

	if (off + len > zp->z_phys->zp_size)
		len = zp->z_phys->zp_size - off;

	error = dmu_free_long_range(zfsvfs->z_os, zp->z_id, off, len);

	if (error == 0) {
		/*
		 * In NetBSD we cannot free block in the middle of a file,
		 * but only at the end of a file.
		 */
		uvm_vnp_setsize(ZTOV(zp), off);
	}

	zfs_range_unlock(rl);

	return (error);
}

/*
 * Truncate a file
 *
 *	IN:	zp	- znode of file to free data in.
 *		end	- new end-of-file.
 *
 * 	RETURN:	0 if success
 *		error code if failure
 */
static int
zfs_trunc(znode_t *zp, uint64_t end)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	vnode_t *vp = ZTOV(zp);
	dmu_tx_t *tx;
	rl_t *rl;
	int error;

	/*
	 * We will change zp_size, lock the whole file.
	 */
	rl = zfs_range_lock(zp, 0, UINT64_MAX, RL_WRITER);

	/*
	 * Nothing to do if file already at desired length.
	 */
	if (end >= zp->z_phys->zp_size) {
		zfs_range_unlock(rl);
		return (0);
	}

	error = dmu_free_long_range(zfsvfs->z_os, zp->z_id, end,  -1);
	if (error) {
		zfs_range_unlock(rl);
		return (error);
	}
top:
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_bonus(tx, zp->z_id);
	error = dmu_tx_assign(tx, TXG_NOWAIT);
	if (error) {
		if (error == ERESTART) {
			dmu_tx_wait(tx);
			dmu_tx_abort(tx);
			goto top;
		}
		dmu_tx_abort(tx);
		zfs_range_unlock(rl);
		return (error);
	}
	dmu_buf_will_dirty(zp->z_dbuf, tx);

	zp->z_phys->zp_size = end;

	dmu_tx_commit(tx);

	/*
	 * Clear any mapped pages in the truncated region.  This has to
	 * happen outside of the transaction to avoid the possibility of
	 * a deadlock with someone trying to push a page that we are
	 * about to invalidate.
	 */

	uvm_vnp_setsize(vp, end);

	return (0);
}

/*
 * Free space in a file
 *
 *	IN:	zp	- znode of file to free data in.
 *		off	- start of range
 *		len	- end of range (0 => EOF)
 *		flag	- current file open mode flags.
 *		log	- TRUE if this action should be logged
 *
 * 	RETURN:	0 if success
 *		error code if failure
 */
int
zfs_freesp(znode_t *zp, uint64_t off, uint64_t len, int flag, boolean_t log)
{
	vnode_t *vp = ZTOV(zp);
	dmu_tx_t *tx;
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	zilog_t *zilog = zfsvfs->z_log;
	int error;

	if (off > zp->z_phys->zp_size) {
		error =  zfs_extend(zp, off+len);
		if (error == 0 && log)
			goto log;
		else
			return (error);
	}

	if (len == 0) {
		error = zfs_trunc(zp, off);
	} else {
		if ((error = zfs_free_range(zp, off, len)) == 0 &&
		    off + len > zp->z_phys->zp_size)
			error = zfs_extend(zp, off+len);
	}
	if (error || !log)
		return (error);
log:
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_bonus(tx, zp->z_id);
	error = dmu_tx_assign(tx, TXG_NOWAIT);
	if (error) {
		if (error == ERESTART) {
			dmu_tx_wait(tx);
			dmu_tx_abort(tx);
			goto log;
		}
		dmu_tx_abort(tx);
		return (error);
	}

	zfs_time_stamper(zp, CONTENT_MODIFIED, tx);
	zfs_log_truncate(zilog, tx, TX_TRUNCATE, zp, off, len);

	dmu_tx_commit(tx);
	return (0);
}

void
zfs_create_fs(objset_t *os, cred_t *cr, nvlist_t *zplprops, dmu_tx_t *tx)
{
	zfsvfs_t	zfsvfs;
	uint64_t	moid, obj, version;
	uint64_t	sense = ZFS_CASE_SENSITIVE;
	uint64_t	norm = 0;
	nvpair_t	*elem;
	int		error;
	int		i;
	znode_t		*rootzp = NULL;
	vnode_t		*vp;
	vattr_t		vattr;
	znode_t		*zp;
	zfs_acl_ids_t	acl_ids;

	/*
	 * First attempt to create master node.
	 */
	/*
	 * In an empty objset, there are no blocks to read and thus
	 * there can be no i/o errors (which we assert below).
	 */
	moid = MASTER_NODE_OBJ;
	error = zap_create_claim(os, moid, DMU_OT_MASTER_NODE,
	    DMU_OT_NONE, 0, tx);
	ASSERT(error == 0);

	/*
	 * Set starting attributes.
	 */
	if (spa_version(dmu_objset_spa(os)) >= SPA_VERSION_USERSPACE)
		version = ZPL_VERSION;
	else if (spa_version(dmu_objset_spa(os)) >= SPA_VERSION_FUID)
		version = ZPL_VERSION_USERSPACE - 1;
	else
		version = ZPL_VERSION_FUID - 1;
	elem = NULL;
	while ((elem = nvlist_next_nvpair(zplprops, elem)) != NULL) {
		/* For the moment we expect all zpl props to be uint64_ts */
		uint64_t val;
		char *name;

		ASSERT(nvpair_type(elem) == DATA_TYPE_UINT64);
		VERIFY(nvpair_value_uint64(elem, &val) == 0);
		name = nvpair_name(elem);
		if (strcmp(name, zfs_prop_to_name(ZFS_PROP_VERSION)) == 0) {
			if (val < version)
				version = val;
		} else {
			error = zap_update(os, moid, name, 8, 1, &val, tx);
		}
		ASSERT(error == 0);
		if (strcmp(name, zfs_prop_to_name(ZFS_PROP_NORMALIZE)) == 0)
			norm = val;
		else if (strcmp(name, zfs_prop_to_name(ZFS_PROP_CASE)) == 0)
			sense = val;
	}
	ASSERT(version != 0);
	error = zap_update(os, moid, ZPL_VERSION_STR, 8, 1, &version, tx);

	/*
	 * Create a delete queue.
	 */
	obj = zap_create(os, DMU_OT_UNLINKED_SET, DMU_OT_NONE, 0, tx);

	error = zap_add(os, moid, ZFS_UNLINKED_SET, 8, 1, &obj, tx);
	ASSERT(error == 0);

	/*
	 * Create root znode.  Create minimal znode/vnode/zfsvfs
	 * to allow zfs_mknode to work.
	 */
	vattr_null(&vattr);
	vattr.va_mask = AT_MODE|AT_UID|AT_GID|AT_TYPE;
	vattr.va_type = VDIR;
	vattr.va_mode = S_IFDIR|0755;
	vattr.va_uid = crgetuid(cr);
	vattr.va_gid = crgetgid(cr);

	rootzp = kmem_cache_alloc(znode_cache, KM_SLEEP);
	rootzp->z_unlinked = 0;
	rootzp->z_atime_dirty = 0;

	for (;;) {
		error = getnewvnode(VT_ZFS, NULL, zfs_vnodeop_p,
		    NULL, &rootzp->z_vnode);
		if (error == 0)
			break;
		printf("WARNING: zfs_create_fs: unable to get vnode, "
		    "error=%d\n", error);
		kpause("zfsvn", false, hz, NULL);
	}
	
	vp = ZTOV(rootzp);
	vp->v_type = VDIR;

	bzero(&zfsvfs, sizeof (zfsvfs_t));

	zfsvfs.z_os = os;
	zfsvfs.z_parent = &zfsvfs;
	zfsvfs.z_version = version;
	zfsvfs.z_use_fuids = USE_FUIDS(version, os);
	zfsvfs.z_norm = norm;
	/*
	 * Fold case on file systems that are always or sometimes case
	 * insensitive.
	 */
	if (sense == ZFS_CASE_INSENSITIVE || sense == ZFS_CASE_MIXED)
		zfsvfs.z_norm |= U8_TEXTPREP_TOUPPER;

	mutex_init(&zfsvfs.z_znodes_lock, NULL, MUTEX_DEFAULT, NULL);
	list_create(&zfsvfs.z_all_znodes, sizeof (znode_t),
	    offsetof(znode_t, z_link_node));

	for (i = 0; i != ZFS_OBJ_MTX_SZ; i++)
		mutex_init(&zfsvfs.z_hold_mtx[i], NULL, MUTEX_DEFAULT, NULL);

	ASSERT(!POINTER_IS_VALID(rootzp->z_zfsvfs));
	rootzp->z_zfsvfs = &zfsvfs;
	VERIFY(0 == zfs_acl_ids_create(rootzp, IS_ROOT_NODE, &vattr,
	    cr, NULL, &acl_ids));
	zfs_mknode(rootzp, &vattr, tx, cr, IS_ROOT_NODE, &zp, 0, &acl_ids);
	ASSERT3P(zp, ==, rootzp);
	error = zap_add(os, moid, ZFS_ROOT_OBJ, 8, 1, &rootzp->z_id, tx);
	ASSERT(error == 0);
	zfs_acl_ids_free(&acl_ids);
	POINTER_INVALIDATE(&rootzp->z_zfsvfs);

	dmu_buf_rele(rootzp->z_dbuf, NULL);
	rootzp->z_dbuf = NULL;
	ungetnewvnode(vp);
	kmem_cache_free(znode_cache, rootzp);

	/*
	 * Create shares directory
	 */

	error = zfs_create_share_dir(&zfsvfs, tx);

	ASSERT(error == 0);

	for (i = 0; i != ZFS_OBJ_MTX_SZ; i++)
		mutex_destroy(&zfsvfs.z_hold_mtx[i]);
}

#endif /* _KERNEL */
/*
 * Given an object number, return its parent object number and whether
 * or not the object is an extended attribute directory.
 */
static int
zfs_obj_to_pobj(objset_t *osp, uint64_t obj, uint64_t *pobjp, int *is_xattrdir)
{
	dmu_buf_t *db;
	dmu_object_info_t doi;
	znode_phys_t *zp;
	int error;

	if ((error = dmu_bonus_hold(osp, obj, FTAG, &db)) != 0)
		return (error);

	dmu_object_info_from_db(db, &doi);
	if (doi.doi_bonus_type != DMU_OT_ZNODE ||
	    doi.doi_bonus_size < sizeof (znode_phys_t)) {
		dmu_buf_rele(db, FTAG);
		return (EINVAL);
	}

	zp = db->db_data;
	*pobjp = zp->zp_parent;
	*is_xattrdir = ((zp->zp_flags & ZFS_XATTR) != 0) &&
	    S_ISDIR(zp->zp_mode);
	dmu_buf_rele(db, FTAG);

	return (0);
}

int
zfs_obj_to_path(objset_t *osp, uint64_t obj, char *buf, int len)
{
	char *path = buf + len - 1;
	int error;

	*path = '\0';

	for (;;) {
		uint64_t pobj;
		char component[MAXNAMELEN + 2];
		size_t complen;
		int is_xattrdir;

		if ((error = zfs_obj_to_pobj(osp, obj, &pobj,
		    &is_xattrdir)) != 0)
			break;

		if (pobj == obj) {
			if (path[0] != '/')
				*--path = '/';
			break;
		}

		component[0] = '/';
		if (is_xattrdir) {
			(void) sprintf(component + 1, "<xattrdir>");
		} else {
			error = zap_value_search(osp, pobj, obj,
			    ZFS_DIRENT_OBJ(-1ULL), component + 1);
			if (error != 0)
				break;
		}

		complen = strlen(component);
		path -= complen;
		ASSERT(path >= buf);
		bcopy(component, path, complen);
		obj = pobj;
	}

	if (error == 0)
		(void) memmove(buf, path, buf + len - path);
	return (error);
}
