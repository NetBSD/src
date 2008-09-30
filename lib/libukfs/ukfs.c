/*	$NetBSD: ukfs.c,v 1.9 2008/09/30 19:26:23 pooka Exp $	*/

/*
 * Copyright (c) 2007, 2008  Antti Kantee.  All Rights Reserved.
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

#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64
#endif

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/mount.h>

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <rump/ukfs.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#define UKFS_MODE_DEFAULT 0555

struct ukfs {
	struct mount *ukfs_mp;
	struct vnode *ukfs_rvp;

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

	rvp = ukfs->ukfs_rvp;
	rump_vp_incref(rvp);

	return rvp;
}

static pid_t
nextpid(struct ukfs *ukfs)
{
	pid_t npid;

	pthread_spin_lock(&ukfs->ukfs_spin);
	if (ukfs->ukfs_nextpid == 0)
		ukfs->ukfs_nextpid++;
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
	rump_vp_rele(rvp);
}

static void
postcall(struct ukfs *ukfs)
{
	struct vnode *rvp;

	rvp = ukfs_getrvp(ukfs);
	rump_rcvp_set(NULL, rvp);
	rump_vp_rele(rvp);
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
	rump_fakeblk_deregister(devpath);
	if (rv) {
		goto out;
	}
	fs->ukfs_mp = mp;

	rv = rump_vfs_root(fs->ukfs_mp, &fs->ukfs_rvp, 0);
	fs->ukfs_cdir = ukfs_getrvp(fs);

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
ukfs_release(struct ukfs *fs, int flags)
{
	int rv;

	if ((flags & UKFS_RELFLAG_NOUNMOUNT) == 0) {
		kauth_cred_t cred;

		rump_vp_rele(fs->ukfs_cdir);
		cred = rump_cred_suserget();
		rv = rump_vfs_sync(fs->ukfs_mp, 1, cred);
		rump_cred_suserput(cred);
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

#define STDCALL(ukfs, thecall)						\
	int rv = 0;							\
									\
	precall(ukfs);							\
	thecall;							\
	postcall(ukfs);							\
	if (rv) {							\
		errno = rv;						\
		return -1;						\
	}								\
	return 0;

int
ukfs_getdents(struct ukfs *ukfs, const char *dirname, off_t *off,
	uint8_t *buf, size_t bufsize)
{
	struct uio *uio;
	struct vnode *vp;
	size_t resid;
	kauth_cred_t cred;
	int rv, eofflag;

	precall(ukfs);
	rv = rump_namei(RUMP_NAMEI_LOOKUP, RUMP_NAMEI_LOCKLEAF, dirname,
	    NULL, &vp, NULL);
	postcall(ukfs);
	if (rv)
		goto out;
		
	uio = rump_uio_setup(buf, bufsize, *off, RUMPUIO_READ);
	cred = rump_cred_suserget();
	rv = RUMP_VOP_READDIR(vp, uio, cred, &eofflag, NULL, NULL);
	rump_cred_suserput(cred);
	VUL(vp);
	*off = rump_uio_getoff(uio);
	resid = rump_uio_free(uio);
	rump_vp_rele(vp);

 out:
	if (rv) {
		errno = rv;
		return -1;
	}

	/* LINTED: not totally correct return type, but follows syscall */
	return bufsize - resid;
}

ssize_t
ukfs_read(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize)
{
	int fd, rv = 0, dummy;
	ssize_t xfer = -1; /* XXXgcc */

	precall(ukfs);
	fd = rump_sys_open(filename, RUMP_O_RDONLY, 0, &rv);
	if (rv)
		goto out;

	xfer = rump_sys_pread(fd, buf, bufsize, 0, off, &rv);
	rump_sys_close(fd, &dummy);

 out:
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return xfer;
}

ssize_t
ukfs_write(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize)
{
	int fd, rv = 0, dummy;
	ssize_t xfer = -1; /* XXXgcc */

	precall(ukfs);
	fd = rump_sys_open(filename, RUMP_O_WRONLY, 0, &rv);
	if (rv)
		goto out;

	/* write and commit */
	xfer = rump_sys_pwrite(fd, buf, bufsize, 0, off, &rv);
	if (rv == 0)
		rump_sys_fsync(fd, &dummy);

	rump_sys_close(fd, &dummy);

 out:
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return xfer;
}

int
ukfs_create(struct ukfs *ukfs, const char *filename, mode_t mode)
{
	int rv, fd, dummy;

	precall(ukfs);
	fd = rump_sys_open(filename, RUMP_O_WRONLY | RUMP_O_CREAT, mode, &rv);
	rump_sys_close(fd, &dummy);

	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}

int
ukfs_mknod(struct ukfs *ukfs, const char *path, mode_t mode, dev_t dev)
{

	STDCALL(ukfs, rump_sys_mknod(path, mode, dev, &rv));
}

