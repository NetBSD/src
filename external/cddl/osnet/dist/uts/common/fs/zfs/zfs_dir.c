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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/mode.h>
#include <sys/kmem.h>
#include <sys/uio.h>
#include <sys/pathname.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/sunddi.h>
#include <sys/random.h>
#include <sys/policy.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_acl.h>
#include <sys/fs/zfs.h>
#include <sys/zap.h>
#include <sys/dmu.h>
#include <sys/atomic.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_fuid.h>
#include <sys/dnlc.h>
#include <sys/extdirent.h>

/*
 * zfs_match_find() is used by zfs_dirent_lock() to peform zap lookups
 * of names after deciding which is the appropriate lookup interface.
 */
static int
zfs_match_find(zfsvfs_t *zfsvfs, znode_t *dzp, char *name, boolean_t exact,
    boolean_t update, int *deflags, pathname_t *rpnp, uint64_t *zoid)
{
	int error;

	if (zfsvfs->z_norm) {
		matchtype_t mt = MT_FIRST;
		boolean_t conflict = B_FALSE;
		size_t bufsz = 0;
		char *buf = NULL;

		if (rpnp) {
			buf = rpnp->pn_buf;
			bufsz = rpnp->pn_bufsize;
		}
		if (exact)
			mt = MT_EXACT;
		/*
		 * In the non-mixed case we only expect there would ever
		 * be one match, but we need to use the normalizing lookup.
		 */
		error = zap_lookup_norm(zfsvfs->z_os, dzp->z_id, name, 8, 1,
		    zoid, mt, buf, bufsz, &conflict);
		if (!error && deflags)
			*deflags = conflict ? ED_CASE_CONFLICT : 0;
	} else {
		error = zap_lookup(zfsvfs->z_os, dzp->z_id, name, 8, 1, zoid);
	}
	*zoid = ZFS_DIRENT_OBJ(*zoid);

	if (error == ENOENT && update)
		dnlc_update(ZTOV(dzp), name, DNLC_NO_VNODE);

	return (error);
}

/*
 * Reference counting for dirlocks.  Solaris destroys the condvar as
 * soon as it broadcasts, which works for them because cv_wait doesn't
 * need to use the condvar after it is woken, but which is too fast and
 * loose with the abstraction for us in NetBSD.
 */

static int
zfs_dirlock_hold(zfs_dirlock_t *dl)
{

	KASSERT(mutex_owned(&dl->dl_dzp->z_lock));

	if (dl->dl_refcnt >= ULONG_MAX) /* XXX Name this constant.  */
		return (ENFILE);	/* XXX What to do?  */

	dl->dl_refcnt++;
	return (0);
}

static void
zfs_dirlock_rele(zfs_dirlock_t *dl)
{

	KASSERT(mutex_owned(&dl->dl_dzp->z_lock));
	KASSERT(dl->dl_refcnt > 0);

	if (--dl->dl_refcnt == 0) {
		if (dl->dl_namesize != 0)
			kmem_free(dl->dl_name, dl->dl_namesize);
		cv_destroy(&dl->dl_cv);
		kmem_free(dl, sizeof(*dl));
	}
}

/*
 * Lock a directory entry.  A dirlock on <dzp, name> protects that name
 * in dzp's directory zap object.  As long as you hold a dirlock, you can
 * assume two things: (1) dzp cannot be reaped, and (2) no other thread
 * can change the zap entry for (i.e. link or unlink) this name.
 *
 * Input arguments:
 *	dzp	- znode for directory
 *	name	- name of entry to lock
 *	flag	- ZNEW: if the entry already exists, fail with EEXIST.
 *		  ZEXISTS: if the entry does not exist, fail with ENOENT.
 *		  ZSHARED: allow concurrent access with other ZSHARED callers.
 *		  ZXATTR: we want dzp's xattr directory
 *		  ZCILOOK: On a mixed sensitivity file system,
 *			   this lookup should be case-insensitive.
 *		  ZCIEXACT: On a purely case-insensitive file system,
 *			    this lookup should be case-sensitive.
 *		  ZRENAMING: we are locking for renaming, force narrow locks
 *		  ZHAVELOCK: Don't grab the z_name_lock for this call. The
 *			     current thread already holds it.
 *
 * Output arguments:
 *	zpp	- pointer to the znode for the entry (NULL if there isn't one)
 *	dlpp	- pointer to the dirlock for this entry (NULL on error)
 *      direntflags - (case-insensitive lookup only)
 *		flags if multiple case-sensitive matches exist in directory
 *      realpnp     - (case-insensitive lookup only)
 *		actual name matched within the directory
 *
 * Return value: 0 on success or errno on failure.
 *
 * NOTE: Always checks for, and rejects, '.' and '..'.
 * NOTE: For case-insensitive file systems we take wide locks (see below),
 *	 but return znode pointers to a single match.
 */
