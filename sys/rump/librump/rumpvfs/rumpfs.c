/*	$NetBSD: rumpfs.c,v 1.72 2010/11/11 17:33:22 pooka Exp $	*/

/*
 * Copyright (c) 2009, 2010 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: rumpfs.c,v 1.72 2010/11/11 17:33:22 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/buf.h>
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
#include <sys/unistd.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>

#include <uvm/uvm_extern.h>

#include <rump/rumpuser.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

static int rump_vop_lookup(void *);
static int rump_vop_getattr(void *);
static int rump_vop_mkdir(void *);
static int rump_vop_rmdir(void *);
static int rump_vop_remove(void *);
static int rump_vop_mknod(void *);
static int rump_vop_create(void *);
static int rump_vop_inactive(void *);
static int rump_vop_reclaim(void *);
static int rump_vop_success(void *);
static int rump_vop_readdir(void *);
static int rump_vop_spec(void *);
static int rump_vop_read(void *);
static int rump_vop_write(void *);
static int rump_vop_open(void *);
static int rump_vop_symlink(void *);
static int rump_vop_readlink(void *);
static int rump_vop_whiteout(void *);
static int rump_vop_pathconf(void *);
static int rump_vop_bmap(void *);
static int rump_vop_strategy(void *);

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
	{ &vop_rmdir_desc, rump_vop_rmdir },
	{ &vop_remove_desc, rump_vop_remove },
	{ &vop_mknod_desc, rump_vop_mknod },
	{ &vop_create_desc, rump_vop_create },
	{ &vop_symlink_desc, rump_vop_symlink },
	{ &vop_readlink_desc, rump_vop_readlink },
	{ &vop_access_desc, rump_vop_success },
	{ &vop_readdir_desc, rump_vop_readdir },
	{ &vop_read_desc, rump_vop_read },
	{ &vop_write_desc, rump_vop_write },
	{ &vop_open_desc, rump_vop_open },
	{ &vop_close_desc, genfs_nullop },
	{ &vop_seek_desc, genfs_seek },
	{ &vop_getpages_desc, genfs_getpages },
	{ &vop_putpages_desc, genfs_putpages },
	{ &vop_whiteout_desc, rump_vop_whiteout },
	{ &vop_fsync_desc, rump_vop_success },
	{ &vop_lock_desc, genfs_lock },
	{ &vop_unlock_desc, genfs_unlock },
	{ &vop_islocked_desc, genfs_islocked },
	{ &vop_inactive_desc, rump_vop_inactive },
	{ &vop_reclaim_desc, rump_vop_reclaim },
	{ &vop_link_desc, genfs_eopnotsupp },
	{ &vop_pathconf_desc, rump_vop_pathconf },
	{ &vop_bmap_desc, rump_vop_bmap },
	{ &vop_strategy_desc, rump_vop_strategy },
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

#define RUMPFS_WHITEOUT NULL
#define RDENT_ISWHITEOUT(rdp) (rdp->rd_node == RUMPFS_WHITEOUT)
struct rumpfs_dent {
	char *rd_name;
	int rd_namelen;
	struct rumpfs_node *rd_node;

	LIST_ENTRY(rumpfs_dent) rd_entries;
};

struct genfs_ops rumpfs_genfsops = {
	.gop_size = genfs_size,
	.gop_write = genfs_gop_write,

	/* optional */
	.gop_alloc = NULL,
	.gop_markupdate = NULL,
};

struct rumpfs_node {
	struct genfs_node rn_gn;
	struct vattr rn_va;
	struct vnode *rn_vp;
	char *rn_hostpath;
	int rn_flags;

	union {
		struct {		/* VREG */
			int readfd;
			int writefd;
			uint64_t offset;
		} reg;
		struct {
			void *data;
			size_t dlen;
		} reg_noet;
		struct {		/* VDIR */
			LIST_HEAD(, rumpfs_dent) dents;
			struct rumpfs_node *parent;
			int flags;
		} dir;
		struct {
			char *target;
			size_t len;
		} link;
	} rn_u;
};
#define rn_readfd	rn_u.reg.readfd
#define rn_writefd	rn_u.reg.writefd
#define rn_offset	rn_u.reg.offset
#define rn_data		rn_u.reg_noet.data
#define rn_dlen		rn_u.reg_noet.dlen
#define rn_dir		rn_u.dir.dents
#define rn_parent	rn_u.dir.parent
#define rn_linktarg	rn_u.link.target
#define rn_linklen	rn_u.link.len

