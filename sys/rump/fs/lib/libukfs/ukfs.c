/*	$NetBSD: ukfs.c,v 1.7.2.3 2008/01/09 01:57:58 matt Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
 * This library enables access to files systems directly without
 * involving system calls.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <assert.h>
#include <err.h>
#define _KERNEL
#include <errno.h>
#undef _KERNEL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rump.h"
#include "ukfs.h"

#define UKFS_MODE_DEFAULT 0555

struct ukfs {
	struct mount *ukfs_mp;
};

struct mount *
ukfs_getmp(struct ukfs *ukfs)
{

	return ukfs->ukfs_mp;
}

struct vnode *
ukfs_getrvp(struct ukfs *ukfs)
{
	struct vnode *rvp;
	int rv;

	rv = rump_vfs_root(ukfs->ukfs_mp, &rvp, 0);
	assert(rv == 0);

	return rvp;
}

int
ukfs_init()
{

	rump_init();

	return 0;
}

struct ukfs *
ukfs_mount(const char *vfsname, const char *devpath, const char *mountpath,
	int mntflags, void *arg, size_t alen)
{
	struct ukfs *fs = NULL;
	struct vfsops *vfsops;
	struct mount *mp;
	int rv = 0;

	vfsops = rump_vfs_getopsbyname(vfsname);
	if (vfsops == NULL) {
		rv = ENOENT;
		goto out;
	}

	mp = rump_mnt_init(vfsops, mntflags);

	fs = malloc(sizeof(struct ukfs));
	if (fs == NULL) {
		rv = ENOMEM;
		goto out;
	}
	memset(fs, 0, sizeof(struct ukfs));

	rump_fakeblk_register(devpath);
	rv = rump_mnt_mount(mp, mountpath, arg, &alen);
	if (rv) {
		warnx("VFS_MOUNT %d", rv);
		goto out;
	}
	fs->ukfs_mp = mp;
	rump_fakeblk_deregister(devpath);

 out:
	if (rv) {
		if (fs && fs->ukfs_mp)
			rump_mnt_destroy(fs->ukfs_mp);
		if (fs)
			free(fs);
		errno = rv;
		fs = NULL;
	}

	return fs;
}

void
ukfs_release(struct ukfs *fs, int dounmount)
{
	int rv;

	if (dounmount) {
		rv = rump_vfs_sync(fs->ukfs_mp, 1, NULL);
		rv |= rump_vfs_unmount(fs->ukfs_mp, 0);
		assert(rv == 0);
	}

	rump_vfs_syncwait(fs->ukfs_mp);
	rump_mnt_destroy(fs->ukfs_mp);

	free(fs);
}

/* don't need vn_lock(), since we don't have VXLOCK */
#define VLE(a) rump_vp_lock_exclusive(a)
#define VLS(a) rump_vp_lock_shared(a)
#define VUL(a) rump_vp_unlock(a)
#define AUL(a) assert(rump_vp_islocked(a) == 0)

void
ukfs_ll_recycle(struct vnode *vp)
{
	bool recycle;

	/* XXXXX */
	if (vp == NULL || rump_vp_getref(vp))
		return;

	VLE(vp);
	RUMP_VOP_FSYNC(vp, NULL, 0, 0, 0);
	RUMP_VOP_INACTIVE(vp, &recycle);
	rump_recyclenode(vp);
	rump_putnode(vp);
}

/*
 * simplo (well, horrid) namei.  doesn't do symlinks & anything else
 * hard, though.  (ok, ok, it's a mess, it's a messssss!)
 *
 * XXX: maybe I should just try running the kernel namei(), although
 * it would require a wrapping due to the name collision in
 * librump vfs.c
 */
int
ukfs_ll_namei(struct vnode *rvp, const char **pnp, u_long op,
	struct vnode **dvpp, struct vnode **vpp)
{
	struct vnode *dvp, *vp;
	struct componentname *cnp;
	const char *pn, *p_next, *p_end;
	size_t pnlen;
	u_long flags;
	int rv;

	/* remove trailing slashes */
	pn = *pnp;
	assert(strlen(pn) > 0);
	p_end = pn + strlen(pn)-1;
	while (*p_end == '/' && p_end != *pnp)
		p_end--;

	/* caller wanted root? */
	if (p_end == *pnp) {
		if (dvpp)
			*dvpp = rvp;
		if (vpp)
			*vpp = rvp;

		*pnp = p_end;
		return 0;
	}
		