int
zfs_dirent_lock(zfs_dirlock_t **dlpp, znode_t *dzp, char *name, znode_t **zpp,
    int flag, int *direntflags, pathname_t *realpnp)
{
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zfs_dirlock_t	*dl;
	boolean_t	update;
	boolean_t	exact;
	uint64_t	zoid;
	vnode_t		*vp = NULL;
	int		error = 0;
	int		cmpflags;

	*zpp = NULL;
	*dlpp = NULL;

	/*
	 * Verify that we are not trying to lock '.', '..', or '.zfs'
	 */
	if (name[0] == '.' &&
	    (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')) ||
	    zfs_has_ctldir(dzp) && strcmp(name, ZFS_CTLDIR_NAME) == 0)
		return (EEXIST);

	/*
	 * Case sensitivity and normalization preferences are set when
	 * the file system is created.  These are stored in the
	 * zfsvfs->z_case and zfsvfs->z_norm fields.  These choices
	 * affect what vnodes can be cached in the DNLC, how we
	 * perform zap lookups, and the "width" of our dirlocks.
	 *
	 * A normal dirlock locks a single name.  Note that with
	 * normalization a name can be composed multiple ways, but
	 * when normalized, these names all compare equal.  A wide
	 * dirlock locks multiple names.  We need these when the file
	 * system is supporting mixed-mode access.  It is sometimes
	 * necessary to lock all case permutations of file name at
	 * once so that simultaneous case-insensitive/case-sensitive
	 * behaves as rationally as possible.
	 */

	/*
	 * Decide if exact matches should be requested when performing
	 * a zap lookup on file systems supporting case-insensitive
	 * access.
	 */
	exact =
	    ((zfsvfs->z_case == ZFS_CASE_INSENSITIVE) && (flag & ZCIEXACT)) ||
	    ((zfsvfs->z_case == ZFS_CASE_MIXED) && !(flag & ZCILOOK));

	/*
	 * Only look in or update the DNLC if we are looking for the
	 * name on a file system that does not require normalization
	 * or case folding.  We can also look there if we happen to be
	 * on a non-normalizing, mixed sensitivity file system IF we
	 * are looking for the exact name.
	 *
	 * Maybe can add TO-UPPERed version of name to dnlc in ci-only
	 * case for performance improvement?
	 */
	update = !zfsvfs->z_norm ||
	    ((zfsvfs->z_case == ZFS_CASE_MIXED) &&
	    !(zfsvfs->z_norm & ~U8_TEXTPREP_TOUPPER) && !(flag & ZCILOOK));

	/*
	 * ZRENAMING indicates we are in a situation where we should
	 * take narrow locks regardless of the file system's
	 * preferences for normalizing and case folding.  This will
	 * prevent us deadlocking trying to grab the same wide lock
	 * twice if the two names happen to be case-insensitive
	 * matches.
	 */
	if (flag & ZRENAMING)
		cmpflags = 0;
	else
		cmpflags = zfsvfs->z_norm;

	/*
	 * Wait until there are no locks on this name.
	 *
	 * Don't grab the the lock if it is already held. However, cannot
	 * have both ZSHARED and ZHAVELOCK together.
	 */
	ASSERT(!(flag & ZSHARED) || !(flag & ZHAVELOCK));
	if (!(flag & ZHAVELOCK))
		rw_enter(&dzp->z_name_lock, RW_READER);

	mutex_enter(&dzp->z_lock);
	for (;;) {
		if (dzp->z_unlinked) {
			mutex_exit(&dzp->z_lock);
			if (!(flag & ZHAVELOCK))
				rw_exit(&dzp->z_name_lock);
			return (ENOENT);
		}
		for (dl = dzp->z_dirlocks; dl != NULL; dl = dl->dl_next) {
			if ((u8_strcmp(name, dl->dl_name, 0, cmpflags,
			    U8_UNICODE_LATEST, &error) == 0) || error != 0)
				break;
		}
		if (error != 0) {
			mutex_exit(&dzp->z_lock);
			if (!(flag & ZHAVELOCK))
				rw_exit(&dzp->z_name_lock);
			return (ENOENT);
		}
		if (dl == NULL)	{
			/*
			 * Allocate a new dirlock and add it to the list.
			 */
			dl = kmem_alloc(sizeof (zfs_dirlock_t), KM_SLEEP);
			cv_init(&dl->dl_cv, NULL, CV_DEFAULT, NULL);
			dl->dl_name = name;
			dl->dl_sharecnt = 0;
			dl->dl_namelock = 0;
			dl->dl_namesize = 0;
			dl->dl_refcnt = 1;
			dl->dl_dzp = dzp;
			dl->dl_next = dzp->z_dirlocks;
			dzp->z_dirlocks = dl;
			break;
		}
		if ((flag & ZSHARED) && dl->dl_sharecnt != 0)
			break;
		error = zfs_dirlock_hold(dl);
		if (error) {
			mutex_exit(&dzp->z_lock);
			if (!(flag & ZHAVELOCK))
				rw_exit(&dzp->z_name_lock);
			return (error);
		}
		cv_wait(&dl->dl_cv, &dzp->z_lock);
		zfs_dirlock_rele(dl);
	}

	/*
	 * If the z_name_lock was NOT held for this dirlock record it.
	 */
	if (flag & ZHAVELOCK)
		dl->dl_namelock = 1;

	if ((flag & ZSHARED) && ++dl->dl_sharecnt > 1 && dl->dl_namesize == 0) {
		/*
		 * We're the second shared reference to dl.  Make a copy of
		 * dl_name in case the first thread goes away before we do.
		 * Note that we initialize the new name before storing its
		 * pointer into dl_name, because the first thread may load
		 * dl->dl_name at any time.  He'll either see the old value,
		 * which is his, or the new shared copy; either is OK.
		 */
		dl->dl_namesize = strlen(dl->dl_name) + 1;
		name = kmem_alloc(dl->dl_namesize, KM_SLEEP);
		bcopy(dl->dl_name, name, dl->dl_namesize);
		dl->dl_name = name;
	}

	mutex_exit(&dzp->z_lock);

	/*
	 * We have a dirlock on the name.  (Note that it is the dirlock,
	 * not the dzp's z_lock, that protects the name in the zap object.)
	 * See if there's an object by this name; if so, put a hold on it.
	 */
	if (flag & ZXATTR) {
		zoid = dzp->z_phys->zp_xattr;
		error = (zoid == 0 ? ENOENT : 0);
	} else {
		if (update)
			vp = dnlc_lookup(ZTOV(dzp), name);
		if (vp == DNLC_NO_VNODE) {
			VN_RELE(vp);
			error = ENOENT;
		} else if (vp) {
			if (flag & ZNEW) {
				zfs_dirent_unlock(dl);
				VN_RELE(vp);
				return (EEXIST);
			}
			*dlpp = dl;
			*zpp = VTOZ(vp);
			return (0);
		} else {
			error = zfs_match_find(zfsvfs, dzp, name, exact,
			    update, direntflags, realpnp, &zoid);
		}
	}
	if (error) {
		if (error != ENOENT || (flag & ZEXISTS)) {
			zfs_dirent_unlock(dl);
			return (error);
		}
	} else {
		if (flag & ZNEW) {
			zfs_dirent_unlock(dl);
			return (EEXIST);
		}
		error = zfs_zget(zfsvfs, zoid, zpp);
		if (error) {
			zfs_dirent_unlock(dl);
			return (error);
		}
		if (!(flag & ZXATTR) && update)
			dnlc_update(ZTOV(dzp), name, ZTOV(*zpp));
	}

	*dlpp = dl;

	return (0);
}

/*
 * Unlock this directory entry and wake anyone who was waiting for it.
 */
void
zfs_dirent_unlock(zfs_dirlock_t *dl)
{
	znode_t *dzp = dl->dl_dzp;
	zfs_dirlock_t **prev_dl, *cur_dl;

	mutex_enter(&dzp->z_lock);

	if (!dl->dl_namelock)
		rw_exit(&dzp->z_name_lock);

	if (dl->dl_sharecnt > 1) {
		dl->dl_sharecnt--;
		mutex_exit(&dzp->z_lock);
		return;
	}
	prev_dl = &dzp->z_dirlocks;
	while ((cur_dl = *prev_dl) != dl)
		prev_dl = &cur_dl->dl_next;
	*prev_dl = dl->dl_next;
	cv_broadcast(&dl->dl_cv);
	zfs_dirlock_rele(dl);
	mutex_exit(&dzp->z_lock);
}

/*
 * Look up an entry in a directory.
 *
 * NOTE: '.' and '..' are handled as special cases because
 *	no directory entries are actually stored for them.  If this is
 *	the root of a filesystem, then '.zfs' is also treated as a
 *	special pseudo-directory.
 */
int
zfs_dirlook(znode_t *dzp, char *name, vnode_t **vpp, int flags,
    int *deflg, pathname_t *rpnp)
{
	zfs_dirlock_t *dl;
	znode_t *zp;
	int error = 0;

	if (name[0] == 0 || (name[0] == '.' && name[1] == 0)) {
		*vpp = ZTOV(dzp);
		VN_HOLD(*vpp);
	} else if (name[0] == '.' && name[1] == '.' && name[2] == 0) {
		zfsvfs_t *zfsvfs = dzp->z_zfsvfs;
		/*
		 * If we are a snapshot mounted under .zfs, return
		 * the vp for the snapshot directory.
		 */
		if (dzp->z_phys->zp_parent == dzp->z_id &&
		    zfsvfs->z_parent != zfsvfs) {
			error = zfsctl_root_lookup(zfsvfs->z_parent->z_ctldir,
			    "snapshot", vpp, NULL, 0, NULL, kcred,
			    NULL, NULL, NULL);
			return (error);
		}
		rw_enter(&dzp->z_parent_lock, RW_READER);
		mutex_enter(&dzp->z_lock);
		if (dzp->z_phys->zp_links == 0) {
			/* Directory has been rmdir'd.  */
			error = ENOENT;
		} else {
			error = zfs_zget(zfsvfs, dzp->z_phys->zp_parent, &zp);
			if (error == 0)
				*vpp = ZTOV(zp);
		}
		mutex_exit(&dzp->z_lock);
		rw_exit(&dzp->z_parent_lock);
	} else if (zfs_has_ctldir(dzp) && strcmp(name, ZFS_CTLDIR_NAME) == 0) {
		*vpp = zfsctl_root(dzp);
	} else {
		int zf;

		zf = ZEXISTS | ZSHARED;
		if (flags & FIGNORECASE)
			zf |= ZCILOOK;

		error = zfs_dirent_lock(&dl, dzp, name, &zp, zf, deflg, rpnp);
		if (error == 0) {
			*vpp = ZTOV(zp);
			zfs_dirent_unlock(dl);
			dzp->z_zn_prefetch = B_TRUE; /* enable prefetching */
		}
		rpnp = NULL;
	}

	if ((flags & FIGNORECASE) && rpnp && !error)
		(void) strlcpy(rpnp->pn_buf, name, rpnp->pn_bufsize);

	return (error);
}

/*
 * unlinked Set (formerly known as the "delete queue") Error Handling
 *
 * When dealing with the unlinked set, we dmu_tx_hold_zap(), but we
 * don't specify the name of the entry that we will be manipulating.  We
 * also fib and say that we won't be adding any new entries to the
 * unlinked set, even though we might (this is to lower the minimum file
 * size that can be deleted in a full filesystem).  So on the small
 * chance that the nlink list is using a fat zap (ie. has more than
 * 2000 entries), we *may* not pre-read a block that's needed.
 * Therefore it is remotely possible for some of the assertions
 * regarding the unlinked set below to fail due to i/o error.  On a
 * nondebug system, this will result in the space being leaked.
 */
void
zfs_unlinked_add(znode_t *zp, dmu_tx_t *tx)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;

	ASSERT(zp->z_unlinked);
	ASSERT3U(zp->z_phys->zp_links, ==, 0);

	VERIFY3U(0, ==,
	    zap_add_int(zfsvfs->z_os, zfsvfs->z_unlinkedobj, zp->z_id, tx));
}