#define RUMPNODE_CANRECLAIM	0x01
#define RUMPNODE_DIR_ET		0x02
#define RUMPNODE_DIR_ETSUBS	0x04
#define RUMPNODE_ET_PHONE_HOST	0x10

struct rumpfs_mount {
	struct vnode *rfsmp_rvp;
};

static struct rumpfs_node *makeprivate(enum vtype, dev_t, off_t, bool);

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
	bool et_prefixkey;
	bool et_removing;
	devminor_t et_blkmin;

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
	case RUMP_ETFS_DIR:
		vt = VDIR;
		break;
	case RUMP_ETFS_DIR_SUBDIRS:
		vt = VDIR;
		break;
	default:	
		panic("invalid et type: %d", et);
	}

	return vt;
}

static enum vtype
hft_to_vtype(int hft)
{
	enum vtype vt;

	switch (hft) {
	case RUMPUSER_FT_OTHER:
		vt = VNON;
		break;
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
		vt = VNON;
		break;
	}

	return vt;
}

static bool
etfs_find(const char *key, struct etfs **etp, bool forceprefix)
{
	struct etfs *et;
	size_t keylen = strlen(key);

	KASSERT(mutex_owned(&etfs_lock));

	LIST_FOREACH(et, &etfs_list, et_entries) {
		if ((keylen == et->et_keylen || et->et_prefixkey || forceprefix)
		    && strncmp(key, et->et_key, et->et_keylen) == 0) {
			if (etp)
				*etp = et;
			return true;
		}
	}

	return false;
}

#define REGDIR(ftype) \
    ((ftype) == RUMP_ETFS_DIR || (ftype) == RUMP_ETFS_DIR_SUBDIRS)
static int
doregister(const char *key, const char *hostpath, 
	enum rump_etfs_type ftype, uint64_t begin, uint64_t size)
{
	char buf[9];
	struct etfs *et;
	struct rumpfs_node *rn;
	uint64_t fsize;
	dev_t rdev = NODEV;
	devminor_t dmin = -1;
	int hft, error;

	if (rumpuser_getfileinfo(hostpath, &fsize, &hft, &error))
		return error;

	/* etfs directory requires a directory on the host */
	if (REGDIR(ftype)) {
		if (hft != RUMPUSER_FT_DIR)
			return ENOTDIR;
		if (begin != 0)
			return EISDIR;
		if (size != RUMP_ETFS_SIZE_ENDOFF)
			return EISDIR;
		size = fsize;
	} else {
		if (begin > fsize)
			return EINVAL;
		if (size == RUMP_ETFS_SIZE_ENDOFF)
			size = fsize - begin;
		if (begin + size > fsize)
			return EINVAL;
	}

	if (ftype == RUMP_ETFS_BLK || ftype == RUMP_ETFS_CHR) {
		error = rumpblk_register(hostpath, &dmin, begin, size);
		if (error != 0) {
			return error;
		}
		rdev = makedev(RUMPBLK_DEVMAJOR, dmin);
	}

	et = kmem_alloc(sizeof(*et), KM_SLEEP);
	strcpy(et->et_key, key);
	et->et_keylen = strlen(et->et_key);
	et->et_rn = rn = makeprivate(ettype_to_vtype(ftype), rdev, size, true);
	et->et_removing = false;
	et->et_blkmin = dmin;

	rn->rn_flags |= RUMPNODE_ET_PHONE_HOST;

	if (ftype == RUMP_ETFS_REG || REGDIR(ftype) || et->et_blkmin != -1) {
		size_t len = strlen(hostpath)+1;

		rn->rn_hostpath = malloc(len, M_TEMP, M_WAITOK | M_ZERO);
		memcpy(rn->rn_hostpath, hostpath, len);
		rn->rn_offset = begin;
	}

	if (REGDIR(ftype)) {
		rn->rn_flags |= RUMPNODE_DIR_ET;
		et->et_prefixkey = true;
	} else {
		et->et_prefixkey = false;
	}

	if (ftype == RUMP_ETFS_DIR_SUBDIRS)
		rn->rn_flags |= RUMPNODE_DIR_ETSUBS;

