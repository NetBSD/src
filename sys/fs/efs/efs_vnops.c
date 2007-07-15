/*	$NetBSD: efs_vnops.c,v 1.3.2.2 2007/07/15 16:15:31 ad Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efs_vnops.c,v 1.3.2.2 2007/07/15 16:15:31 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/lockf.h>
#include <sys/unistd.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>

#include <fs/efs/efs.h>
#include <fs/efs/efs_sb.h>
#include <fs/efs/efs_dir.h>
#include <fs/efs/efs_genfs.h>
#include <fs/efs/efs_mount.h>
#include <fs/efs/efs_extent.h>
#include <fs/efs/efs_dinode.h>
#include <fs/efs/efs_inode.h>
#include <fs/efs/efs_subr.h>
#include <fs/efs/efs_ihash.h>

MALLOC_DECLARE(M_EFSTMP);

/*
 * Lookup a pathname component in the given directory.
 *
 * Returns 0 on success.
 */
static int
efs_lookup(void *v)
{
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct componentname *cnp = ap->a_cnp;
	struct vnode *vp;
	ino_t ino;
	int err, nameiop = cnp->cn_nameiop;

	/* ensure that the directory can be accessed first */
        err = VOP_ACCESS(ap->a_dvp, VEXEC, cnp->cn_cred, cnp->cn_lwp);
	if (err)
		return (err);

	err = cache_lookup(ap->a_dvp, ap->a_vpp, cnp);
	if (err != -1)
		return (err);

	/*
	 * Handle the three lookup types: '.', '..', and everything else.
	 */
	if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		vref(ap->a_dvp);
		*ap->a_vpp = ap->a_dvp;
	} else if (cnp->cn_flags & ISDOTDOT) {
		err = efs_inode_lookup(VFSTOEFS(ap->a_dvp->v_mount),
		    EFS_VTOI(ap->a_dvp), ap->a_cnp, &ino);
		if (err)
			return (err);

		VOP_UNLOCK(ap->a_dvp, 0);	/* preserve lock order */

		err = VFS_VGET(ap->a_dvp->v_mount, ino, &vp);
		if (err) {
			vn_lock(ap->a_dvp, LK_EXCLUSIVE | LK_RETRY);
			return (err);
		}
		vn_lock(ap->a_dvp, LK_EXCLUSIVE | LK_RETRY);
		*ap->a_vpp = vp;
	} else {
		err = efs_inode_lookup(VFSTOEFS(ap->a_dvp->v_mount),
		    EFS_VTOI(ap->a_dvp), ap->a_cnp, &ino);
		if (err) {
			if (err == ENOENT && (cnp->cn_flags & MAKEENTRY) &&
			    nameiop != CREATE)
				cache_enter(ap->a_dvp, NULL, cnp);
			if (err == ENOENT && (nameiop == CREATE ||
			    nameiop == RENAME)) {
				err = VOP_ACCESS(vp, VWRITE, cnp->cn_cred,
				    cnp->cn_lwp);
				if (err)
					return (err);
				cnp->cn_flags |= SAVENAME;
				return (EJUSTRETURN);
			}
			return (err);
		}
		err = VFS_VGET(ap->a_dvp->v_mount, ino, &vp);
		if (err)
			return (err);
		*ap->a_vpp = vp;
	}

	if (cnp->cn_flags & MAKEENTRY)
		cache_enter(ap->a_dvp, *ap->a_vpp, cnp);

	return (0);
}

/*
 * Determine the accessiblity of a file based on the permissions allowed by the
 * specified credentials.
 *
 * Returns 0 on success.
 */
static int
efs_access(void *v)
{
	struct vop_access_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct efs_inode *eip = EFS_VTOI(vp);

	if ((ap->a_mode & VWRITE) && (vp->v_mount->mnt_flag & MNT_RDONLY))
		return (EROFS);

	return (vaccess(vp->v_type, eip->ei_mode, eip->ei_uid, eip->ei_gid,
	    ap->a_mode, ap->a_cred));
}

