
/*	$NetBSD: vnode.h,v 1.13.14.1 2018/06/25 07:25:26 pgoyette Exp $	*/

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
 * Copyright (c) 1988, 2010, Oracle and/or its affiliates. All rights reserved.
 */

/*	Copyright (c) 1983, 1984, 1985, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/vnode.h 308691 2016-11-15 18:22:50Z alc $
 */

#ifndef _OPENSOLARIS_SYS_VNODE_H_
#define	_OPENSOLARIS_SYS_VNODE_H_

#ifdef _KERNEL

struct vnode;
struct vattr;

typedef	struct vnode	vnode_t;
typedef	struct vattr	vattr_t;
typedef enum vtype vtype_t;

#include <sys/namei.h>
enum symfollow { NO_FOLLOW = NOFOLLOW };

/*
 * We'll use the two-parameter FreeBSD VOP_UNLOCK() in ZFS code.
 */

#define VOP_UNLOCK __hide_VOP_UNLOCK
#include_next <sys/vnode.h>
/* verify that the real definition is what we expect */
int __hide_VOP_UNLOCK(struct vnode *);
#undef VOP_UNLOCK

int VOP_UNLOCK(struct vnode *, int);

#include <sys/cred.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/buf.h>

#include <sys/vfs_syscalls.h>

typedef int (**vnodeops_t)(void *);

#define	vop_fid		vop_vptofh
#define	vop_fid_args	vop_vptofh_args
#define	a_fid		a_fhp

#define	v_count		v_usecount
#define	v_object	v_uobj

struct vop_vptofh_args {
	struct vnode *a_vp;
	struct fid *a_fid;
};

#define IS_XATTRDIR(vp)	(0)

#define v_lock v_interlock

#define SAVENAME 0

int	vn_is_readonly(vnode_t *);

#define	vn_vfswlock(vp)		(0)
#define	vn_vfsunlock(vp)	do { } while (0)
#define	vn_ismntpt(vp)		((vp)->v_type == VDIR && (vp)->v_mountedhere != NULL)
#define	vn_mountedvfs(vp)	((vp)->v_mountedhere)
#define vn_has_cached_data(vp)	((vp)->v_uobj.uo_npages != 0)
#define	vn_exists(vp)		do { } while (0)
#define	vn_invalid(vp)		do { } while (0)
#define	vn_free(vp)		do { } while (0)
#define vn_renamepath(tdvp, svp, tnm, lentnm)   do { } while (0)
#define	vn_matchops(vp, vops)	((vp)->v_op == &(vops))

#define	VN_HOLD(v)	vref(v)
#define	VN_RELE(v)						      \
do {								      \
	if ((v)->v_usecount == 0) {				      \
		printf("VN_RELE(%s,%d): %p unused\n", __FILE__, __LINE__, v); \
		vprint("VN_RELE", (v));				      \
		panic("VN_RELE");				      \
	} else {						      \
		vrele(v);					      \
	}							      \
} while (/*CONSTCOND*/0)
#define	VN_URELE(v)	vput(v)
#undef VN_RELE_ASYNC
#define VN_RELE_ASYNC(vp, taskq) 	vrele_async((vp))
#define VN_RELE_CLEANER(vp, taskq)	/* nothing */

#define	vnevent_create(vp, ct)			do { } while (0)
#define	vnevent_link(vp, ct)			do { } while (0)
#define	vnevent_remove(vp, dvp, name, ct)	do { } while (0)
#define	vnevent_rmdir(vp, dvp, name, ct)	do { } while (0)
#define	vnevent_rename_src(vp, dvp, name, ct)	do { } while (0)
#define	vnevent_rename_dest(vp, dvp, name, ct)	do { } while (0)
#define	vnevent_rename_dest_dir(vp, ct)		do { } while (0)