	mutex_enter(&etfs_lock);
	if (etfs_find(key, NULL, REGDIR(ftype))) {
		mutex_exit(&etfs_lock);
		if (et->et_blkmin != -1)
			rumpblk_deregister(hostpath);
		if (et->et_rn->rn_hostpath != NULL)
			free(et->et_rn->rn_hostpath, M_TEMP);
		kmem_free(et->et_rn, sizeof(*et->et_rn));
		kmem_free(et, sizeof(*et));
		return EEXIST;
	}
	LIST_INSERT_HEAD(&etfs_list, et, et_entries);
	mutex_exit(&etfs_lock);

	if (ftype == RUMP_ETFS_BLK) {
		format_bytes(buf, sizeof(buf), size);
		aprint_verbose("%s: hostpath %s (%s)\n", key, hostpath, buf);
	}

	return 0;
}
#undef REGDIR

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

	return doregister(key, hostpath, ftype, begin, size);
}

/* remove etfs mapping.  caller's responsibility to make sure it's not in use */
int
rump_etfs_remove(const char *key)
{
	struct etfs *et;
	size_t keylen = strlen(key);
	int rv;

	mutex_enter(&etfs_lock);
	LIST_FOREACH(et, &etfs_list, et_entries) {
		if (keylen == et->et_keylen && strcmp(et->et_key, key) == 0) {
			if (et->et_removing)
				et = NULL;
			else
				et->et_removing = true;
			break;
		}
	}
	mutex_exit(&etfs_lock);
	if (!et)
		return ENOENT;

	/*
	 * ok, we know what we want to remove and have signalled there
	 * actually are men at work.  first, unregister from rumpblk
	 */
	if (et->et_blkmin != -1) {
		rv = rumpblk_deregister(et->et_rn->rn_hostpath);
	} else {
		rv = 0;
	}
	KASSERT(rv == 0);

	/* then do the actual removal */
	mutex_enter(&etfs_lock);
	LIST_REMOVE(et, et_entries);
	mutex_exit(&etfs_lock);

	/* node is unreachable, safe to nuke all device copies */
	if (et->et_blkmin != -1)
		vdevgone(RUMPBLK_DEVMAJOR, et->et_blkmin, et->et_blkmin, VBLK);

	if (et->et_rn->rn_hostpath != NULL)
		free(et->et_rn->rn_hostpath, M_TEMP);
	kmem_free(et->et_rn, sizeof(*et->et_rn));
	kmem_free(et, sizeof(*et));

	return 0;
}

/*
 * rumpfs
 */

#define INO_WHITEOUT 1
static int lastino = 2;
static kmutex_t reclock;

static struct rumpfs_node *
makeprivate(enum vtype vt, dev_t rdev, off_t size, bool et)
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
		if (et) {
			rn->rn_readfd = -1;
			rn->rn_writefd = -1;
		}
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

	KASSERT(!mutex_owned(&reclock));

	if (va->va_type == VCHR || va->va_type == VBLK) {
		vpops = rump_specop_p;
	} else {
		vpops = rump_vnodeop_p;
	}

	rv = getnewvnode(VT_RUMP, mp, vpops, &vp);
	if (rv)
		return rv;

	vp->v_size = vp->v_writesize = va->va_size;
	vp->v_type = va->va_type;

	if (vpops == rump_specop_p) {
		spec_node_init(vp, va->va_rdev);
	}
	vp->v_data = rn;

	genfs_node_init(vp, &rumpfs_genfsops);
	vn_lock(vp, LK_RETRY | LK_EXCLUSIVE);
	mutex_enter(&reclock);
	rn->rn_vp = vp;
	mutex_exit(&reclock);

	*vpp = vp;

	return 0;
}


static void
makedir(struct rumpfs_node *rnd,
	struct componentname *cnp, struct rumpfs_node *rn)
{
	struct rumpfs_dent *rdent;

	rdent = kmem_alloc(sizeof(*rdent), KM_SLEEP);
	rdent->rd_name = kmem_alloc(cnp->cn_namelen+1, KM_SLEEP);
	rdent->rd_node = rn;
	strlcpy(rdent->rd_name, cnp->cn_nameptr, cnp->cn_namelen+1);
	rdent->rd_namelen = strlen(rdent->rd_name);

	LIST_INSERT_HEAD(&rnd->rn_dir, rdent, rd_entries);
}

