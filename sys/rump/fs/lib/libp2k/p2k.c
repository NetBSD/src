/*	$NetBSD: p2k.c,v 1.19.4.7 2008/02/04 09:24:49 yamt Exp $	*/

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

/*
 * puffs 2k, i.e. puffs 2 kernel.  Converts the puffs protocol to
 * the kernel vfs protocol and vice versa.
 *
 * A word about reference counting: puffs in the kernel is the king of
 * reference counting.  We must maintain a vnode alive and kicking
 * until the kernel tells us to reclaim it.  Therefore we make sure
 * we never accidentally lose a vnode.  Before calling operations which
 * decrease the refcount we always bump the refcount up to compensate.
 * Come inactive, if the file system thinks that the vnode should be
 * put out of its misery, it will set the recycle flag.  We use this
 * to tell the kernel to reclaim the vnode.  Only in reclaim do we
 * really nuke the last reference.
 */

#define __VFSOPS_EXPOSE

#include <sys/mount.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/namei.h>
#include <sys/dirent.h>

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdlib.h>

#include "rump.h"
#include "p2k.h"
#include "ukfs.h"

PUFFSOP_PROTOS(p2k)

static kauth_cred_t
cred_create(const struct puffs_cred *pcr)
{
	gid_t groups[NGROUPS];
	uid_t uid;
	gid_t gid;
	short ngroups = 0;

	if (puffs_cred_getuid(pcr, &uid) == -1)
		uid = 0;
	if (puffs_cred_getgid(pcr, &gid) == -1)
		gid = 0;
	puffs_cred_getgroups(pcr, groups, &ngroups);

	return rump_cred_create(uid, gid, ngroups, groups);
}

static __inline void
cred_destroy(kauth_cred_t cred)
{

	rump_cred_destroy(cred);
}

static struct componentname *
makecn(const struct puffs_cn *pcn)
{
	kauth_cred_t cred;

	cred = cred_create(pcn->pcn_cred);
	return rump_makecn(pcn->pcn_nameiop, pcn->pcn_flags, pcn->pcn_name,
	    pcn->pcn_namelen, cred, curlwp);
}

static __inline void
freecn(struct componentname *cnp, int flags)
{

	rump_freecn(cnp, flags | RUMPCN_FREECRED);
}

static void
makelwp(struct puffs_usermount *pu)
{
	pid_t pid;
	lwpid_t lid;

	puffs_cc_getcaller(puffs_cc_getcc(pu), &pid, &lid);
	rump_setup_curlwp(pid, lid, 1);
}

static void
clearlwp(struct puffs_usermount *pu)
{

	rump_clear_curlwp();
}

int
p2k_run_fs(const char *vfsname, const char *devpath, const char *mountpath,
	int mntflags, void *arg, size_t alen, uint32_t puffs_flags)
{
	char typebuf[PUFFS_TYPELEN];
	struct puffs_ops *pops;
	struct puffs_usermount *pu;
	struct puffs_node *pn_root;
	struct vnode *rvp;
	struct ukfs *ukfs;
	extern int puffs_fakecc;
	int rv, sverrno;

	rv = -1;
	ukfs_init();
	ukfs = ukfs_mount(vfsname, devpath, mountpath, mntflags, arg, alen);
	if (ukfs == NULL)
		return -1;

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, p2k, fs, statvfs);
	PUFFSOP_SET(pops, p2k, fs, unmount);
	PUFFSOP_SET(pops, p2k, fs, sync);
	PUFFSOP_SET(pops, p2k, fs, fhtonode);
	PUFFSOP_SET(pops, p2k, fs, nodetofh);

	PUFFSOP_SET(pops, p2k, node, lookup);
	PUFFSOP_SET(pops, p2k, node, create);
	PUFFSOP_SET(pops, p2k, node, mknod);
	PUFFSOP_SET(pops, p2k, node, open);
	PUFFSOP_SET(pops, p2k, node, close);
	PUFFSOP_SET(pops, p2k, node, access);
	PUFFSOP_SET(pops, p2k, node, getattr);
	PUFFSOP_SET(pops, p2k, node, setattr);
#if 0
	PUFFSOP_SET(pops, p2k, node, poll);
