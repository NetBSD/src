
/*	$NetBSD: vnode.h,v 1.9.8.1 2014/08/19 23:52:23 tls Exp $	*/

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
 * $FreeBSD: src/sys/compat/opensolaris/sys/vnode.h,v 1.3 2007/05/31 11:51:49 kib Exp $
 */

#include <sys/mount.h>
#include_next <sys/vnode.h>

#ifndef _OPENSOLARIS_SYS_VNODE_H_
#define	_OPENSOLARIS_SYS_VNODE_H_

#include <sys/cred.h>
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/buf.h>
#include <sys/debug.h>


#ifdef _KERNEL
#include <sys/vfs_syscalls.h>
#endif

typedef	struct vattr	vattr_t;
typedef	enum vtype	vtype_t;
typedef	void		caller_context_t;

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

/*
 * Structure used on VOP_GETSECATTR and VOP_SETSECATTR operations
 */

/*
 * Flags to VOP_SETATTR/VOP_GETATTR.
 */
#define ATTR_UTIME  0x01    /* non-default utime(2) request */
#define ATTR_EXEC   0x02    /* invocation from exec(2) */
#define ATTR_COMM   0x04    /* yield common vp attributes */
#define ATTR_HINT   0x08    /* information returned will be `hint' */
#define ATTR_REAL   0x10    /* yield attributes of the real vp */
#define ATTR_NOACLCHECK 0x20    /* Don't check ACL when checking permissions */
#define ATTR_TRIGGER    0x40    /* Mount first if vnode is a trigger mount */

typedef struct vsecattr {
	uint_t		vsa_mask;	/* See below */
	int		vsa_aclcnt;	/* ACL entry count */
	void		*vsa_aclentp;	/* pointer to ACL entries */
	int		vsa_dfaclcnt;	/* default ACL entry count */
	void		*vsa_dfaclentp;	/* pointer to default ACL entries */
	size_t		vsa_aclentsz;	/* ACE size in bytes of vsa_aclentp */
	uint_t		vsa_aclflags;	/* ACE ACL flags */
} vsecattr_t;

#define V_XATTRDIR      0x0000  /* attribute unnamed directory */
#define IS_XATTRDIR(vp)	(0)

#define	AV_SCANSTAMP_SZ	32		/* length of anti-virus scanstamp */

/*
 * Structure of all optional attributes.
 */
typedef struct xoptattr {
	timestruc_t	xoa_createtime;	/* Create time of file */
	uint8_t		xoa_archive;
	uint8_t		xoa_system;
	uint8_t		xoa_readonly;
	uint8_t		xoa_hidden;
	uint8_t		xoa_nounlink;
	uint8_t		xoa_immutable;
	uint8_t		xoa_appendonly;
	uint8_t		xoa_nodump;
	uint8_t		xoa_opaque;
	uint8_t		xoa_av_quarantined;
	uint8_t		xoa_av_modified;
	uint8_t		xoa_av_scanstamp[AV_SCANSTAMP_SZ];
	uint8_t 	xoa_reparse;
} xoptattr_t;


/*
 * The xvattr structure is really a variable length structure that
 * is made up of:
 * - The classic vattr_t (xva_vattr)
 * - a 32 bit quantity (xva_mapsize) that specifies the size of the
 *   attribute bitmaps in 32 bit words.
 * - A pointer to the returned attribute bitmap (needed because the
 *   previous element, the requested attribute bitmap) is variable lenth.
 * - The requested attribute bitmap, which is an array of 32 bit words.
 *   Callers use the XVA_SET_REQ() macro to set the bits corresponding to
 *   the attributes that are being requested.
 * - The returned attribute bitmap, which is an array of 32 bit words.
 *   File systems that support optional attributes use the XVA_SET_RTN()
 *   macro to set the bits corresponding to the attributes that are being
 *   returned.
 * - The xoptattr_t structure which contains the attribute values
 *
 * xva_mapsize determines how many words in the attribute bitmaps.
 * Immediately following the attribute bitmaps is the xoptattr_t.
 * xva_getxoptattr() is used to get the pointer to the xoptattr_t
 * section.
 */