static void
freedir(struct rumpfs_node *rnd, struct componentname *cnp)
{
	struct rumpfs_dent *rd = NULL;

	LIST_FOREACH(rd, &rnd->rn_dir, rd_entries) {
		if (rd->rd_namelen == cnp->cn_namelen &&
		    strncmp(rd->rd_name, cnp->cn_nameptr,
		            cnp->cn_namelen) == 0)
			break;
	}
	if (rd == NULL)
		panic("could not find directory entry: %s", cnp->cn_nameptr);

	LIST_REMOVE(rd, rd_entries);
	kmem_free(rd->rd_name, rd->rd_namelen+1);
	kmem_free(rd, sizeof(*rd));
}

/*
 * Simple lookup for rump file systems.
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
	struct etfs *et;
	bool dotdot = (cnp->cn_flags & ISDOTDOT) != 0;
	int rv = 0;

	/* check for dot, return directly if the case */
	if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		vref(dvp);
		*vpp = dvp;
		return 0;
	}

	/* we don't do rename */
	if (!(((cnp->cn_flags & ISLASTCN) == 0) || (cnp->cn_nameiop != RENAME)))
		return EOPNOTSUPP;

	/* check for etfs */
	if (dvp == rootvnode && cnp->cn_nameiop == LOOKUP) {
		bool found;
		mutex_enter(&etfs_lock);
		found = etfs_find(cnp->cn_pnbuf, &et, false);
		mutex_exit(&etfs_lock);

		if (found) {
			char *offset;

			offset = strstr(cnp->cn_pnbuf, et->et_key);
			KASSERT(offset);

			rn = et->et_rn;
			cnp->cn_consume += et->et_keylen
			    - (cnp->cn_nameptr - offset) - cnp->cn_namelen;
			if (rn->rn_va.va_type != VDIR)
				cnp->cn_flags &= ~REQUIREDIR;
			goto getvnode;
		}
	}

	if (rnd->rn_flags & RUMPNODE_DIR_ET) {
		uint64_t fsize;
		char *newpath;
		size_t newpathlen;
		int hft, error;

		if (dotdot)
			return EOPNOTSUPP;

		newpathlen = strlen(rnd->rn_hostpath) + 1 + cnp->cn_namelen + 1;
		newpath = malloc(newpathlen, M_TEMP, M_WAITOK);

		strlcpy(newpath, rnd->rn_hostpath, newpathlen);
		strlcat(newpath, "/", newpathlen);
		strlcat(newpath, cnp->cn_nameptr, newpathlen);

		if (rumpuser_getfileinfo(newpath, &fsize, &hft, &error)) {
			free(newpath, M_TEMP);
			return error;
		}

		/* allow only dirs and regular files */
		if (hft != RUMPUSER_FT_REG && hft != RUMPUSER_FT_DIR) {
			free(newpath, M_TEMP);
			return ENOENT;
		}

		rn = makeprivate(hft_to_vtype(hft), NODEV, fsize, true);
		rn->rn_flags |= RUMPNODE_CANRECLAIM;
		if (rnd->rn_flags & RUMPNODE_DIR_ETSUBS) {
			rn->rn_flags |= RUMPNODE_DIR_ET | RUMPNODE_DIR_ETSUBS;
			rn->rn_flags |= RUMPNODE_ET_PHONE_HOST;
		}
		rn->rn_hostpath = newpath;

		goto getvnode;
	} else {
		if (dotdot) {
			rn = rnd->rn_parent;
			goto getvnode;
		} else {
			LIST_FOREACH(rd, &rnd->rn_dir, rd_entries) {
				if (rd->rd_namelen == cnp->cn_namelen &&
				    strncmp(rd->rd_name, cnp->cn_nameptr,
				      cnp->cn_namelen) == 0)
					break;
			}
		}
	}

	if (!rd && ((cnp->cn_flags & ISLASTCN) == 0||cnp->cn_nameiop != CREATE))
		return ENOENT;

	if (!rd && (cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop == CREATE) {
		cnp->cn_flags |= SAVENAME;
		return EJUSTRETURN;
	}
	if ((cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop == DELETE)
		cnp->cn_flags |= SAVENAME;

	rn = rd->rd_node;

 getvnode:
	KASSERT(rn);
	if (dotdot)
		VOP_UNLOCK(dvp);
	mutex_enter(&reclock);
	if ((vp = rn->rn_vp)) {
		mutex_enter(&vp->v_interlock);
		mutex_exit(&reclock);
		if (vget(vp, LK_EXCLUSIVE)) {
			vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
			goto getvnode;
		}
		*vpp = vp;
	} else {
		mutex_exit(&reclock);
		rv = makevnode(dvp->v_mount, rn, vpp);
	}
	if (dotdot)
		vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);

	return rv;
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
	int rv = 0;

	rn = makeprivate(VDIR, NODEV, DEV_BSIZE, false);
	rn->rn_parent = rnd;
	rv = makevnode(dvp->v_mount, rn, vpp);
	if (rv)
		goto out;

	makedir(rnd, cnp, rn);

 out:
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	return rv;
}

