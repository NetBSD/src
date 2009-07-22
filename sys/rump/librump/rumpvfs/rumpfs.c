/*	$NetBSD: rumpfs.c,v 1.20 2009/07/22 21:06:56 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rumpfs.c,v 1.20 2009/07/22 21:06:56 pooka Exp $");

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
static int rump_vop_mkdir(void *);
static int rump_vop_mknod(void *);
static int rump_vop_inactive(void *);
static int rump_vop_reclaim(void *);
static int rump_vop_success(void *);
static int rump_vop_spec(void *);

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
	{ &vop_mkdir_desc, rump_vop_mkdir },
	{ &vop_mknod_desc, rump_vop_mknod },
	{ &vop_access_desc, rump_vop_success },
	{ &vop_putpages_desc, genfs_null_putpages },
	{ &vop_fsync_desc, rump_vop_success },
	{ &vop_lock_desc, genfs_lock },
	{ &vop_unlock_desc, genfs_unlock },
	{ &vop_inactive_desc, rump_vop_inactive },
	{ &vop_reclaim_desc, rump_vop_reclaim },
	{ NULL, NULL }
};
const struct vnodeopv_desc rump_vnodeop_opv_desc =
	{ &rump_vnodeop_p, rump_vnodeop_entries };

int (**rump_specop_p)(void *);
const struct vnodeopv_entry_desc rump_specop_entries[] = {
	{ &vop_default_desc, rump_vop_spec },
	{ NULL, NULL }
};
const struct vnodeopv_desc rump_specop_opv_desc =
	{ &rump_specop_p, rump_specop_entries };

const struct vnodeopv_desc * const rump_opv_descs[] = {
	&rump_vnodeop_opv_desc,
	&rump_specop_opv_desc,
	NULL
};

static struct mount rump_mnt;
static int lastino = 1;
static kmutex_t reclock;

struct rumpfs_dent {
	char *rd_name;
	struct rumpfs_node *rd_node;

	LIST_ENTRY(rumpfs_dent) rd_entries;
};

struct rumpfs_node {
	struct vattr rn_va;
	struct vnode *rn_vp;

	/* only for VDIR */
	LIST_HEAD(, rumpfs_dent) rn_dir;
};

static struct rumpfs_node *
makeprivate(enum vtype vt, dev_t rdev, voff_t size)
{
	struct rumpfs_node *rn;
	struct vattr *va;
	struct timespec ts;

	rn = kmem_alloc(sizeof(*rn), KM_SLEEP);
	LIST_INIT(&rn->rn_dir);
	nanotime(&ts);

	va = &rn->rn_va;
	va->va_type = vt;
	va->va_mode = 0755;
	if (vt == VDIR)
		va->va_nlink = 2;
	else
		va->va_nlink = 1;
	va->va_uid = 0;
	va->va_gid = 0;
	va->va_fsid = 
	va->va_fileid = atomic_inc_uint_nv(&lastino);
	va->va_size = size;
	va->va_blocksize = 512;
	va->va_atime = ts;
	va->va_mtime = ts;
	va->va_ctime = ts;
	va->va_birthtime = ts;
	va->va_gen = 0;
	va->va_flags = 0;
	va->va_rdev = rdev;
	va->va_bytes = 512;
	va->va_filerev = 0;
	va->va_vaflags = 0;

	return rn;
}

static int
rump_makevnode(const char *path, voff_t size, enum vtype vt, dev_t rdev,
	struct vnode **vpp, bool regrumpblk)
{
	struct vnode *vp;
	struct rumpfs_node *rn;
	int (**vpops)(void *);
	int rv;

	if (vt == VREG || vt == VCHR || vt == VBLK) {
		vt = VBLK;
		vpops = rump_specop_p;
	} else {
		vpops = rump_vnodeop_p;
	}
	if (vt != VBLK && vt != VDIR)
		panic("rump_makevnode: only VBLK/VDIR vnodes supported");

	rv = getnewvnode(VT_RUMP, &rump_mnt, vpops, &vp);
	if (rv)
		return rv;

	vp->v_size = vp->v_writesize = size;
	vp->v_type = vt;

	if (vp->v_type == VBLK) {
		if (regrumpblk) {
			rv = rumpblk_register(path);
			if (rv == -1)
				panic("rump_makevnode: lazy bum");
			rdev = makedev(RUMPBLK, rv);
			spec_node_init(vp, rdev);
		} else {
			spec_node_init(vp, rdev);
		}
	}
	rn = makeprivate(vp->v_type, rdev, size);
	rn->rn_vp = vp;
	vp->v_data = rn;

	vn_lock(vp, LK_RETRY | LK_EXCLUSIVE);
	*vpp = vp;

	return 0;
}