/*
 * Clean up any znodes that had no links when we either crashed or
 * (force) umounted the file system.
 */
void
zfs_unlinked_drain(zfsvfs_t *zfsvfs)
{
	zap_cursor_t	zc;
	zap_attribute_t zap;
	dmu_object_info_t doi;
	znode_t		*zp;
	int		error;

	/*
	 * Interate over the contents of the unlinked set.
	 */
	for (zap_cursor_init(&zc, zfsvfs->z_os, zfsvfs->z_unlinkedobj);
	    zap_cursor_retrieve(&zc, &zap) == 0;
	    zap_cursor_advance(&zc)) {

		/*
		 * See what kind of object we have in list
		 */

		error = dmu_object_info(zfsvfs->z_os,
		    zap.za_first_integer, &doi);
		if (error != 0)
			continue;

		ASSERT((doi.doi_type == DMU_OT_PLAIN_FILE_CONTENTS) ||
		    (doi.doi_type == DMU_OT_DIRECTORY_CONTENTS));
		/*
		 * We need to re-mark these list entries for deletion,
		 * so we pull them back into core and set zp->z_unlinked.
		 */
		error = zfs_zget(zfsvfs, zap.za_first_integer, &zp);

		/*
		 * We may pick up znodes that are already marked for deletion.
		 * This could happen during the purge of an extended attribute
		 * directory.  All we need to do is skip over them, since they
		 * are already in the system marked z_unlinked.
		 */
		if (error != 0)
			continue;

		zp->z_unlinked = B_TRUE;
		VN_RELE(ZTOV(zp));
	}
	zap_cursor_fini(&zc);
}