static int
rump_vop_rmdir(void *v)
{
        struct vop_rmdir_args /* {
                struct vnode *a_dvp;
                struct vnode *a_vp;
                struct componentname *a_cnp;
        }; */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct componentname *cnp = ap->a_cnp;
	struct rumpfs_node *rnd = dvp->v_data;
	struct rumpfs_node *rn = vp->v_data;
	int rv = 0;

	if (!LIST_EMPTY(&rn->rn_dir)) {
		rv = ENOTEMPTY;
		goto out;
	}

	freedir(rnd, cnp);
	rn->rn_flags |= RUMPNODE_CANRECLAIM;

out:
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	vput(vp);

	return rv;
}

static int
rump_vop_remove(void *v)
{
        struct vop_rmdir_args /* {
                struct vnode *a_dvp;
                struct vnode *a_vp;
                struct componentname *a_cnp;
        }; */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct componentname *cnp = ap->a_cnp;
	struct rumpfs_node *rnd = dvp->v_data;
	struct rumpfs_node *rn = vp->v_data;
	int rv = 0;

	if (rn->rn_flags & RUMPNODE_ET_PHONE_HOST)
		return EOPNOTSUPP;

	if (vp->v_type == VREG) {
		rump_hyperfree(rn->rn_data, rn->rn_dlen);
	}

	freedir(rnd, cnp);
	rn->rn_flags |= RUMPNODE_CANRECLAIM;

	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	vput(vp);

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
	int rv;

	rn = makeprivate(va->va_type, va->va_rdev, DEV_BSIZE, false);
	rv = makevnode(dvp->v_mount, rn, vpp);
	if (rv)
		goto out;

	makedir(rnd, cnp, rn);

 out:
	PNBUF_PUT(cnp->cn_pnbuf);
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
	off_t newsize;
	int rv;

	newsize = va->va_type == VSOCK ? DEV_BSIZE : 0;
	rn = makeprivate(va->va_type, NODEV, newsize, false);
	rv = makevnode(dvp->v_mount, rn, vpp);
	if (rv)
		goto out;

	makedir(rnd, cnp, rn);

 out:
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	return rv;
}

static int
rump_vop_symlink(void *v)
{
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	}; */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct rumpfs_node *rnd = dvp->v_data, *rn;
	const char *target = ap->a_target;
	size_t linklen;
	int rv;

	linklen = strlen(target);
	KASSERT(linklen < MAXPATHLEN);
	rn = makeprivate(VLNK, NODEV, linklen, false);
	rv = makevnode(dvp->v_mount, rn, vpp);
	if (rv)
		goto out;

	makedir(rnd, cnp, rn);

	KASSERT(linklen < MAXPATHLEN);
	rn->rn_linktarg = PNBUF_GET();
	rn->rn_linklen = linklen;
	strcpy(rn->rn_linktarg, target);

 out:
	vput(dvp);
	return rv;
}

static int
rump_vop_readlink(void *v)
{
	struct vop_readlink_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
	}; */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	struct uio *uio = ap->a_uio;

	return uiomove(rn->rn_linktarg, rn->rn_linklen, uio);
}

static int
rump_vop_whiteout(void *v)
{
	struct vop_whiteout_args /* {
		struct vnode            *a_dvp;
		struct componentname    *a_cnp;
		int                     a_flags;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct rumpfs_node *rnd = dvp->v_data;
	struct componentname *cnp = ap->a_cnp;
	int flags = ap->a_flags;

	switch (flags) {
	case LOOKUP:
		break;
	case CREATE:
		makedir(rnd, cnp, RUMPFS_WHITEOUT);
		break;
	case DELETE:
		cnp->cn_flags &= ~DOWHITEOUT; /* cargo culting never fails ? */
		freedir(rnd, cnp);
		break;
	default:
		panic("unknown whiteout op %d", flags);
	}

	return 0;
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

	if (vp->v_type != VREG || (rn->rn_flags & RUMPNODE_ET_PHONE_HOST) == 0)
		return 0;

	if (mode & FREAD) {
		if (rn->rn_readfd != -1)
			return 0;
		rn->rn_readfd = rumpuser_open(rn->rn_hostpath,
		    O_RDONLY, &error);
	}

	if (mode & FWRITE) {
		if (rn->rn_writefd != -1)
			return 0;
		rn->rn_writefd = rumpuser_open(rn->rn_hostpath,
		    O_WRONLY, &error);
	}

	return error;
}