#define	XVA_MAPSIZE	3		/* Size of attr bitmaps */
#define	XVA_MAGIC	0x78766174	/* Magic # for verification */

/*
 * The xvattr structure is an extensible structure which permits optional
 * attributes to be requested/returned.  File systems may or may not support
 * optional attributes.  They do so at their own discretion but if they do
 * support optional attributes, they must register the VFSFT_XVATTR feature
 * so that the optional attributes can be set/retrived.
 *
 * The fields of the xvattr structure are:
 *
 * xva_vattr - The first element of an xvattr is a legacy vattr structure
 * which includes the common attributes.  If AT_XVATTR is set in the va_mask
 * then the entire structure is treated as an xvattr.  If AT_XVATTR is not
 * set, then only the xva_vattr structure can be used.
 *
 * xva_magic - 0x78766174 (hex for "xvat"). Magic number for verification.
 *
 * xva_mapsize - Size of requested and returned attribute bitmaps.
 *
 * xva_rtnattrmapp - Pointer to xva_rtnattrmap[].  We need this since the
 * size of the array before it, xva_reqattrmap[], could change which means
 * the location of xva_rtnattrmap[] could change.  This will allow unbundled
 * file systems to find the location of xva_rtnattrmap[] when the sizes change.
 *
 * xva_reqattrmap[] - Array of requested attributes.  Attributes are
 * represented by a specific bit in a specific element of the attribute
 * map array.  Callers set the bits corresponding to the attributes
 * that the caller wants to get/set.
 *
 * xva_rtnattrmap[] - Array of attributes that the file system was able to
 * process.  Not all file systems support all optional attributes.  This map
 * informs the caller which attributes the underlying file system was able
 * to set/get.  (Same structure as the requested attributes array in terms
 * of each attribute  corresponding to specific bits and array elements.)
 *
 * xva_xoptattrs - Structure containing values of optional attributes.
 * These values are only valid if the corresponding bits in xva_reqattrmap
 * are set and the underlying file system supports those attributes.
 */
typedef struct xvattr {
	vattr_t		xva_vattr;	/* Embedded vattr structure */
	uint32_t	xva_magic;	/* Magic Number */
	uint32_t	xva_mapsize;	/* Size of attr bitmap (32-bit words) */
	uint32_t	*xva_rtnattrmapp;	/* Ptr to xva_rtnattrmap[] */
	uint32_t	xva_reqattrmap[XVA_MAPSIZE];	/* Requested attrs */
	uint32_t	xva_rtnattrmap[XVA_MAPSIZE];	/* Returned attrs */
	xoptattr_t	xva_xoptattrs;	/* Optional attributes */
} xvattr_t;

/* vsa_mask values */
#define	VSA_ACL			0x0001
#define	VSA_ACLCNT		0x0002
#define	VSA_DFACL		0x0004
#define	VSA_DFACLCNT		0x0008
#define	VSA_ACE			0x0010
#define	VSA_ACECNT		0x0020
#define	VSA_ACE_ALLTYPES	0x0040
#define	VSA_ACE_ACLFLAGS	0x0080	/* get/set ACE ACL flags */

#define v_lock v_interlock

/*
 * vnode flags.
 */
#define VROOT		VV_ROOT/* root of its file system */
#define VNOCACHE	0x00/* don't keep cache pages on vnode */
#define VNOMAP		VV_MAPPED/* file cannot be mapped/faulted */
#define VDUP		0x00/* file should be dup'ed rather then opened */
#define VNOSWAP		0x00/* file cannot be used as virtual swap device */
#define VNOMOUNT	0x00/* file cannot be covered by mount */
#define VISSWAP		0x00/* vnode is being used for swap */
#define VSWAPLIKE	0x00/* vnode acts like swap (but may not be) */

int	vn_is_readonly(vnode_t *);

