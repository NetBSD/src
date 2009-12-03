/*	$NetBSD: rumpfs.c,v 1.35 2009/12/03 12:35:35 pooka Exp $	*/

/*
 * Copyright (c) 2009  Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: rumpfs.c,v 1.35 2009/12/03 12:35:35 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/lock.h>
#include <sys/lockf.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/vnode.h>

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
static int rump_vop_create(void *);
static int rump_vop_inactive(void *);
static int rump_vop_reclaim(void *);
static int rump_vop_success(void *);
static int rump_vop_spec(void *);
static int rump_vop_read(void *);
static int rump_vop_write(void *);
static int rump_vop_open(void *);

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
	{ &vop_create_desc, rump_vop_create },
	{ &vop_access_desc, rump_vop_success },
	{ &vop_read_desc, rump_vop_read },
	{ &vop_write_desc, rump_vop_write },
	{ &vop_open_desc, rump_vop_open },
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

	union {
		struct {
			char *hostpath;		/* VREG */
			int readfd;
			int writefd;
			uint64_t offset;
		} reg;
		LIST_HEAD(, rumpfs_dent) dir;	/* VDIR */
	} rn_u;
};
#define rn_hostpath	rn_u.reg.hostpath
#define rn_readfd	rn_u.reg.readfd
#define rn_writefd	rn_u.reg.writefd
#define rn_offset	rn_u.reg.offset
#define rn_dir		rn_u.dir

struct rumpfs_mount {
	struct vnode *rfsmp_rvp;
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
	size_t et_keylen;

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
	size_t keylen = strlen(key);
	bool rv = false;

	KASSERT(mutex_owned(&etfs_lock));

	LIST_FOREACH(et, &etfs_list, et_entries) {
		if (keylen == et->et_keylen && strcmp(key, et->et_key) == 0) {
			*rnp = et->et_rn;
			rv = true;
			break;
		}
	}

	return rv;
}

static int
doregister(const char *key, const char *hostpath, 
	enum rump_etfs_type ftype, uint64_t begin, uint64_t size)
{
	struct etfs *et;
	struct rumpfs_node *rn_dummy, *rn;
	uint64_t fsize;
	dev_t rdev = NODEV;
	devminor_t dmin;
	int hft, error;

	if (rumpuser_getfileinfo(hostpath, &fsize, &hft, &error))
		return error;

	/* check that we give sensible arguments */
	if (begin > fsize)
		return EINVAL;
	if (size == RUMP_ETFS_SIZE_ENDOFF)
		size = fsize - begin;
	if (begin + size > fsize)
		return EINVAL;

	if (ftype == RUMP_ETFS_BLK || ftype == RUMP_ETFS_CHR) {
		error = rumpblk_register(hostpath, &dmin, begin, size);
		if (error != 0) {
			return error;
		}
		rdev = makedev(RUMPBLK, dmin);
	}

	et = kmem_alloc(sizeof(*et), KM_SLEEP);
	strcpy(et->et_key, key);
	et->et_keylen = strlen(et->et_key);
	et->et_rn = rn = makeprivate(ettype_to_vtype(ftype), rdev, size);
	if (ftype == RUMP_ETFS_REG) {
		size_t len = strlen(hostpath)+1;

		rn->rn_hostpath = malloc(len, M_TEMP, M_WAITOK | M_ZERO);
		memcpy(rn->rn_hostpath, hostpath, len);
		rn->rn_offset = begin;
	}

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
rump_etfs_register(const char *key, const char *hostpath,
	enum rump_etfs_type ftype)
{

	return doregister(key, hostpath, ftype, 0, RUMP_ETFS_SIZE_ENDOFF);
}

int
rump_etfs_register_withsize(const char *key, const char *hostpath,
	enum rump_etfs_type ftype, uint64_t begin, uint64_t size)
{

	/*
	 * Check that we're mapping at block offsets.  I guess this
	 * is not technically necessary except for BLK/CHR backends
	 * (i.e. what getfileinfo() returns, not ftype) and can be
	 * removed later if there are problems.
	 */
	if ((begin & (DEV_BSIZE-1)) != 0)
		return EINVAL;
	if (size != RUMP_ETFS_SIZE_ENDOFF && (size & (DEV_BSIZE-1)) != 0)
		return EINVAL;

