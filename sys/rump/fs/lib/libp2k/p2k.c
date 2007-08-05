/*	$NetBSD: p2k.c,v 1.1 2007/08/05 22:28:05 pooka Exp $	*/

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

#include <sys/types.h>
#define __UIO_EXPOSE
#include <sys/uio.h>
#undef __UIO_EXPOSE
#define __VFSOPS_EXPOSE
#include <sys/mount.h>
#undef __VFSOPS_EXPOSE
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>

#include <stdbool.h>

#include <assert.h>
#include <err.h>
#define _KERNEL /* XXX */
#include <errno.h>
#undef _KERNEL
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rump.h"
#include "p2k.h"

PUFFSOP_PROTOS(p2k)

struct p2k {
	struct mount *p2k_mp;
	struct vnode *p2k_rvp;
};

struct p2k *
p2k_init(struct vfsops *vfsops, const char *mountpath, int mntflags,
	void *arg, size_t alen)
{
	struct p2k *p2k;
	struct mount *mp;
	int rv = 0;

	p2k = malloc(sizeof(struct p2k));
	if (p2k == NULL)
		return NULL;
	memset(p2k, 0, sizeof(struct p2k));

	rump_init();
	rump_mountinit(&mp, vfsops);

	mp->mnt_flag = mntflags;

	rv = VFS_MOUNT(mp, mountpath, arg, &alen, curlwp);
	if (rv) {
		warnx("VFS_MOUNT %d", rv);
		goto out;
	}
	p2k->p2k_mp = mp;

	rv = VFS_STATVFS(mp, &mp->mnt_stat, NULL);
	if (rv) {
		warnx("VFS_STATVFS %d", rv);
		goto out;
	}

	rv = VFS_ROOT(mp, &p2k->p2k_rvp);
	if (rv) {
		warnx("VFS_ROOT %d", rv);
		goto out;
	}

 out:
	if (rv) {
		if (p2k->p2k_mp)
			rump_mountdestroy(p2k->p2k_mp);
		free(p2k);
		errno = rv;
		p2k = NULL;
	}

	return p2k;
}

void
p2k_destroy(struct p2k *p2k, int dounmount)
{
	int rv;

	if (dounmount) {
		rv = VFS_SYNC(p2k->p2k_mp, MNT_WAIT, rump_cred, curlwp);
		rv += VFS_UNMOUNT(p2k->p2k_mp, 0, curlwp);
		assert(rv == 0);
	}

	rump_mountdestroy(p2k->p2k_mp);

	free(p2k);
}

int
p2k_run_fs(struct vfsops *vfsops, const char *devpath, const char *mountpath,
	int mntflags, void *arg, size_t alen, uint32_t puffs_flags)
{
	char typebuf[PUFFS_TYPELEN];
	struct p2k *p2k;
	struct puffs_ops *pops;
	struct puffs_usermount *pu;
	struct puffs_node *pn_root;
	extern int puffs_fakecc;
	int rv, sverrno;

	p2k = p2k_init(vfsops, mountpath, mntflags, arg, alen);
	if (p2k == NULL)
		return -1;

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, p2k, fs, statvfs);
	PUFFSOP_SET(pops, p2k, fs, unmount);
	PUFFSOP_SET(pops, p2k, fs, sync);

	PUFFSOP_SET(pops, p2k, node, lookup);
	PUFFSOP_SET(pops, p2k, node, create);
	PUFFSOP_SET(pops, p2k, node, mknod);
	PUFFSOP_SET(pops, p2k, node, open);
	PUFFSOP_SET(pops, p2k, node, close);
	PUFFSOP_SET(pops, p2k, node, access);
	PUFFSOP_SET(pops, p2k, node, getattr);
	PUFFSOP_SET(pops, p2k, node, setattr);
	PUFFSOP_SET(pops, p2k, node, poll);
	PUFFSOP_SET(pops, p2k, node, mmap);
	PUFFSOP_SET(pops, p2k, node, fsync);
	PUFFSOP_SET(pops, p2k, node, seek);
	PUFFSOP_SET(pops, p2k, node, remove);
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
	strlcat(typebuf, vfsops->vfs_name, sizeof(typebuf));

	pu = puffs_init(pops, devpath, typebuf, p2k->p2k_mp, puffs_flags);

	pn_root = puffs_pn_new(pu, p2k->p2k_rvp);
	puffs_setroot(pu, pn_root);
	puffs_fakecc = 1;

	if ((rv = puffs_mount(pu, mountpath, 0, p2k->p2k_rvp)) == -1)
		goto out;
	if ((rv = puffs_mainloop(pu, PUFFSLOOP_NODAEMON)) == -1)
		goto out;

 out:
	if (rv)
		sverrno = errno;
	p2k_destroy(p2k, rv);
	if (rv) {
		errno = sverrno;
		rv = -1;
	}
	
	return rv;
}