#define	vn_free(vp)		vrele((vp))
#define	vn_setops(vp, ops)	(0)
#define	vn_vfswlock(vp)		(0)
#define	vn_vfsunlock(vp)	do { } while (0)
#define	vn_ismntpt(vp)		((vp)->v_type == VDIR && (vp)->v_mountedhere != NULL)
#define	vn_mountedvfs(vp)	((vp)->v_mountedhere)
#define	vn_has_cached_data(vp)	((vp)->v_uobj.uo_npages > 0)
#define vn_renamepath(tdvp, svp, tnm, lentnm)   do { } while (0)

#define	VN_HOLD(v)	vref(v)
#define	VN_RELE(v)	do { \
	    if ((v)->v_usecount == 0) \
		    printf("%s, %d: %p unused\n", __FILE__, __LINE__, v); \
	    else \
		    vrele(v); \
    } while (/*CONSTCOND*/0)
#define	VN_URELE(v)	vput(v)
#define	VN_SET_VFS_TYPE_DEV(vp, vfs, type, flag)	(0)

#define VI_LOCK(vp)     mutex_enter((vp)->v_interlock)
#define VI_UNLOCK(vp)   mutex_exit((vp)->v_interlock)

#define	VOP_REALVP(vp, vpp, ct)	(*(vpp) = (vp), 0)

#define vnevent_remove(vp, dvp, name, ct) do { } while (0)
#define vnevent_rmdir(vp, dvp, name, ct) do { } while (0)
#define vnevent_rename_src(vp, dvp, name, ct) do { } while (0)
#define	vnevent_rename_dest(vp, dvp, name, ct)	do { } while (0)
#define vnevent_rename_dest_dir(vp, ct)     do { } while (0)
#define vnevent_create(vp, ct)  do { } while (0)
#define vnevent_link(vp, ct)            do { } while (0)

#define	IS_DEVVP(vp)	\
	((vp)->v_type == VCHR || (vp)->v_type == VBLK || (vp)->v_type == VFIFO)

#define	MODEMASK	ALLPERMS

#define	specvp(vp, rdev, type, cr)	(VN_HOLD(vp), (vp))
#define	MANDMODE(mode)	(0)
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

#define	AT_TYPE		0x0001
#define	AT_MODE		0x0002
#define	AT_UID		0x0004
#define	AT_GID		0x0008
#define	AT_FSID		0x0010
#define	AT_NODEID	0x0020
#define	AT_NLINK	0x0040
#define	AT_SIZE		0x0080
#define	AT_ATIME	0x0100
#define	AT_MTIME	0x0200
#define	AT_CTIME	0x0400
#define	AT_RDEV		0x0800
#define	AT_BLKSIZE	0x1000
#define	AT_NBLOCKS	0x2000
#define	AT_SEQ		0x4000
#define	AT_NOSET	(AT_NLINK|AT_RDEV|AT_FSID|AT_NODEID|AT_TYPE|\
			 AT_BLKSIZE|AT_NBLOCKS|AT_SEQ)

#define	ACCESSED		(AT_ATIME)
#define	STATE_CHANGED		(AT_CTIME)
#define	CONTENT_MODIFIED	(AT_MTIME | AT_CTIME)

/*
 * If AT_XVATTR is set then there are additional bits to process in
 * the xvattr_t's attribute bitmap.  If this is not set then the bitmap
 * MUST be ignored.  Note that this bit must be set/cleared explicitly.
 * That is, setting AT_ALL will NOT set AT_XVATTR.
 */
#define	AT_XVATTR	0x10000

#define	AT_ALL		(AT_TYPE|AT_MODE|AT_UID|AT_GID|AT_FSID|AT_NODEID|\
			AT_NLINK|AT_SIZE|AT_ATIME|AT_MTIME|AT_CTIME|\
			AT_RDEV|AT_BLKSIZE|AT_NBLOCKS|AT_SEQ)

#define	AT_STAT		(AT_MODE|AT_UID|AT_GID|AT_FSID|AT_NODEID|AT_NLINK|\
			AT_SIZE|AT_ATIME|AT_MTIME|AT_CTIME|AT_RDEV|AT_TYPE)

#define	AT_TIMES	(AT_ATIME|AT_MTIME|AT_CTIME)

