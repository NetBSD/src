/*	$NetBSD: exfatfs_vnops.c,v 1.1.2.5 2024/07/03 18:57:42 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exfatfs_vnops.c,v 1.1.2.5 2024/07/03 18:57:42 perseant Exp $");

#include <sys/buf.h>
#include <sys/dirent.h>
#include <sys/file.h>		/* define FWRITE ... */
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/lockf.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>	/* defines plimit structure in proc struct */
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h> /* XXX */	/* defines v_rdev */

#include <uvm/uvm_extern.h>

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_balloc.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_mount.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_rename.h>
#include <fs/exfatfs/exfatfs_tables.h>
#include <fs/exfatfs/exfatfs_vnops.h>
#include <fs/exfatfs/exfatfs_vfsops.h>

struct componentname;

/*
 * Directory entry manipulation
 */
static int detrunc(struct xfinode *, off_t, int, kauth_cred_t);
static int deextend(struct xfinode *, off_t, int, kauth_cred_t);
static int exfatfs_alloc(struct vnode *, struct vnode **,
			 struct componentname *,
			 struct vattr *, uint32_t);
static unsigned first_valid_file(void *v, struct xfinode *xip, off_t);
static unsigned lookup_validfunc(void *v, struct xfinode *xip, off_t);
static unsigned readdir_validfunc(void *v, struct xfinode *xip, off_t);

extern struct genfs_ops exfatfs_genfsops;

static uint8_t PRIMARY_IGNORE[2] = { 2, 3 };
static uint8_t PRIMARY_IGNORE_LEN = 2;

#define USE_INACTIVE

#ifdef EXFATFS_VNOPS_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x) __nothing
#endif

/*
 * Create just allocated a new entry of type "file".
 */
int
exfatfs_create(void *v)
{
	struct vop_create_v3_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;

	return exfatfs_alloc(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap, 0);
}

int
exfatfs_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct xfinode *xip = VTOXI(vp);

	mutex_enter(vp->v_interlock);
	if (vp->v_usecount > 1)
		XITIMES(xip, NULL, NULL, NULL);
	mutex_exit(vp->v_interlock);
	return (0);
}

int
exfatfs_access(void *v)
{
	struct vop_access_args /* {
		struct vnode *a_vp;
		accmode_t a_accmode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct xfinode *xip = VTOXI(vp);
	struct exfatfs *fs = xip->xi_fs;
	mode_t file_mode, mode = ap->a_accmode;
	kauth_cred_t cred = ap->a_cred;
	int r;

	/* No writes to read-only file systems, regardless of permissions */
	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY)
				return (EROFS);
		default:
			break;
		}
	}

	/* Gen up Unix-style permissions based on mount options */
	if (ISREADONLY(xip))
		file_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
	else
		file_mode = S_IRWXU|S_IRWXG|S_IRWXO;
	file_mode &= (vp->v_type == VDIR ? fs->xf_dirmask : fs->xf_mask);

	/* Apply the standard permissions check. */
	r = kauth_authorize_vnode(cred,
				  KAUTH_ACCESS_ACTION(mode,
						      vp->v_type,
						      file_mode),
				  vp, NULL,
				  genfs_can_access(vp, cred, fs->xf_uid,
						   fs->xf_gid, file_mode,
						   NULL, mode));
#ifdef TRACE_INUM
	if (xip->xi_parentvp != NULL
	    && INUM(VTOXI(xip->xi_parentvp)) == TRACE_INUM
	    && (mode & VEXEC)) {
		printf("access() inum 0x%lx mode %o returning %d\n",
		       INUM(xip), mode, r);
		xip->xi_trace = 1;
	}
#endif /* TRACE_INUM */

	return r;
}

int
exfatfs_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct xfinode *xip = VTOXI(ap->a_vp);
	struct exfatfs *fs = xip->xi_fs;
	struct vattr *vap = ap->a_vap;
	mode_t mode;

	DPRINTF(("getattr %u/%u fs=%p\n", xip->xi_dirclust, xip->xi_diroffset,
		 fs));
	
	XITIMES(xip, NULL, NULL, NULL);
	vap->va_fsid = xip->xi_devvp->v_rdev;
	vap->va_fileid = INUM(xip);

	if (ISREADONLY(xip))
		mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
	else
		mode = S_IRWXU|S_IRWXG|S_IRWXO;
	vap->va_mode =
	    mode & (ap->a_vp->v_type == VDIR ? fs->xf_dirmask : fs->xf_mask);
	vap->va_uid = fs->xf_uid;
	vap->va_gid = fs->xf_gid;
	vap->va_nlink = 1;
	vap->va_rdev = 0;
	vap->va_size = ap->a_vp->v_size;
	DPRINTF((" dos2unixtime\n"));
	exfatfs_dos2unixtime(GET_DFE_LAST_MODIFIED(xip),
			     GET_DFE_LAST_MODIFIED10MS(xip), fs->xf_gmtoff,
			     &vap->va_mtime);
	exfatfs_dos2unixtime(GET_DFE_LAST_ACCESSED(xip), 0, fs->xf_gmtoff,
			     &vap->va_atime);
	exfatfs_dos2unixtime(GET_DFE_CREATE(xip),
			     GET_DFE_CREATE10MS(xip), fs->xf_gmtoff,
			     &vap->va_ctime);
	vap->va_flags = 0;
	if (ISARCHIVE(xip)) {
		vap->va_flags |= SF_ARCHIVED;
		vap->va_mode  |= S_ARCH1;
	}
	vap->va_gen = 0;
	vap->va_blocksize = SECSIZE(fs);
	vap->va_bytes = GET_DSE_DATALENGTH_BLK(xip, fs);
	vap->va_type = ap->a_vp->v_type;
	DPRINTF((" getattr returning 0\n"));
	return (0);
}

int
exfatfs_setattr(void *v)
{
	struct vop_setattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
	} */ *ap = v;
	int error = 0, de_changed = 0;
	struct xfinode *xip = VTOXI(ap->a_vp);
	struct exfatfs *fs = xip->xi_fs;
	struct vnode *vp  = ap->a_vp;
	struct vattr *vap = ap->a_vap;
	kauth_cred_t cred = ap->a_cred;

	DPRINTF(("exfatfs_setattr(): vp %p, vap %p, cred %p\n",
		 ap->a_vp, vap, cred));
	/*
	 * Note we silently ignore uid or gid changes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != (nlink_t)VNOVAL) ||
#if 0
	    (vap->va_uid != VNOVAL && vap->va_uid != fs->xf_uid) ||
	    (vap->va_gid != VNOVAL && vap->va_gid != fs->xf_gid) ||
#endif /* 0 */
	    (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
	    (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
	    (vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
		return (EINVAL);
	}
	/*
	 * Silently ignore attributes modifications on directories.
	 */
	if (ap->a_vp->v_type == VDIR)
		return 0;

	if (vap->va_size != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto bad;
		}
		error = detrunc(xip, (u_long)vap->va_size, 0, cred);
		if (error)
			goto bad;
		de_changed = 1;
	}
	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto bad;
		}
		error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_TIMES,
			ap->a_vp, NULL, genfs_can_chtimes(ap->a_vp, cred,
			fs->xf_uid, vap->va_vaflags));
		if (error)
			goto bad;
		if (vap->va_atime.tv_sec != VNOVAL)
			exfatfs_unix2dostime(&vap->va_atime, fs->xf_gmtoff,
				&DFE(xip)->xd_lastAccessedTimestamp, NULL);
		if (vap->va_mtime.tv_sec != VNOVAL)
			exfatfs_unix2dostime(&vap->va_mtime, fs->xf_gmtoff,
				&DFE(xip)->xd_lastModifiedTimestamp,
				&DFE(xip)->xd_lastModified10msIncrement);
		DFE(xip)->xd_fileAttributes |= XD_FILEATTR_ARCHIVE;
		xip->xi_flag |= XI_MODIFIED;
		de_changed = 1;
	}
	/*
	 * DOS files only have the ability to have their writability
	 * attribute set, so we use the owner write bit to set the readonly
	 * attribute.
	 */
	if (vap->va_mode != (mode_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto bad;
		}
		error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_FLAGS, vp,
			NULL, genfs_can_chflags(vp, cred, fs->xf_uid, false));
		if (error)
			goto bad;
		/* We ignore the read and execute bits. */
		if (vap->va_mode & S_IWUSR)
			DFE(xip)->xd_fileAttributes &= ~XD_FILEATTR_READONLY;
		else
			DFE(xip)->xd_fileAttributes |= XD_FILEATTR_READONLY;
		xip->xi_flag |= XI_MODIFIED;
		de_changed = 1;
	}
	/*
	 * Allow the `archived' bit to be toggled.
	 */
	if (vap->va_flags != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto bad;
		}
		error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_FLAGS, vp,
			NULL, genfs_can_chflags(vp, cred, fs->xf_uid, false));
		if (error)
			goto bad;
		if (vap->va_flags & SF_ARCHIVED)
			DFE(xip)->xd_fileAttributes &= ~XD_FILEATTR_ARCHIVE;
		else
			DFE(xip)->xd_fileAttributes |= XD_FILEATTR_ARCHIVE;
		xip->xi_flag |= XI_MODIFIED;
		de_changed = 1;
	}

	if (de_changed) {
		uvm_vnp_setsize(vp, GET_DSE_VALIDDATALENGTH(xip));
		VN_KNOTE(vp, NOTE_ATTRIB);
		error = exfatfs_update(vp, NULL, NULL, UPDATE_WAIT);
		if (error)
			goto bad;
	}

bad:
	return error;
}

int
exfatfs_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	int error = 0;
	long n, off;
	daddr_t lbn;
	vsize_t bytelen;
	struct buf *bp;
	struct vnode *vp = ap->a_vp;
	struct xfinode *xip = VTOXI(vp);
	struct exfatfs *fs = xip->xi_fs;
	struct uio *uio = ap->a_uio;

	/*
	 * If they didn't ask for any data, then we are done.
	 */

	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);
	if (uio->uio_offset >= GET_DSE_VALIDDATALENGTH(xip))
		return (0);

	if (vp->v_type == VREG) {
		const int advice = IO_ADV_DECODE(ap->a_ioflag);

		while (uio->uio_resid > 0) {
			bytelen = MIN(GET_DSE_VALIDDATALENGTH(xip)
				      - uio->uio_offset,
				      uio->uio_resid);

			if (bytelen == 0)
				break;
			error = ubc_uiomove(&vp->v_uobj, uio, bytelen, advice,
			    UBC_READ | UBC_PARTIALOK | UBC_VNODE_FLAGS(vp));
			if (error)
				break;
		}
		xip->xi_flag |= XI_ACCESS;
		goto out;
	}

	/* this loop is only for directories now */
	do {
		if (uio->uio_offset >= GET_DSE_VALIDDATALENGTH(xip))
			return 0;

		lbn = uio->uio_offset >> IOSHIFT(fs);
		error = bread(vp, lbn, IOSIZE(fs), 0, &bp);
		if (error)
			goto bad;

		off = uio->uio_offset & IOMASK(fs);
		n = uio->uio_resid - uio->uio_offset;
		if (uio->uio_offset + uio->uio_resid >
		    GET_DSE_VALIDDATALENGTH(xip))
			n -= uio->uio_offset + uio->uio_resid
			     - GET_DSE_VALIDDATALENGTH(xip);
		n = MIN(n, bp->b_resid - off);
		if (n > 0)
			error = uiomove((char *)bp->b_data + off, n, uio);
		brelse(bp, 0);
	} while (error == 0 && uio->uio_resid > 0 && n > 0);