int
p2k_fs_statvfs(struct puffs_cc *pcc, struct statvfs *sbp,
	const struct puffs_cid *pcid)
{
	struct mount *mp = puffs_cc_getspecific(pcc);

	return VFS_STATVFS(mp, sbp, NULL);
}

int
p2k_fs_unmount(struct puffs_cc *pcc, int flags, const struct puffs_cid *pcid)
{
	struct mount *mp = puffs_cc_getspecific(pcc);

	return VFS_UNMOUNT(mp, flags, curlwp);
}

int
p2k_fs_sync(struct puffs_cc *pcc, int waitfor,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{
	struct mount *mp = puffs_cc_getspecific(pcc);

	return VFS_SYNC(mp, waitfor, NULL, curlwp);
}

int
p2k_node_lookup(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn)
{
	struct componentname *cn;
	struct vnode *vp;
	struct vattr va;
	int rv;

	cn = P2K_MAKECN(pcn);
	rv = VOP_LOOKUP(opc, &vp, cn);
	rump_freecn(cn, 1);
	if (rv) {
		if (rv == EJUSTRETURN)
			rv = ENOENT;
		return rv;
	}

	rv = VOP_GETATTR(vp, &va, NULL, curlwp);
	/* XXX: "unlookup" */
	if (rv) {
		warn("%s: lookup succesful but failed GETATTR", __func__);
		abort();
	}

	puffs_newinfo_setcookie(pni, vp);
	puffs_newinfo_setvtype(pni, va.va_type);
	puffs_newinfo_setsize(pni, va.va_size);
	puffs_newinfo_setrdev(pni, va.va_rdev);

	return 0;
}

int
p2k_node_create(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *vap)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = P2K_MAKECN(pcn);
	rv = VOP_CREATE(opc, &vp, cn, __UNCONST(vap));
	rump_freecn(cn, 0);
	if (rv == 0)
		puffs_newinfo_setcookie(pni, vp);

	return rv;
}

int
p2k_node_mknod(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *vap)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = P2K_MAKECN(pcn);
	rv = VOP_MKNOD(opc, &vp, cn, __UNCONST(vap));
	rump_freecn(cn, 0);
	if (rv == 0)
		puffs_newinfo_setcookie(pni, vp);

	return rv;
}

int
p2k_node_open(struct puffs_cc *pcc, void *opc, int mode,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return 0;
}

int
p2k_node_close(struct puffs_cc *pcc, void *opc, int flags,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return 0;
}

int
p2k_node_access(struct puffs_cc *pcc, void *opc, int mode,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return 0;
}

int
p2k_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *vap,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return VOP_GETATTR(opc, vap, NULL, curlwp);
}

int
p2k_node_setattr(struct puffs_cc *pcc, void *opc, const struct vattr *vap,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return VOP_SETATTR(opc, __UNCONST(vap), NULL, curlwp);
}

int
p2k_node_poll(struct puffs_cc *pcc, void *opc, int *events,
	const struct puffs_cid *pcid)
{

	return 0;
}

int
p2k_node_mmap(struct puffs_cc *pcc, void *opc, int flags,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return 0;
}

int
p2k_node_fsync(struct puffs_cc *pcc, void *opc, const struct puffs_cred *pcr,
	int flags, off_t offlo, off_t offhi, const struct puffs_cid *pcid)
{

	return VOP_FSYNC(opc, NULL, flags, offlo, offhi, curlwp);
}

int
p2k_node_seek(struct puffs_cc *pcc, void *opc, off_t oldoff, off_t newoff,
	const struct puffs_cred *pcr)
{

	return 0;
}

int
p2k_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct componentname *cn;
	int rv;

	cn = P2K_MAKECN(pcn);
	rv = VOP_REMOVE(opc, targ, cn);
	rump_freecn(cn, 0);

	return rv;
}