/* simple readdir.  event omits dotstuff and periods */
static int
rump_vop_readdir(void *v)
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
	struct uio *uio = ap->a_uio;
	struct rumpfs_node *rnd = vp->v_data;
	struct rumpfs_dent *rdent;
	unsigned i;
	int rv = 0;

	/* seek to current entry */
	for (i = 0, rdent = LIST_FIRST(&rnd->rn_dir);
	    (i < uio->uio_offset) && rdent;
	    i++, rdent = LIST_NEXT(rdent, rd_entries))
		continue;
	if (!rdent)
		goto out;

	/* copy entries */
	for (; rdent && uio->uio_resid > 0;
	    rdent = LIST_NEXT(rdent, rd_entries), i++) {
		struct dirent dent;

		strlcpy(dent.d_name, rdent->rd_name, sizeof(dent.d_name));
		dent.d_namlen = strlen(dent.d_name);
		dent.d_reclen = _DIRENT_RECLEN(&dent, dent.d_namlen);

		if (__predict_false(RDENT_ISWHITEOUT(rdent))) {
			dent.d_fileno = INO_WHITEOUT;
			dent.d_type = DT_WHT;
		} else {
			dent.d_fileno = rdent->rd_node->rn_va.va_fileid;
			dent.d_type = vtype2dt(rdent->rd_node->rn_va.va_type);
		}

		if (uio->uio_resid < dent.d_reclen) {
			i--;
			break;
		}

		rv = uiomove(&dent, dent.d_reclen, uio); 
		if (rv) {
			i--;
			break;
		}
	}

 out:
	if (ap->a_cookies) {
		*ap->a_ncookies = 0;
		*ap->a_cookies = NULL;
	}
	if (rdent)
		*ap->a_eofflag = 0;
	else
		*ap->a_eofflag = 1;
	uio->uio_offset = i;

	return rv;
}

static int
etread(struct rumpfs_node *rn, struct uio *uio)
{
	uint8_t *buf;
	size_t bufsize;
	ssize_t n;
	int error = 0;

	bufsize = uio->uio_resid;
	buf = kmem_alloc(bufsize, KM_SLEEP);
	if ((n = rumpuser_pread(rn->rn_readfd, buf, bufsize,
	    uio->uio_offset + rn->rn_offset, &error)) == -1)
		goto out;
	KASSERT(n <= bufsize);
	error = uiomove(buf, n, uio);

 out:
	kmem_free(buf, bufsize);
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
	const int advice = IO_ADV_DECODE(ap->a_ioflag);
	off_t chunk;
	int error;

	/* et op? */
	if (rn->rn_flags & RUMPNODE_ET_PHONE_HOST)
		return etread(rn, uio);

	/* otherwise, it's off to ubc with us */
	while (uio->uio_resid > 0) {
		chunk = MIN(uio->uio_resid, (off_t)rn->rn_dlen-uio->uio_offset);
		if (chunk == 0)
			break;
		error = ubc_uiomove(&vp->v_uobj, uio, chunk, advice,
		    UBC_READ | UBC_PARTIALOK | UBC_WANT_UNMAP(vp)?UBC_UNMAP:0);
		if (error)
			break;
	}

	return error;
}

