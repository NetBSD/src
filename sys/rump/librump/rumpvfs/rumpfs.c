/*	$NetBSD: rumpfs.c,v 1.11 2009/04/07 18:35:49 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rumpfs.c,v 1.11 2009/04/07 18:35:49 pooka Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/lock.h>
#include <sys/lockf.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/queue.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>
#include <sys/atomic.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/genfs.h>

#include <rump/rumpuser.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

static int rump_vop_lookup(void *);
static int rump_vop_getattr(void *);
static int rump_vop_success(void *);

int (**dead_vnodeop_p)(void *);
const struct vnodeopv_entry_desc dead_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ NULL, NULL }
};
const struct vnodeopv_desc dead_vnodeop_opv_desc =
	{ &dead_vnodeop_p, dead_vnodeop_entries };

int (**fifo_vnodeop_p)(void *);
const struct vnodeopv_entry_desc fifo_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ NULL, NULL }
};
const struct vnodeopv_desc fifo_vnodeop_opv_desc =
	{ &fifo_vnodeop_p, fifo_vnodeop_entries };

int (**rump_vnodeop_p)(void *);
const struct vnodeopv_entry_desc rump_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, rump_vop_lookup },
	{ &vop_getattr_desc, rump_vop_getattr },
	{ &vop_putpages_desc, rump_vop_success },
	{ &vop_fsync_desc, rump_vop_success },
	{ &vop_lock_desc, genfs_lock },
	{ &vop_unlock_desc, genfs_unlock },
	{ NULL, NULL }
};
const struct vnodeopv_desc rump_vnodeop_opv_desc =
	{ &rump_vnodeop_p, rump_vnodeop_entries };
const struct vnodeopv_desc * const rump_opv_descs[] = {
	&rump_vnodeop_opv_desc,
	NULL
};

static struct mount rump_mnt;
static int lastino = 1;

static struct vattr *
makevattr(enum vtype vt)
{
	struct vattr *va;
	struct timespec ts;

	nanotime(&ts);

	va = kmem_alloc(sizeof(*va), KM_SLEEP);
	va->va_type = vt;
	va->va_mode = 0755;
	va->va_nlink = 2;
	va->va_uid = 0;
	va->va_gid = 0;
	va->va_fsid = 
	va->va_fileid = atomic_inc_uint_nv(&lastino);
	va->va_size = 512;
	va->va_blocksize = 512;
	va->va_atime = ts;
	va->va_mtime = ts;
	va->va_ctime = ts;
	va->va_birthtime = ts;
	va->va_gen = 0;
	va->va_flags = 0;
	va->va_rdev = -1;
	va->va_bytes = 512;
	va->va_filerev = 0;
	va->va_vaflags = 0;

	return va;
}

static int
rump_makevnode(const char *path, size_t size, enum vtype vt, struct vnode **vpp)
{
	struct vnode *vp;
	int rv;

	vp = kmem_alloc(sizeof(struct vnode), KM_SLEEP);
	vp->v_size = vp->v_writesize = size;
	vp->v_type = vt;
	if (vp->v_type == VREG || vp->v_type == VCHR)
		vp->v_type = VBLK;

	if (vp->v_type != VBLK && vp->v_type != VDIR)
		panic("rump_makevnode: only VBLK/VDIR vnodes supported");

	if (vp->v_type == VBLK) {
		rv = rumpblk_register(path);
		if (rv == -1) {
			rv = EINVAL;
			goto bad;
		}
		spec_node_init(vp, makedev(RUMPBLK, rv));
		vp->v_op = spec_vnodeop_p;
	} else {
		vp->v_op = rump_vnodeop_p;
	}
	vp->v_tag = VT_RUMP;
	vp->v_mount = &rump_mnt;
	vp->v_vnlock = &vp->v_lock;
	vp->v_usecount = 1;
	vp->v_data = makevattr(vp->v_type);
	mutex_init(&vp->v_interlock, MUTEX_DEFAULT, IPL_NONE);
	memset(&vp->v_lock, 0, sizeof(vp->v_lock));
	rw_init(&vp->v_lock.vl_lock);
	cv_init(&vp->v_cv, "vnode");
	*vpp = vp;

	return 0;

 bad:
	kmem_free(vp, sizeof(*vp));
	return rv;
}

/*
 * Simple lookup for faking lookup of device entry for rump file systems 
 */
static int
rump_vop_lookup(void *v)
{
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	}; */ *ap = v;
	struct componentname *cnp = ap->a_cnp;
	uint64_t fsize;
	enum vtype vt;
	int rv, error, ft;

	/* we handle only some "non-special" cases */
	KASSERT(cnp->cn_nameiop == LOOKUP);
	KASSERT(cnp->cn_flags & FOLLOW);
	KASSERT((cnp->cn_flags & ISDOTDOT) == 0);
	KASSERT(cnp->cn_namelen != 0 && cnp->cn_pnbuf[0] != '.');

	rv = rumpuser_getfileinfo(cnp->cn_pnbuf, &fsize, &ft, &error);
	if (rv)
		return error;
	switch (ft) {
	case RUMPUSER_FT_DIR:
		vt = VDIR;
		break;
	case RUMPUSER_FT_REG:
		vt = VREG;
		break;
	case RUMPUSER_FT_BLK:
		vt = VBLK;
		break;
	case RUMPUSER_FT_CHR:
		vt = VCHR;
		break;
	default:
		vt = VBAD;
		break;
	}

	error = rump_makevnode(cnp->cn_pnbuf, fsize, vt, ap->a_vpp);
	if (error)
		return error;

	vn_lock(*ap->a_vpp, LK_RETRY | LK_EXCLUSIVE);
	cnp->cn_consume = strlen(cnp->cn_nameptr + cnp->cn_namelen);
	cnp->cn_flags &= ~REQUIREDIR;

	return 0;
}

static int
rump_vop_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
	} */ *ap = v;

	memcpy(ap->a_vap, ap->a_vp->v_data, sizeof(struct vattr));
	return 0;
}

static int
rump_vop_success(void *v)
{

	return 0;
}

void
rumpfs_init(void)
{
	int rv;

	vfs_opv_init(rump_opv_descs);
	rv = rump_makevnode("/", 0, VDIR, &rootvnode);
	if (rv)
		panic("could not create root vnode: %d", rv);
	rootvnode->v_vflag |= VV_ROOT;
}