#define	AT_NOSET	(AT_NLINK|AT_RDEV|AT_FSID|AT_NODEID|AT_TYPE|\
			AT_BLKSIZE|AT_NBLOCKS|AT_SEQ)

/*
 * Attribute bits used in the extensible attribute's (xva's) attribute
 * bitmaps.  Note that the bitmaps are made up of a variable length number
 * of 32-bit words.  The convention is to use XAT{n}_{attrname} where "n"
 * is the element in the bitmap (starting at 1).  This convention is for
 * the convenience of the maintainer to keep track of which element each
 * attribute belongs to.
 *
 * NOTE THAT CONSUMERS MUST *NOT* USE THE XATn_* DEFINES DIRECTLY.  CONSUMERS
 * MUST USE THE XAT_* DEFINES.
 */
#define	XAT0_INDEX	0LL		/* Index into bitmap for XAT0 attrs */
#define	XAT0_CREATETIME	0x00000001	/* Create time of file */
#define	XAT0_ARCHIVE	0x00000002	/* Archive */
#define	XAT0_SYSTEM	0x00000004	/* System */
#define	XAT0_READONLY	0x00000008	/* Readonly */
#define	XAT0_HIDDEN	0x00000010	/* Hidden */
#define	XAT0_NOUNLINK	0x00000020	/* Nounlink */
#define	XAT0_IMMUTABLE	0x00000040	/* immutable */
#define	XAT0_APPENDONLY	0x00000080	/* appendonly */
#define	XAT0_NODUMP	0x00000100	/* nodump */
#define	XAT0_OPAQUE	0x00000200	/* opaque */
#define	XAT0_AV_QUARANTINED	0x00000400	/* anti-virus quarantine */
#define	XAT0_AV_MODIFIED	0x00000800	/* anti-virus modified */
#define	XAT0_AV_SCANSTAMP	0x00001000	/* anti-virus scanstamp */
#define XAT0_REPARSE 	0x00002000 	/* FS reparse point */

#define	XAT0_ALL_ATTRS	(XAT0_CREATETIME|XAT0_ARCHIVE|XAT0_SYSTEM| \
    XAT0_READONLY|XAT0_HIDDEN|XAT0_NOUNLINK|XAT0_IMMUTABLE|XAT0_APPENDONLY| \
    XAT0_NODUMP|XAT0_OPAQUE|XAT0_AV_QUARANTINED| \
    XAT0_AV_MODIFIED|XAT0_AV_SCANSTAMP)

/* Support for XAT_* optional attributes */
#define	XVA_MASK		0xffffffff	/* Used to mask off 32 bits */
#define	XVA_SHFT		32		/* Used to shift index */

/*
 * Used to pry out the index and attribute bits from the XAT_* attributes
 * defined below.  Note that we're masking things down to 32 bits then
 * casting to uint32_t.
 */
#define	XVA_INDEX(attr)		((uint32_t)(((attr) >> XVA_SHFT) & XVA_MASK))
#define	XVA_ATTRBIT(attr)	((uint32_t)((attr) & XVA_MASK))

/*
 * The following defines present a "flat namespace" so that consumers don't
 * need to keep track of which element belongs to which bitmap entry.
 *
 * NOTE THAT THESE MUST NEVER BE OR-ed TOGETHER
 */
#define	XAT_CREATETIME		((XAT0_INDEX << XVA_SHFT) | XAT0_CREATETIME)
#define	XAT_ARCHIVE		((XAT0_INDEX << XVA_SHFT) | XAT0_ARCHIVE)
#define	XAT_SYSTEM		((XAT0_INDEX << XVA_SHFT) | XAT0_SYSTEM)
#define	XAT_READONLY		((XAT0_INDEX << XVA_SHFT) | XAT0_READONLY)
#define	XAT_HIDDEN		((XAT0_INDEX << XVA_SHFT) | XAT0_HIDDEN)
#define	XAT_NOUNLINK		((XAT0_INDEX << XVA_SHFT) | XAT0_NOUNLINK)
#define	XAT_IMMUTABLE		((XAT0_INDEX << XVA_SHFT) | XAT0_IMMUTABLE)
#define	XAT_APPENDONLY		((XAT0_INDEX << XVA_SHFT) | XAT0_APPENDONLY)
#define	XAT_NODUMP		((XAT0_INDEX << XVA_SHFT) | XAT0_NODUMP)
#define	XAT_OPAQUE		((XAT0_INDEX << XVA_SHFT) | XAT0_OPAQUE)
#define	XAT_AV_QUARANTINED	((XAT0_INDEX << XVA_SHFT) | XAT0_AV_QUARANTINED)
#define	XAT_AV_MODIFIED		((XAT0_INDEX << XVA_SHFT) | XAT0_AV_MODIFIED)
#define	XAT_AV_SCANSTAMP	((XAT0_INDEX << XVA_SHFT) | XAT0_AV_SCANSTAMP)
#define XAT_REPARSE 		((XAT0_INDEX << XVA_SHFT) | XAT0_REPARSE)