static int
etwrite(struct rumpfs_node *rn, struct uio *uio)
{
	uint8_t *buf;
	size_t bufsize;
	ssize_t n;
	int error = 0;

	bufsize = uio->uio_resid;
	buf = kmem_alloc(bufsize, KM_SLEEP);
	error = uiomove(buf, bufsize, uio);
	if (error)
		goto out;
	KASSERT(uio->uio_resid == 0);
	n = rumpuser_pwrite(rn->rn_writefd, buf, bufsize,
	    (uio->uio_offset-bufsize) + rn->rn_offset, &error);
	if (n >= 0) {
		KASSERT(n <= bufsize);
		uio->uio_resid = bufsize - n;
	}

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
	const int advice = IO_ADV_DECODE(ap->a_ioflag);
	void *olddata;
	size_t oldlen, newlen;
	off_t chunk;
	int error;
	bool allocd = false;

	/* consult et? */
	if (rn->rn_flags & RUMPNODE_ET_PHONE_HOST)
		return etwrite(rn, uio);

	/*
	 * Otherwise, it's a case of ubcmove.
	 */

	/*
	 * First, make sure we have enough storage.
	 *
	 * No, you don't need to tell me it's not very efficient.
	 * No, it doesn't really support sparse files, just fakes it.
	 */
	newlen = uio->uio_offset + uio->uio_resid;
	if (rn->rn_dlen < newlen) {
		oldlen = rn->rn_dlen;
		olddata = rn->rn_data;

		rn->rn_data = rump_hypermalloc(newlen, 0, true, "rumpfs");
		rn->rn_dlen = newlen;
		memset(rn->rn_data, 0, newlen);
		memcpy(rn->rn_data, olddata, oldlen);
		allocd = true;
		uvm_vnp_setsize(vp, newlen);
	}

	/* ok, we have enough stooorage.  write */
	while (uio->uio_resid > 0) {
		chunk = MIN(uio->uio_resid, (off_t)rn->rn_dlen-uio->uio_offset);
		if (chunk == 0)
			break;
		error = ubc_uiomove(&vp->v_uobj, uio, chunk, advice,
		    UBC_WRITE | UBC_PARTIALOK | UBC_WANT_UNMAP(vp)?UBC_UNMAP:0);
		if (error)
			break;
	}

	if (allocd) {
		if (error) {
			rump_hyperfree(rn->rn_data, newlen);
			rn->rn_data = olddata;
			rn->rn_dlen = oldlen;
			uvm_vnp_setsize(vp, oldlen);
		} else {
			rump_hyperfree(olddata, oldlen);
		}
	}

	return error;
}

static int
rump_vop_bmap(void *v)
{
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;

	/* 1:1 mapping */
	if (ap->a_vpp)
		*ap->a_vpp = ap->a_vp;
	if (ap->a_bnp)
		*ap->a_bnp = ap->a_bn;
	if (ap->a_runp)
		*ap->a_runp = 16;

	return 0;
}

static int
rump_vop_strategy(void *v)
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	struct buf *bp = ap->a_bp;
	off_t copylen, copyoff;
	int error;

	if (vp->v_type != VREG || rn->rn_flags & RUMPNODE_ET_PHONE_HOST) {
		error = EINVAL;
		goto out;
	}

	copyoff = bp->b_blkno << DEV_BSHIFT;
	copylen = MIN(rn->rn_dlen - copyoff, bp->b_bcount);
	if (BUF_ISWRITE(bp)) {
		memcpy((uint8_t *)rn->rn_data + copyoff, bp->b_data, copylen);
	} else {
		memset((uint8_t*)bp->b_data + copylen, 0, bp->b_bcount-copylen);
		memcpy(bp->b_data, (uint8_t *)rn->rn_data + copyoff, copylen);
	}
	bp->b_resid = 0;
	error = 0;

 out:
	bp->b_error = error;
	biodone(bp);
	return 0;
}

static int
rump_vop_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	}; */ *ap = v;
	int name = ap->a_name;
	register_t *retval = ap->a_retval;

	switch (name) {
	case _PC_LINK_MAX:
		*retval = LINK_MAX;
		return 0;
	case _PC_NAME_MAX:
		*retval = NAME_MAX;
		return 0;
	case _PC_PATH_MAX:
		*retval = PATH_MAX;
		return 0;
	case _PC_PIPE_BUF:
		*retval = PIPE_BUF;
		return 0;
	case _PC_CHOWN_RESTRICTED:
		*retval = 1;
		return 0;
	case _PC_NO_TRUNC:
		*retval = 1;
		return 0;
	case _PC_SYNC_IO:
		*retval = 1;
		return 0;
	case _PC_FILESIZEBITS:
		*retval = 43; /* this one goes to 11 */
		return 0;
	case _PC_SYMLINK_MAX:
		*retval = MAXPATHLEN;
		return 0;
	case _PC_2_SYMLINKS:
		*retval = 1;
		return 0;
	default:
		return EINVAL;
	}
}