out:
	if ((ap->a_ioflag & IO_SYNC) == IO_SYNC) {
		int uerror;

		uerror = exfatfs_update(XITOV(xip), NULL, NULL, UPDATE_WAIT);
		if (error == 0)
			error = uerror;
	}
bad:
	return (error);
}

/*
 * Write data to a file or directory.
 */
int
exfatfs_write(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	int resid, extended = 0;
	int error = 0;
	int ioflag = ap->a_ioflag;
	u_long osize;
	vsize_t bytelen;
	off_t oldoff;
	size_t rem;
	struct uio *uio = ap->a_uio;
	struct vnode *vp = ap->a_vp;
	struct xfinode *xip = VTOXI(vp);
	kauth_cred_t cred = ap->a_cred;
	bool async;

#if 0	
	DPRINTF(("exfatfs_write(vp %p, uio %p, ioflag %x, cred %p\n",
		 vp, uio, ioflag, cred));
	DPRINTF(("exfatfs_write(uio_offset %lld, uio_resid %lld\n",
		 (long long)uio->uio_offset, (long long)uio->uio_resid));
	DPRINTF(("exfatfs_write(): diroff %u, dirclust %u, startcluster %u\n",
		 xip->xi_diroffset, xip->xi_dirclust, xip->xi_firstCluster));
#endif
	
	switch (vp->v_type) {
	case VREG:
		if (ioflag & IO_APPEND)
			uio->uio_offset = GET_DSE_VALIDDATALENGTH(xip);
		break;
	case VDIR:
		return EISDIR;
	default:
		panic("exfatfs_write(): bad file type");
	}

	if (uio->uio_offset < 0)
		return (EINVAL);

	if (uio->uio_resid == 0)
		return (0);

	/* Don't bother to try to write files larger than the fs limit */
	if (uio->uio_offset + uio->uio_resid > EXFATFS_FILESIZE_MAX)
		return (EFBIG);

	/*
	 * Remember some values in case the write fails.
	 */
	async = vp->v_mount->mnt_flag & MNT_ASYNC;
	resid = uio->uio_resid;
	osize = GET_DSE_VALIDDATALENGTH(xip);

	/*
	 * If we write beyond the end of the file, extend it to its ultimate
	 * size ahead of time to hopefully get a contiguous area.
	 */
	if (uio->uio_offset + resid > GET_DSE_VALIDDATALENGTH(xip)) {
		/* DPRINTF(("write past valid data length\n")); */
		if (uio->uio_offset + resid > GET_DSE_DATALENGTH_BLK(xip,
								xip->xi_fs)) {
			DPRINTF(("write past allocation\n"));
			if ((error = deextend(xip, uio->uio_offset + resid,
					      ioflag, cred)) != 0)
				goto errexit;
			DPRINTF(("now vdl=%llu dl=%llu\n",
				 (unsigned long long)
					GET_DSE_VALIDDATALENGTH(xip),
				 (unsigned long long)
					GET_DSE_DATALENGTH_BLK(xip, fs)));
		}

		SET_DSE_DATALENGTH(xip, uio->uio_offset + resid);
		SET_DSE_VALIDDATALENGTH(xip, uio->uio_offset + resid);
		/* hint uvm to not read in extended part */
		uvm_vnp_setwritesize(vp, GET_DSE_VALIDDATALENGTH(xip));
		/* zero out the remainder of the last page */
		rem = round_page(GET_DSE_VALIDDATALENGTH(xip))
			- GET_DSE_VALIDDATALENGTH(xip);
		if (rem > 0)
			ubc_zerorange(&vp->v_uobj,
				      (off_t)GET_DSE_VALIDDATALENGTH(xip),
			    rem, UBC_VNODE_FLAGS(vp));
		extended = 1;
	}

	do {
		oldoff = uio->uio_offset;
		bytelen = uio->uio_resid;

		error = ubc_uiomove(&vp->v_uobj, uio, bytelen,
		    IO_ADV_DECODE(ioflag), UBC_WRITE | UBC_VNODE_FLAGS(vp));
		if (error) {
			printf("ubc_uiomove error %d\n", error);
			break;
		}

		/*
		 * flush what we just wrote if necessary.
		 * XXXUBC simplistic async flushing.
		 */

		if (!async && oldoff >> 16 != uio->uio_offset >> 16) {
			rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
			error = VOP_PUTPAGES(vp, (oldoff >> 16) << 16,
			    (uio->uio_offset >> 16) << 16,
			    PGO_CLEANIT | PGO_LAZY);
		}
	} while (error == 0 && uio->uio_resid > 0);

	/* set final size */
	uvm_vnp_setsize(vp, GET_DSE_VALIDDATALENGTH(xip));
	if (error == 0 && ioflag & IO_SYNC) {
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		error = VOP_PUTPAGES(vp, trunc_page(oldoff),
		    round_page(oldoff + bytelen), PGO_CLEANIT | PGO_SYNCIO);
	}
	xip->xi_flag |= XI_UPDATE;

	/*
	 * If the write failed and they want us to, truncate the file back
	 * to the size it was before the write was attempted.
	 */
errexit:
	if (resid > uio->uio_resid)
		VN_KNOTE(vp, NOTE_WRITE | (extended ? NOTE_EXTEND : 0));
	if (error) {
		detrunc(xip, osize, ioflag & IO_SYNC, NOCRED);
		uio->uio_offset -= resid - uio->uio_resid;
		uio->uio_resid = resid;
		uvm_vnp_setsize(vp, GET_DSE_VALIDDATALENGTH(xip));
	} else if ((ioflag & IO_SYNC) == IO_SYNC)
		error = exfatfs_update(vp, NULL, NULL, UPDATE_WAIT);
	KASSERT(vp->v_size == GET_DSE_VALIDDATALENGTH(xip));
#if 0
	DPRINTF(("exfatfs_write returning %d, flags 0x%x\n", error,
		 (unsigned)xip->xi_flag));
#endif
	return (error);
}

int
exfatfs_writeback(struct xfinode *xip)
{
	int i, error, inuse;
	uint16_t cksum;
	struct buf *bp;
	struct exfatfs *fs = xip->xi_fs;

	DPRINTF(("exfatfs_writeback(%p) with fs=%p\n", xip, fs));

	if (xip->xi_refcnt == 0)
		return 0;

	inuse = xip->xi_direntp[0]->xd_entryType & XD_ENTRYTYPE_INUSE_MASK;
	if (XITOV(xip) == fs->xf_rootvp)
		inuse = 1;

	/* Compute new checksum */
	cksum = 0;
	for (i = 0; i < EXFATFS_MAXDIRENT && xip->xi_direntp[i]; i++) {
		DPRINTF((" cksum i=%i (%p)\n", i, xip->xi_direntp[i]));
		cksum = exfatfs_cksum16(cksum, (uint8_t *)xip->xi_direntp[i],
					sizeof(struct exfatfs_dirent),
					PRIMARY_IGNORE,
					(i == 0 ? PRIMARY_IGNORE_LEN : 0));
	}
	SET_DFE_SET_CHECKSUM(xip, cksum);

	/*
	 * Write to the pre-recorded file locations.
	 */
	for (i = 0; i < EXFATFS_MAXDIRENT
		     && xip->xi_direntp[i]
		     && xip->xi_dirent_blk[i]; i++) {
		DPRINTF((" write inum 0x%lx i=%d type 0x%x bn %lx off %d"
			 " = cn 0x%lx off 0x%lx\n",
			 (unsigned long)INUM(xip), i,
			 xip->xi_direntp[i]->xd_entryType,
			 xip->xi_dirent_blk[i],
			 xip->xi_dirent_off[i],
			 EXFATFS_HWADDR2CLUSTER(fs, xip->xi_dirent_blk[i]),
			 EXFATFS_HWADDR2DIRENT(fs, xip->xi_dirent_blk[i])
			 + xip->xi_dirent_off[i]));
		if ((error = bread(xip->xi_devvp, xip->xi_dirent_blk[i],
				   SECSIZE(fs), 0, &bp)) != 0)
			return error;
		KASSERT(bp != NULL);
		KASSERT(bp->b_data != NULL);
		memcpy(((struct exfatfs_dirent *)bp->b_data)
		       + xip->xi_dirent_off[i], xip->xi_direntp[i],
		       sizeof(struct exfatfs_dirent));
		if (!inuse)
			bwrite(bp);
		else
			bdwrite(bp);
	}

	/*
	 * Once we've written the deleted file to disk, never
	 * write it back again.  Because we can't change it on disk,
	 * if it does not have an allocation, we can't allocate to it
	 * either. 
	 */
	if (!inuse) {
#if 0
		CLR_DSE_ALLOCPOSSIBLE(xip);
#endif /* 0 */
		xip->xi_refcnt = 0;
	}

	return 0;
}

/*
 * Write an xinode to disk.  Because we need to checksum the
 * entire set, we need to read the other dirents off disk too
 * before we can write the one(s) that changed.
 */
int
exfatfs_update(struct vnode *vp, const struct timespec *acc,
    const struct timespec *mod, int flags)
{
	struct xfinode *xip;
	
	DPRINTF(("exfatfs_update %p (0x%lx) flags 0x%x xiflags 0x%lx\n",
		 vp, (unsigned long)INUM(VTOXI(vp)), flags,
		 VTOXI(vp)->xi_flag));
	
	if (vp->v_mount->mnt_flag & MNT_RDONLY) {
		DPRINTF(("exfatfs_update: ro\n"));
		return (0);
	}
	xip = VTOXI(vp);
	XITIMES(xip, acc, mod, NULL);
	if ((xip->xi_flag & XI_MODIFIED) == 0) {
		DPRINTF(("exfatfs_update: not modified\n"));
		return (0);
	}
	xip->xi_flag &= ~XI_MODIFIED;
	if (xip->xi_refcnt <= 0) {
		DPRINTF(("exfatfs_update: refcount %d\n", (int)xip->xi_refcnt));
		return (0);
	}

	return exfatfs_writeback(xip);
}

/*
 * Free file blocks and delete the file entry.
 * XXX much of this should be in exfatfs_inactive, instead.
 */
int
exfatfs_remove(void *v)
{
	struct vop_remove_v3_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
		nlink_t ctx_vp_new_nlink;
	} */ *ap = v;
	struct xfinode *xip;
	struct xfinode *dxip;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