	dvp = NULL;
	vp = rvp;
	p_end++;
	for (;;) {
		while (*pn == '/')
			pn++;
		assert(*pn != '\0');

		flags = 0;
		dvp = vp;
		vp = NULL;

		p_next = strchr(pn, '/');
		if (p_next == NULL || p_next == p_end) {
			p_next = p_end;
			flags |= RUMP_NAMEI_ISLASTCN;
		}
		pnlen = p_next - pn;

		if (pnlen == 2 && strcmp(pn, "..") == 0)
			flags |= RUMP_NAMEI_ISDOTDOT;

		VLE(dvp);
		cnp = rump_makecn(op, flags, pn, pnlen, RUMPCRED_SUSER, curlwp);
		rv = RUMP_VOP_LOOKUP(dvp, &vp, cnp);
		rump_freecn(cnp, RUMPCN_ISLOOKUP | RUMPCN_FREECRED);
		VUL(dvp);
		if (rv == 0)
			VUL(vp);

		if (!((flags & RUMP_NAMEI_ISLASTCN) && dvpp))
			ukfs_ll_recycle(dvp);

		if (rv || (flags & RUMP_NAMEI_ISLASTCN))
			break;

		pn += pnlen;
	}
	assert((rv != 0) || (flags & RUMP_NAMEI_ISLASTCN));
	if (vp && vpp == NULL)
		ukfs_ll_recycle(vp);

	if (dvpp)
		*dvpp = dvp;
	if (vpp)
		*vpp = vp;
	*pnp = pn;

	return rv;
}

int
ukfs_getdents(struct ukfs *ukfs, const char *dirname, off_t off,
	uint8_t *buf, size_t bufsize)
{
	struct uio *uio;
	struct vnode *vp;
	size_t resid;
	int rv, eofflag;

	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &dirname, RUMP_NAMEI_LOOKUP,
	    NULL, &vp);
	if (rv)
		goto out;
		
	uio = rump_uio_setup(buf, bufsize, off, RUMPUIO_READ);
	VLE(vp);
	rv = RUMP_VOP_READDIR(vp, uio, NULL, &eofflag, NULL, NULL);
	VUL(vp);
	resid = rump_uio_free(uio);

 out:
	ukfs_ll_recycle(vp);

	if (rv) {
		errno = rv;
		return -1;
	}

	return bufsize - resid;
}

enum ukfs_rw {UKFS_READ, UKFS_WRITE, UKFS_READLINK};

static ssize_t
readwrite(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize, enum rump_uiorw rw, enum ukfs_rw ukrw)
{
	struct uio *uio;
	struct vnode *vp;
	size_t resid;
	int rv;

	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &filename, RUMP_NAMEI_LOOKUP,
	    NULL, &vp);
	if (rv)
		goto out;

	uio = rump_uio_setup(buf, bufsize, off, rw);
	VLS(vp);
	switch (ukrw) {
	case UKFS_READ:
		rv = RUMP_VOP_READ(vp, uio, 0, NULL);
		break;
	case UKFS_WRITE:
		rv = RUMP_VOP_WRITE(vp, uio, 0, NULL);
		break;
	case UKFS_READLINK:
		rv = RUMP_VOP_READLINK(vp, uio, NULL);
		break;
	}
	VUL(vp);
	resid = rump_uio_free(uio);

 out:
	ukfs_ll_recycle(vp);

	if (rv) {
		errno = rv;
		return -1;
	}

	return bufsize - resid;
}

ssize_t
ukfs_read(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize)
{

	return readwrite(ukfs, filename, off, buf, bufsize,
	    RUMPUIO_READ, UKFS_READ);
}

ssize_t
ukfs_write(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize)
{

	return readwrite(ukfs, filename, off, buf, bufsize,
	    RUMPUIO_WRITE, UKFS_WRITE);
}

static int
create(struct ukfs *ukfs, const char *filename, mode_t mode,
	enum vtype vt, dev_t dev)
{
	struct componentname *cnp;
	struct vnode *dvp = NULL, *vp = NULL;
	struct vattr *vap;
	int rv;
	int (*do_fn)(struct vnode *, struct vnode **,
		     struct componentname *, struct vattr *);

	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &filename, RUMP_NAMEI_CREATE,
	    &dvp, NULL);
	if (rv == 0)
		rv = EEXIST;
	if (rv != EJUSTRETURN)
		goto out;

	switch (vt) {
	case VDIR:
		do_fn = RUMP_VOP_MKDIR;
		break;
	case VREG:
	case VSOCK:
		do_fn = RUMP_VOP_CREATE;
		break;
	case VBLK:
	case VCHR:
		do_fn = RUMP_VOP_MKNOD;
		break;
	default:
		rv = EINVAL;
		goto out;
	}

	vap = rump_vattr_init();
	rump_vattr_settype(vap, vt);
	rump_vattr_setmode(vap, mode);
	rump_vattr_setrdev(vap, dev);
	cnp = rump_makecn(RUMP_NAMEI_CREATE,
	    RUMP_NAMEI_HASBUF|RUMP_NAMEI_SAVENAME, filename,
	    strlen(filename), RUMPCRED_SUSER, curlwp);

	VLE(dvp);
	rv = do_fn(dvp, &vp, cnp, vap);
	if (rv == 0)
		VUL(vp);

	rump_freecn(cnp, RUMPCN_FREECRED);
	rump_vattr_free(vap);

 out:
	ukfs_ll_recycle(dvp);
	ukfs_ll_recycle(vp);

	if (rv) {
		errno = rv;
		return -1;
	}

	return 0;
}