#define	specvp(vp, rdev, type, cr)	(VN_HOLD(vp), (vp))
#define	MANDMODE(mode)	(0)
#define	MANDLOCK(vp, mode)	(0)
#define	chklock(vp, op, offset, size, mode, ct)	(0)
#define	cleanlocks(vp, pid, foo)	do { } while (0)
#define	cleanshares(vp, pid)		do { } while (0)

/*
 * We will use va_spare is place of Solaris' va_mask.
 * This field is initialized in zfs_setattr().
 */
#define	va_mask		va_spare
/* TODO: va_fileid is shorter than va_nodeid !!! */
#define	va_nodeid	va_fileid
/* TODO: This field needs conversion! */
#define	va_nblocks	va_bytes
#define	va_blksize	va_blocksize
#define	va_seq		va_gen

#define	EXCL		0

#define	ACCESSED		(AT_ATIME)
#define	STATE_CHANGED		(AT_CTIME)
#define	CONTENT_MODIFIED	(AT_MTIME | AT_CTIME)

static inline void
vattr_init_mask(vattr_t *vap)
{

	vap->va_mask = 0;

	if (vap->va_type != VNON)
		vap->va_mask |= AT_TYPE;
	if (vap->va_uid != (uid_t)VNOVAL)
		vap->va_mask |= AT_UID;
	if (vap->va_gid != (gid_t)VNOVAL)
		vap->va_mask |= AT_GID;
	if (vap->va_size != (u_quad_t)VNOVAL)
		vap->va_mask |= AT_SIZE;
	if (vap->va_atime.tv_sec != VNOVAL)
		vap->va_mask |= AT_ATIME;
	if (vap->va_mtime.tv_sec != VNOVAL)
		vap->va_mask |= AT_MTIME;
	if (vap->va_mode != (mode_t)VNOVAL)
		vap->va_mask |= AT_MODE;
}

#define	FCREAT	O_CREAT
#define	FTRUNC	O_TRUNC
#define	FSYNC	FFSYNC
#define	FOFFMAX	0x00

static inline int
zfs_vn_open(const char *pnamep, enum uio_seg seg, int filemode, int createmode,
    vnode_t **vpp, enum create crwhy, mode_t umask)
{
	struct pathbuf *pb;
	struct nameidata nd;
	int error;

	ASSERT(seg == UIO_SYSSPACE);
	ASSERT((filemode & (FWRITE | FCREAT | FTRUNC | FOFFMAX)) != 0);
	ASSERT(crwhy == CRCREAT);
	ASSERT(umask == 0);

	pb = pathbuf_create(pnamep);
	NDINIT(&nd, LOOKUP, NOFOLLOW, pb);
	error = vn_open(&nd, filemode, createmode);
	if (error == 0) {
		VOP_UNLOCK(nd.ni_vp, 0);
		*vpp = nd.ni_vp;
	}
	pathbuf_destroy(pb);
	return (error);
}
#define	vn_open(pnamep, seg, filemode, createmode, vpp, crwhy, umask)	\
	zfs_vn_open((pnamep), (seg), (filemode), (createmode), (vpp), (crwhy), (umask))

#define	vn_openat(pnamep, seg, filemode, createmode, vpp, crwhy, umask, rootvn, unk)	\
	zfs_vn_open((pnamep), (seg), (filemode), (createmode), (vpp), (crwhy), (umask))

#define	RLIM64_INFINITY	0
static inline int
zfs_vn_rdwr(enum uio_rw rw, vnode_t *vp, caddr_t base, ssize_t len,
    offset_t offset, enum uio_seg seg, int ioflag, uint64_t ulimit, cred_t *cr,
    ssize_t *residp)
{
	int error;
	size_t resid;

	ASSERT(rw == UIO_WRITE);
	ASSERT(ioflag == 0);
	ASSERT(ulimit == RLIM64_INFINITY);

	ioflag = IO_UNIT;

	error = vn_rdwr(rw, vp, base, len, offset, seg, ioflag, cr,
	    &resid, curlwp);
	if (residp != NULL)
		*residp = (ssize_t)resid;
	return (error);
}
#define	vn_rdwr(rw, vp, base, len, offset, seg, ioflag, ulimit, cr, residp) \
	zfs_vn_rdwr((rw), (vp), (base), (len), (offset), (seg), (ioflag), (ulimit), (cr), (residp))