int
p2k_node_rename(struct puffs_cc *pcc, void *src_dir, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct componentname *cn_src, *cn_targ;
	int rv;

	cn_src = P2K_MAKECN(pcn_src);
	cn_targ = P2K_MAKECN(pcn_targ);
	rv = VOP_RENAME(src_dir, src, cn_src, targ_dir, targ, cn_targ);
	rump_freecn(cn_src, 0);
	rump_freecn(cn_targ, 0);

	return rv;
}

int
p2k_node_mkdir(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn, const struct vattr *vap)
{
	struct componentname *cn;
	struct vnode *vp;
	int rv;

	cn = P2K_MAKECN(pcn);
	rv = VOP_MKDIR(opc, &vp, cn, __UNCONST(vap));
	rump_freecn(cn, 0);
	if (rv == 0)
		puffs_newinfo_setcookie(pni, vp);

	return rv;
}

int
p2k_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct componentname *cn;
	int rv;

	cn = P2K_MAKECN(pcn);
	rv = VOP_RMDIR(opc, targ, cn);
	rump_freecn(cn, 0);

	return rv;
}

int
p2k_node_symlink(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn_src, const struct vattr *vap,
	const char *link_target)
{
	struct componentname *cn;
	int rv;

	cn = P2K_MAKECN(pcn_src);
	return rv;
}

int
p2k_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *dent,
	off_t *readoff, size_t *reslen, const struct puffs_cred *pcr,
	int *eofflag, off_t *cookies, size_t *ncookies)
{
	struct uio uio;
	struct iovec iov;
	int rv;

	iov.iov_base = dent;
	iov.iov_len = *reslen;

	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = *readoff;
	uio.uio_resid = *reslen;
	uio.uio_rw = UIO_READ;
	uio.uio_vmspace = UIO_VMSPACE_SYS;

	rv = VOP_READDIR(opc, &uio, NULL, eofflag, NULL, NULL);
	if (rv == 0) {
		*reslen = uio.uio_resid;
		*readoff = uio.uio_offset;
	}

	return rv;
}

int
p2k_node_readlink(struct puffs_cc *pcc, void *opc,
	const struct puffs_cred *pcr, char *link, size_t *linklen)
{
	struct uio uio;
	struct iovec iov;
	int rv;

	iov.iov_base = link;
	iov.iov_len = *linklen;

	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = *linklen;
	uio.uio_rw = UIO_READ;
	uio.uio_vmspace = UIO_VMSPACE_SYS;

	rv = VOP_READLINK(opc, &uio, NULL);
	if (rv == 0)
		*linklen = uio.uio_offset;

	return rv;
}

int
p2k_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf, off_t offset,
	size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct uio uio;
	struct iovec iov;
	int rv;

	iov.iov_base = buf;
	iov.iov_len = *resid;

	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = offset;
	uio.uio_resid = *resid;
	uio.uio_rw = UIO_READ;
	uio.uio_vmspace = UIO_VMSPACE_SYS;

	rv = VOP_READ(opc, &uio, ioflag, NULL);
	if (rv == 0)
		*resid = uio.uio_resid;

	return rv;
}

int
p2k_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf, off_t offset,
	size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct uio uio;
	struct iovec iov;
	int rv;

	iov.iov_base = buf;
	iov.iov_len = *resid;

	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = offset;
	uio.uio_resid = *resid;
	uio.uio_rw = UIO_WRITE;
	uio.uio_vmspace = UIO_VMSPACE_SYS;

	rv = rump_vopwrite_fault(opc, offset, *resid, NULL);
	if (rv)
		return rv;
	rv = VOP_WRITE(opc, &uio, ioflag, NULL);
	if (rv == 0)
		*resid = uio.uio_resid;

	/* XXX: should do this */
#if 0
	return VOP_PUTPAGES(opc, 0, 0, PGO_ALLPAGES);
#endif
	return rv;
}

int
p2k_node_reclaim(struct puffs_cc *pcc, void *opc, const struct puffs_cid *pcid)
{

	rump_recyclenode(opc);
	rump_putnode(opc);

	return 0;
}

int
p2k_node_inactive(struct puffs_cc *pcc, void *opc, const struct puffs_cid *pcid)
{
	struct vnode *vp;
	int rv;

	vp = opc;
	rv = VOP_INACTIVE(vp, curlwp);
	if (vp->v_data == (void *)1)
		puffs_setback(pcc, PUFFS_SETBACK_NOREF_N1);

	return rv;
}