#ifndef USE_INACTIVE
	struct componentname *cnp = ap->a_cnp;
#endif /* USE_INACTIVE */
	struct exfatfs_dirent_key new_key;
	int error, i;

	KASSERT(ap != NULL);
	KASSERT(ap->a_vp != NULL);
	KASSERT(ap->a_dvp != NULL);
	KASSERT(ap->a_cnp != NULL);
	KASSERT(vp != dvp);
	xip = VTOXI(ap->a_vp);
	dxip = VTOXI(ap->a_dvp);
	KASSERT(xip != NULL);
	KASSERT(dxip != NULL);
	KASSERT(xip != dxip);
	new_key = xip->xi_key;

#ifdef TRACE_INUM
	if (INUM(xip) == TRACE_INUM)
		printf("remove 0x%x...\n", TRACE_INUM);
#endif /* TRACE_INUM */

	if (vp->v_type == VDIR)
		return EISDIR;

#ifndef USE_INACTIVE
	if ((error = detrunc(xip, 0, 0, cnp->cn_cred)) != 0) {
		printf("remove: detrunc returned %d\n", error);
		return error;
	}
	uvm_vnp_setsize(vp, 0);
#endif /* USE_INACTIVE */

	/* Set inactive, write to disk and set refcnt := 0 */
	for (i = 0; xip->xi_direntp[i]; i++)
		xip->xi_direntp[i]->xd_entryType &= ~XD_ENTRYTYPE_INUSE_MASK;
	if ((error = exfatfs_writeback(xip)) != 0)
		return error;

	KASSERT(xip->xi_refcnt <= 0);
	
	/* No longer valid on disk, update its generation number */
	new_key = xip->xi_key;
	new_key.dk_dirgen = xip;
	exfatfs_rekey(vp, &new_key);

#ifdef TRACE_INUM
	if (INUM(xip) == TRACE_INUM)
		vprint("remove trace", vp);
#endif /* TRACE_INUM */
	
	DPRINTF(("exfatfs_remove(), xip %p, v_usecount %d\n",
		 xip, vp->v_usecount));
	VN_KNOTE(vp, NOTE_DELETE);
	VN_KNOTE(dvp, NOTE_WRITE);
	vput(vp);
	/* cache_purge(vp); */

	dxip->xi_flag |= XI_MODIFIED;
	exfatfs_update(dvp, NULL, NULL, 0);
	cache_purge(dvp);

	return (error);
}

int
exfatfs_rekey(struct vnode *vp, struct exfatfs_dirent_key *nkeyp)
{
	struct xfinode *xip = VTOXI(vp);
	struct exfatfs_dirent_key old_key = xip->xi_key;
	struct mount *mp = vp->v_mount;
	int e;
	
	DPRINTF((" begin rekey %x/%x/%p to %x/%x/%p\n",
		 xip->xi_key.dk_dirclust,
		 xip->xi_key.dk_diroffset,
		 xip->xi_key.dk_dirgen,
		 nkeyp->dk_dirclust,
		 nkeyp->dk_diroffset,
		 nkeyp->dk_dirgen));
	if ((e = vcache_rekey_enter(mp, vp, &old_key, sizeof(old_key),
				    nkeyp, sizeof(*nkeyp))) != 0) {
		printf("  vcache_rekey_enter returned %d\n", e);
	}
	KASSERT(e == 0);
	xip->xi_key = *nkeyp;
	vcache_rekey_exit(mp, vp,
			  &old_key, sizeof(old_key),
			  &xip->xi_key, sizeof(xip->xi_key));
	DPRINTF((" end rekey\n"));
	return 0;
}

/*
 * Locate a contiguous unused sequence of "len" entries
 * in directory "dvp".  If one does not exist, allocate
 * one or more new clusters to accommodate and fill the
 * new block numbers and offsets in in xip->dirent_blk
 * and xip->dirent_off.
 */
int
exfatfs_findempty(struct vnode *dvp, struct xfinode *xip)
{
	off_t off = 0, r = -1, newsize, so;
	int i;
	daddr_t blkno;
	struct exfatfs_dirent *dirent;
	struct xfinode *dxip;
	struct exfatfs *fs;
	int error = 0;
	int len, contig = 0;
	daddr_t lbn, rblkno;
	struct buf *bp;

	(void)rblkno;
	KASSERT(dvp != NULL);
	KASSERT(xip != NULL);
	dxip = VTOXI(dvp);
	KASSERT(dxip != NULL);
	fs = dxip->xi_fs;
	KASSERT(fs != NULL);
	len = GET_DFE_SECONDARY_COUNT(xip) + 1;
	KASSERT(len > 2);
	KASSERT(len <= EXFATFS_MAXDIRENT);
	
	DPRINTF(("exfatfs_findempty search dir 0x%lx bytes 0..%llu of %llu\n",
		 (unsigned long)INUM(dxip),
		 (unsigned long long)GET_DSE_VALIDDATALENGTH(dxip),
		 (unsigned long long)GET_DSE_DATALENGTH_BLK(dxip, fs)));

	/*
	 * Search the directory for empty space.  If there is any, we won't
	 * need to allocate blocks or adjust its size.
	 */
	newsize = GET_DSE_VALIDDATALENGTH(dxip);
	for (off = 0; off < GET_DSE_VALIDDATALENGTH(dxip); off += SECSIZE(fs)) {
		lbn = EXFATFS_BYTES2FSSEC(fs, off);
		VOP_BMAP(dvp, lbn, NULL, &blkno, NULL);
		if ((error = bread(fs->xf_devvp, blkno, SECSIZE(fs), 0, &bp))
		    != 0)
			return error;
		for (so = 0; so < SECSIZE(fs); so += sizeof(*dirent)) {
			dirent = (struct exfatfs_dirent *)
				(((char *)bp->b_data) + so);
			if (ISINUSE(dirent)) {
				contig = 0;
				r = -1;
			} else {
				if (contig == 0) {
					r = off + so;
					rblkno = blkno;
				}
				++contig;
				if (contig >= len) {
					brelse(bp, 0);
					DPRINTF(("dir 0x%lx has %d empty"
						 " entries at byte %lld"
						 " (entry %u) bn 0x%lx..0x%lx"
						 "\n",
						 (unsigned long)INUM(dxip),
						 contig,
						 (long long)r,
						 (unsigned)
						  EXFATFS_BYTES2DIRENT(fs, r),
						 rblkno, blkno));
					goto havespace;
				}
			}
		}
		brelse(bp, 0);
	}

	/* r = -1; */ /* XXX never reallocate XXX */
	
	/*
	 * Now, if contig > 0, r contains the offset that will contain
	 * enough space, though we may need to allocate clusters.
	 */
	if (r < 0) /* We never found a single empty spot */
		r = GET_DSE_VALIDDATALENGTH(dxip);
	newsize = r + len * sizeof(*dirent);
	if (GET_DSE_DATALENGTH_BLK(dxip, fs) < newsize) {
		/* We need to allocate blocks before extending */
		if ((error = deextend(dxip, roundup2(newsize, CLUSTERSIZE(fs)),
				      0, NOCRED)) != 0)
			return error;
		DPRINTF((" allocated dir 0x%lx to %llu\n",
			 (unsigned long)INUM(dxip),
			 (unsigned long long)GET_DSE_DATALENGTH_BLK(dxip, fs)));
	}

	/* Space is now allocated.  Adjust validDataLength if necessary. */
	/* exFAT requires ValidDataLength == DataLength for directories */
	SET_DSE_VALIDDATALENGTH(dxip, GET_DSE_DATALENGTH(dxip));
	uvm_vnp_setsize(dvp, GET_DSE_VALIDDATALENGTH(dxip));

havespace:
	dxip->xi_flag |= XI_MODIFIED;
	if ((error = exfatfs_update(dvp, NULL, NULL, 0)) != 0)
		return error;

	DPRINTF(("new empty space at byte %lld (entry %u)\n",
		 (long long)r, (unsigned)(r >> EXFATFS_DIRENT_BASESHIFT)));

	/*
	 * Assign disk addresses
	 */
	for (i = 0; i < len; i++) {
		off = r + i * sizeof(struct exfatfs_dirent);
		lbn = EXFATFS_BYTES2FSSEC(fs, off);
		if (i == 0 || (off & SECMASK(fs)) == 0) {
			VOP_BMAP(dvp, lbn, NULL, &blkno, NULL);
		}
		xip->xi_dirent_blk[i] = blkno;
		xip->xi_dirent_off[i] = (off & SECMASK(fs))
			>> EXFATFS_DIRENT_BASESHIFT;
		DPRINTF((" entry %d at 0x%lx/%d\n", i,
			 xip->xi_dirent_blk[i], xip->xi_dirent_off[i]));
	}
	
	/* Fix key to new location */
	xip->xi_dirclust = EXFATFS_HWADDR2CLUSTER(fs, xip->xi_dirent_blk[0]);
	xip->xi_diroffset = EXFATFS_HWADDR2DIRENT(fs, xip->xi_dirent_blk[0]) +
		xip->xi_dirent_off[0];

	return 0;
}

/*
 * Create an empty directory entry, of any type.
 */
static int
exfatfs_alloc(struct vnode *dvp, struct vnode **vpp,
	      struct componentname *cnp,
	      struct vattr *vap, uint32_t fileAttributes)
{
	int i, error;
	struct xfinode *dxip;
	struct exfatfs *fs;
	uint16_t ucs2filename[EXFATFS_MAX_NAMELEN];
	int ucs2len;
	int contig;
	struct xfinode *xip;
	struct exfatfs_dfn *di_namep;
	struct timespec now;
	uint32_t dosnow;
	uint8_t dosnow10ms;
	/* int async = dvp->v_mount->mnt_flag & MNT_ASYNC; */ /* XXX */

	DPRINTF(("exfatfs_alloc(%p, ., %p, %p, 0x%x\n",
		 dvp, cnp, vap, fileAttributes));
	
	KASSERT(dvp != NULL);
	dxip = VTOXI(dvp);
	KASSERT(dxip != NULL);
	fs = dxip->xi_fs;
	KASSERT(fs != NULL);
	
	/* Create a new inode */
	xip = exfatfs_newxfinode(fs, 0, 0);
	
	/*
	 * Find an empty location in a directory.  If there is none,
	 * extend the directory by enough to hold three entries.
	 * Remember the logical byte offset of the empty space.
	 */
	ucs2len = exfatfs_utf8ucs2str(cnp->cn_nameptr, cnp->cn_namelen,
				      ucs2filename, EXFATFS_MAX_NAMELEN);
	exfatfs_upcase_str(fs, ucs2filename, ucs2len);
	
	contig = 2 + howmany(ucs2len, EXFATFS_NAME_CHUNKSIZE);
	DPRINTF(("alloc: namelen %lu -> ucs2len=%lu, contig=%lu\n",
		 (unsigned long)cnp->cn_namelen,
		 (unsigned long)ucs2len,
		 (unsigned long)contig));
	KASSERT(contig > 2);
	