/*
 * Delete the entire contents of a directory.  Return a count
 * of the number of entries that could not be deleted. If we encounter
 * an error, return a count of at least one so that the directory stays
 * in the unlinked set.
 *
 * NOTE: this function assumes that the directory is inactive,
 *	so there is no need to lock its entries before deletion.
 *	Also, it assumes the directory contents is *only* regular
 *	files.
 */
static int
zfs_purgedir(znode_t *dzp)
{
	zap_cursor_t	zc;
	zap_attribute_t	zap;
	znode_t		*xzp;
	dmu_tx_t	*tx;
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zfs_dirlock_t	dl;
	int skipped = 0;
	int error;

	for (zap_cursor_init(&zc, zfsvfs->z_os, dzp->z_id);
	    (error = zap_cursor_retrieve(&zc, &zap)) == 0;
	    zap_cursor_advance(&zc)) {
		error = zfs_zget(zfsvfs,
		    ZFS_DIRENT_OBJ(zap.za_first_integer), &xzp);
		if (error) {
			skipped += 1;
			continue;
		}

		ASSERT((ZTOV(xzp)->v_type == VREG) ||
		    (ZTOV(xzp)->v_type == VLNK));

		tx = dmu_tx_create(zfsvfs->z_os);
		dmu_tx_hold_bonus(tx, dzp->z_id);
		dmu_tx_hold_zap(tx, dzp->z_id, FALSE, zap.za_name);
		dmu_tx_hold_bonus(tx, xzp->z_id);
		dmu_tx_hold_zap(tx, zfsvfs->z_unlinkedobj, FALSE, NULL);
		error = dmu_tx_assign(tx, TXG_WAIT);
		if (error) {
			dmu_tx_abort(tx);
			VN_RELE(ZTOV(xzp));
			skipped += 1;
			continue;
		}
		bzero(&dl, sizeof (dl));
		dl.dl_dzp = dzp;
		dl.dl_name = zap.za_name;

		error = zfs_link_destroy(&dl, xzp, tx, 0, NULL);
		if (error)
			skipped += 1;
		dmu_tx_commit(tx);

		VN_RELE(ZTOV(xzp));
	}
	zap_cursor_fini(&zc);
	if (error != ENOENT)
		skipped += 1;
	return (skipped);
}