#endif
	PUFFSOP_SET(pops, p2k, node, mmap);
	PUFFSOP_SET(pops, p2k, node, fsync);
	PUFFSOP_SET(pops, p2k, node, seek);
	PUFFSOP_SET(pops, p2k, node, remove);
	PUFFSOP_SET(pops, p2k, node, link);
	PUFFSOP_SET(pops, p2k, node, rename);
	PUFFSOP_SET(pops, p2k, node, mkdir);
	PUFFSOP_SET(pops, p2k, node, rmdir);
	PUFFSOP_SET(pops, p2k, node, symlink);
	PUFFSOP_SET(pops, p2k, node, readdir);
	PUFFSOP_SET(pops, p2k, node, readlink);
	PUFFSOP_SET(pops, p2k, node, read);
	PUFFSOP_SET(pops, p2k, node, write);

	PUFFSOP_SET(pops, p2k, node, inactive);
	PUFFSOP_SET(pops, p2k, node, reclaim);

	strcpy(typebuf, "p2k|");
	if (strcmp(vfsname, "puffs") == 0) { /* XXX */
		struct puffs_kargs *args = arg;
		strlcat(typebuf, args->pa_typename, sizeof(typebuf));
	} else {
		strlcat(typebuf, vfsname, sizeof(typebuf));
	}

	pu = puffs_init(pops, devpath, typebuf, ukfs_getmp(ukfs), puffs_flags);
	if (pu == NULL)
		goto out;

	rvp = ukfs_getrvp(ukfs);
	pn_root = puffs_pn_new(pu, rvp);
	puffs_setroot(pu, pn_root);
	puffs_setfhsize(pu, 0, PUFFS_FHFLAG_PASSTHROUGH);
	puffs_setstacksize(pu, PUFFS_STACKSIZE_MIN);
	puffs_fakecc = 1;

	puffs_set_prepost(pu, makelwp, clearlwp);

	if ((rv = puffs_mount(pu, mountpath, mntflags, rvp))== -1)
		goto out;
	rv = puffs_mainloop(pu);

 out:
	sverrno = errno;
	ukfs_release(ukfs, 0);
	if (rv) {
		errno = sverrno;
		rv = -1;
	}

	return rv;
}

/* XXX: vn_lock() */
#define VLE(a) RUMP_VOP_LOCK(a, LK_EXCLUSIVE)
#define VLS(a) RUMP_VOP_LOCK(a, LK_SHARED)
#define VUL(a) RUMP_VOP_UNLOCK(a, 0);
#define AUL(a) assert(RUMP_VOP_ISLOCKED(a) == 0)

int
p2k_fs_statvfs(struct puffs_usermount *pu, struct statvfs *sbp)
{
	struct mount *mp = puffs_getspecific(pu);

	return VFS_STATVFS(mp, sbp);
}

int
p2k_fs_unmount(struct puffs_usermount *pu, int flags)
{
	struct mount *mp = puffs_getspecific(pu);
	struct puffs_node *pn_root = puffs_getroot(pu);
	struct vnode *rvp = pn_root->pn_data, *rvp2;
	int rv;

	/*
	 * We recycle the root node already here (god knows how
	 * many references it has due to lookup).  This is good
	 * because VFS_UNMOUNT would do it anyway.  But it is
	 * very bad if VFS_UNMOUNT fails for a reason or another
	 * (puffs guards against busy fs, but there might be other
	 * reasons).
	 *
	 * Theoretically we're going south, sinking fast & dying
	 * out here because the old vnode will be freed and we are
	 * unlikely to get a vnode at the same address.  But try
	 * anyway.
	 *
	 * XXX: reallyfixmesomeday.  either introduce VFS_ROOT to
	 * puffs (blah) or check the cookie in every routine
	 * against the root cookie, which might change (blah2).
	 */
	rump_vp_recycle_nokidding(rvp);
	rv = VFS_UNMOUNT(mp, flags);
	if (rv) {
		int rv2;

		rv2 = VFS_ROOT(mp, &rvp2);
		assert(rv2 == 0 && rvp == rvp2);
		VUL(rvp);
	}
	return rv;
}

int
p2k_fs_sync(struct puffs_usermount *pu, int waitfor,
	const struct puffs_cred *pcr)
{
	struct mount *mp = puffs_getspecific(pu);
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	rv = VFS_SYNC(mp, waitfor, (kauth_cred_t)cred);
	cred_destroy(cred);

	rump_bioops_sync();

	return rv;
}

int
p2k_fs_fhtonode(struct puffs_usermount *pu, void *fid, size_t fidsize,
	struct puffs_newinfo *pni)
{
	struct mount *mp = puffs_getspecific(pu);
	struct vnode *vp;
	enum vtype vtype;
	voff_t vsize;
	dev_t rdev;
	int rv;

	rv = VFS_FHTOVP(mp, fid, &vp);
	if (rv)
		return rv;

	puffs_newinfo_setcookie(pni, vp);
	rump_getvninfo(vp, &vtype, &vsize, &rdev);
	puffs_newinfo_setvtype(pni, vtype);
	puffs_newinfo_setsize(pni, vsize);
	puffs_newinfo_setrdev(pni, rdev);