	/* Create empty directory entries */
	for (i = 0; i < contig; i++) {
		xip->xi_direntp[i] = exfatfs_newdirent();
		memset(xip->xi_direntp[i], 0, sizeof(struct exfatfs_dirent));
	}
	SET_DFE_SECONDARY_COUNT(xip, contig - 1);

	/* Find space and set the addresses in xip */
	if ((error = exfatfs_findempty(dvp, xip)) != 0) {
		printf("exfatfs_findempty returned %d\n", error);
		goto errout;
	}
	xip->xi_dirgen = 0;
	DPRINTF(("new first cluster in dvp is %u\n",
		 GET_DSE_FIRSTCLUSTER(VTOXI(dvp))));
	
	/*
	 * Prepare the File directory entry.
	 */
	getnanotime(&now);
	xip->xi_direntp[0]->xd_entryType = XD_ENTRYTYPE_FILE;
	SET_DFE_SECONDARY_COUNT(xip, contig - 1);
	SET_DFE_FILE_ATTRIBUTES(xip, fileAttributes);
	/* XXX other attributes from a_vap? */
	exfatfs_unix2dostime(&now, fs->xf_gmtoff, &dosnow, &dosnow10ms);
	SET_DFE_CREATE(xip, dosnow);
	SET_DFE_CREATE10MS(xip, dosnow10ms);
	SET_DFE_LAST_MODIFIED(xip, dosnow);
	SET_DFE_LAST_MODIFIED10MS(xip, dosnow10ms);
	SET_DFE_LAST_ACCESSED(xip, dosnow);
	SET_DFE_CREATE_UTCOFF(xip, fs->xf_gmtoff); 
	SET_DFE_CREATE_UTCOFF(xip, fs->xf_gmtoff); 
	SET_DFE_LAST_MODIFIED_UTCOFF(xip, fs->xf_gmtoff); 
	SET_DFE_LAST_ACCESSED_UTCOFF(xip, fs->xf_gmtoff); 

	/*
	 * Prepare a File Stream Extension directory entry.
	 */
	xip->xi_direntp[1]->xd_entryType = XD_ENTRYTYPE_STREAM_EXTENSION;
	SET_DSE_NAMELENGTH(xip, ucs2len);
	SET_DSE_NAMEHASH(xip, exfatfs_namehash(xip->xi_fs, 0, ucs2filename,
					       ucs2len));
	SET_DSE_DATALENGTH(xip, 0);
	SET_DSE_VALIDDATALENGTH(xip, 0);
	SET_DSE_FIRSTCLUSTER(xip, 0); /* No cluster allocation exists */
	SET_DSE_ALLOCPOSSIBLE(xip);

	/*
	 * Prepare one or more File Name directory entries.
	 */
	for (i = 0; i < contig - 2; i++) {
		int size = 2 * MIN(EXFATFS_NAME_CHUNKSIZE,
				   ucs2len - i * EXFATFS_NAME_CHUNKSIZE);
		di_namep = (struct exfatfs_dfn *)
			xip->xi_direntp[2 + i];
		di_namep->xd_entryType = XD_ENTRYTYPE_FILE_NAME;
		di_namep->xd_generalSecondaryFlags = 0;
		memcpy(&di_namep->xd_fileName, ucs2filename
		       + i * EXFATFS_NAME_CHUNKSIZE, size);
		if (size < EXFATFS_NAME_CHUNKSIZE * sizeof(uint16_t))
			memset(((char*)&di_namep->xd_fileName) + size, 0,
			       EXFATFS_NAME_CHUNKSIZE * sizeof(uint16_t)
			       - size);
	}
	
	/* Compute checksum and write */
	if ((error = exfatfs_writeback(xip)) != 0) {
		printf("exfatfs_writeback returned %d\n", error);
		goto errout;
	}
	
	/*
	 * Associate our new xip with a vnode now that its data
	 * is on disk.
	 */
	error = exfatfs_getnewvnode(fs, dvp, xip->xi_dirclust,
				    xip->xi_diroffset,
				    vap->va_type, xip, vpp);
	if (error) {
		printf("exfatfs_getnewvnode returned %d\n", error);
		goto errout;
	}
	GETPARENT(xip, dvp);
	(*vpp)->v_data = xip;
	xip->xi_vnode = (*vpp);
	vref(xip->xi_devvp); /* XXX */
	(*vpp)->v_tag = VT_EXFATFS;
	if ((*vpp)->v_type == VREG)
		genfs_node_init((*vpp), &exfatfs_genfsops);
	uvm_vnp_setsize((*vpp), GET_DSE_VALIDDATALENGTH(xip));

	dxip->xi_flag |= XI_MODIFIED;
	exfatfs_update(dvp, NULL, NULL, 0);
	
#ifdef EXFATFS_VNOPS_DEBUG
	vprint("alloc", *vpp);
	vprint("alloc/dvp", dvp);
#endif

#ifdef TRACE_INUM
	if (INUM(xip) == TRACE_INUM) {
		printf("after getnewvnode xip=%p\n", VTOXI(*vpp));
		vprint("alloc", *vpp);
	}
#endif /* TRACE_INUM */

	cache_enter(dvp, *vpp, cnp->cn_nameptr, cnp->cn_namelen,
		    cnp->cn_flags);

	VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);
	DPRINTF(("exfatfs_alloc: new entry is 0x%lx\n",
		 (unsigned long)INUM(xip)));
	return (0);

errout:
	exfatfs_freexfinode(xip);
	return error;
}

/*
 * mkdir() creates a directory file entry.
 * This is exactly the same as a creat(), except that
 * the created entry is marked XD_FILEATTR_DIRECTORY.
 */
int
exfatfs_mkdir(void *v)
{
	struct vop_mkdir_v3_args /* {
		struct vnode *a_dvp;
		struvt vnode **a_vpp;
		struvt componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;

	return exfatfs_alloc(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap,
			     XD_FILEATTR_DIRECTORY);
}

/*
 * We only need the *first* valid entry.
 */	
static unsigned first_valid_file(void *v, struct xfinode *xip, off_t unused)
{
	int *intp = (int *)v;

	++*intp;

	DPRINTF(("found valid file ino 0x%lx with ET 0x%hhx\n",
		 INUM(xip), xip->xi_direntp[0]->xd_entryType));
	return SCANDIR_STOP;
}

int
exfatfs_rmdir(void *v)
{
	struct vop_rmdir_v2_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
#ifndef USE_INACTIVE
	struct componentname *cnp = ap->a_cnp;
#endif /* USE_INACTIVE */
	struct xfinode *xip, *dxip;
	struct exfatfs_dirent_key new_key;
	int error, cnt, i;

	xip = VTOXI(vp);
	dxip = VTOXI(dvp);
	/*
	 * No rmdir "." please.
	 */
	if (dxip == xip) {
		vrele(vp);
		return (EINVAL);
	}
	/*
	 * Verify the directory is empty (and valid).
	 * (Rmdir ".." won't be valid since
	 *  ".." will contain a reference to
	 *  the current directory and thus be
	 *  non-empty.)
	 */
	cnt = 0;
	error = exfatfs_scandir(vp, 0, NULL, NULL, first_valid_file, &cnt);
	if (error == ENOENT)
		error = 0;

	if (error)
		goto out;
	if (cnt > 0) {
		error = ENOTEMPTY;
		goto out;
	}

#ifndef USE_INACTIVE
	/*
	 * Truncate the directory that is being deleted.
	 */
	if ((error = detrunc(xip, (u_long)0, IO_SYNC, cnp->cn_cred)) != 0)
		goto out;
	uvm_vnp_setsize(vp, 0);
#endif /* USE_INACTIVE */
	
	/*
	 * Delete the entry from the directory.  For dos filesystems this
	 * gets rid of the directory entry on disk, the in memory copy
	 * still exists but the de_refcnt is <= 0.  This prevents it from
	 * being found by deget().  When the vput() on xip is done we give
	 * up access and eventually exfatfs_reclaim() will be called which
	 * will remove it from the xfinode cache.
	 */
	for (i = 0; xip->xi_direntp[i]; i++)
		xip->xi_direntp[i]->xd_entryType &= ~XD_ENTRYTYPE_INUSE_MASK;
	if ((error = exfatfs_writeback(xip)) != 0)
		goto out;

	KASSERT(xip->xi_refcnt <= 0);
	
	/* No longer valid on disk, update its generation number */
	new_key = xip->xi_key;
	new_key.dk_dirgen = xip;
	exfatfs_rekey(vp, &new_key);
	
	/*
	 * This is where we decrement the link count in the parent
	 * directory.  Since dos filesystems don't do this we just purge
	 * the name cache and let go of the parent directory xfinode.
	 */
	dxip->xi_flag |= XI_MODIFIED;
	exfatfs_update(dvp, NULL, NULL, 0);
	
	VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);
	cache_purge(dvp);
out:
	VN_KNOTE(vp, NOTE_DELETE);
	cache_purge(vp);
	vput(vp);
	return (error);
}

/*
 * Assemble filename from xip and compare it to the
 * filename given as an argument.  If it matches,
 * store xip and return true.
 */
struct filename_xip {
	struct xfinode *xip;
	uint16_t name[EXFATFS_NAMEMAX];
	uint16_t namehash;
	uint8_t namelen;
};
	
static unsigned lookup_validfunc(void *v, struct xfinode *xip, off_t unused)
{
	struct filename_xip *fxp = (struct filename_xip *)v;
	struct exfatfs_dfn *dnp;
	uint16_t nbuf[EXFATFS_NAMEMAX];
	int i, nameoff;

	if (GET_DSE_NAMEHASH(xip) != fxp->namehash) {
		DPRINTF(("Found hash 0x%hx expecting 0x%hx\n",
			 GET_DSE_NAMEHASH(xip), fxp->namehash));
		return 0;
	}
	if (GET_DSE_NAMELENGTH(xip) != fxp->namelen) {
		DPRINTF(("Found len %hhd expecting %hhd\n",
			 GET_DSE_NAMELENGTH(xip), fxp->namelen));
		return 0;
	}

	/* Assemble filename for direct comparison */
	nameoff = 0;
	for (i = 2; i < EXFATFS_MAXDIRENT && xip->xi_direntp[i]
		     && nameoff < fxp->namelen; i++) {
		if (xip->xi_direntp[i]->xd_entryType != XD_ENTRYTYPE_FILE_NAME)
			continue;
		dnp = (struct exfatfs_dfn *)xip->xi_direntp[i];
		memcpy(nbuf + nameoff, dnp->xd_fileName,
		       EXFATFS_NAME_CHUNKSIZE * sizeof(uint16_t));
		nameoff += EXFATFS_NAME_CHUNKSIZE;
	}
	
	if (exfatfs_upcase_cmp(xip->xi_fs, nbuf, fxp->namelen,
			       fxp->name, fxp->namelen)) {
		DPRINTF(("Heuristics passed but no match\n"));
		return 0;
	}
	
	/* Take ownership of xip */
	DPRINTF(("Match with inum 0x%lx\n", (unsigned long)INUM(xip)));
	fxp->xip = xip;
	return (SCANDIR_STOP | SCANDIR_DONTFREE);
}