void
zfs_rmnode(znode_t *zp)
{
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	objset_t	*os = zfsvfs->z_os;
	znode_t		*xzp = NULL;
	dmu_tx_t	*tx;
	uint64_t	acl_obj;
	int		error;

	ASSERT(ZTOV(zp)->v_count == 0);
	ASSERT(zp->z_phys->zp_links == 0);

	/*
	 * If this is an attribute directory, purge its contents.
	 */
	if (ZTOV(zp)->v_type == VDIR && (zp->z_phys->zp_flags & ZFS_XATTR)) {
		if (zfs_purgedir(zp) != 0) {
			/*
			 * Not enough space to delete some xattrs.
			 * Leave it in the unlinked set.
			 */
			zfs_znode_dmu_fini(zp);
			zfs_znode_free(zp);
			return;
		}
	}

	/*
	 * Free up all the data in the file.
	 */
	error = dmu_free_long_range(os, zp->z_id, 0, DMU_OBJECT_END);
	if (error) {
		/*
		 * Not enough space.  Leave the file in the unlinked set.
		 */
		zfs_znode_dmu_fini(zp);
		zfs_znode_free(zp);
		return;
	}

	/*
	 * If the file has extended attributes, we're going to unlink
	 * the xattr dir.
	 */
	if (zp->z_phys->zp_xattr) {
		error = zfs_zget(zfsvfs, zp->z_phys->zp_xattr, &xzp);
		ASSERT(error == 0);
	}

	acl_obj = zp->z_phys->zp_acl.z_acl_extern_obj;

	/*
	 * Set up the final transaction.
	 */
	tx = dmu_tx_create(os);
	dmu_tx_hold_free(tx, zp->z_id, 0, DMU_OBJECT_END);
	dmu_tx_hold_zap(tx, zfsvfs->z_unlinkedobj, FALSE, NULL);
	if (xzp) {
		dmu_tx_hold_bonus(tx, xzp->z_id);
		dmu_tx_hold_zap(tx, zfsvfs->z_unlinkedobj, TRUE, NULL);
	}
	if (acl_obj)
		dmu_tx_hold_free(tx, acl_obj, 0, DMU_OBJECT_END);
	error = dmu_tx_assign(tx, TXG_WAIT);
	if (error) {
		/*
		 * Not enough space to delete the file.  Leave it in the
		 * unlinked set, leaking it until the fs is remounted (at
		 * which point we'll call zfs_unlinked_drain() to process it).
		 */
		dmu_tx_abort(tx);
		zfs_znode_dmu_fini(zp);
		zfs_znode_free(zp);
		goto out;
	}

	if (xzp) {
		dmu_buf_will_dirty(xzp->z_dbuf, tx);
		mutex_enter(&xzp->z_lock);
		xzp->z_unlinked = B_TRUE;	/* mark xzp for deletion */
		xzp->z_phys->zp_links = 0;	/* no more links to it */
		mutex_exit(&xzp->z_lock);
		zfs_unlinked_add(xzp, tx);
	}

	/* Remove this znode from the unlinked set */
	VERIFY3U(0, ==,
	    zap_remove_int(zfsvfs->z_os, zfsvfs->z_unlinkedobj, zp->z_id, tx));

	zfs_znode_delete(zp, tx);

	dmu_tx_commit(tx);
out:
	if (xzp)
		VN_RELE(ZTOV(xzp));
}