	return 0;
}

int
p2k_fs_nodetofh(struct puffs_usermount *pu, void *cookie, void *fid,
	size_t *fidsize)
{
	struct vnode *vp = cookie;

	return VFS_VPTOFH(vp, fid, fidsize);
}

int
p2k_node_lookup(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn)
{
	struct componentname *cn;
	struct vnode *vp;
	enum vtype vtype;
	voff_t vsize;
	dev_t rdev;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rv = RUMP_VOP_LOOKUP(opc, &vp, cn);
	VUL(opc);
	freecn(cn, RUMPCN_ISLOOKUP);
	if (rv) {
		if (rv == EJUSTRETURN)
			rv = ENOENT;
		return rv;
	}
	VUL(vp);

	puffs_newinfo_setcookie(pni, vp);
	rump_getvninfo(vp, &vtype, &vsize, &rdev);
	puffs_newinfo_setvtype(pni, vtype);
	puffs_newinfo_setsize(pni, vsize);
	puffs_newinfo_setrdev(pni, rdev);

	return 0;
}

int
p2k_node_create(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *vap)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rump_vp_incref(opc);
	rv = RUMP_VOP_CREATE(opc, &vp, cn, __UNCONST(vap));
	AUL(opc);
	freecn(cn, 0);
	if (rv == 0) {
		VUL(vp);
		puffs_newinfo_setcookie(pni, vp);
	}

	return rv;
}

int
p2k_node_mknod(struct puffs_usermount *pu, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *vap)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rump_vp_incref(opc);
	rv = RUMP_VOP_MKNOD(opc, &vp, cn, __UNCONST(vap));
	AUL(opc);
	freecn(cn, 0);
	if (rv == 0) {
		VUL(vp);
		puffs_newinfo_setcookie(pni, vp);
	}

	return rv;
}