/*
 * Get specific vnode attributes on a file. See vattr(9).
 *
 * Returns 0 on success.
 */
static int
efs_getattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap; 
		struct ucred *a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	struct vattr *vap = ap->a_vap;
	struct efs_inode *eip = EFS_VTOI(ap->a_vp);

	vattr_null(ap->a_vap);
	vap->va_type		= ap->a_vp->v_type;
	vap->va_mode		= eip->ei_mode;
	vap->va_nlink		= eip->ei_nlink;
	vap->va_uid		= eip->ei_uid;
	vap->va_gid		= eip->ei_gid;
	vap->va_fsid 		= ap->a_vp->v_mount->mnt_stat.f_fsid;
	vap->va_fileid 		= eip->ei_number;
	vap->va_size 		= eip->ei_size;

	if (ap->a_vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (ap->a_vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = EFS_BB_SIZE;

	vap->va_atime.tv_sec	= eip->ei_atime;
	vap->va_mtime.tv_sec	= eip->ei_mtime;
	vap->va_ctime.tv_sec	= eip->ei_ctime;
/*	vap->va_birthtime 	= */
	vap->va_gen		= eip->ei_gen;
	vap->va_flags		= ap->a_vp->v_vflag | ap->a_vp->v_iflag;

	if (ap->a_vp->v_type == VBLK || ap->a_vp->v_type == VCHR) {
		uint32_t dmaj, dmin;

		if (be16toh(eip->ei_di.di_odev) != EFS_DINODE_ODEV_INVALID) {
			dmaj = EFS_DINODE_ODEV_MAJ(be16toh(eip->ei_di.di_odev));
			dmin = EFS_DINODE_ODEV_MIN(be16toh(eip->ei_di.di_odev));
		} else {
			dmaj = EFS_DINODE_NDEV_MAJ(be32toh(eip->ei_di.di_ndev));
			dmin = EFS_DINODE_NDEV_MIN(be32toh(eip->ei_di.di_ndev));
		}

		vap->va_rdev = makedev(dmaj, dmin);
	}

	vap->va_bytes		= eip->ei_size;
/*	vap->va_filerev		= */
/*	vap->va_vaflags		= */
	
	return (0);
}

/*
 * Read a file.
 *
 * Returns 0 on success.
 */
static int
efs_read(void *v)
{
	struct vop_read_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio; 
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct efs_extent ex;
	struct efs_extent_iterator exi;
	void *win;
	struct uio *uio = ap->a_uio;
	struct efs_inode *eip = EFS_VTOI(ap->a_vp);
	off_t start;
	vsize_t len;
	int err, ret, flags;
	const int advice = IO_ADV_DECODE(ap->a_ioflag);

	if (ap->a_vp->v_type == VDIR)
		return (EISDIR);

	if (ap->a_vp->v_type != VREG)
		return (EINVAL);

	efs_extent_iterator_init(&exi, eip, uio->uio_offset);
	ret = efs_extent_iterator_next(&exi, &ex);
	while (ret == 0) {
		if (uio->uio_offset < 0 || uio->uio_offset >= eip->ei_size ||
		    uio->uio_resid == 0)
			break;

		start = ex.ex_offset * EFS_BB_SIZE;
		len   = ex.ex_length * EFS_BB_SIZE;

		if (!(uio->uio_offset >= start &&
		      uio->uio_offset < (start + len))) {
			ret = efs_extent_iterator_next(&exi, &ex);
			continue;
		}

		start = uio->uio_offset - start;

		len = MIN(len - start, uio->uio_resid);
		len = MIN(len, eip->ei_size - uio->uio_offset);

		win = ubc_alloc(&ap->a_vp->v_uobj, uio->uio_offset,
		    &len, advice, UBC_READ);

		flags = UBC_WANT_UNMAP(ap->a_vp) ? UBC_UNMAP : 0;

		err = uiomove(win, len, uio);
		ubc_release(win, flags);
		if (err) {
			EFS_DPRINTF(("efs_read: uiomove error %d\n",
			    err));
			return (err);
		}
	}

	return ((ret == -1) ? 0 : ret);
}

static int
efs_readdir(void *v)
{
	struct vop_readdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp; 
		struct uio *a_uio; 
		struct ucred *a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	struct efs_dinode edi;
	struct efs_extent ex;
	struct efs_extent_iterator exi;
	struct buf *bp;
	struct dirent *dp;
	struct efs_dirent *de;
	struct efs_dirblk *db;
	struct uio *uio = ap->a_uio;
	struct efs_inode *ei = EFS_VTOI(ap->a_vp);
	off_t offset;
	int i, j, err, ret, s, slot;

	/* XXX - ncookies, cookies for NFS */

	if (ap->a_vp->v_type != VDIR)
		return (ENOTDIR);

	offset = 0;

	efs_extent_iterator_init(&exi, ei, 0);
	while ((ret = efs_extent_iterator_next(&exi, &ex)) == 0) {
		for (i = 0; i < ex.ex_length; i++) {
			err = efs_bread(VFSTOEFS(ap->a_vp->v_mount),
			    ex.ex_bn + i, NULL, &bp);
			if (err) {
				brelse(bp, 0);
				return (err);
			}

			db = (struct efs_dirblk *)bp->b_data;

			if (be16toh(db->db_magic) != EFS_DIRBLK_MAGIC) {
				printf("efs_readdir: bad dirblk\n");
				brelse(bp, 0);
				continue;
			}

			for (j = 0; j < db->db_slots; j++) {
				slot = EFS_DIRENT_OFF_EXPND(db->db_space[j]);
				if (slot == EFS_DIRBLK_SLOT_FREE)
					continue;

				if (!EFS_DIRENT_OFF_VALID(slot)) {
					printf("efs_readdir: bad dirent\n");
					continue;
				}

				de = EFS_DIRBLK_TO_DIRENT(db, slot);
				s = _DIRENT_RECLEN(dp, de->de_namelen);

				if (offset < uio->uio_offset) {
					offset += s;
					continue;
				}

				if (offset > uio->uio_offset) {
					/* XXX - shouldn't happen, right? */
					brelse(bp, 0);
					return (0);
				}

				if (s > uio->uio_resid) {
					brelse(bp, 0);
					return (0);
				}

				dp = malloc(s, M_EFSTMP, M_ZERO | M_WAITOK);
				dp->d_fileno = be32toh(de->de_inumber);
				dp->d_reclen = s;
				dp->d_namlen = de->de_namelen;
				memcpy(dp->d_name, de->de_name,
				    de->de_namelen);
				dp->d_name[de->de_namelen] = '\0';

				/* look up inode to get type */
				err = efs_read_inode(
				    VFSTOEFS(ap->a_vp->v_mount),
				    dp->d_fileno, NULL, &edi);
				if (err) {
					brelse(bp, 0);
					free(dp, M_EFSTMP);
					return (err);
				}

				switch (be16toh(edi.di_mode) & EFS_IFMT) {
				case EFS_IFIFO:
					dp->d_type = DT_FIFO;
					break;
				case EFS_IFCHR:
					dp->d_type = DT_CHR;
					break;
				case EFS_IFDIR:
					dp->d_type = DT_DIR;
					break;
				case EFS_IFBLK:
					dp->d_type = DT_BLK;
					break;
				case EFS_IFREG:
					dp->d_type = DT_REG;
					break;
				case EFS_IFLNK:
					dp->d_type = DT_LNK;
					break;
				case EFS_IFSOCK:
					dp->d_type = DT_SOCK;
					break;
				default:
					dp->d_type = DT_UNKNOWN;
					break;
				}

				err = uiomove(dp, s, uio);
				free(dp, M_EFSTMP);
				if (err) {
					brelse(bp, 0);
					return (err);	
				}

				offset += s;
			}

			brelse(bp, 0);
		}
	}

	if (ret != -1)
		return (ret);

	if (ap->a_eofflag != NULL)
		*ap->a_eofflag = true;

	return (0);
}

static int
efs_readlink(void *v)
{
	struct vop_readlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap = v;
	struct uio *uio = ap->a_uio;
	struct efs_inode *eip = EFS_VTOI(ap->a_vp);
	char *buf;
	size_t len;
	int err, i;

	if ((eip->ei_mode & EFS_IFMT) != EFS_IFLNK)
		return (EINVAL);

	if (uio->uio_resid < 1)
		return (EINVAL);

	buf = malloc(eip->ei_size + 1, M_EFSTMP, M_ZERO | M_WAITOK);

	/* symlinks are either inlined in the inode, or in extents */
	if (eip->ei_numextents == 0) {
		if (eip->ei_size > sizeof(eip->ei_di.di_symlink)) {
			EFS_DPRINTF(("efs_readlink: too big for inline\n"));
			free(buf, M_EFSTMP);
			return (EBADF);
		}

		memcpy(buf, eip->ei_di.di_symlink, eip->ei_size);
		len = MIN(uio->uio_resid, eip->ei_size + 1);
	} else {
		struct efs_extent_iterator exi;
		struct efs_extent ex;
		struct buf *bp;
		int resid, off, ret;

		off = 0;
		resid = eip->ei_size;

		efs_extent_iterator_init(&exi, eip, 0);
		while ((ret = efs_extent_iterator_next(&exi, &ex)) == 0) {
			for (i = 0; i < ex.ex_length; i++) {
				err = efs_bread(VFSTOEFS(ap->a_vp->v_mount),
				    ex.ex_bn + i, NULL, &bp);
				if (err) {
					brelse(bp, 0);
					free(buf, M_EFSTMP);
					return (err);
				}

				len = MIN(resid, bp->b_bcount);
				memcpy(buf + off, bp->b_data, len);
				brelse(bp, 0);

				off += len;
				resid -= len;

				if (resid == 0)
					break;
			}

			if (resid == 0)
				break;
		}

		if (ret != 0 && ret != -1) {
			free(buf, M_EFSTMP);
			return (ret);
		}

		len = off + 1;
	}

	KASSERT(len >= 1 && len <= (eip->ei_size + 1));
	buf[len - 1] = '\0';
	err = uiomove(buf, len, uio);
	free(buf, M_EFSTMP);

	return (err);
}

/*
 * Release an inactive vnode. The vnode _must_ be unlocked on return.
 * It is either nolonger being used by the kernel, or an unmount is being
 * forced.
 *
 * Returns 0 on success.
 */
static int
efs_inactive(void *v)
{
	struct vop_inactive_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct efs_inode *eip = EFS_VTOI(ap->a_vp);

	VOP_UNLOCK(ap->a_vp, 0);

	if (eip->ei_mode == 0)
		vrecycle(ap->a_vp, NULL, ap->a_l);

	return (0);
}

static int
efs_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	efs_ihashrem(EFS_VTOI(vp));
	cache_purge(vp);
	genfs_node_destroy(vp);
	pool_put(&efs_inode_pool, vp->v_data);
	vp->v_data = NULL;

	return (0);
}