/*
 * Simple lookup for faking lookup of device entry for rump file systems
 * and for locating/creating directories.  Yes, this will panic if you
 * call it with the wrong arguments.
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
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct vnode *vp;
	struct rumpfs_node *rn = dvp->v_data;
	struct rumpfs_dent *rd;
	uint64_t fsize;
	enum vtype vt;
	int rv, error, ft;

	/* we handle only some "non-special" cases */
	if (!(((cnp->cn_flags & ISLASTCN) == 0)
	    || (cnp->cn_nameiop == LOOKUP || cnp->cn_nameiop == CREATE)))
		return EOPNOTSUPP;
	if (!((cnp->cn_flags & ISDOTDOT) == 0))
		return EOPNOTSUPP;
	if (!(cnp->cn_namelen != 0 && cnp->cn_pnbuf[0] != '.'))
		return EOPNOTSUPP;

	/* check if we are returning a faked block device */
	if (dvp == rootvnode && cnp->cn_nameiop == LOOKUP) {
		if (rump_fakeblk_find(cnp->cn_pnbuf)) {
			rv = rumpuser_getfileinfo(cnp->cn_pnbuf, &fsize,
			    &ft, &error);
			if (rv)
				return rv;
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
			error = rump_makevnode(cnp->cn_pnbuf, fsize, vt, -1,
			    vpp, true);
			if (error)
				return error;
			cnp->cn_consume = strlen(cnp->cn_nameptr
			    + cnp->cn_namelen);
			cnp->cn_flags &= ~REQUIREDIR;

			return 0;
		}
	}

	LIST_FOREACH(rd, &rn->rn_dir, rd_entries) {
		if (strncmp(rd->rd_name, cnp->cn_nameptr,
		    cnp->cn_namelen) == 0)
			break;
	}

	if (!rd && ((cnp->cn_flags & ISLASTCN) == 0||cnp->cn_nameiop != CREATE))
		return ENOENT;

	if (!rd && (cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop == CREATE) {
		cnp->cn_flags |= SAVENAME;
		return EJUSTRETURN;
	}
	KASSERT(rd);

 retry:
	mutex_enter(&reclock);
	if ((vp = rd->rd_node->rn_vp)) {
		mutex_enter(&vp->v_interlock);
		mutex_exit(&reclock);
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
			goto retry;
		*vpp = vp;
	} else {
		rv = rump_makevnode(cnp->cn_nameptr, DEV_BSIZE, VDIR, -1,
		    vpp, false);
		if (rv)
			return rv;
	}

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
	struct rumpfs_node *rn = ap->a_vp->v_data;

	memcpy(ap->a_vap, &rn->rn_va, sizeof(struct vattr));
	return 0;
}

static int
rump_vop_mkdir(void *v)
{
	struct vop_mkdir_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	}; */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct rumpfs_node *rnd = dvp->v_data;
	struct rumpfs_dent *rdent;
	int rv = 0;

	if ((rv = rump_makevnode(cnp->cn_nameptr, DEV_BSIZE, VDIR, -1,
	    vpp, false)) != 0)
		goto out;

	rdent = kmem_alloc(sizeof(*rdent), KM_SLEEP);
	rdent->rd_name = kmem_alloc(cnp->cn_namelen+1, KM_SLEEP);
	rdent->rd_node = (*vpp)->v_data;
	strlcpy(rdent->rd_name, cnp->cn_nameptr, cnp->cn_namelen+1);

	LIST_INSERT_HEAD(&rnd->rn_dir, rdent, rd_entries);

 out:
	vput(dvp);
	return rv;
}

static int
rump_vop_mknod(void *v)
{
	struct vop_mknod_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	}; */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct vattr *va = ap->a_vap;
	struct rumpfs_node *rnd = dvp->v_data;
	struct rumpfs_dent *rdent;
	int rv;

	if ((rv = rump_makevnode(cnp->cn_nameptr, DEV_BSIZE, va->va_type,
	    va->va_rdev, vpp, false)) != 0)
		goto out;

	rdent = kmem_alloc(sizeof(*rdent), KM_SLEEP);
	rdent->rd_name = kmem_alloc(cnp->cn_namelen+1, KM_SLEEP);
	rdent->rd_node = (*vpp)->v_data;
	rdent->rd_node->rn_va.va_rdev = va->va_rdev;
	strlcpy(rdent->rd_name, cnp->cn_nameptr, cnp->cn_namelen+1);

	LIST_INSERT_HEAD(&rnd->rn_dir, rdent, rd_entries);

 out:
	vput(dvp);
	return rv;
}

static int
rump_vop_success(void *v)
{

	return 0;
}

static int
rump_vop_inactive(void *v)
{
	struct vop_inactive_args *ap = v;

	VOP_UNLOCK(ap->a_vp, 0);
	return 0;
}

static int
rump_vop_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;

	mutex_enter(&reclock);
	rn->rn_vp = NULL;
	mutex_exit(&reclock);
	vp->v_data = NULL;

	return 0;
}

static int
rump_vop_spec(void *v)
{
	struct vop_generic_args *ap = v;
	int (**opvec)(void *);

	switch (ap->a_desc->vdesc_offset) {
	case VOP_ACCESS_DESCOFFSET:
	case VOP_GETATTR_DESCOFFSET:
		opvec = rump_vnodeop_p;
		break;
	default:
		opvec = spec_vnodeop_p;
		break;
	}

	return VOCALL(opvec, ap->a_desc->vdesc_offset, v);
}

void
rumpfs_init(void)
{
	int rv;

	mutex_init(&reclock, MUTEX_DEFAULT, IPL_NONE);

	/* XXX: init properly instead of this crap */
	rump_mnt.mnt_refcnt = 1;
	rump_mnt.mnt_flag = MNT_ROOTFS;
	rw_init(&rump_mnt.mnt_unmounting);
	TAILQ_INIT(&rump_mnt.mnt_vnodelist);

	vfs_opv_init(rump_opv_descs);
	rv = rump_makevnode("/", 0, VDIR, -1, &rootvnode, false);
	if (rv)
		panic("could not create root vnode: %d", rv);
	rootvnode->v_vflag |= VV_ROOT;
	VOP_UNLOCK(rootvnode, 0);
}
