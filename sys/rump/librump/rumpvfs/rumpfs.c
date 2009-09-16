/*	$NetBSD: rumpfs.c,v 1.16.2.5 2009/09/16 13:38:05 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rumpfs.c,v 1.16.2.5 2009/09/16 13:38:05 yamt Exp $");

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

static struct rumpfs_node *makeprivate(enum vtype, dev_t, off_t);

/*
 * Extra Terrestrial stuff.  We map a given key (pathname) to a file on
 * the host FS.  ET phones home only from the root node of rumpfs.
 *
 * When an etfs node is removed, a vnode potentially behind it is not
 * immediately recycled.
 */

struct etfs {
	char et_key[MAXPATHLEN];
	LIST_ENTRY(etfs) et_entries;

	struct rumpfs_node *et_rn;
};
static kmutex_t etfs_lock;
static LIST_HEAD(, etfs) etfs_list = LIST_HEAD_INITIALIZER(etfs_list);

static enum vtype
ettype_to_vtype(enum rump_etfs_type et)
{
	enum vtype vt;

	switch (et) {
	case RUMP_ETFS_REG:
		vt = VREG;
		break;
	case RUMP_ETFS_BLK:
		vt = VBLK;
		break;
	case RUMP_ETFS_CHR:
		vt = VCHR;
		break;
	default:	
		panic("invalid et type: %d", et);
	}

	return vt;
}

static bool
etfs_find(const char *key, struct rumpfs_node **rnp)
{
	struct etfs *et;
	bool rv = false;

	KASSERT(mutex_owned(&etfs_lock));

	LIST_FOREACH(et, &etfs_list, et_entries) {
		if (strcmp(key, et->et_key) == 0) {
			*rnp = et->et_rn;
			rv = true;
			break;
		}
	}

	return rv;
}

int
rump_etfs_register(const char *key, const char *hostpath, 
	enum rump_etfs_type ftype)
{
	struct etfs *et;
	struct rumpfs_node *rn_dummy;
	uint64_t fsize;
	dev_t rdev;
	devminor_t dmin;
	int hft, error;

	/* not supported for now, need r/w VOPs ... */
	if (ftype == RUMP_ETFS_REG)
		return EOPNOTSUPP;

	if (rumpuser_getfileinfo(hostpath, &fsize, &hft, &error))
		return error;

	error = rumpblk_register(hostpath, &dmin);
	if (error != 0) {
		return error;
	}
	rdev = makedev(RUMPBLK, dmin);

	et = kmem_alloc(sizeof(*et), KM_SLEEP);
	strcpy(et->et_key, key);
	et->et_rn = makeprivate(ettype_to_vtype(ftype), rdev, fsize);

	mutex_enter(&etfs_lock);
	if (etfs_find(key, &rn_dummy)) {
		mutex_exit(&etfs_lock);
		kmem_free(et, sizeof(*et));
		/* XXX: rumpblk_deregister(hostpath); */
		return EEXIST;
	}
	LIST_INSERT_HEAD(&etfs_list, et, et_entries);
	mutex_exit(&etfs_lock);

	return 0;
}

int
rump_etfs_remove(const char *key)
{
	struct etfs *et;

	mutex_enter(&etfs_lock);
	LIST_FOREACH(et, &etfs_list, et_entries) {
		if (strcmp(et->et_key, key) == 0) {
			LIST_REMOVE(et, et_entries);
			kmem_free(et, sizeof(*et));
			break;
		}
	}
	mutex_exit(&etfs_lock);

	if (!et)
		return ENOENT;
	return 0;
}

/*
 * rumpfs
 */

static struct mount rump_mnt;
static int lastino = 1;
static kmutex_t reclock;