	return doregister(key, hostpath, ftype, begin, size);
}

int
rump_etfs_remove(const char *key)
{
	struct etfs *et;
	size_t keylen = strlen(key);

	mutex_enter(&etfs_lock);
	LIST_FOREACH(et, &etfs_list, et_entries) {
		if (keylen == et->et_keylen && strcmp(et->et_key, key) == 0) {
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

static int lastino = 1;
static kmutex_t reclock;

static struct rumpfs_node *
makeprivate(enum vtype vt, dev_t rdev, off_t size)
{
	struct rumpfs_node *rn;
	struct vattr *va;
	struct timespec ts;

	rn = kmem_zalloc(sizeof(*rn), KM_SLEEP);

	switch (vt) {
	case VDIR:
		LIST_INIT(&rn->rn_dir);
		break;
	case VREG:
		rn->rn_readfd = -1;
		rn->rn_writefd = -1;
		break;
	default:
		break;
	}

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
makevnode(struct mount *mp, struct rumpfs_node *rn, struct vnode **vpp)
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
	if (vpops != rump_specop_p && va->va_type != VDIR
	    && !(va->va_type == VREG && rn->rn_hostpath != NULL)
	    && va->va_type != VSOCK)
		return EOPNOTSUPP;

	rv = getnewvnode(VT_RUMP, mp, vpops, &vp);
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

	/* check for dot, return directly if the case */
	if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		vref(dvp);
		*vpp = dvp;
		goto out;
	}

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
		rv = makevnode(dvp->v_mount, rn, vpp);
		rn->rn_vp = *vpp;
		mutex_exit(&reclock);
		if (rv)
			return rv;
	}

 out:
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
	rv = makevnode(dvp->v_mount, rn, vpp);
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
	rv = makevnode(dvp->v_mount, rn, vpp);
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
rump_vop_create(void *v)
{
	struct vop_create_args /* {
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

	if (va->va_type != VSOCK) {
		rv = EOPNOTSUPP;
		goto out;
	}
	rn = makeprivate(VSOCK, NODEV, DEV_BSIZE);
	mutex_enter(&reclock);
	rv = makevnode(dvp->v_mount, rn, vpp);
	mutex_exit(&reclock);
	if (rv)
		goto out;

	rdent = kmem_alloc(sizeof(*rdent), KM_SLEEP);
	rdent->rd_name = kmem_alloc(cnp->cn_namelen+1, KM_SLEEP);
	rdent->rd_node = (*vpp)->v_data;
	rdent->rd_node->rn_va.va_rdev = NODEV;
	strlcpy(rdent->rd_name, cnp->cn_nameptr, cnp->cn_namelen+1);

	LIST_INSERT_HEAD(&rnd->rn_dir, rdent, rd_entries);

 out:
	vput(dvp);
	return rv;
}

static int
rump_vop_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	int mode = ap->a_mode;
	int error = EINVAL;

	if (vp->v_type != VREG)
		return 0;

	if (mode & FREAD) {
		if (rn->rn_readfd != -1)
			return 0;
		rn->rn_readfd = rumpuser_open(rn->rn_hostpath,
		    O_RDONLY, &error);
	} else if (mode & FWRITE) {
		if (rn->rn_writefd != -1)
			return 0;
		rn->rn_writefd = rumpuser_open(rn->rn_hostpath,
		    O_WRONLY, &error);
	}

	return error;
}

static int
rump_vop_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int ioflags a_ioflag;
		kauth_cred_t a_cred;
	}; */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	struct uio *uio = ap->a_uio;
	uint8_t *buf;
	size_t bufsize;
	int error = 0;

	bufsize = uio->uio_resid;
	buf = kmem_alloc(bufsize, KM_SLEEP);
	if (rumpuser_pread(rn->rn_readfd, buf, bufsize,
	    uio->uio_offset + rn->rn_offset, &error) == -1)
		goto out;
	error = uiomove(buf, bufsize, uio);

 out:
	kmem_free(buf, bufsize);
	return error;
}

static int
rump_vop_write(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int ioflags a_ioflag;
		kauth_cred_t a_cred;
	}; */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	struct uio *uio = ap->a_uio;
	uint8_t *buf;
	size_t bufsize;
	int error = 0;

	bufsize = uio->uio_resid;
	buf = kmem_alloc(bufsize, KM_SLEEP);
	error = uiomove(buf, bufsize, uio);
	if (error)
		goto out;
	KASSERT(uio->uio_resid == 0);
	rumpuser_pwrite(rn->rn_writefd, buf, bufsize,
	    uio->uio_offset + rn->rn_offset, &error);

 out:
	kmem_free(buf, bufsize);
	return error;
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
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	int error;

	if (vp->v_type == VREG) {
		if (rn->rn_readfd != -1) {
			rumpuser_close(rn->rn_readfd, &error);
			rn->rn_readfd = -1;
		}
		if (rn->rn_writefd != -1) {
			rumpuser_close(rn->rn_writefd, &error);
			rn->rn_writefd = -1;
		}
	}
		
	VOP_UNLOCK(vp, 0);
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

/*
 * Begin vfs-level stuff
 */

VFS_PROTOS(rumpfs);
struct vfsops rumpfs_vfsops = {
	.vfs_name =		MOUNT_RUMPFS,
	.vfs_min_mount_data = 	0,
	.vfs_mount =		rumpfs_mount,
	.vfs_start =		(void *)nullop,
	.vfs_unmount = 		rumpfs_unmount,
	.vfs_root =		rumpfs_root,
	.vfs_quotactl =		(void *)eopnotsupp,
	.vfs_statvfs =		genfs_statvfs,
	.vfs_sync =		(void *)nullop,
	.vfs_vget =		rumpfs_vget,
	.vfs_fhtovp =		(void *)eopnotsupp,
	.vfs_vptofh =		(void *)eopnotsupp,
	.vfs_init =		rumpfs_init,
	.vfs_reinit =		NULL,
	.vfs_done =		rumpfs_done,
	.vfs_mountroot =	rumpfs_mountroot,
	.vfs_snapshot =		(void *)eopnotsupp,
	.vfs_extattrctl =	(void *)eopnotsupp,
	.vfs_suspendctl =	(void *)eopnotsupp,
	.vfs_opv_descs =	rump_opv_descs,
	/* vfs_refcount */
	/* vfs_list */
};

int
rumpfs_mount(struct mount *mp, const char *mntpath, void *arg, size_t *alen)
{

	return EOPNOTSUPP;
}

int
rumpfs_unmount(struct mount *mp, int flags)
{

	return EOPNOTSUPP; /* ;) */
}

int
rumpfs_root(struct mount *mp, struct vnode **vpp)
{
	struct rumpfs_mount *rfsmp = mp->mnt_data;

	vget(rfsmp->rfsmp_rvp, LK_EXCLUSIVE | LK_RETRY);
	*vpp = rfsmp->rfsmp_rvp;
	return 0;
}

int
rumpfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{

	return EOPNOTSUPP;
}

void
rumpfs_init()
{

	CTASSERT(RUMP_ETFS_SIZE_ENDOFF == RUMPBLK_SIZENOTSET);

	mutex_init(&reclock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&etfs_lock, MUTEX_DEFAULT, IPL_NONE);
}

void
rumpfs_done()
{

	mutex_destroy(&reclock);
	mutex_destroy(&etfs_lock);
}

int
rumpfs_mountroot()
{
	struct mount *mp;
	struct rumpfs_mount *rfsmp;
	struct rumpfs_node *rn;
	int error;

	if ((error = vfs_rootmountalloc(MOUNT_RUMPFS, "rootdev", &mp)) != 0) {
		vrele(rootvp);
		return error;
	}

	rfsmp = kmem_alloc(sizeof(*rfsmp), KM_SLEEP);

	rn = makeprivate(VDIR, NODEV, DEV_BSIZE);
	mutex_enter(&reclock);
	error = makevnode(mp, rn, &rfsmp->rfsmp_rvp);
	mutex_exit(&reclock);
	if (error)
		panic("could not create root vnode: %d", error);
	rfsmp->rfsmp_rvp->v_vflag |= VV_ROOT;
	VOP_UNLOCK(rfsmp->rfsmp_rvp, 0);

	mutex_enter(&mountlist_lock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);

	mp->mnt_data = rfsmp;
	mp->mnt_stat.f_namemax = MAXNAMLEN;
	mp->mnt_flag |= MNT_LOCAL;
	vfs_getnewfsid(mp);

	error = set_statvfs_info("/", UIO_SYSSPACE, "rumpfs", UIO_SYSSPACE,
	    mp->mnt_op->vfs_name, mp, curlwp);
	if (error)
		panic("set statvfsinfo for rootfs failed");

	vfs_unbusy(mp, false, NULL);

	return 0;
}