static int
efs_bmap(void *v)
{
	struct vop_bmap_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		daddr_t a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;
	struct efs_extent ex;
	struct efs_extent_iterator exi;
	struct vnode *vp = ap->a_vp;
	struct efs_inode *eip = EFS_VTOI(vp);
	bool found;
	int ret;

	if (ap->a_vpp != NULL)
		*ap->a_vpp = VFSTOEFS(vp->v_mount)->em_devvp;

	found = false;
	efs_extent_iterator_init(&exi, eip, ap->a_bn * EFS_BB_SIZE);
	while ((ret = efs_extent_iterator_next(&exi, &ex)) == 0) {
		if (ap->a_bn >= ex.ex_offset &&
		    ap->a_bn < (ex.ex_offset + ex.ex_length)) {
			found = true;
			break;
		}
	}

	KASSERT(!found || (found && ret == 0));

	if (!found) {
		EFS_DPRINTF(("efs_bmap: ap->a_bn not in extents\n"));
		return ((ret == -1) ? EIO : ret);
	}

	if (ex.ex_magic != EFS_EXTENT_MAGIC) {
		EFS_DPRINTF(("efs_bmap: exn.ex_magic != EFS_EXTENT_MAGIC\n"));
		return (EIO);
	}

	if (ap->a_bn < ex.ex_offset) {
		EFS_DPRINTF(("efs_bmap: ap->a_bn < exn.ex_offset\n"));
		return (EIO);
	}

	KASSERT(ap->a_bn >= ex.ex_offset);
	KASSERT(ex.ex_length > ap->a_bn - ex.ex_offset);

	*ap->a_bnp = ex.ex_bn + (ap->a_bn - ex.ex_offset);
	if (ap->a_runp != NULL)
		*ap->a_runp = ex.ex_length - (ap->a_bn - ex.ex_offset) - 1;

	return (0);
}