int
p2k_node_open(struct puffs_usermount *pu, void *opc, int mode,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_OPEN(opc, mode, cred);
	VUL(opc);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_close(struct puffs_usermount *pu, void *opc, int flags,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_CLOSE(opc, flags, cred);
	VUL(opc);
	cred_destroy(cred);

	return 0;
}

int
p2k_node_access(struct puffs_usermount *pu, void *opc, int mode,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_ACCESS(opc, mode, cred);
	VUL(opc);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_getattr(struct puffs_usermount *pu, void *opc, struct vattr *vap,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_GETATTR(opc, vap, cred);
	VUL(opc);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_setattr(struct puffs_usermount *pu, void *opc, const struct vattr *vap,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_SETATTR(opc, __UNCONST(vap), cred);
	VUL(opc);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_fsync(struct puffs_usermount *pu, void *opc,
	const struct puffs_cred *pcr, int flags, off_t offlo, off_t offhi)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_FSYNC(opc, cred, flags, offlo, offhi);
	VUL(opc);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_mmap(struct puffs_usermount *pu, void *opc, vm_prot_t flags,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	rv = RUMP_VOP_MMAP(opc, flags, cred);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_seek(struct puffs_usermount *pu, void *opc, off_t oldoff, off_t newoff,
	const struct puffs_cred *pcr)
{
	kauth_cred_t cred;
	int rv;

	cred = cred_create(pcr);
	VLE(opc);
	rv = RUMP_VOP_SEEK(opc, oldoff, newoff, cred);
	VUL(opc);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_remove(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct componentname *cn;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rump_vp_incref(opc);
	VLE(targ);
	rump_vp_incref(targ);
	rv = RUMP_VOP_REMOVE(opc, targ, cn);
	AUL(opc);
	AUL(targ);
	freecn(cn, 0);

	return rv;
}

int
p2k_node_link(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct componentname *cn;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rump_vp_incref(opc);
	rv = RUMP_VOP_LINK(opc, targ, cn);
	freecn(cn, 0);

	return rv;
}

int
p2k_node_rename(struct puffs_usermount *pu, void *src_dir, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct componentname *cn_src, *cn_targ;
	int rv;

	cn_src = makecn(pcn_src);
	cn_targ = makecn(pcn_targ);
	rump_vp_incref(src_dir);
	rump_vp_incref(src);
	VLE(targ_dir);
	rump_vp_incref(targ_dir);
	if (targ) {
		VLE(targ);
		rump_vp_incref(targ);
	}
	rv = RUMP_VOP_RENAME(src_dir, src, cn_src, targ_dir, targ, cn_targ);
	AUL(targ_dir);
	if (targ)
		AUL(targ);
	freecn(cn_src, 0);
	freecn(cn_targ, 0);

	return rv;
}

int
p2k_node_mkdir(struct puffs_usermount *pu, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *vap)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rump_vp_incref(opc);
	rv = RUMP_VOP_MKDIR(opc, &vp, cn, __UNCONST(vap));
	AUL(opc);
	freecn(cn, 0);
	if (rv == 0) {
		VUL(vp);
		puffs_newinfo_setcookie(pni, vp);
	}

	return rv;
}

int
p2k_node_rmdir(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct componentname *cn;
	int rv;

	cn = makecn(pcn);
	VLE(opc);
	rump_vp_incref(opc);
	VLE(targ);
	rump_vp_incref(targ);
	rv = RUMP_VOP_RMDIR(opc, targ, cn);
	AUL(targ);
	AUL(opc);
	freecn(cn, 0);

	return rv;
}

int
p2k_node_symlink(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn_src,
	const struct vattr *vap, const char *link_target)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = makecn(pcn_src);
	VLE(opc);
	rump_vp_incref(opc);
	rv = RUMP_VOP_SYMLINK(opc, &vp, cn,
	    __UNCONST(vap), __UNCONST(link_target));
	AUL(opc);
	freecn(cn, 0);
	if (rv == 0) {
		VUL(vp);
		puffs_newinfo_setcookie(pni, vp);
	}

	return rv;
}

int
p2k_node_readdir(struct puffs_usermount *pu, void *opc, struct dirent *dent,
	off_t *readoff, size_t *reslen, const struct puffs_cred *pcr,
	int *eofflag, off_t *cookies, size_t *ncookies)
{
	kauth_cred_t cred;
	struct uio *uio;
	off_t *vop_cookies;
	int vop_ncookies;
	int rv;

	cred = cred_create(pcr);
	uio = rump_uio_setup(dent, *reslen, *readoff, RUMPUIO_READ);
	VLS(opc);
	if (cookies) {
		rv = RUMP_VOP_READDIR(opc, uio, cred, eofflag,
		    &vop_cookies, &vop_ncookies);
		memcpy(cookies, vop_cookies, vop_ncookies * sizeof(*cookies));
		*ncookies = vop_ncookies;
		free(vop_cookies);
	} else {
		rv = RUMP_VOP_READDIR(opc, uio, cred, eofflag, NULL, NULL);
	}
	VUL(opc);
	if (rv == 0) {
		*reslen = rump_uio_getresid(uio);
		*readoff = rump_uio_getoff(uio);
	}
	rump_uio_free(uio);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_readlink(struct puffs_usermount *pu, void *opc,
	const struct puffs_cred *pcr, char *linkname, size_t *linklen)
{
	kauth_cred_t cred;
	struct uio *uio;
	int rv;

	cred = cred_create(pcr);
	uio = rump_uio_setup(linkname, *linklen, 0, RUMPUIO_READ);
	VLE(opc);
	rv = RUMP_VOP_READLINK(opc, uio, cred);
	VUL(opc);
	*linklen -= rump_uio_free(uio);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_read(struct puffs_usermount *pu, void *opc,
	uint8_t *buf, off_t offset, size_t *resid,
	const struct puffs_cred *pcr, int ioflag)
{
	kauth_cred_t cred;
	struct uio *uio;
	int rv;

	cred = cred_create(pcr);
	uio = rump_uio_setup(buf, *resid, offset, RUMPUIO_READ);
	VLS(opc);
	rv = RUMP_VOP_READ(opc, uio, ioflag, cred);
	VUL(opc);
	*resid = rump_uio_free(uio);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_write(struct puffs_usermount *pu, void *opc,
	uint8_t *buf, off_t offset, size_t *resid,
	const struct puffs_cred *pcr, int ioflag)
{
	kauth_cred_t cred;
	struct uio *uio;
	int rv;

	cred = cred_create(pcr);
	uio = rump_uio_setup(buf, *resid, offset, RUMPUIO_WRITE);
	VLE(opc);
	rv = RUMP_VOP_WRITE(opc, uio, ioflag, cred);
	VUL(opc);
	*resid = rump_uio_free(uio);
	cred_destroy(cred);

	return rv;
}

int
p2k_node_inactive(struct puffs_usermount *pu, void *opc)
{
	struct vnode *vp = opc;
	bool recycle;
	int rv;

	rump_vp_interlock(vp);
	(void) RUMP_VOP_PUTPAGES(vp, 0, 0, PGO_ALLPAGES);
	VLE(vp);
	rv = RUMP_VOP_INACTIVE(vp, &recycle);
	if (recycle)
		puffs_setback(puffs_cc_getcc(pu), PUFFS_SETBACK_NOREF_N1);

	return rv;
}

int
p2k_node_reclaim(struct puffs_usermount *pu, void *opc)
{

	rump_vp_recycle_nokidding(opc);
	return 0;
}