/*
 *
 *% lookup     dvp     L L L
 *% lookup     vpp     - U -
 *
 *    Note especially that *vpp may equal dvp.
 *
 *    More details:
 *     All lookups find the named node (creating the vnode if needed) and
 *          return it, referenced and unlocked, in *vpp.
 *     On failure, *vpp is NULL, and *dvp is left locked.
 */
int
exfatfs_lookup(void *v)
{
	struct vop_lookup_v2_args /* {
				     struct vnode *a_dvp;
				     struct vnode **a_vpp;
				     struct componentname *a_cnp;
        } */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct vnode *nvp;
	struct exfatfs *fs;
	struct filename_xip fx;
	struct xfinode *dxip;
	int iswhiteout;
	int error = 0;

	KASSERT(dvp != NULL);
	dxip = VTOXI(dvp);
	KASSERT(dxip != NULL);
	fs = dxip->xi_fs;
	KASSERT(vpp != NULL);
	KASSERT(cnp != NULL);

	*vpp = NULL;
	
	DPRINTF(("exfatfs_lookup(%p, %p, {%x, %x, %d:\"%*s\"=%hhx,%hhx,%hhx})"
		 "\n",
		 dvp, vpp,
		 (unsigned)cnp->cn_nameiop,
		 (unsigned)cnp->cn_flags,
		 (int)cnp->cn_namelen,
		 MIN((int)cnp->cn_namelen, 8),
		 cnp->cn_nameptr,
		 (unsigned char)cnp->cn_nameptr[0],
		 (unsigned char)cnp->cn_nameptr[1],
		 (unsigned char)cnp->cn_nameptr[2]));

	/*
	 * Check accessiblity of directory.
	 */
	if ((error = VOP_ACCESS(dvp, VEXEC, cnp->cn_cred)) != 0) {
		DPRINTF((" no access\n"));
		return (error);
	}

	if ((cnp->cn_flags & ISLASTCN) &&
	    (dvp->v_mount->mnt_flag & MNT_RDONLY) &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		DPRINTF((" read-only\n"));
		return (EROFS);
	}

	/*
	 * We now have a segment name to search for, and a directory to search.
	 *
	 * Before tediously performing a linear scan of the directory,
	 * check the name cache to see if the directory/name pair
	 * we are looking for is known already.
	 */
	if (cache_lookup(dvp, cnp->cn_nameptr, cnp->cn_namelen,
			 cnp->cn_nameiop, cnp->cn_flags, &iswhiteout,
			 vpp)) {
		if (iswhiteout) {
			cnp->cn_flags |= ISWHITEOUT;
		}
#ifdef TRACE_INUM
		if (INUM(dxip) == TRACE_INUM) {
			if (*vpp == NULLVP) {
				printf("cache_lookup returned ENOENT for name"
				       " %.*s\n",
				       (int)cnp->cn_namelen, cnp->cn_nameptr);
			} else {
				KASSERT(VTOXI(*vpp) != NULL);
				printf("cache_lookup returned ino 0x%lx for"
				       " name %.*s\n",
				       INUM(VTOXI(*vpp)), (int)cnp->cn_namelen,
				       cnp->cn_nameptr);
			}
		}
#endif /* TRACE_INUM */
		DPRINTF((" returning cached result\n"));
		return *vpp == NULLVP ? ENOENT : 0;
	}

	/*
	 * Special case: the names "." and ".." refer to
	 * specific, well-known vnodes.
	 */
	if (cnp->cn_nameptr[0] == '.' && cnp->cn_namelen == 1) {
		*vpp = dvp;
		vref(dvp);
		DPRINTF(("exfatfs_lookup returning this\n"));
		/* goto out; */
		return 0;
	}
	if (cnp->cn_nameptr[0] == '.'
	    && cnp->cn_nameptr[1] == '.'
	    && cnp->cn_namelen == 2) {
		if (dvp->v_vflag & VV_ROOT) {
			*vpp = dvp;
			vref(dvp);
			DPRINTF(("exfatfs_lookup returning this (as ..)\n"));
			return 0;
		}

		/* .., but not root's: we want parentvp. */
		KASSERT(dxip->xi_parentvp != NULL);
		*vpp = dxip->xi_parentvp;
		vref(*vpp);
		
		DPRINTF(("exfatfs_lookup returning parent\n"));
		/* goto out; */
		return 0;
	}

	/*
	 * Scan the directory for a matching name.
	 * If we find one, the file entry will be returned
	 * in fx.xip.
	 */
	fx.namelen = exfatfs_utf8ucs2str(cnp->cn_nameptr,
					 cnp->cn_namelen,
					 fx.name,
					 EXFATFS_NAMEMAX);
	fx.namehash = exfatfs_namehash(fs, 0, fx.name, fx.namelen);
	fx.xip = NULL;
	error = exfatfs_scandir(dvp, 0, NULL, NULL, lookup_validfunc, &fx);
	if (error)
		return error;

	if (fx.xip != NULL) {
#ifdef TRACE_INUM
		if (INUM(dxip) == TRACE_INUM || INUM(fx.xip) == TRACE_INUM)
			printf("lookup dino 0x%lx found ino 0x%lx with xip=%p"
			       " gen %p\n", INUM(dxip), INUM(fx.xip), fx.xip,
			       fx.xip->xi_dirgen);
#endif /* TRACE_INUM */
		nvp = NULL;
		/* Store xip where loadvnode can find it */
		mutex_enter(&fs->xf_lock);
		LIST_INSERT_HEAD(&fs->xf_newxip, fx.xip, xi_newxip);
		mutex_exit(&fs->xf_lock);
		error = vcache_get(dvp->v_mount,
				   &fx.xip->xi_key,
				   sizeof(fx.xip->xi_key), &nvp);
		if (error) {
			exfatfs_freexfinode(fx.xip);
			return error;
		}
		KASSERT(VTOXI(nvp) != NULL);
		if (VTOXI(nvp) != fx.xip) {
			/* In vnode cache, though not in name cache */
			LIST_REMOVE(fx.xip, xi_newxip);
			exfatfs_freexfinode(fx.xip);
		}
		GETPARENT(VTOXI(nvp), dvp);
		*vpp = nvp;
		goto out;
	}

	/*
	 * The requested name was not found.
	 */
	if ((cnp->cn_nameiop == CREATE || cnp->cn_nameiop == RENAME) &&
	    (cnp->cn_flags & ISLASTCN) && dxip->xi_refcnt != 0) {
		/*
		 * Access for write is interpreted as allowing
		 * creation of files in the directory.
		 */
		error = VOP_ACCESS(dvp, VWRITE, cnp->cn_cred);
		if (error)
			return (error);

		return EJUSTRETURN; /* Taken from ufs_lookup.c */
	}
	error = ENOENT;
	
out:
	/*
	 * Insert name into cache (as non-existent, if appropriate).
	 */
	if (cnp->cn_nameiop != CREATE) {
		cache_enter(dvp, *vpp, cnp->cn_nameptr, cnp->cn_namelen,
			    cnp->cn_flags);
	}

	return error;
}

#define BIAS (2 * sizeof(struct exfatfs_dirent))

/*
 * Assemble filename from xip and push it into uio.
 * Also keep track of the offset read from the directory.
 */
struct filename_offset {
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap;
	int nc;
	int ncookies;
};

	
static unsigned readdir_validfunc(void *v, struct xfinode *xip, off_t off)
{
	struct filename_offset *fo = (struct filename_offset *)v;
	uint16_t ucs2[EXFATFS_NAMEMAX];
	int error, namelen;
	struct dirent dirbuf;

	/* Assemble filename and convert to UTF8 */
	exfatfs_get_file_name(xip, ucs2, &namelen, sizeof(ucs2));
	namelen = exfatfs_ucs2utf8str(ucs2, namelen,
				      (uint8_t *)&dirbuf.d_name,
				      sizeof(dirbuf.d_name) - 1);

	/* Assemble a new output record */
	dirbuf.d_fileno = INUM(xip);
	dirbuf.d_type = ISDIRECTORY(xip) ? DT_DIR :
		(ISSYMLINK(xip) ? DT_LNK : DT_REG);
	dirbuf.d_namlen = namelen;
	dirbuf.d_name[dirbuf.d_namlen] = '\0';
	dirbuf.d_reclen = _DIRENT_SIZE(&dirbuf);
	if (fo->ap->a_uio->uio_resid < dirbuf.d_reclen) {
		/* If there was not room, tell scandir to stop */
		return SCANDIR_STOP;
	}
	
	/* There is space.  Copy the record into uio. */
	error = uiomove(&dirbuf, dirbuf.d_reclen, fo->ap->a_uio);
	if (error) {
		DPRINTF(("uiomove returned %d\n", error));
		return SCANDIR_STOP;
	}
	if (fo->ap->a_cookies) {
		*(off_t *)fo->ap->a_cookies++ = off;
		fo->ncookies++;
		if (fo->ncookies >= fo->nc)
			return SCANDIR_STOP;
	}

	/*
	 * uiomove() advanced uio_offset by the number of bytes
	 * copied.  Store the actual directory offset there instead.
	 */
	fo->ap->a_uio->uio_offset = off + BIAS;
	return 0;
}

/*
 * Read one or more directory entries fom a directory,
 * place read entries in a_uio.  This is complicated
 * by the fact that raw directory offset and uio offset
 * are not closely related to one another.
 */
