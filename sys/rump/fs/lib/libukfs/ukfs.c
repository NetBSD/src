/*	$NetBSD: ukfs.c,v 1.22 2008/03/12 14:49:19 pooka Exp $	*/

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
#include <sys/namei.h>
#include <sys/uio.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rump.h"
#include "rump_syscalls.h"
#include "ukfs.h"

#define UKFS_MODE_DEFAULT 0555

struct ukfs {
	struct mount *ukfs_mp;

	pthread_spinlock_t ukfs_spin;
	pid_t ukfs_nextpid;
	struct vnode *ukfs_cdir;
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

static pid_t
nextpid(struct ukfs *ukfs)
{
	pid_t npid;

	pthread_spin_lock(&ukfs->ukfs_spin);
	npid = ukfs->ukfs_nextpid++;
	pthread_spin_unlock(&ukfs->ukfs_spin);

	return npid;
}

static void
precall(struct ukfs *ukfs)
{
	struct vnode *rvp, *cvp;

	rump_setup_curlwp(nextpid(ukfs), 1, 1);
	rvp = ukfs_getrvp(ukfs);
	pthread_spin_lock(&ukfs->ukfs_spin);
	cvp = ukfs->ukfs_cdir;
	pthread_spin_unlock(&ukfs->ukfs_spin);
	rump_rcvp_set(rvp, cvp); /* takes refs */
	ukfs_ll_rele(rvp);
}

static void
postcall(struct ukfs *ukfs)
{
	struct vnode *rvp;

	rvp = ukfs_getrvp(ukfs);
	rump_rcvp_set(NULL, rvp);
	ukfs_ll_rele(rvp);
	rump_clear_curlwp();
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
	pthread_spin_init(&fs->ukfs_spin, PTHREAD_PROCESS_SHARED);

	rump_fakeblk_register(devpath);
	rv = rump_mnt_mount(mp, mountpath, arg, &alen);
	if (rv) {
		warnx("VFS_MOUNT %d", rv);
		goto out;
	}
	fs->ukfs_mp = mp;
	fs->ukfs_cdir = ukfs_getrvp(fs);
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

	ukfs_ll_rele(fs->ukfs_cdir);

	if (dounmount) {
		rv = rump_vfs_sync(fs->ukfs_mp, 1, NULL);
		rump_vp_recycle_nokidding(ukfs_getrvp(fs));
		rv |= rump_vfs_unmount(fs->ukfs_mp, 0);
		assert(rv == 0);
	}

	rump_vfs_syncwait(fs->ukfs_mp);
	rump_mnt_destroy(fs->ukfs_mp);

	pthread_spin_destroy(&fs->ukfs_spin);
	free(fs);
}

/* don't need vn_lock(), since we don't have VXLOCK */
#define VLE(a) rump_vp_lock_exclusive(a)
#define VLS(a) rump_vp_lock_shared(a)
#define VUL(a) rump_vp_unlock(a)
#define AUL(a) assert(rump_vp_islocked(a) == 0)

/* free willy */
void
ukfs_ll_rele(struct vnode *vp)
{

	rump_vp_rele(vp);
}

int
ukfs_ll_namei(struct ukfs *ukfs, uint32_t op, uint32_t flags, const char *name,
	struct vnode **dvpp, struct vnode **vpp, struct componentname **cnpp)
{

	/* XXX: rootvnode dance */

	return rump_namei(op, flags, name, dvpp, vpp, cnpp);
}

int
ukfs_getdents(struct ukfs *ukfs, const char *dirname, off_t *off,
	uint8_t *buf, size_t bufsize)
{
	struct uio *uio;
	struct vnode *vp;
	size_t resid;
	int rv, eofflag;

	rv = ukfs_ll_namei(ukfs, RUMP_NAMEI_LOOKUP, NAMEI_LOCKLEAF, dirname,
	    NULL, &vp, NULL);
	if (rv)
		goto out;
		
	uio = rump_uio_setup(buf, bufsize, *off, RUMPUIO_READ);
	rv = RUMP_VOP_READDIR(vp, uio, NULL, &eofflag, NULL, NULL);
	VUL(vp);
	*off = rump_uio_getoff(uio);
	resid = rump_uio_free(uio);
	rump_vp_rele(vp);

 out:
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

	precall(ukfs);
	rv = ukfs_ll_namei(ukfs, RUMP_NAMEI_LOOKUP, 0, filename,
	    NULL, &vp, NULL);
	if (rv)
		goto out;

	uio = rump_uio_setup(buf, bufsize, off, rw);
	switch (ukrw) {
	case UKFS_READ:
		VLS(vp);
		rv = RUMP_VOP_READ(vp, uio, 0, NULL);
		break;
	case UKFS_WRITE:
		VLE(vp);
		rv = RUMP_VOP_WRITE(vp, uio, 0, NULL);
		break;
	case UKFS_READLINK:
		VLE(vp);
		rv = RUMP_VOP_READLINK(vp, uio, NULL);
		break;
	}
	VUL(vp);
	ukfs_ll_rele(vp);
	resid = rump_uio_free(uio);

 out:
	postcall(ukfs);
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
	ssize_t rv;

	rv = readwrite(ukfs, filename, off, buf, bufsize,
	    RUMPUIO_READ, UKFS_READ);

	return rv;
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
	int rv, flags;
	int (*do_fn)(struct vnode *, struct vnode **,
		     struct componentname *, struct vattr *);

	precall(ukfs);

	flags = NAMEI_LOCKPARENT;
	if (vt == VDIR) {
		flags |= NAMEI_CREATEDIR;
	}
	rv = ukfs_ll_namei(ukfs, RUMP_NAMEI_CREATE, NAMEI_LOCKPARENT, filename,
	    &dvp, &vp, &cnp);
	if (rv) {
		goto out;
	}

	if (vp) {
		rv = EEXIST;
		VUL(dvp);
		ukfs_ll_rele(dvp);
		ukfs_ll_rele(vp);
		goto out;
	}

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
		ukfs_ll_rele(dvp);
		rv = EINVAL;
		goto out;
	}