static uint64_t
zfs_dirent(znode_t *zp)
{
	uint64_t de = zp->z_id;
	if (zp->z_zfsvfs->z_version >= ZPL_VERSION_DIRENT_TYPE)
		de |= IFTODT((zp)->z_phys->zp_mode) << 60;
	return (de);
}

/*
 * Link zp into dl.  Can only fail if zp has been unlinked.
 */
int
zfs_link_create(zfs_dirlock_t *dl, znode_t *zp, dmu_tx_t *tx, int flag)
{
	znode_t *dzp = dl->dl_dzp;
	vnode_t *vp = ZTOV(zp);
	uint64_t value;
	int zp_is_dir = (vp->v_type == VDIR);
	int error;

	dmu_buf_will_dirty(zp->z_dbuf, tx);
	mutex_enter(&zp->z_lock);

	if (!(flag & ZRENAMING)) {
		if (zp->z_unlinked) {	/* no new links to unlinked zp */
			ASSERT(!(flag & (ZNEW | ZEXISTS)));
			mutex_exit(&zp->z_lock);
			return (ENOENT);
		}
		zp->z_phys->zp_links++;
	}
	zp->z_phys->zp_parent = dzp->z_id;	/* dzp is now zp's parent */

	if (!(flag & ZNEW))
		zfs_time_stamper_locked(zp, STATE_CHANGED, tx);
	mutex_exit(&zp->z_lock);

	dmu_buf_will_dirty(dzp->z_dbuf, tx);
	mutex_enter(&dzp->z_lock);
	dzp->z_phys->zp_size++;			/* one dirent added */
	dzp->z_phys->zp_links += zp_is_dir;	/* ".." link from zp */
	zfs_time_stamper_locked(dzp, CONTENT_MODIFIED, tx);
	mutex_exit(&dzp->z_lock);

	value = zfs_dirent(zp);
	error = zap_add(zp->z_zfsvfs->z_os, dzp->z_id, dl->dl_name,
	    8, 1, &value, tx);
	ASSERT(error == 0);

	dnlc_update(ZTOV(dzp), dl->dl_name, vp);

	return (0);
}

/*
 * Unlink zp from dl, and mark zp for deletion if this was the last link.
 * Can fail if zp is a mount point (EBUSY) or a non-empty directory (EEXIST).
 * If 'unlinkedp' is NULL, we put unlinked znodes on the unlinked list.
 * If it's non-NULL, we use it to indicate whether the znode needs deletion,
 * and it's the caller's job to do it.
 */