int
exfatfs_readdir(void *v)
{
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct xfinode *xip = VTOXI(vp);
	struct exfatfs *fs = xip->xi_fs;
	struct filename_offset fo;
	struct uio *uio = ap->a_uio;
	int error = 0;
	int ncookies = 0, nc = 0;
	long n;
	off_t offset, *cookies = NULL;
	struct dirent dirbuf;

	exfatfs_check_fence(fs);
	DPRINTF(("readdir: vp %p, uio %p, cred %p, eofflagp %p\n",
		 ap->a_vp, uio, ap->a_cred, ap->a_eofflag));
	DPRINTF(("         fs = %p, ino=%u/%u, secsize %u\n",
		 fs, xip->xi_dirclust, xip->xi_diroffset, SECSIZE(fs)));
	DPRINTF(("         offset=%lu, resid=%lu\n",
	       (unsigned long)uio->uio_offset,
		 (unsigned long)uio->uio_resid));
#ifdef EXFATFS_VNOPS_DEBUG
	vprint("readdir", ap->a_vp);
#endif
	
	/* Only readdir() on directories please */
	if (ISDIRECTORY(xip) == 0) {
		return (ENOTDIR);
	}

	/*
	 * If the user buffer is smaller than the size of one directory
	 * entry or the file offset is not a multiple of the size of a
	 * directory entry, then we fail the read.
	 */
	offset = uio->uio_offset;
	if (uio->uio_resid < _DIRENT_MINSIZE(&dirbuf)
	    || (offset & (sizeof(struct exfatfs_dirent) - 1))) {
		exfatfs_check_fence(fs);
		return (EINVAL);
	}

	if (ap->a_ncookies) {
		nc = uio->uio_resid / _DIRENT_MINSIZE((struct dirent *)0);
		*ap->a_cookies = malloc(nc * sizeof (off_t), M_TEMP, M_WAITOK);
	}

	/*
	 * Exfatfs does not encode . or .. entries in the directory data.
	 * Construct these and place them at the beginning of the output.
	 */
	if (offset < BIAS) {
		for (n = (int)offset / sizeof(struct exfatfs_dirent);
		     n < 2; n++) {
			switch (n) {
			case 0:
				dirbuf.d_fileno = INUM(xip);
				dirbuf.d_namlen = 1;
				strlcpy(dirbuf.d_name, ".",
					sizeof(dirbuf.d_name));
				break;
			case 1:
				if (ap->a_vp->v_vflag & VV_ROOT)
					dirbuf.d_fileno = INUM(xip);
#if 0
				else
					dirbuf.d_fileno = xip->xi_parent;
#endif
				dirbuf.d_namlen = 2;
				strlcpy(dirbuf.d_name, "..",
					sizeof(dirbuf.d_name));
				break;
			}
			dirbuf.d_type = DT_DIR;
			dirbuf.d_reclen = _DIRENT_SIZE(&dirbuf);
			DPRINTF(("dirbuf fileno=0x%lx type=%d namlen=%d"
				 " name=%.*s\n",
			         dirbuf.d_fileno, dirbuf.d_type,
			         dirbuf.d_namlen,
				 dirbuf.d_namlen, dirbuf.d_name));
			if (uio->uio_resid < dirbuf.d_reclen)
				goto out;
			error = uiomove(&dirbuf, dirbuf.d_reclen, uio);
			if (error)
				goto out;
			offset += sizeof(struct exfatfs_dirent);
			if (cookies) {
				*cookies++ = offset;
				ncookies++;
				if (ncookies >= nc)
					goto out;
			}
		}
	}

	KASSERT(GET_DSE_DATALENGTH(xip) == GET_DSE_DATALENGTH_BLK(xip, fs));
	KASSERT(GET_DSE_VALIDDATALENGTH(xip) == GET_DSE_DATALENGTH(xip));

	/* Scan the directory to fill in uio. */
	fo.ap = ap;
	fo.nc = nc;
	fo.ncookies = ncookies;
	error = exfatfs_scandir(vp, offset - BIAS,
				&offset, NULL, readdir_validfunc, &fo);
	offset += BIAS;
out:
	if (offset >= GET_DSE_VALIDDATALENGTH(xip)) {
		uio->uio_offset = offset; /* XXX should be already */
		*ap->a_eofflag = 1;
	} else
		*ap->a_eofflag = 0;

	if (ap->a_ncookies) {
		if (error) {
			free(*ap->a_cookies, M_TEMP);
			*ap->a_ncookies = 0;
			*ap->a_cookies = NULL;
		} else
			*ap->a_ncookies = fo.ncookies;
	}

	DPRINTF(("readdir: returning %d with resid=%u, offset-BIAS=%llu,"
		 " eof=%d\n",
	         error, (unsigned)uio->uio_resid,
	         (unsigned long long)(offset - BIAS),
		 *ap->a_eofflag));
	exfatfs_check_fence(fs);
	return (error);
}

int
exfatfs_bmap(void *v)
{
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;

	return exfatfs_bmap_shared(ap->a_vp, ap->a_bn, ap->a_vpp,
				   ap->a_bnp, ap->a_runp);
}

/*
 * Read or write a block to/from disk.
 */
int
exfatfs_strategy(void *v)
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct buf	*bp;
	struct vnode	*vp;
	struct xfinode	*xip;
	struct exfatfs  *fs;
	int		error;

	bp = ap->a_bp;
	vp = ap->a_vp;
	xip = VTOXI(vp);
	fs = xip->xi_fs;
	exfatfs_check_fence(fs);
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		panic("exfatfs_strategy: spec");
	KASSERT(bp->b_bcount != 0);
	if (bp->b_blkno == bp->b_lblkno) {
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno,
				 NULL);
		if (error) {
			bp->b_error = error;
			biodone(bp);
			exfatfs_check_fence(fs);
			return (error);
		}
		if (bp->b_blkno == -1) /* no valid data */
			clrbuf(bp);
	}
	if (bp->b_blkno < 0) { /* block is not on disk */
		biodone(bp);
		exfatfs_check_fence(fs);
		return (0);
	}
	vp = xip->xi_devvp;

	error = VOP_STRATEGY(vp, bp);
	if (error) {
		exfatfs_check_fence(fs);
		return error;
	}

	if (!BUF_ISREAD(bp))
		return 0;

	error = biowait(bp);
	exfatfs_check_fence(fs);
	return error;
}

int
exfatfs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode *vp;
	} */ *ap = v;
	struct xfinode *xip = VTOXI(ap->a_vp);

	if (xip == NULL) {
		DPRINTF(("xip==NULL!\n"));
		return 0;
	}
	
	printf(
		"xip %p, fs %p\n"
		"startcluster %lu\n"
		" ino 0x%lx gen %p\n",
		xip, xip->xi_fs,
		(unsigned long)GET_DSE_FIRSTCLUSTER(xip),
		(unsigned long)INUM(xip), xip->xi_dirgen);

	if (xip->xi_devvp == NULL) {
		printf(" xip->xi_devvp==NULL!\n");
		return 0;
	}
	
	printf(" dev %llu, %llu\n",
	    (unsigned long long)major(xip->xi_devvp->v_rdev),
		 (unsigned long long)minor(xip->xi_devvp->v_rdev));
	
	return (0);
}

int
exfatfs_advlock(void *v)
{
	struct vop_advlock_args /* {
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	struct xfinode *xip = VTOXI(ap->a_vp);

	return lf_advlock(ap, &xip->xi_lockf, GET_DSE_VALIDDATALENGTH(xip));
}

int
exfatfs_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = 1;
		return (0);
	case _PC_NAME_MAX:
		*ap->a_retval = ap->a_vp->v_mount->mnt_stat.f_namemax;
		return (0);
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		return (0);
	case _PC_SYNC_IO:
		*ap->a_retval = 1;
		return (0);
	case _PC_FILESIZEBITS:
		*ap->a_retval = 64;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

int
exfatfs_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t offlo;
		off_t offhi;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct xfinode *xip = VTOXI(vp);
	struct vnode *devvp = xip->xi_devvp;
	int l, error;

	if ((error = vflushbuf(vp, ap->a_flags)) != 0)
		return error;
	
	if (!(ap->a_flags & FSYNC_DATAONLY))
		if ((error = exfatfs_update(vp, NULL, NULL,
					    (ap->a_flags & FSYNC_WAIT)
					    ? UPDATE_WAIT : 0)) != 0)
			return error;

	if (ap->a_flags & FSYNC_CACHE) {
		if ((error = VOP_IOCTL(devvp, DIOCCACHESYNC, &l, FWRITE,
				       curlwp->l_cred)) != 0)
			return error;
	}

	return 0;
}

void
exfatfs_itimes(struct xfinode *xip, const struct timespec *acc,
	       const struct timespec *mod, const struct timespec *cre)
{
	struct timespec now;
	int gmtoff = xip->xi_fs->xf_gmtoff;

	if (!(xip->xi_flag & (XI_UPDATE | XI_CREATE | XI_ACCESS)))
		return;

	exfatfs_check_fence(xip->xi_fs);
	getnanotime(&now);
	
	xip->xi_flag |= XI_MODIFIED;
	if (xip->xi_flag & XI_UPDATE) {
		if (mod == NULL)
			mod = &now;
		exfatfs_unix2dostime(mod, gmtoff,
				     &DFE(xip)->xd_lastModifiedTimestamp,
				     &DFE(xip)->xd_lastModified10msIncrement);
		SET_DFE_FILE_ATTRIBUTES(xip,
					GET_DFE_FILE_ATTRIBUTES(xip)
					| XD_FILEATTR_ARCHIVE);
	}
	if (xip->xi_flag & XI_ACCESS)  {
		if (acc == NULL)
			acc = &now;
		exfatfs_unix2dostime(acc, gmtoff,
				     &DFE(xip)->xd_lastAccessedTimestamp, NULL);
	}
	if (xip->xi_flag & XI_CREATE) {
		if (cre == NULL)
			cre = &now;
		exfatfs_unix2dostime(cre, gmtoff, &DFE(xip)->xd_createTimestamp,
				     &DFE(xip)->xd_create10msIncrement);
	}
	
	xip->xi_flag &= ~(XI_UPDATE | XI_CREATE | XI_ACCESS);
	exfatfs_check_fence(xip->xi_fs);
}


int
exfatfs_inactive(void *v)
{
        struct vop_inactive_v2_args /* {
                struct vnode *a_vp;
                bool *a_recycle;
        } */ *ap = v;
        struct vnode *vp = ap->a_vp;
        struct xfinode *xip = VTOXI(vp);
#ifdef USE_INACTIVE
        int error = 0;
#endif /* USE_INACTIVE */
	
        /*
         * If the file has been deleted and it is on a read/write
         * filesystem, then truncate the file.  We already disassociated
	 * it from the directory earlier.
         */
	if (xip->xi_refcnt > 0) {
		DPRINTF(("exfatfs_inactive: ino 0x%lx no recycle\n",
			 (unsigned long)INUM(xip)));
		*ap->a_recycle = 0;
	} else {
		DPRINTF(("exfatfs_inactive: ino 0x%lx yes recycle\n",
			 (unsigned long)INUM(xip)));
		*ap->a_recycle = 1;
#ifdef USE_INACTIVE
		if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0 &&
		    GET_DSE_VALIDDATALENGTH(xip) != 0) {
			if ((error = detrunc(xip, 0, 1, NOCRED)) != 0)
				return error;
		}
		vtruncbuf(vp, 0, 0, 0);
		if (vp->v_type == VREG && !(vp->v_vflag & VV_SYSTEM)) {
			genfs_node_wrlock(vp);
			uvm_vnp_setsize(vp, 0);
			genfs_node_unlock(vp);
		} else {
			uvm_vnp_setsize(vp, 0);
		}
#endif /* USE_INACTIVE */
	}

        return 0;
}

/*
 * Purge old data structures associated with the denode.
 */
int
exfatfs_reclaim(void *v)
{
        struct vop_reclaim_v2_args /* {
                struct vnode *a_vp;
        } */ *ap = v;
        struct vnode *vp = ap->a_vp;
        struct xfinode *xip = VTOXI(vp);
	int error;

	DPRINTF(("exfatfs_reclaim: ino 0x%lx\n", (unsigned long)INUM(xip)));
	
	if ((error = exfatfs_update(vp, NULL, NULL, UPDATE_WAIT)) != 0) {
		printf("exfatfs_reclaim: update returned %d\n", error);
		return error;
	}
	VOP_UNLOCK(vp);

	/* Remove reference to parent, if any */
	PUTPARENT(xip);
	
	/* Remove reference to device */
        if (xip->xi_devvp) {
                vrele(xip->xi_devvp);
                xip->xi_devvp = NULL;
        }

	if (vp->v_type == VREG && !(vp->v_vflag & VV_SYSTEM))
		genfs_node_destroy(vp);
        mutex_enter(vp->v_interlock);
        vp->v_data = NULL;
        mutex_exit(vp->v_interlock);
	exfatfs_freexfinode(xip);

        return (0);
}