	vap = rump_vattr_init();
	rump_vattr_settype(vap, vt);
	rump_vattr_setmode(vap, mode);
	rump_vattr_setrdev(vap, dev);

	rv = do_fn(dvp, &vp, cnp, vap);
	if (rv == 0) {
		VUL(vp);
		ukfs_ll_rele(vp);
	}

	rump_freecn(cnp, RUMPCN_FREECRED);
	rump_vattr_free(vap);

 out:
	postcall(ukfs);
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
		errno = EINVAL;
		return -1;
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
		errno = EINVAL;
		return -1;
	}

	return create(ukfs, filename, mode, vt, dev);
}

static int
builddirs(struct ukfs *ukfs, const char *filename, mode_t mode)
{
	char *f1, *f2;
	int rv;

	/* does it exist? */
	rv = ukfs_ll_namei(ukfs, RUMP_NAMEI_LOOKUP, 0, filename,
	    NULL, NULL, NULL);
	if (rv == 0)
		return 0;

	/* If we didn't find it, create dirs by path games */
	f1 = f2 = strdup(filename);
	if (!f1) {
		errno = ENOMEM;
		return -1;
	}

	for (;;) {
		/* find next component */
		f2 += strspn(f2, "/");
		f2 += strcspn(f2, "/");
		*f2 = '\0';

		rv = ukfs_mkdir(ukfs, f1, 0777, false); 
		if (rv == EEXIST)
			rv = 0;
		if (rv)
			break;

		if (!*f2)
			break;
		*f2 = '/';
	}

	free(f1);
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}

int
ukfs_mkdir(struct ukfs *ukfs, const char *filename, mode_t mode, bool p)
{

	if (p) {
		return builddirs(ukfs, filename, mode);
	} else {
		return create(ukfs, filename, mode, VDIR, (dev_t)-1);
	}
}

int
ukfs_remove(struct ukfs *ukfs, const char *filename)
{
	int rv;

	precall(ukfs);
	rump_sys_unlink(filename, &rv);
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}

int
ukfs_rmdir(struct ukfs *ukfs, const char *filename)
{
	int rv;

	precall(ukfs);
	rump_sys_rmdir(filename, &rv);
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}

int
ukfs_link(struct ukfs *ukfs, const char *filename, const char *f_create)
{
	struct vnode *dvp = NULL, *vp = NULL;
	struct componentname *cnp;
	int rv;

	precall(ukfs);
	rv = ukfs_ll_namei(ukfs, RUMP_NAMEI_LOOKUP, NAMEI_LOCKLEAF, filename,
	    NULL, &vp, NULL);
	if (rv)
		goto out;
	VUL(vp);

	rv = ukfs_ll_namei(ukfs, RUMP_NAMEI_CREATE, NAMEI_LOCKPARENT, f_create,
	    &dvp, NULL, &cnp);
	if (rv) {
		ukfs_ll_rele(vp);
		goto out;
	}

	rv = RUMP_VOP_LINK(dvp, vp, cnp);
	rump_freecn(cnp, RUMPCN_FREECRED);

 out:
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}

	return 0;
}

int
ukfs_symlink(struct ukfs *ukfs, const char *filename, const char *linkname)
{
	int rv;

	precall(ukfs);
	rump_sys_symlink(filename, linkname, &rv);
	postcall(ukfs);
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

int
ukfs_rename(struct ukfs *ukfs, const char *from, const char *to)
{
	int rv;

	precall(ukfs);
	rump_sys_rename(from, to, &rv);
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}

int
ukfs_chdir(struct ukfs *ukfs, const char *path)
{
	struct vnode *newvp, *oldvp;
	int rv;

	precall(ukfs);
	rump_sys_chdir(path, &rv);
	if (rv)
		goto out;

	newvp = rump_cdir_get();
	pthread_spin_lock(&ukfs->ukfs_spin);
	oldvp = ukfs->ukfs_cdir;
	ukfs->ukfs_cdir = newvp;
	pthread_spin_unlock(&ukfs->ukfs_spin);
	if (oldvp)
		ukfs_ll_rele(oldvp);
	postcall(ukfs);

 out:
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}