int
ukfs_mkfifo(struct ukfs *ukfs, const char *path, mode_t mode)
{

	STDCALL(ukfs, rump_sys_mkfifo(path, mode, &rv));
}

int
ukfs_mkdir(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	STDCALL(ukfs, rump_sys_mkdir(filename, mode, &rv));
}

int
ukfs_remove(struct ukfs *ukfs, const char *filename)
{

	STDCALL(ukfs, rump_sys_unlink(filename, &rv));
}

int
ukfs_rmdir(struct ukfs *ukfs, const char *filename)
{

	STDCALL(ukfs, rump_sys_rmdir(filename, &rv));
}

int
ukfs_link(struct ukfs *ukfs, const char *filename, const char *f_create)
{

	STDCALL(ukfs, rump_sys_link(filename, f_create, &rv));
}

int
ukfs_symlink(struct ukfs *ukfs, const char *filename, const char *linkname)
{

	STDCALL(ukfs, rump_sys_symlink(filename, linkname, &rv));
}

ssize_t
ukfs_readlink(struct ukfs *ukfs, const char *filename,
	char *linkbuf, size_t buflen)
{
	ssize_t rv;
	int myerr = 0;

	precall(ukfs);
	rv = rump_sys_readlink(filename, linkbuf, buflen, &myerr);
	postcall(ukfs);
	if (myerr) {
		errno = myerr;
		return -1;
	}
	return rv;
}

int
ukfs_rename(struct ukfs *ukfs, const char *from, const char *to)
{

	STDCALL(ukfs, rump_sys_rename(from, to, &rv));
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
		rump_vp_rele(oldvp);

 out:
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}
	return 0;
}

int
ukfs_stat(struct ukfs *ukfs, const char *filename, struct stat *file_stat)
{

	STDCALL(ukfs, rump_sys___stat30(filename, file_stat, &rv));
}

int
ukfs_lstat(struct ukfs *ukfs, const char *filename, struct stat *file_stat)
{

	STDCALL(ukfs, rump_sys___lstat30(filename, file_stat, &rv));
}

int
ukfs_chmod(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	STDCALL(ukfs, rump_sys_chmod(filename, mode, &rv));
}

int
ukfs_lchmod(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	STDCALL(ukfs, rump_sys_lchmod(filename, mode, &rv));
}

int
ukfs_chown(struct ukfs *ukfs, const char *filename, uid_t uid, gid_t gid)
{

	STDCALL(ukfs, rump_sys_chown(filename, uid, gid, &rv));
}

int
ukfs_lchown(struct ukfs *ukfs, const char *filename, uid_t uid, gid_t gid)
{

	STDCALL(ukfs, rump_sys_lchown(filename, uid, gid, &rv));
}

int
ukfs_chflags(struct ukfs *ukfs, const char *filename, u_long flags)
{

	STDCALL(ukfs, rump_sys_chflags(filename, flags, &rv));
}

int
ukfs_lchflags(struct ukfs *ukfs, const char *filename, u_long flags)
{

	STDCALL(ukfs, rump_sys_lchflags(filename, flags, &rv));
}

int
ukfs_utimes(struct ukfs *ukfs, const char *filename, const struct timeval *tptr)
{

	STDCALL(ukfs, rump_sys_utimes(filename, tptr, &rv));
}

int
ukfs_lutimes(struct ukfs *ukfs, const char *filename, 
	      const struct timeval *tptr)
{

	STDCALL(ukfs, rump_sys_lutimes(filename, tptr, &rv));
}

/*
 * Dynamic module support
 */

/* load one library */

/*
 * XXX: the dlerror stuff isn't really threadsafe, but then again I
 * can't protect against other threads calling dl*() outside of ukfs,
 * so just live with it being flimsy
 */
#define UFSLIB "librumpfs_ufs.so"
int
ukfs_modload(const char *fname)
{
	void *handle, *thesym;
	struct stat sb;
	const char *p;
	int error;

	if (stat(fname, &sb) == -1)
		return -1;

	handle = dlopen(fname, RTLD_GLOBAL);
	if (handle == NULL) {
		if (strstr(dlerror(), "Undefined symbol"))
			return 0;
		warnx("dlopen %s failed: %s\n", fname, dlerror());
		/* XXXerrno */
		return -1;
	}

	/*
	 * XXX: the ufs module is not loaded in the same fashion as the
	 * others.  But we can't do dlclose() for it, since that would
	 * lead to not being able to load ffs/ext2fs/lfs.  Hence hardcode
	 * and kludge around the issue for now.  But this should really
	 * be fixed by fixing sys/ufs/ufs to be a kernel module.
	 */
	if ((p = strrchr(fname, '/')) != NULL)
		p++;
	else
		p = fname;
	if (strcmp(p, UFSLIB) == 0)
		return 1;

	thesym = dlsym(handle, "__start_link_set_modules");
	if (thesym) {
		error = rump_vfs_load(thesym);
		if (error)
			goto errclose;
		return 1;
	}
	error = EINVAL;

 errclose:
	dlclose(handle);
	errno = error;
	return -1;
}