/*
 * The returned attribute map array (xva_rtnattrmap[]) is located past the
 * requested attribute map array (xva_reqattrmap[]).  Its location changes
 * when the array sizes change.  We use a separate pointer in a known location
 * (xva_rtnattrmapp) to hold the location of xva_rtnattrmap[].  This is
 * set in xva_init()
 */
#define	XVA_RTNATTRMAP(xvap)	((xvap)->xva_rtnattrmapp)

/*
 * XVA_SET_REQ() sets an attribute bit in the proper element in the bitmap
 * of requested attributes (xva_reqattrmap[]).
 */
#define	XVA_SET_REQ(xvap, attr)					\
	ASSERT((xvap)->xva_vattr.va_mask | AT_XVATTR);		\
	ASSERT((xvap)->xva_magic == XVA_MAGIC);			\
	(xvap)->xva_reqattrmap[XVA_INDEX(attr)] |= XVA_ATTRBIT(attr)

/*
 * XVA_CLR_REQ() clears an attribute bit in the proper element in the bitmap
 * of requested attributes (xva_reqattrmap[]).
 */
#define XVA_CLR_REQ(xvap, attr)                                 \
	ASSERT((xvap)->xva_vattr.va_mask | AT_XVATTR);          \
	ASSERT((xvap)->xva_magic == XVA_MAGIC);                 \
	(xvap)->xva_reqattrmap[XVA_INDEX(attr)] &= ~XVA_ATTRBIT(attr)
/*
 * XVA_SET_RTN() sets an attribute bit in the proper element in the bitmap
 * of returned attributes (xva_rtnattrmap[]).
 */
#define	XVA_SET_RTN(xvap, attr)					\
	ASSERT((xvap)->xva_vattr.va_mask | AT_XVATTR);		\
	ASSERT((xvap)->xva_magic == XVA_MAGIC);			\
	(XVA_RTNATTRMAP(xvap))[XVA_INDEX(attr)] |= XVA_ATTRBIT(attr)

/*
 * XVA_ISSET_REQ() checks the requested attribute bitmap (xva_reqattrmap[])
 * to see of the corresponding attribute bit is set.  If so, returns non-zero.
 */
#define	XVA_ISSET_REQ(xvap, attr)					\
	((((xvap)->xva_vattr.va_mask | AT_XVATTR) &&			\
		((xvap)->xva_magic == XVA_MAGIC) &&			\
		((xvap)->xva_mapsize > XVA_INDEX(attr))) ?		\
	((xvap)->xva_reqattrmap[XVA_INDEX(attr)] & XVA_ATTRBIT(attr)) :	0)

/*
 * XVA_ISSET_RTN() checks the returned attribute bitmap (xva_rtnattrmap[])
 * to see of the corresponding attribute bit is set.  If so, returns non-zero.
 */
#define	XVA_ISSET_RTN(xvap, attr)					\
	((((xvap)->xva_vattr.va_mask | AT_XVATTR) &&			\
		((xvap)->xva_magic == XVA_MAGIC) &&			\
		((xvap)->xva_mapsize > XVA_INDEX(attr))) ?		\
	((XVA_RTNATTRMAP(xvap))[XVA_INDEX(attr)] & XVA_ATTRBIT(attr)) : 0)