/*
 * XXX this function exists solely for debugging
 */
#ifdef EXFATFS_LOCKDEBUG
static int
exfatfs_lock(void *v);

static int
exfatfs_lock(void *v) {
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	int flags = ap->a_flags;
	int error;

	DPRINTF(("exfatfs_lock(%p, 0x%x)\n", vp, flags));
	DPRINTF(("    vnode is %p/%u/%u, refcount %d\n", vp,
	       VTOXI(vp)->xi_dirclust,
	       VTOXI(vp)->xi_diroffset,
		 (int)vrefcnt(vp)));
	error = genfs_lock(v);
	DPRINTF(("    returning %d\n", error));
	return error;
}
#endif

/*
 * symlink -- make a symbolic link
 */
int
exfatfs_symlink(void *v)
{
	struct vop_symlink_v3_args /* {
		struct vnode		*a_dvp;
		struct vnode		**a_vpp;
		struct componentname	*a_cnp;
		struct vattr		*a_vap;
		char			*a_target;
	} */ *ap = v;
	struct vnode	*vp, **vpp;
	int		len, error, resid;
	struct xfinode	*xip;
	struct exfatfs	*fs;
	struct buf	*bp;
	off_t off;

	vpp = ap->a_vpp;
	len = strlen(ap->a_target);

	DPRINTF(("exfatfs_symlink with len=%d\n", len));
	
	/* Create the xfinode */
	KASSERT(ap->a_vap->va_type == VLNK);
	error = exfatfs_alloc(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap,
			      XD_FILEATTR_SYMLINK);
	if (error)
		goto out;
	vn_lock(*vpp, LK_EXCLUSIVE);
	vp = *vpp;
	xip = VTOXI(vp);
	fs = xip->xi_fs;

	/* XXX Convert link target to UCS2? */
	
	/* Write the target into the buffer as if it were file data */
	error = deextend(xip, roundup2(len, CLUSTERSIZE(fs)), 0, NOCRED);
	if (error) {
		exfatfs_deactivate(xip, true);
		goto out;
	}

	resid = len;
	for (off = 0; off < len; off += SECSIZE(fs)) {
#ifdef SYMLINK_SELF
		bp = getblk(vp, EXFATFS_BYTES2FSSEC(fs, off),
			    SECSIZE(fs), 0, 0);
#else /* 1 */
		daddr_t blkno;
		VOP_BMAP(vp, EXFATFS_BYTES2FSSEC(fs, off), NULL, &blkno, NULL);
		bp = getblk(xip->xi_devvp, blkno, SECSIZE(fs), 0, 0);
#endif /* 1 */
		memcpy(bp->b_data, ap->a_target + off,
		       MIN(resid, SECSIZE(fs)));
		bdwrite(bp);
	}
	SET_DSE_DATALENGTH(xip, len);
	SET_DSE_VALIDDATALENGTH(xip, len);
	uvm_vnp_setsize(vp, len);
	xip->xi_flag |= XI_MODIFIED;
	exfatfs_update(vp, NULL, NULL, 0);

	VOP_UNLOCK(vp);
	if (error)
		vrele(vp);
out:
	return (error);
}

/*
 * Return target name of a symbolic link
 */
int
exfatfs_readlink(void *v)
{
	struct vop_readlink_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct uio	*uio = ap->a_uio;
	struct vnode	*vp = ap->a_vp;
	struct xfinode	*xip;
	struct exfatfs	*fs;
	struct buf	*bp;
	daddr_t		lbn;
	off_t		off, n, filelen;
	int		error;

	KASSERT(uio != NULL);
	KASSERT(vp != NULL);
	xip = VTOXI(vp);
	KASSERT(xip != NULL);
	fs = xip->xi_fs;
	
	/* XXX Convert from UCS2? */

	filelen = GET_DSE_VALIDDATALENGTH(xip);
	DPRINTF(("exfatfs_readlink with offset %ld, filelen=%lu\n",
		 uio->uio_offset, filelen));
	do {
		if (uio->uio_offset >= filelen)
			return 0;

		lbn = EXFATFS_BYTES2FSSEC(fs, uio->uio_offset);
#ifdef SYMLINK_SELF
		error = bread(vp, lbn, SECSIZE(fs), 0, &bp);
#else /* !SYMLINK_SELF */
		{
			daddr_t blkno;
			VOP_BMAP(vp, lbn, NULL, &blkno, NULL);
			error = bread(xip->xi_devvp, blkno, SECSIZE(fs), 0,
				      &bp);
		}
#endif /* !SYMLINK_SELF */
		if (error)
			return error;

		DPRINTF(("uio_offset=%ld, uio_resid=%ld, b_bufsize=%ld\n",
			 (long)uio->uio_offset,
			 (long)uio->uio_resid,
			 (long)bp->b_bufsize));
		
		off = uio->uio_offset & SECMASK(fs);
		n = MIN(uio->uio_resid, filelen - uio->uio_offset);
		n = MIN(n, bp->b_bufsize - off);
		if (n > 0) {
			error = uiomove((char *)bp->b_data + off, n, uio);
			DPRINTF(("link target off=%lld: %*s\n",
				 (long long)off,
				 (int)n, (char *)bp->b_data + off));
		}
		brelse(bp, 0);
	} while (error == 0 && uio->uio_resid > 0 && n > 0);

	return error;
}

/* Global vfs data structures for exfatfs */
int (**exfatfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc exfatfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_parsepath_desc, genfs_parsepath },	/* parsepath */
	{ &vop_lookup_desc, exfatfs_lookup },		/* lookup */
	{ &vop_create_desc, exfatfs_create },		/* create */
	{ &vop_mknod_desc, genfs_eopnotsupp },		/* mknod */
	{ &vop_open_desc, genfs_nullop },		/* open */
	{ &vop_close_desc, exfatfs_close },		/* close */
	{ &vop_access_desc, exfatfs_access },		/* access */
	{ &vop_getattr_desc, exfatfs_getattr },		/* getattr */
	{ &vop_setattr_desc, exfatfs_setattr },		/* setattr */
	{ &vop_read_desc, exfatfs_read },		/* read */
	{ &vop_write_desc, exfatfs_write },		/* write */
	{ &vop_fallocate_desc, genfs_eopnotsupp },	/* fallocate */
	{ &vop_fdiscard_desc, genfs_eopnotsupp },	/* fdiscard */
	{ &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, genfs_enoioctl },		/* ioctl */
	{ &vop_poll_desc, genfs_poll },			/* poll */
	{ &vop_kqfilter_desc, genfs_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, genfs_revoke },		/* revoke */
	{ &vop_mmap_desc, genfs_mmap },			/* mmap */
	{ &vop_fsync_desc, exfatfs_fsync },		/* fsync */
	{ &vop_seek_desc, genfs_seek },			/* seek */
	{ &vop_remove_desc, exfatfs_remove },		/* remove */
	{ &vop_link_desc, genfs_eopnotsupp },		/* link */
	{ &vop_rename_desc, exfatfs_rename },		/* rename */
	{ &vop_mkdir_desc, exfatfs_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, exfatfs_rmdir },		/* rmdir */
	{ &vop_symlink_desc, exfatfs_symlink },		/* symlink */
	{ &vop_readdir_desc, exfatfs_readdir },		/* readdir */
	{ &vop_readlink_desc, exfatfs_readlink },	/* readlink */
	{ &vop_abortop_desc, genfs_abortop },		/* abortop */
	{ &vop_inactive_desc, exfatfs_inactive },	/* inactive */
	{ &vop_reclaim_desc, exfatfs_reclaim },		/* reclaim */
#ifdef EXFATFS_LOCKDEBUG
	{ &vop_lock_desc, exfatfs_lock },		/* lock */
#else
	{ &vop_lock_desc, genfs_lock },			/* lock */