int
ukfs_create(struct ukfs *ukfs, const char *filename, mode_t mode)
{
	enum vtype vt;

	switch (mode & S_IFMT) {
	case S_IFREG:
		vt = VREG;
		break;
	case S_IFSOCK:
		vt = VSOCK;
		break;
	default:
		return EINVAL;
	}

	return create(ukfs, filename, mode, vt, /*XXX*/(dev_t)-1);
}

int
ukfs_mknod(struct ukfs *ukfs, const char *filename, mode_t mode, dev_t dev)
{
	enum vtype vt;

	switch (mode & S_IFMT) {
	case S_IFBLK:
		vt = VBLK;
		break;
	case S_IFCHR:
		vt = VCHR;
		break;
	default:
		return EINVAL;
	}

	return create(ukfs, filename, mode, vt, dev);
}

int
ukfs_mkdir(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	return create(ukfs, filename, mode, VDIR, (dev_t)-1);
}

static int
doremove(struct ukfs *ukfs, const char *filename,
	int (*do_fn)(struct vnode *, struct vnode *, struct componentname *))
{
	struct componentname *cnp;
	struct vnode *dvp = NULL, *vp = NULL;
	int rv;

	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &filename, RUMP_NAMEI_DELETE,
	    &dvp, &vp);
	if (rv)
		goto out;

	cnp = rump_makecn(RUMP_NAMEI_DELETE, 0, filename,
	    strlen(filename), RUMPCRED_SUSER, curlwp);
	VLE(dvp);
	VLE(vp);
	rv = do_fn(dvp, vp, cnp);
	rump_freecn(cnp, RUMPCN_FREECRED);

 out:
	ukfs_ll_recycle(dvp);
	ukfs_ll_recycle(vp);

	if (rv) {
		errno = rv;
		return -1;
	}

	return 0;
}

int
ukfs_remove(struct ukfs *ukfs, const char *filename)
{

	return doremove(ukfs, filename, RUMP_VOP_REMOVE);
}

int
ukfs_rmdir(struct ukfs *ukfs, const char *filename)
{

	return doremove(ukfs, filename, RUMP_VOP_RMDIR);
}

int
ukfs_link(struct ukfs *ukfs, const char *filename, const char *f_create)
{
	struct vnode *dvp = NULL, *vp = NULL;
	struct componentname *cnp;
	int rv;

	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &filename, RUMP_NAMEI_LOOKUP,
	    NULL, &vp);
	if (rv)
		goto out;

	rump_vp_incref(vp); /* XXX kludge of the year */
	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &f_create, RUMP_NAMEI_CREATE,
	    &dvp, NULL);
	rump_vp_decref(vp); /* XXX */

	if (rv == 0)
		rv = EEXIST;
	if (rv != EJUSTRETURN)
		goto out;

	cnp = rump_makecn(RUMP_NAMEI_CREATE,
	    RUMP_NAMEI_HASBUF | RUMP_NAMEI_SAVENAME,
	    f_create, strlen(f_create), RUMPCRED_SUSER, curlwp);
	VLE(dvp);
	rv = RUMP_VOP_LINK(dvp, vp, cnp);
	rump_freecn(cnp, RUMPCN_FREECRED);

 out:
	ukfs_ll_recycle(dvp);
	ukfs_ll_recycle(vp);

	if (rv) {
		errno = rv;
		return -1;
	}

	return 0;
}

int
ukfs_symlink(struct ukfs *ukfs, const char *filename, char *linkname)
{
	struct componentname *cnp;
	struct vnode *dvp = NULL, *vp = NULL;
	struct vattr *vap;
	int rv;

	rv = ukfs_ll_namei(ukfs_getrvp(ukfs), &filename, RUMP_NAMEI_CREATE,
	    &dvp, NULL);
	if (rv == 0)
		rv = EEXIST;
	if (rv != EJUSTRETURN)
		goto out;

	vap = rump_vattr_init();
	rump_vattr_setmode(vap, UKFS_MODE_DEFAULT);
	rump_vattr_settype(vap, VLNK);
	cnp = rump_makecn(RUMP_NAMEI_CREATE,
	    RUMP_NAMEI_HASBUF|RUMP_NAMEI_SAVENAME, filename,
	    strlen(filename), RUMPCRED_SUSER, curlwp);

	VLE(dvp);
	rv = RUMP_VOP_SYMLINK(dvp, &vp, cnp, vap, linkname);
	if (rv == 0)
		VUL(vp);

	rump_freecn(cnp, RUMPCN_FREECRED);
	rump_vattr_free(vap);

 out:
	ukfs_ll_recycle(dvp);
	ukfs_ll_recycle(vp);

	if (rv) {
		errno = rv;
		return -1;
	}

	return 0;
}

ssize_t
ukfs_readlink(struct ukfs *ukfs, const char *filename,
	char *linkbuf, size_t buflen)
{

	return readwrite(ukfs, filename, 0,
	    (uint8_t *)linkbuf, buflen, RUMPUIO_READ, UKFS_READLINK);
}