int
zfs_link_destroy(zfs_dirlock_t *dl, znode_t *zp, dmu_tx_t *tx, int flag,
	boolean_t *unlinkedp)
{
	znode_t *dzp = dl->dl_dzp;
	vnode_t *vp = ZTOV(zp);
	int zp_is_dir = (vp->v_type == VDIR);
	boolean_t unlinked = B_FALSE;
	int error;

	dnlc_remove(ZTOV(dzp), dl->dl_name);

	if (!(flag & ZRENAMING)) {
		dmu_buf_will_dirty(zp->z_dbuf, tx);

		if (vn_vfswlock(vp))		/* prevent new mounts on zp */
			return (EBUSY);

		if (vn_ismntpt(vp)) {		/* don't remove mount point */
			vn_vfsunlock(vp);
			return (EBUSY);
		}

		mutex_enter(&zp->z_lock);
		if (zp_is_dir && !zfs_dirempty(zp)) {	/* dir not empty */
			mutex_exit(&zp->z_lock);
			vn_vfsunlock(vp);
			return (EEXIST);
		}
		if (zp->z_phys->zp_links <= zp_is_dir) {
			zfs_panic_recover("zfs: link count on vnode %p is %u, "
				"should be at least %u",
				zp->z_vnode, (int)zp->z_phys->zp_links,
		                zp_is_dir + 1);
			zp->z_phys->zp_links = zp_is_dir + 1;
		}
		if (--zp->z_phys->zp_links == zp_is_dir) {
			zp->z_unlinked = B_TRUE;
			zp->z_phys->zp_links = 0;
			unlinked = B_TRUE;
		} else {
			zfs_time_stamper_locked(zp, STATE_CHANGED, tx);
		}
		mutex_exit(&zp->z_lock);
		vn_vfsunlock(vp);
	}

	dmu_buf_will_dirty(dzp->z_dbuf, tx);
	mutex_enter(&dzp->z_lock);
	dzp->z_phys->zp_size--;			/* one dirent removed */
	dzp->z_phys->zp_links -= zp_is_dir;	/* ".." link from zp */
	zfs_time_stamper_locked(dzp, CONTENT_MODIFIED, tx);
	mutex_exit(&dzp->z_lock);

	if (zp->z_zfsvfs->z_norm) {
		if (((zp->z_zfsvfs->z_case == ZFS_CASE_INSENSITIVE) &&
		    (flag & ZCIEXACT)) ||
		    ((zp->z_zfsvfs->z_case == ZFS_CASE_MIXED) &&
		    !(flag & ZCILOOK)))
			error = zap_remove_norm(zp->z_zfsvfs->z_os,
			    dzp->z_id, dl->dl_name, MT_EXACT, tx);
		else
			error = zap_remove_norm(zp->z_zfsvfs->z_os,
			    dzp->z_id, dl->dl_name, MT_FIRST, tx);
	} else {
		error = zap_remove(zp->z_zfsvfs->z_os,
		    dzp->z_id, dl->dl_name, tx);
	}
	ASSERT(error == 0);

	if (unlinkedp != NULL)
		*unlinkedp = unlinked;
	else if (unlinked)
		zfs_unlinked_add(zp, tx);

	return (0);
}

/*
 * Indicate whether the directory is empty.  Works with or without z_lock
 * held, but can only be consider a hint in the latter case.  Returns true
 * if only "." and ".." remain and there's no work in progress.
 */
boolean_t
zfs_dirempty(znode_t *dzp)
{
	return (dzp->z_phys->zp_size == 2 && dzp->z_dirlocks == 0);
}

int
zfs_make_xattrdir(znode_t *zp, vattr_t *vap, vnode_t **xvpp, cred_t *cr)
{
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	znode_t *xzp;
	dmu_tx_t *tx;
	int error;
	zfs_acl_ids_t acl_ids;
	boolean_t fuid_dirtied;

	*xvpp = NULL;

	if (error = zfs_zaccess(zp, ACE_WRITE_NAMED_ATTRS, 0, B_FALSE, cr))
		return (error);

	if ((error = zfs_acl_ids_create(zp, IS_XATTR, vap, cr, NULL,
	    &acl_ids)) != 0)
		return (error);
	if (zfs_acl_ids_overquota(zfsvfs, &acl_ids)) {
		zfs_acl_ids_free(&acl_ids);
		return (EDQUOT);
	}

	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_bonus(tx, zp->z_id);
	dmu_tx_hold_zap(tx, DMU_NEW_OBJECT, FALSE, NULL);
	fuid_dirtied = zfsvfs->z_fuid_dirty;
	if (fuid_dirtied)
		zfs_fuid_txhold(zfsvfs, tx);
	error = dmu_tx_assign(tx, TXG_NOWAIT);
	if (error) {
		zfs_acl_ids_free(&acl_ids);
		if (error == ERESTART)
			dmu_tx_wait(tx);
		dmu_tx_abort(tx);
		return (error);
	}
	zfs_mknode(zp, vap, tx, cr, IS_XATTR, &xzp, 0, &acl_ids);

	if (fuid_dirtied)
		zfs_fuid_sync(zfsvfs, tx);

	ASSERT(xzp->z_phys->zp_parent == zp->z_id);
	dmu_buf_will_dirty(zp->z_dbuf, tx);
	zp->z_phys->zp_xattr = xzp->z_id;

	(void) zfs_log_create(zfsvfs->z_log, tx, TX_MKXATTR, zp,
	    xzp, "", NULL, acl_ids.z_fuidp, vap);

	zfs_acl_ids_free(&acl_ids);
	dmu_tx_commit(tx);

	*xvpp = ZTOV(xzp);

	return (0);
}