#endif
	{ &vop_unlock_desc, genfs_unlock },		/* unlock */
	{ &vop_bmap_desc, exfatfs_bmap },		/* bmap */
	{ &vop_strategy_desc, exfatfs_strategy },	/* strategy */
	{ &vop_print_desc, exfatfs_print },		/* print */
	{ &vop_islocked_desc, genfs_islocked },		/* islocked */
	{ &vop_pathconf_desc, exfatfs_pathconf },	/* pathconf */
	{ &vop_advlock_desc, exfatfs_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, genfs_getpages },		/* getpages */
	{ &vop_putpages_desc, genfs_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc exfatfs_vnodeop_opv_desc =
	{ &exfatfs_vnodeop_p, exfatfs_vnodeop_entries };

/*
 * Truncate.  Remove all clusters not required to handle offset "bytes".
 * This essentially means walking the FAT chain and freeing the end.
 */
static int
detrunc(struct xfinode *xip, off_t bytes, int ioflags, kauth_cred_t cred) {
	struct exfatfs *fs = xip->xi_fs;
	uint32_t newcount = EXFATFS_BYTES2CLUSTER(fs, bytes) +
		((bytes & CLUSTERMASK(fs)) ? 1 : 0);
	uint32_t oldcount = EXFATFS_BYTES2CLUSTER(fs,
				GET_DSE_DATALENGTH_BLK(xip, fs));
	uint32_t pcn, opcn, lcn;
#if 0
	daddr_t lbn;
#endif
	struct buf *bp;

	assert(/* oldcount >= 0 && */ oldcount <= fs->xf_ClusterCount);

	/* If already at target length, nothing to do */
	if (oldcount <= newcount)
		return 0;

	/*
	 * Walk the FAT chain until we reach the address of interest
	 * By default we start from zero
	 */
	lcn = 0;
	pcn = GET_DSE_FIRSTCLUSTER(xip);

	/* If we are contiguous and have an allocation, jump to the end */
	if (newcount > 0 && pcn > 0 && IS_DSE_NOFATCHAIN(xip)) {
		lcn = newcount;
		pcn += lcn;
	}
#ifdef USE_FATCACHE
	/* But if we have a value cached, use that instead */
	else if (xip->xi_fatcache_lc > 0 && xip->xi_fatcache_lc <= newcount) {
		lcn = xip->xi_fatcache_lc;
		pcn = xip->xi_fatcache_pc;
	}
#endif /* USE_FATCACHE */

	DPRINTF(("inum 0x%lx trunc bytes=%lx lcn=0x%x pcn=0x%x oldcount=0x%x"
		 " newcount=0x%x\n", (unsigned long)INUM(xip),
		 (unsigned long)bytes, (unsigned)lcn, (unsigned)pcn,
		 (unsigned)oldcount, (unsigned)newcount));
	
	while (lcn < oldcount) {
		assert(pcn != 0xFFFFFFFF);
		assert(pcn >= 2 && pcn < fs->xf_ClusterCount + 2);

		/* Read the FAT to find the next cluster */
		bread(fs->xf_devvp, EXFATFS_FATBLK(fs, pcn), SECSIZE(fs), 0,
		      &bp);
		opcn = pcn;
		pcn = ((uint32_t *)bp->b_data)[EXFATFS_FATOFF(pcn)];
		if (lcn >= newcount) {
			/* Deallocate cluster */
			DPRINTF(("dealloc cluster 0x%x from ino 0x%x coff"
				 " %u/%u\n",
				 (unsigned)opcn, (unsigned)INUM(xip),
				 (unsigned)lcn,
				 (unsigned)EXFATFS_BYTES2CLUSTER(fs,
					GET_DSE_DATALENGTH_BLK(xip, fs))));
			exfatfs_bitmap_dealloc(fs, opcn);
			((uint32_t *)bp->b_data)[EXFATFS_FATOFF(opcn)]
				= 0xFFFFFFFF;
			if (ioflags)
				bwrite(bp);
			else
				bdwrite(bp);
			
			/* Invalidate the pages, if any, in the buffer cache */
#if 0 /* use vtruncbuf instead */
			if (ISDIRECTORY(xip) || ISSYMLINK(xip)) {
				for (lbn = 0;
				     lbn < EXFATFS_CLUSTER2FSSEC(fs, 1); ++lbn) {
					binvalbuf(xip->xi_devvp,
						  EXFATFS_CLUSTER2HWADDR(fs,
							pcn)
						  + EXFATFS_FSSEC2DEVBSIZE(fs,
							lbn));
				}
			}
#endif /* 0 */
		
		} else
			brelse(bp, 0);
		++lcn;
	}
	assert(lcn == oldcount);
	SET_DSE_DATALENGTH(xip, bytes);
	SET_DSE_VALIDDATALENGTH(xip, bytes);
	vtruncbuf(XITOV(xip), newcount, 0, 0);

	DPRINTF(("inum 0x%x terminating with lcn=0x%x pcn=0x%x dl=%u\n",
		 (unsigned)INUM(xip),
		 (unsigned)lcn,
		 (unsigned)pcn,
		 (unsigned)EXFATFS_BYTES2CLUSTER(fs,
			GET_DSE_DATALENGTH_BLK(xip, fs))));
	
	if (newcount == 0) {
		CLR_DSE_NOFATCHAIN(xip);
		SET_DSE_FIRSTCLUSTER(xip, 0);
	}

	exfatfs_writeback(xip);

	/* Invalidate cache */
	xip->xi_fatcache_lc = 0;
	xip->xi_fatcache_pc = 0;

	return 0;
}

/*
 * Allocate clusters from the free map.
 */
static int deextend(struct xfinode *xip, off_t bytes, int ioflags,
	kauth_cred_t cred)
{
	struct exfatfs *fs = xip->xi_fs;
	uint32_t newcount = EXFATFS_BYTES2CLUSTER(fs, bytes) +
		((bytes & CLUSTERMASK(fs)) ? 1 : 0);
	uint32_t oldcount = EXFATFS_BYTES2CLUSTER(fs,
		GET_DSE_DATALENGTH_BLK(xip, fs));
	uint32_t pcn, lcn, opcn;
	daddr_t bn;
	struct buf *bp = NULL;
	int error, allocated;

	DPRINTF(("deextend(0x%lx, %ld, 0x%x, .) old=%u new=%u\n",
		 (unsigned long)INUM(xip), bytes, ioflags, oldcount, newcount));
	
	/* If the file is already large enough, nothing to do */
	if (oldcount >= newcount)
		return 0;

	if (!IS_DSE_ALLOCPOSSIBLE(xip))
		return ENOSPC; /* XXX */
	
	/*
	 * Walk the FAT chain until we reach the address of interest
	 * By default we start from zero
	 */
	lcn = 0;
	pcn = GET_DSE_FIRSTCLUSTER(xip);
	opcn = 0;

	/* If we are contiguous and have an allocation, jump to the end */
	if (oldcount > 0 && pcn > 0 && IS_DSE_NOFATCHAIN(xip)) {
		lcn = oldcount - 1;
		pcn += lcn;
	}
#if defined USE_FATCACHE
	/* But if we have a value cached, use that instead */
	else if (xip->xi_fatcache_lc > 0 && xip->xi_fatcache_lc < newcount) {
		lcn = xip->xi_fatcache_lc;
		pcn = xip->xi_fatcache_pc;
		DPRINTF(("Cached starting at cluster #%u = physical %u\n",
			 (unsigned)lcn, (unsigned)pcn));
	}
#endif /* USE_FATCACHE */

	if (lcn >= newcount) {
		printf("impossible: bytes=%ld datalength=%lu lcn=%u old=%u"
		       " new=%u\n", (long)bytes,
		       (unsigned long)GET_DSE_DATALENGTH_BLK(xip, fs),
		       lcn, oldcount, newcount);
	}
	KASSERT(lcn < newcount);
	while (lcn < newcount) {
		DPRINTF(("deextend pcn %u lcn %u < newcount %u\n",
			 pcn, lcn, newcount));
		
		/* If pcn is not allocated, allocate it */
		allocated = 0;
		if (pcn == 0 || pcn == 0xFFFFFFFF) {
			uint32_t tpcn = opcn + 1;
#ifndef ALLOC_SEQUENTIAL
			mutex_enter(&fs->xf_lock);
			tpcn = fs->xf_next_cluster;
			mutex_exit(&fs->xf_lock);
#endif /* ALLOC_SEQUENTIAL */
			/* Get a new cluster from the free map */
			if ((error = exfatfs_bitmap_alloc(fs,
							  tpcn,
							  &pcn)) != 0) {
				if (bp)
					brelse(bp, 0);
				return error;
			}
#ifndef ALLOC_SEQUENTIAL
			mutex_enter(&fs->xf_lock);
			fs->xf_next_cluster = pcn + 1;
			mutex_exit(&fs->xf_lock);
#endif /* !ALLOC_SEQUENTIAL */
			allocated = 1;
			DPRINTF(("  allocated lclust %u = pclust 0x%x"
				 " (following 0x%x)\n", lcn, pcn, opcn));
			/* Store the allocation */
			if (lcn == 0) {
				/* Store it in the directory entry */
				SET_DSE_FIRSTCLUSTER(xip, pcn);
				SET_DSE_NOFATCHAIN(xip);
			} else {
				/* Store it in the FAT, just after opcn */
				if ((error = bread(fs->xf_devvp,
						   EXFATFS_FATBLK(fs, opcn),
						   SECSIZE(fs), 0, &bp)) != 0)
					return error;
				((uint32_t *)bp->b_data)[EXFATFS_FATOFF(opcn)]
					= pcn;
				DPRINTF(("FAT %lu -> %lu\n",
					 (unsigned long)opcn,
					 (unsigned long)pcn));
				/*
				 * If the clusters are not adjacent,
				 * readers can no longer use dead
				 * reckoning for bmap.
				 */
				if (pcn != opcn + 1 && IS_DSE_NOFATCHAIN(xip)) {
					DPRINTF(("inum 0x%lx not consecutive"
						 " with 0x%x != 0x%x+1\n",
						 INUM(xip), pcn, opcn));
					CLR_DSE_NOFATCHAIN(xip);
				}
				if (ioflags)
					bwrite(bp);
				else
					bdwrite(bp);
				bp = NULL;
			}

			/* Fill a new directory cluster with zero */
			if (ISDIRECTORY(xip)) {
				for (bn = EXFATFS_CLUSTER2HWADDR(fs, pcn);
				     EXFATFS_HWADDR2CLUSTER(fs, bn) == pcn;
				     bn += EXFATFS_FSSEC2DEVBSIZE(fs, 1)) {
					bp = getblk(fs->xf_devvp, bn,
						    SECSIZE(fs), 0, 0);
					memset(bp->b_data, 0, SECSIZE(fs));
					bdwrite(bp);
					bp = NULL;
				}
			}

		}
		
		/* Increment lcn */
		++lcn;
		if (lcn >= newcount) {
			--lcn; /* Need lcn to match pcn */
			break;
		}

		/* Read next pcn */
		opcn = pcn;
		if (allocated) {
			pcn = 0;
		} else {
			if ((error = bread(fs->xf_devvp,
					   EXFATFS_FATBLK(fs, pcn),
					   SECSIZE(fs), 0, &bp)) != 0)
				return error;
			pcn = ((uint32_t *)bp->b_data)[EXFATFS_FATOFF(pcn)];
			brelse(bp, 0);
			bp = NULL;
		}
	}
	KASSERT(lcn < newcount);

	DPRINTF(("deextend end with pcn %u lcn %u\n", pcn, lcn));

	/* Update size */
	SET_DSE_DATALENGTH(xip, bytes);
	exfatfs_writeback(xip);
	
	if (lcn == newcount - 1 && pcn >= 2 && pcn < fs->xf_ClusterCount + 2) {
		xip->xi_fatcache_lc = lcn;
		xip->xi_fatcache_pc = pcn;
	} else {
		if (lcn < newcount - 1)
			printf("Couldn't find lcn #%lu\n", (unsigned long)lcn);
		else
			printf("pcn 0x%x not in 0x2..0x%x\n",
			       (unsigned)pcn, fs->xf_ClusterCount + 1);
		return EIO;
	}

	return 0;
}

int
exfatfs_deactivate(struct xfinode *xip, bool rekey)
{
	int i, error;
	struct exfatfs_dirent_key new_key;

	KASSERT(xip != NULL);
	
	new_key = xip->xi_key;
	new_key.dk_dirgen = xip;
	
	for (i = 0; i < EXFATFS_MAXDIRENT && xip->xi_direntp[i] != NULL; i++)
		xip->xi_direntp[i]->xd_entryType &= ~XD_ENTRYTYPE_INUSE_MASK;

	if ((error = exfatfs_writeback(xip)) != 0)
		return error;

	if (rekey && XITOV(xip) != NULL)
		return exfatfs_rekey(XITOV(xip), &new_key);
	else
		return 0;
}

int
exfatfs_activate(struct xfinode *xip, bool rekey)
{
	int i, error;
	struct exfatfs_dirent_key new_key;

	KASSERT(xip != NULL);

	new_key = xip->xi_key;
	new_key.dk_dirgen = NULL;
	
	for (i = 0; i < EXFATFS_MAXDIRENT && xip->xi_direntp[i] != NULL; i++)
		xip->xi_direntp[i]->xd_entryType |= XD_ENTRYTYPE_INUSE_MASK;

	if ((error = exfatfs_writeback(xip)) != 0)
		return error;

	if (rekey && XITOV(xip) != NULL)
		return exfatfs_rekey(XITOV(xip), &new_key);
	else
		return 0;
}