static __inline void
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
	if (vap->va_mode != (u_short)VNOVAL)
		vap->va_mask |= AT_MODE;
}

#define	FCREAT	O_CREAT
#define	FTRUNC	O_TRUNC
#define	FSYNC	FFSYNC
#define	FOFFMAX	0x00

enum create	{ CRCREAT };

static __inline int
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
	if (pb == NULL) {
		return ENOMEM;
	}
	NDINIT(&nd, LOOKUP, NOFOLLOW, pb);
	error = vn_open(&nd, filemode, createmode);
	if (error == 0) {
		VOP_UNLOCK(nd.ni_vp);
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
static __inline int
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

static __inline int
zfs_vop_fsync(vnode_t *vp, int flag, cred_t *cr)
{
	int error;

	ASSERT(flag == FSYNC);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, cr, FSYNC_WAIT, 0, 0);
	VOP_UNLOCK(vp);
	return (error);
}
#define	VOP_FSYNC(vp, flag, cr, unk)	zfs_vop_fsync((vp), (flag), (cr))

static __inline int
zfs_vop_close(vnode_t *vp, int flag, int count, offset_t offset, cred_t *cr)
{

	ASSERT(flag == (FWRITE | FCREAT | FTRUNC | FOFFMAX));
	ASSERT(count == 1);
	ASSERT(offset == 0);

	return (vn_close(vp, flag, cr));
}
#define	VOP_CLOSE(vp, oflags, count, offset, cr, unk) \
	zfs_vop_close((vp), (oflags), (count), (offset), (cr))

static __inline int
zfs_vop_getattr(vnode_t *vp, vattr_t *ap, int flag, cred_t *cr)
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_GETATTR(vp, ap, cr);
	VOP_UNLOCK(vp);
	return (error);
}
#define	VOP_GETATTR(vp, ap, flag, cr, unk)	zfs_vop_getattr((vp), (ap), (flag), (cr))

static __inline int
zfs_vop_seek(vnode_t *vp, off_t off, off_t *offp)
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_SEEK(vp, off, *offp, kauth_cred_get());
	VOP_UNLOCK(vp);
	return (error);
}
#define	VOP_SEEK(vp, off, offp, unk)	zfs_vop_seek(vp, off, offp)

#define	B_INVAL		BC_INVAL

static __inline int
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

static __inline int
vn_rename(char *from, char *to, enum uio_seg seg)
{

	ASSERT(seg == UIO_SYSSPACE);

	return (do_sys_rename(from, to, seg, 0));
}

enum rm	{ RMFILE };
static __inline int
vn_remove(char *fnamep, enum uio_seg seg, enum rm dirflag)
{

	ASSERT(seg == UIO_SYSSPACE);
	ASSERT(dirflag == RMFILE);

	return (do_sys_unlink(fnamep, seg));
}

#define VN_RELE_ASYNC(vp, taskq) 	vrele_async((vp))
#define vn_exists(a) 	do { } while(0)

/*
 * Flags for VOP_LOOKUP
 *
 * Defined in file.h, but also possible, FIGNORECASE
 *
 */
#define LOOKUP_XATTR 		0x02	/* lookup up extended attr dir */

/*
 * Flags for VOP_READDIR
 */
#define V_RDDIR_ENTFLAGS 	0x01    /* request dirent flags */
#define	V_RDDIR_ACCFILTER 	0x02	/* filter out inaccessible dirents */

/*
 * Extensible vnode attribute (xva) routines:
 * xva_init() initializes an xvattr_t (zero struct, init mapsize, set AT_XATTR)
 * xva_getxoptattr() returns a ponter to the xoptattr_t section of xvattr_t
 */
void            xva_init(xvattr_t *);
xoptattr_t	*xva_getxoptattr(xvattr_t *);

/*
 * VOP_ACCESS flags
 */
#define V_ACE_MASK  0x1 /* mask represents  NFSv4 ACE permissions */
#define V_APPEND    0x2 /* want to do append only check */

#endif	/* _OPENSOLARIS_SYS_VNODE_H_ */