/*
 * Return a znode for the extended attribute directory for zp.
 * ** If the directory does not already exist, it is created **
 *
 *	IN:	zp	- znode to obtain attribute directory from
 *		cr	- credentials of caller
 *		flags	- flags from the VOP_LOOKUP call
 *
 *	OUT:	xzpp	- pointer to extended attribute znode
 *
 *	RETURN:	0 on success
 *		error number on failure
 */
int
zfs_get_xattrdir(znode_t *zp, vnode_t **xvpp, cred_t *cr, int flags)
{
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	znode_t		*xzp;
	zfs_dirlock_t	*dl;
	vattr_t		va;
	int		error;
top:
	error = zfs_dirent_lock(&dl, zp, "", &xzp, ZXATTR, NULL, NULL);
	if (error)
		return (error);

	if (xzp != NULL) {
		*xvpp = ZTOV(xzp);
		zfs_dirent_unlock(dl);
		return (0);
	}

	ASSERT(zp->z_phys->zp_xattr == 0);

	if (!(flags & CREATE_XATTR_DIR)) {
		zfs_dirent_unlock(dl);
		return (ENOENT);
	}

	if (zfsvfs->z_vfs->vfs_flag & VFS_RDONLY) {
		zfs_dirent_unlock(dl);
		return (EROFS);
	}

	/*
	 * The ability to 'create' files in an attribute
	 * directory comes from the write_xattr permission on the base file.
	 *
	 * The ability to 'search' an attribute directory requires
	 * read_xattr permission on the base file.
	 *
	 * Once in a directory the ability to read/write attributes
	 * is controlled by the permissions on the attribute file.
	 */
	va.va_mask = AT_TYPE | AT_MODE | AT_UID | AT_GID;
	va.va_type = VDIR;
	va.va_mode = S_IFDIR | S_ISVTX | 0777;
	zfs_fuid_map_ids(zp, cr, &va.va_uid, &va.va_gid);

	error = zfs_make_xattrdir(zp, &va, xvpp, cr);
	zfs_dirent_unlock(dl);

	if (error == ERESTART) {
		/* NB: we already did dmu_tx_wait() if necessary */
		goto top;
	}

	return (error);
}

/*
 * Decide whether it is okay to remove within a sticky directory.
 *
 * In sticky directories, write access is not sufficient;
 * you can remove entries from a directory only if:
 *
 *	you own the directory,
 *	you own the entry,
 *	the entry is a plain file and you have write access,
 *	or you are privileged (checked in secpolicy...).
 *
 * The function returns 0 if remove access is granted.
 */
int
zfs_sticky_remove_access(znode_t *zdp, znode_t *zp, cred_t *cr)
{
	uid_t  		uid;
	uid_t		downer;
	uid_t		fowner;
	zfsvfs_t	*zfsvfs = zdp->z_zfsvfs;

	if (zdp->z_zfsvfs->z_replay)
		return (0);

	if ((zdp->z_phys->zp_mode & S_ISVTX) == 0)
		return (0);

	downer = zfs_fuid_map_id(zfsvfs, zdp->z_phys->zp_uid, cr, ZFS_OWNER);
	fowner = zfs_fuid_map_id(zfsvfs, zp->z_phys->zp_uid, cr, ZFS_OWNER);

	if ((uid = crgetuid(cr)) == downer || uid == fowner ||
	    (ZTOV(zp)->v_type == VREG &&
	    zfs_zaccess(zp, ACE_WRITE_DATA, 0, B_FALSE, cr) == 0))
		return (0);
	else
		return (secpolicy_vnode_remove(cr));
}