static inline int
zfs_vop_fsync(vnode_t *vp, int flag, cred_t *cr)
{
	int error;

	ASSERT(flag == FSYNC);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, cr, FSYNC_WAIT, 0, 0);
	VOP_UNLOCK(vp, 0);
	return (error);
}
#define	VOP_FSYNC(vp, flag, cr, unk)	zfs_vop_fsync((vp), (flag), (cr))

static inline int
zfs_vop_close(vnode_t *vp, int flag, int count, offset_t offset, cred_t *cr)
{

	ASSERT(flag == (FWRITE | FCREAT | FTRUNC | FOFFMAX));
	ASSERT(count == 1);
	ASSERT(offset == 0);

	return (vn_close(vp, flag, cr));
}
#define	VOP_CLOSE(vp, oflags, count, offset, cr, unk) \
	zfs_vop_close((vp), (oflags), (count), (offset), (cr))

static inline int
zfs_vop_getattr(vnode_t *vp, vattr_t *ap, int flag, cred_t *cr)
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_GETATTR(vp, ap, cr);
	VOP_UNLOCK(vp, 0);
	return (error);
}
#define	VOP_GETATTR(vp, ap, flag, cr, unk)	zfs_vop_getattr((vp), (ap), (flag), (cr))

static inline int
zfs_vop_seek(vnode_t *vp, off_t off, off_t *offp)
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_SEEK(vp, off, *offp, CRED());
	VOP_UNLOCK(vp, 0);
	return (error);
}
#define	VOP_SEEK(vp, off, offp, unk)	zfs_vop_seek(vp, off, offp)

#define	B_INVAL		BC_INVAL

static inline int
zfs_vop_putpage(vnode_t *vp, off_t off, size_t len, int flag)
{
	int nbflag;

	nbflag = 0;
	if (len == 0) {
		nbflag |= PGO_ALLPAGES;
	}
	if ((flag & B_ASYNC) == 0) {
		nbflag |= PGO_SYNCIO;
	}
	if ((flag & B_INVAL) != 0) {
		nbflag |= PGO_FREE;
	} else {
		nbflag |= PGO_CLEANIT;
	}

	mutex_enter(vp->v_interlock);
	return VOP_PUTPAGES(vp, off, len, nbflag);
}
#define	VOP_PUTPAGE(vp, off, len, flag, cr, ct)	zfs_vop_putpage((vp), (off), (len), (flag))

static inline int
vn_rename(char *from, char *to, enum uio_seg seg)
{

	ASSERT(seg == UIO_SYSSPACE);

	return (do_sys_rename(from, to, seg, 0));
}

static inline int
vn_remove(char *fnamep, enum uio_seg seg, enum rm dirflag)
{

	ASSERT(seg == UIO_SYSSPACE);
	ASSERT(dirflag == RMFILE);

	return (do_sys_unlink(fnamep, seg));
}

/*
 * VOP_ACCESS flags
 */
#define V_APPEND    0x2 /* want to do append only check */

#define VV_NOSYNC	0

#define VI_LOCK(vp)     mutex_enter((vp)->v_interlock)
#define VI_UNLOCK(vp)   mutex_exit((vp)->v_interlock)

#define VATTR_NULL(x) vattr_null(x)

#define getnewvnode_reserve(x)
#define getnewvnode_drop_reserve()
#define cache_purge_negative(vp) cache_purge(vp)

#endif	/* _KERNEL */

#endif	/* _OPENSOLARIS_SYS_VNODE_H_ */