static struct rumpfs_node *
makeprivate(enum vtype vt, dev_t rdev, off_t size)
{
	struct rumpfs_node *rn;
	struct vattr *va;
	struct timespec ts;

	rn = kmem_zalloc(sizeof(*rn), KM_SLEEP);
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
makevnode(struct rumpfs_node *rn, struct vnode **vpp)
{
	struct vnode *vp;
	int (**vpops)(void *);
	struct vattr *va = &rn->rn_va;
	int rv;

	KASSERT(mutex_owned(&reclock));

	if (va->va_type == VCHR || va->va_type == VBLK) {
		vpops = rump_specop_p;
	} else {
		vpops = rump_vnodeop_p;
	}
	if (vpops != rump_specop_p && va->va_type != VDIR)
		return EOPNOTSUPP;

	rv = getnewvnode(VT_RUMP, &rump_mnt, vpops, &vp);
	if (rv)
		return rv;

	vp->v_size = vp->v_writesize = va->va_size;
	vp->v_type = va->va_type;

	if (vpops == rump_specop_p) {
		spec_node_init(vp, va->va_rdev);
	}
	vp->v_data = rn;

	vn_lock(vp, LK_RETRY | LK_EXCLUSIVE);
	rn->rn_vp = vp;
	*vpp = vp;

	return 0;
}

/*
 * Simple lookup for faking lookup of device entry for rump file systems
 * and for locating/creating directories.  Yes, this will panic if you
 * call it with the wrong arguments.
 *
 * uhm, this is twisted.  C F C C, hope of C C F C looming
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
	struct rumpfs_node *rnd = dvp->v_data, *rn;
	struct rumpfs_dent *rd = NULL;
	int rv;

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
		mutex_enter(&etfs_lock);
		if (etfs_find(cnp->cn_pnbuf, &rn)) {
			mutex_exit(&etfs_lock);
			cnp->cn_consume = strlen(cnp->cn_nameptr
			    + cnp->cn_namelen);
			cnp->cn_flags &= ~REQUIREDIR;
			goto getvnode;
		}
		mutex_exit(&etfs_lock);
	}

	if (!rd) {
		LIST_FOREACH(rd, &rnd->rn_dir, rd_entries) {
			if (strncmp(rd->rd_name, cnp->cn_nameptr,
			    cnp->cn_namelen) == 0)
				break;
		}
	}

	if (!rd && ((cnp->cn_flags & ISLASTCN) == 0||cnp->cn_nameiop != CREATE))
		return ENOENT;

	if (!rd && (cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop == CREATE) {
		cnp->cn_flags |= SAVENAME;
		return EJUSTRETURN;
	}
	rn = rd->rd_node;
	rd = NULL;

 getvnode:
	KASSERT(rn);
	mutex_enter(&reclock);
	if ((vp = rn->rn_vp)) {
		mutex_enter(&vp->v_interlock);
		mutex_exit(&reclock);
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
			goto getvnode;
		*vpp = vp;
	} else {
		rv = makevnode(rn, vpp);
		rn->rn_vp = *vpp;
		mutex_exit(&reclock);
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
	struct rumpfs_node *rnd = dvp->v_data, *rn;
	struct rumpfs_dent *rdent;
	int rv = 0;

	rn = makeprivate(VDIR, NODEV, DEV_BSIZE);
	mutex_enter(&reclock);
	rv = makevnode(rn, vpp);
	mutex_exit(&reclock);
	if (rv)
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
	struct rumpfs_node *rnd = dvp->v_data, *rn;
	struct rumpfs_dent *rdent;
	int rv;

	rn = makeprivate(va->va_type, va->va_rdev, DEV_BSIZE);
	mutex_enter(&reclock);
	rv = makevnode(rn, vpp);
	mutex_exit(&reclock);
	if (rv)
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
	case VOP_LOCK_DESCOFFSET:
	case VOP_UNLOCK_DESCOFFSET:
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
	struct rumpfs_node *rn;
	int rv;

	mutex_init(&reclock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&etfs_lock, MUTEX_DEFAULT, IPL_NONE);

	/* XXX: init properly instead of this crap */
	rump_mnt.mnt_refcnt = 1;
	rump_mnt.mnt_flag = MNT_ROOTFS;
	rw_init(&rump_mnt.mnt_unmounting);
	TAILQ_INIT(&rump_mnt.mnt_vnodelist);

	vfs_opv_init(rump_opv_descs);
	rn = makeprivate(VDIR, NODEV, DEV_BSIZE);
	mutex_enter(&reclock);
	rv = makevnode(rn, &rootvnode);
	mutex_exit(&reclock);
	if (rv)
		panic("could not create root vnode: %d", rv);
	rootvnode->v_vflag |= VV_ROOT;
	VOP_UNLOCK(rootvnode, 0);
}