static int
efs_strategy(void *v)
{
	struct vop_strategy_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf *bp = ap->a_bp;
	int error;

	if (vp == NULL) {
		biodone(bp, EIO, 0);
		return (EIO);
	}

	if (bp->b_blkno == bp->b_lblkno) {
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
		if (error) {
			biodone(bp, error, 0);
			return (error);
		}
		if ((long)bp->b_blkno == -1)
			clrbuf(bp);
	}

	if ((long)bp->b_blkno == -1) {
		biodone(bp, 0, bp->b_resid);
		return (0);
	}

	return (VOP_STRATEGY(VFSTOEFS(vp->v_mount)->em_devvp, bp));
}

static int
efs_print(void *v)
{
	struct vop_print_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
	} */ *ap = v;
	struct efs_inode *eip = EFS_VTOI(ap->a_vp);

	printf(	"efs_inode (ino %lu):\n"
		"    ei_mode:         %07o\n"
		"    ei_nlink:        %d\n"
		"    ei_uid:          %d\n"
		"    ei_gid:          %d\n"
		"    ei_size:         %d\n"
		"    ei_atime:        %d\n"
		"    ei_mtime:        %d\n"
		"    ei_ctime:        %d\n"
		"    ei_gen:          %d\n"
		"    ei_numextents:   %d\n"
		"    ei_version:      %d\n",
		(unsigned long)eip->ei_number,
		(unsigned int)eip->ei_mode,
		eip->ei_nlink,
		eip->ei_uid,
		eip->ei_gid,
		eip->ei_size,
		(int32_t)eip->ei_atime,
		(int32_t)eip->ei_mtime,
		(int32_t)eip->ei_ctime,
		eip->ei_gen,
		eip->ei_numextents,
		eip->ei_version);

	return (0);
}