static int
rump_vop_success(void *v)
{

	return 0;
}

static int
rump_vop_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		bool *a_recycle;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct rumpfs_node *rn = vp->v_data;
	int error;

	if (rn->rn_flags & RUMPNODE_ET_PHONE_HOST && vp->v_type == VREG) {
		if (rn->rn_readfd != -1) {
			rumpuser_close(rn->rn_readfd, &error);
			rn->rn_readfd = -1;
		}
		if (rn->rn_writefd != -1) {
			rumpuser_close(rn->rn_writefd, &error);
			rn->rn_writefd = -1;
		}
	}
	*ap->a_recycle = (rn->rn_flags & RUMPNODE_CANRECLAIM) ? true : false;

	VOP_UNLOCK(vp);
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
	genfs_node_destroy(vp);
	vp->v_data = NULL;

	if (rn->rn_flags & RUMPNODE_CANRECLAIM) {
		if (vp->v_type == VLNK)
			PNBUF_PUT(rn->rn_linktarg);
		if (rn->rn_hostpath)
			free(rn->rn_hostpath, M_TEMP);
		kmem_free(rn, sizeof(*rn));
	}

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
	case VOP_RECLAIM_DESCOFFSET:
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
	.vfs_renamelock_enter =	genfs_renamelock_enter,
	.vfs_renamelock_exit =	genfs_renamelock_exit,
	.vfs_opv_descs =	rump_opv_descs,
	/* vfs_refcount */
	/* vfs_list */
};

static int
rumpfs_mountfs(struct mount *mp)
{
	struct rumpfs_mount *rfsmp;
	struct rumpfs_node *rn;
	int error;

	rfsmp = kmem_alloc(sizeof(*rfsmp), KM_SLEEP);

	rn = makeprivate(VDIR, NODEV, DEV_BSIZE, false);
	rn->rn_parent = rn;
	if ((error = makevnode(mp, rn, &rfsmp->rfsmp_rvp)) != 0)
		return error;

	rfsmp->rfsmp_rvp->v_vflag |= VV_ROOT;
	VOP_UNLOCK(rfsmp->rfsmp_rvp);

	mp->mnt_data = rfsmp;
	mp->mnt_stat.f_namemax = MAXNAMLEN;
	mp->mnt_stat.f_iosize = 512;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_iflag |= IMNT_MPSAFE;
	mp->mnt_fs_bshift = DEV_BSHIFT;
	vfs_getnewfsid(mp);

	return 0;
}

int
rumpfs_mount(struct mount *mp, const char *mntpath, void *arg, size_t *alen)
{
	int error;

	error = set_statvfs_info(mntpath, UIO_USERSPACE, "rumpfs", UIO_SYSSPACE,
	    mp->mnt_op->vfs_name, mp, curlwp);
	if (error)
		return error;

	return rumpfs_mountfs(mp);
}

int
rumpfs_unmount(struct mount *mp, int mntflags)
{
	struct rumpfs_mount *rfsmp = mp->mnt_data;
	int flags = 0, error;

	if (panicstr || mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if ((error = vflush(mp, rfsmp->rfsmp_rvp, flags)) != 0)
		return error;
	vgone(rfsmp->rfsmp_rvp); /* XXX */

	kmem_free(rfsmp, sizeof(*rfsmp));

	return 0;
}

int
rumpfs_root(struct mount *mp, struct vnode **vpp)
{
	struct rumpfs_mount *rfsmp = mp->mnt_data;

	vref(rfsmp->rfsmp_rvp);
	vn_lock(rfsmp->rfsmp_rvp, LK_EXCLUSIVE | LK_RETRY);
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
	int error;

	if ((error = vfs_rootmountalloc(MOUNT_RUMPFS, "rootdev", &mp)) != 0) {
		vrele(rootvp);
		return error;
	}

	if ((error = rumpfs_mountfs(mp)) != 0)
		panic("mounting rootfs failed: %d", error);

	mutex_enter(&mountlist_lock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);

	error = set_statvfs_info("/", UIO_SYSSPACE, "rumpfs", UIO_SYSSPACE,
	    mp->mnt_op->vfs_name, mp, curlwp);
	if (error)
		panic("set_statvfs_info failed for rootfs: %d", error);

	vfs_unbusy(mp, false, NULL);

	return 0;
}