struct loadfail {
	char *pname;

	LIST_ENTRY(loadfail) entries;
};

#define RUMPFSMOD_PREFIX "librumpfs_"
#define RUMPFSMOD_SUFFIX ".so"

int
ukfs_modload_dir(const char *dir)
{
	char nbuf[MAXPATHLEN+1], *p;
	struct dirent entry, *result;
	DIR *libdir;
	struct loadfail *lf, *nlf;
	int error, nloaded = 0, redo;
	LIST_HEAD(, loadfail) lfs;

	libdir = opendir(dir);
	if (libdir == NULL)
		return -1;

	LIST_INIT(&lfs);
	for (;;) {
		if ((error = readdir_r(libdir, &entry, &result)) != 0)
			break;
		if (!result)
			break;
		if (strncmp(result->d_name, RUMPFSMOD_PREFIX,
		    strlen(RUMPFSMOD_PREFIX)) != 0)
			continue;
		if (((p = strstr(result->d_name, RUMPFSMOD_SUFFIX)) == NULL)
		    || strlen(p) != strlen(RUMPFSMOD_SUFFIX))
			continue;
		strlcpy(nbuf, dir, sizeof(nbuf));
		strlcat(nbuf, "/", sizeof(nbuf));
		strlcat(nbuf, result->d_name, sizeof(nbuf));
		switch (ukfs_modload(nbuf)) {
		case 0:
			lf = malloc(sizeof(*lf));
			if (lf == NULL) {
				error = ENOMEM;
				break;
			}
			lf->pname = strdup(nbuf);
			if (lf->pname == NULL) {
				free(lf);
				error = ENOMEM;
				break;
			}
			LIST_INSERT_HEAD(&lfs, lf, entries);
			break;
		case 1:
			nloaded++;
			break;
		default:
			/* ignore errors */
			break;
		}
	}
	closedir(libdir);
	if (error && nloaded != 0)
		error = 0;

	/*
	 * El-cheapo dependency calculator.  Just try to load the
	 * modules n times in a loop
	 */
	for (redo = 1; redo;) {
		redo = 0;
		nlf = LIST_FIRST(&lfs);
		while ((lf = nlf) != NULL) {
			nlf = LIST_NEXT(lf, entries);
			if (ukfs_modload(lf->pname) == 1) {
				nloaded++;
				redo = 1;
				LIST_REMOVE(lf, entries);
				free(lf->pname);
				free(lf);
			}
		}
	}

	while ((lf = LIST_FIRST(&lfs)) != NULL) {
		LIST_REMOVE(lf, entries);
		free(lf->pname);
		free(lf);
	}

	if (error && nloaded == 0) {
		errno = error;
		return -1;
	}

	return nloaded;
}

/* XXX: this code uses definitions from NetBSD, needs rumpdefs */
ssize_t
ukfs_vfstypes(char *buf, size_t buflen)
{
	int mib[3];
	struct sysctlnode q, ans[128];
	size_t alen;
	int error, i;

	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = CTL_QUERY;
	alen = sizeof(ans);

	memset(&q, 0, sizeof(q));
	q.sysctl_flags = SYSCTL_VERSION;

	if (rump_sys___sysctl(mib, 3, ans, &alen, &q, sizeof(q), &error) == -1){
		errno = error;
		return -1;
	}

	for (i = 0; i < alen/sizeof(ans[0]); i++)
		if (strcmp("fstypes", ans[i].sysctl_name) == 0)
			break;
	if (i == alen/sizeof(ans[0])) {
		errno = ENXIO;
		return -1;
	}

	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = ans[i].sysctl_num;

	if (rump_sys___sysctl(mib, 3, buf, &buflen, NULL, 0, &error) == -1) {
		errno = error;
		return -1;
	}

	return buflen;
}

/*
 * Utilities
 */
int
ukfs_util_builddirs(struct ukfs *ukfs, const char *pathname, mode_t mode)
{
	char *f1, *f2;
	int rv;
	mode_t mask;
	bool end;

	/*ukfs_umask((mask = ukfs_umask(0)));*/
	umask((mask = umask(0)));

	f1 = f2 = strdup(pathname);
	if (f1 == NULL) {
		errno = ENOMEM;
		return -1;
	}

	end = false;
	for (;;) {
		/* find next component */
		f2 += strspn(f2, "/");
		f2 += strcspn(f2, "/");
		if (*f2 == '\0')
			end = true;
		else
			*f2 = '\0';

		rv = ukfs_mkdir(ukfs, f1, mode & ~mask); 
		if (errno == EEXIST)
			rv = 0;

		if (rv == -1 || *f2 != '\0' || end)
			break;

		*f2 = '/';
	}

	free(f1);

	return rv;
}