static int
efs_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;

	/* IRIX 4 values */
	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = 30000; 
		break;
	case _PC_NAME_MAX:
		*ap->a_retval = 255;
		break;
	case _PC_PATH_MAX:
		*ap->a_retval = 1024;
		break;
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		break;
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		break;
	case _PC_SYNC_IO:
		*ap->a_retval = 1;
		break;
	case _PC_FILESIZEBITS:
		*ap->a_retval = 32;
		break;
	default:
		return (EINVAL);
	}

	return (0);
}

static int
efs_advlock(void *v)
{
	struct vop_advlock_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	struct efs_inode *eip = EFS_VTOI(ap->a_vp);

	return (lf_advlock(ap, &eip->ei_lockf, eip->ei_size));
}

/* Global vfs data structures for efs */
int (**efs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc efs_vnodeop_entries[] = {
	{ &vop_default_desc,	vn_default_error},	/* error handler */
	{ &vop_lookup_desc,	efs_lookup	},	/* lookup */
	{ &vop_create_desc,	genfs_eopnotsupp},	/* create */
	{ &vop_mknod_desc,	genfs_eopnotsupp},	/* mknod */
	{ &vop_open_desc,	genfs_nullop	},	/* open */
	{ &vop_close_desc,	genfs_nullop	},	/* close */
	{ &vop_access_desc,	efs_access	},	/* access */
	{ &vop_getattr_desc,	efs_getattr	},	/* getattr */
	{ &vop_setattr_desc,	genfs_eopnotsupp},	/* setattr */
	{ &vop_read_desc,	efs_read	},	/* read */
	{ &vop_write_desc,	genfs_eopnotsupp},	/* write */
	{ &vop_ioctl_desc,	genfs_enoioctl	},	/* ioctl */
	{ &vop_fcntl_desc,	genfs_fcntl	},	/* fcntl */
	{ &vop_poll_desc,	genfs_poll	},	/* poll */
	{ &vop_kqfilter_desc,	genfs_kqfilter	},	/* kqfilter */
	{ &vop_revoke_desc,	genfs_revoke	},	/* revoke */
	{ &vop_mmap_desc,	genfs_mmap	},	/* mmap */
	{ &vop_fsync_desc,	genfs_eopnotsupp},	/* fsync */
	{ &vop_seek_desc,	genfs_seek	},	/* seek */
	{ &vop_remove_desc,	genfs_eopnotsupp},	/* remove */
	{ &vop_link_desc,	genfs_eopnotsupp},	/* link */
	{ &vop_rename_desc,	genfs_eopnotsupp},	/* rename */
	{ &vop_mkdir_desc,	genfs_eopnotsupp},	/* mkdir */
	{ &vop_rmdir_desc,	genfs_eopnotsupp},	/* rmdir */
	{ &vop_symlink_desc,	genfs_eopnotsupp},	/* symlink */
	{ &vop_readdir_desc,	efs_readdir	},	/* readdir */
	{ &vop_readlink_desc,	efs_readlink	},	/* readlink */
	{ &vop_abortop_desc,	genfs_abortop	},	/* abortop */
	{ &vop_inactive_desc,	efs_inactive	},	/* inactive */
	{ &vop_reclaim_desc,	efs_reclaim	},	/* reclaim */
	{ &vop_lock_desc,	genfs_lock,	},	/* lock */
	{ &vop_unlock_desc,	genfs_unlock,	},	/* unlock */
	{ &vop_islocked_desc,	genfs_islocked,	},	/* islocked */
	{ &vop_bmap_desc,	efs_bmap	},	/* bmap */
	{ &vop_print_desc,	efs_print	},	/* print */
	{ &vop_pathconf_desc,	efs_pathconf	},	/* pathconf */
	{ &vop_advlock_desc,	efs_advlock	},	/* advlock */
							/* blkatoff */
							/* valloc */
							/* balloc */
							/* vfree */
							/* truncate */
	{ &vop_lease_desc,	genfs_lease_check },	/* lease */
							/* whiteout */
	{ &vop_getpages_desc,	genfs_getpages	},	/* getpages */
	{ &vop_putpages_desc,	genfs_putpages	},	/* putpages */
	{ &vop_bwrite_desc,	vn_bwrite	},	/* bwrite */
	{ &vop_strategy_desc,	efs_strategy	},	/* strategy */
	{ NULL, NULL }
};
const struct vnodeopv_desc efs_vnodeop_opv_desc = {
	&efs_vnodeop_p,
	efs_vnodeop_entries
};
