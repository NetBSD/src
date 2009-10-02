/*	$NetBSD: ukfs.c,v 1.37 2009/10/02 09:32:01 pooka Exp $	*/

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
#include <fcntl.h>
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
	void *ukfs_specific;

	pthread_spinlock_t ukfs_spin;
	pid_t ukfs_nextpid;
	struct vnode *ukfs_cdir;
	int ukfs_devfd;
	char *ukfs_devpath;
	char *ukfs_mountpath;
};

static int builddirs(const char *, mode_t,
    int (*mkdirfn)(struct ukfs *, const char *, mode_t), struct ukfs *);

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

void
ukfs_setspecific(struct ukfs *ukfs, void *priv)
{

	ukfs->ukfs_specific = priv;
}

void *
ukfs_getspecific(struct ukfs *ukfs)
{

	return ukfs->ukfs_specific;
}

#ifdef DONT_WANT_PTHREAD_LINKAGE
#define pthread_spin_lock(a)
#define pthread_spin_unlock(a)
#define pthread_spin_init(a,b)
#define pthread_spin_destroy(a)
#endif

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
_ukfs_init(int version)
{
	int rv;

	if (version != UKFS_VERSION) {
		printf("incompatible ukfs version, %d vs. %d\n",
		    version, UKFS_VERSION);
		errno = EPROGMISMATCH;
		return -1;
	}

	if ((rv = rump_init()) != 0) {
		errno = rv;
		return -1;
	}

	return 0;
}

/*ARGSUSED*/
static int
rumpmkdir(struct ukfs *dummy, const char *path, mode_t mode)
{

	return rump_sys_mkdir(path, mode);
}

struct ukfs *
ukfs_mount(const char *vfsname, const char *devpath, const char *mountpath,
	int mntflags, void *arg, size_t alen)
{
	struct stat sb;
	struct ukfs *fs = NULL;
	int rv = 0, devfd = -1, rdonly;
	int mounted = 0;
	int regged = 0;
	int doreg = 0;

	/*
	 * Try open and lock the device.  if we can't open it, assume
	 * it's a file system which doesn't use a real device and let
	 * it slide.  The mount will fail anyway if the fs requires a
	 * device.
	 *
	 * XXX: strictly speaking this is not 100% correct, as virtual
	 * file systems can use a device path which does exist and can
	 * be opened.  E.g. tmpfs should be mountable multiple times
	 * with "device" path "/swap", but now isn't.  But I think the
	 * chances are so low that it's currently acceptable to let
	 * this one slip.
	 */
	rdonly = mntflags & MNT_RDONLY;
	devfd = open(devpath, rdonly ? O_RDONLY : O_RDWR);
	if (devfd != -1) {
		if (fstat(devfd, &sb) == -1) {
			close(devfd);
			devfd = -1;
			rv = errno;
			goto out;
		}

		/*
		 * We do this only for non-block device since the
		 * (NetBSD) kernel allows block device open only once.
		 */
		if (!S_ISBLK(sb.st_mode)) {
			if (flock(devfd, LOCK_NB | (rdonly ? LOCK_SH:LOCK_EX))
			    == -1) {
				warnx("ukfs_mount: cannot get %s lock on "
				    "device", rdonly ? "shared" : "exclusive");
				close(devfd);
				devfd = -1;
				rv = errno;
				goto out;
			}
		} else {
			close(devfd);
			devfd = -1;
		}
		doreg = 1;
	} else if (errno != ENOENT) {
		doreg = 1;
	}

	fs = malloc(sizeof(struct ukfs));
	if (fs == NULL) {
		rv = ENOMEM;
		goto out;
	}
	memset(fs, 0, sizeof(struct ukfs));

	/* create our mountpoint.  this is never removed. */
	if (builddirs(mountpath, 0777, rumpmkdir, NULL) == -1) {
		if (errno != EEXIST) {
			rv = errno;
			goto out;
		}
	}

	if (doreg) {
		rv = rump_etfs_register(devpath, devpath, RUMP_ETFS_BLK);
		if (rv) {
			goto out;
		}
		regged = 1;
	}
	rv = rump_sys_mount(vfsname, mountpath, mntflags, arg, alen);
	if (rv) {
		rv = errno;
		goto out;
	}
	mounted = 1;
	rv = rump_vfs_getmp(mountpath, &fs->ukfs_mp);
	if (rv) {
		goto out;
	}
	rv = rump_vfs_root(fs->ukfs_mp, &fs->ukfs_rvp, 0);
	if (rv) {
		goto out;
	}

	if (regged) {
		fs->ukfs_devpath = strdup(devpath);
	}
	fs->ukfs_mountpath = strdup(mountpath);
	fs->ukfs_cdir = ukfs_getrvp(fs);
	pthread_spin_init(&fs->ukfs_spin, PTHREAD_PROCESS_SHARED);
	fs->ukfs_devfd = devfd;
	assert(rv == 0);

 out:
	if (rv) {
		if (fs) {
			if (fs->ukfs_rvp)
				rump_vp_rele(fs->ukfs_rvp);
			free(fs);
			fs = NULL;
		}
		if (mounted)
			rump_sys_unmount(mountpath, MNT_FORCE);
		if (regged)
			rump_etfs_remove(devpath);
		if (devfd != -1) {
			flock(devfd, LOCK_UN);
			close(devfd);
		}
		errno = rv;
	}

	return fs;
}

int
ukfs_release(struct ukfs *fs, int flags)
{

	if ((flags & UKFS_RELFLAG_NOUNMOUNT) == 0) {
		int rv, mntflag, error;

		ukfs_chdir(fs, "/");
		mntflag = 0;
		if (flags & UKFS_RELFLAG_FORCE)
			mntflag = MNT_FORCE;
		rump_setup_curlwp(nextpid(fs), 1, 1);
		rump_vp_rele(fs->ukfs_rvp);
		fs->ukfs_rvp = NULL;
		rv = rump_sys_unmount(fs->ukfs_mountpath, mntflag);
		if (rv == -1) {
			error = errno;
			rump_vfs_root(fs->ukfs_mp, &fs->ukfs_rvp, 0);
			rump_clear_curlwp();
			ukfs_chdir(fs, fs->ukfs_mountpath);
			errno = error;
			return -1;
		}
		rump_clear_curlwp();
	}

	if (fs->ukfs_devpath) {
		rump_etfs_remove(fs->ukfs_devpath);
		free(fs->ukfs_devpath);
	}
	free(fs->ukfs_mountpath);

	pthread_spin_destroy(&fs->ukfs_spin);
	if (fs->ukfs_devfd != -1) {
		flock(fs->ukfs_devfd, LOCK_UN);
		close(fs->ukfs_devfd);
	}
	free(fs);

	return 0;
}

#define STDCALL(ukfs, thecall)						\
	int rv = 0;							\
									\
	precall(ukfs);							\
	rv = thecall;							\
	postcall(ukfs);							\
	return rv;

int
ukfs_opendir(struct ukfs *ukfs, const char *dirname, struct ukfs_dircookie **c)
{
	struct vnode *vp;
	int rv;

	precall(ukfs);
	rv = rump_namei(RUMP_NAMEI_LOOKUP, RUMP_NAMEI_LOCKLEAF, dirname,
	    NULL, &vp, NULL);
	postcall(ukfs);

	if (rv == 0) {
		RUMP_VOP_UNLOCK(vp, 0);
	} else {
		errno = rv;
		rv = -1;
	}

	/*LINTED*/
	*c = (struct ukfs_dircookie *)vp;
	return rv;
}

static int
getmydents(struct vnode *vp, off_t *off, uint8_t *buf, size_t bufsize)
{
	struct uio *uio;
	size_t resid;
	int rv, eofflag;
	kauth_cred_t cred;
	
	uio = rump_uio_setup(buf, bufsize, *off, RUMPUIO_READ);
	cred = rump_cred_suserget();
	rv = RUMP_VOP_READDIR(vp, uio, cred, &eofflag, NULL, NULL);
	rump_cred_suserput(cred);
	RUMP_VOP_UNLOCK(vp, 0);
	*off = rump_uio_getoff(uio);
	resid = rump_uio_free(uio);

	if (rv) {
		errno = rv;
		return -1;
	}

	/* LINTED: not totally correct return type, but follows syscall */
	return bufsize - resid;
}

/*ARGSUSED*/
int
ukfs_getdents_cookie(struct ukfs *ukfs, struct ukfs_dircookie *c, off_t *off,
	uint8_t *buf, size_t bufsize)
{
	/*LINTED*/
	struct vnode *vp = (struct vnode *)c;

	RUMP_VOP_LOCK(vp, RUMP_LK_SHARED);
	return getmydents(vp, off, buf, bufsize);
}

int
ukfs_getdents(struct ukfs *ukfs, const char *dirname, off_t *off,
	uint8_t *buf, size_t bufsize)
{
	struct vnode *vp;
	int rv;

	precall(ukfs);
	rv = rump_namei(RUMP_NAMEI_LOOKUP, RUMP_NAMEI_LOCKLEAF, dirname,
	    NULL, &vp, NULL);
	postcall(ukfs);
	if (rv) {
		errno = rv;
		return -1;
	}

	rv = getmydents(vp, off, buf, bufsize);
	rump_vp_rele(vp);
	return rv;
}

/*ARGSUSED*/
int
ukfs_closedir(struct ukfs *ukfs, struct ukfs_dircookie *c)
{

	/*LINTED*/
	rump_vp_rele((struct vnode *)c);
	return 0;
}

int
ukfs_open(struct ukfs *ukfs, const char *filename, int flags)
{
	int fd;

	precall(ukfs);
	fd = rump_sys_open(filename, flags, 0);
	postcall(ukfs);
	if (fd == -1)
		return -1;

	return fd;
}

ssize_t
ukfs_read(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize)
{
	int fd;
	ssize_t xfer = -1; /* XXXgcc */

	precall(ukfs);
	fd = rump_sys_open(filename, RUMP_O_RDONLY, 0);
	if (fd == -1)
		goto out;

	xfer = rump_sys_pread(fd, buf, bufsize, off);
	rump_sys_close(fd);

 out:
	postcall(ukfs);
	if (fd == -1) {
		return -1;
	}
	return xfer;
}

/*ARGSUSED*/
ssize_t
ukfs_read_fd(struct ukfs *ukfs, int fd, off_t off, uint8_t *buf, size_t buflen)
{

	return rump_sys_pread(fd, buf, buflen, off);
}

ssize_t
ukfs_write(struct ukfs *ukfs, const char *filename, off_t off,
	uint8_t *buf, size_t bufsize)
{
	int fd;
	ssize_t xfer = -1; /* XXXgcc */

	precall(ukfs);
	fd = rump_sys_open(filename, RUMP_O_WRONLY, 0);
	if (fd == -1)
		goto out;

	/* write and commit */
	xfer = rump_sys_pwrite(fd, buf, bufsize, off);
	if (xfer > 0)
		rump_sys_fsync(fd);

	rump_sys_close(fd);

 out:
	postcall(ukfs);
	if (fd == -1) {
		return -1;
	}
	return xfer;
}

/*ARGSUSED*/
ssize_t
ukfs_write_fd(struct ukfs *ukfs, int fd, off_t off, uint8_t *buf, size_t buflen,
	int dosync)
{
	ssize_t xfer;

	xfer = rump_sys_pwrite(fd, buf, buflen, off);
	if (xfer > 0 && dosync)
		rump_sys_fsync(fd);

	return xfer;
}

/*ARGSUSED*/
int
ukfs_close(struct ukfs *ukfs, int fd)
{

	rump_sys_close(fd);
	return 0;
}

int
ukfs_create(struct ukfs *ukfs, const char *filename, mode_t mode)
{
	int fd;

	precall(ukfs);
	fd = rump_sys_open(filename, RUMP_O_WRONLY | RUMP_O_CREAT, mode);
	if (fd == -1)
		return -1;
	rump_sys_close(fd);

	postcall(ukfs);
	return 0;
}

int
ukfs_mknod(struct ukfs *ukfs, const char *path, mode_t mode, dev_t dev)
{

	STDCALL(ukfs, rump_sys_mknod(path, mode, dev));
}

int
ukfs_mkfifo(struct ukfs *ukfs, const char *path, mode_t mode)
{

	STDCALL(ukfs, rump_sys_mkfifo(path, mode));
}

int
ukfs_mkdir(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	STDCALL(ukfs, rump_sys_mkdir(filename, mode));
}

int
ukfs_remove(struct ukfs *ukfs, const char *filename)
{

	STDCALL(ukfs, rump_sys_unlink(filename));
}

int
ukfs_rmdir(struct ukfs *ukfs, const char *filename)
{

	STDCALL(ukfs, rump_sys_rmdir(filename));
}

int
ukfs_link(struct ukfs *ukfs, const char *filename, const char *f_create)
{

	STDCALL(ukfs, rump_sys_link(filename, f_create));
}

int
ukfs_symlink(struct ukfs *ukfs, const char *filename, const char *linkname)
{

	STDCALL(ukfs, rump_sys_symlink(filename, linkname));
}

ssize_t
ukfs_readlink(struct ukfs *ukfs, const char *filename,
	char *linkbuf, size_t buflen)
{
	ssize_t rv;

	precall(ukfs);
	rv = rump_sys_readlink(filename, linkbuf, buflen);
	postcall(ukfs);
	return rv;
}

int
ukfs_rename(struct ukfs *ukfs, const char *from, const char *to)
{

	STDCALL(ukfs, rump_sys_rename(from, to));
}

int
ukfs_chdir(struct ukfs *ukfs, const char *path)
{
	struct vnode *newvp, *oldvp;
	int rv;

	precall(ukfs);
	rv = rump_sys_chdir(path);
	if (rv == -1)
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
	return rv;
}

/*
 * If we want to use post-time_t file systems on pre-time_t hosts,
 * we must translate the stat structure.  Since we don't currently
 * have a general method for making compat calls in rump, special-case
 * this one.
 *
 * Note that this does not allow making system calls to older rump
 * kernels from newer hosts.
 */
#define VERS_TIMECHANGE 599000700

static int
needcompat(void)
{

#ifdef __NetBSD__
	/*LINTED*/
	return __NetBSD_Version__ < VERS_TIMECHANGE
	    && rump_getversion() >= VERS_TIMECHANGE;
#else
	return 0;
#endif
}

int
ukfs_stat(struct ukfs *ukfs, const char *filename, struct stat *file_stat)
{
	int rv;

	precall(ukfs);
	if (needcompat())
		rv = rump_sys___stat30(filename, file_stat);
	else
		rv = rump_sys_stat(filename, file_stat);
	postcall(ukfs);

	return rv;
}

int
ukfs_lstat(struct ukfs *ukfs, const char *filename, struct stat *file_stat)
{
	int rv;

	precall(ukfs);
	if (needcompat())
		rv = rump_sys___lstat30(filename, file_stat);
	else
		rv = rump_sys_lstat(filename, file_stat);
	postcall(ukfs);

	return rv;
}

int
ukfs_chmod(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	STDCALL(ukfs, rump_sys_chmod(filename, mode));
}

int
ukfs_lchmod(struct ukfs *ukfs, const char *filename, mode_t mode)
{

	STDCALL(ukfs, rump_sys_lchmod(filename, mode));
}

int
ukfs_chown(struct ukfs *ukfs, const char *filename, uid_t uid, gid_t gid)
{

	STDCALL(ukfs, rump_sys_chown(filename, uid, gid));
}

int
ukfs_lchown(struct ukfs *ukfs, const char *filename, uid_t uid, gid_t gid)
{

	STDCALL(ukfs, rump_sys_lchown(filename, uid, gid));
}

int
ukfs_chflags(struct ukfs *ukfs, const char *filename, u_long flags)
{

	STDCALL(ukfs, rump_sys_chflags(filename, flags));
}

int
ukfs_lchflags(struct ukfs *ukfs, const char *filename, u_long flags)
{

	STDCALL(ukfs, rump_sys_lchflags(filename, flags));
}

int
ukfs_utimes(struct ukfs *ukfs, const char *filename, const struct timeval *tptr)
{

	STDCALL(ukfs, rump_sys_utimes(filename, tptr));
}

int
ukfs_lutimes(struct ukfs *ukfs, const char *filename, 
	      const struct timeval *tptr)
{

	STDCALL(ukfs, rump_sys_lutimes(filename, tptr));
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
int
ukfs_modload(const char *fname)
{
	void *handle;
	struct modinfo **mi;
	int error;

	handle = dlopen(fname, RTLD_GLOBAL);
	if (handle == NULL) {
		const char *dlmsg = dlerror();
		if (strstr(dlmsg, "Undefined symbol"))
			return 0;
		warnx("dlopen %s failed: %s\n", fname, dlmsg);
		/* XXXerrno */
		return -1;
	}

	mi = dlsym(handle, "__start_link_set_modules");
	if (mi) {
		error = rump_module_init(*mi, NULL);
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
	int i;

	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = CTL_QUERY;
	alen = sizeof(ans);

	memset(&q, 0, sizeof(q));
	q.sysctl_flags = SYSCTL_VERSION;

	if (rump_sys___sysctl(mib, 3, ans, &alen, &q, sizeof(q)) == -1) {
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

	if (rump_sys___sysctl(mib, 3, buf, &buflen, NULL, 0) == -1) {
		return -1;
	}

	return buflen;
}

/*
 * Utilities
 */
static int
builddirs(const char *pathname, mode_t mode,
	int (*mkdirfn)(struct ukfs *, const char *, mode_t), struct ukfs *fs)
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

		rv = mkdirfn(fs, f1, mode & ~mask); 
		if (errno == EEXIST)
			rv = 0;

		if (rv == -1 || *f2 != '\0' || end)
			break;

		*f2 = '/';
	}

	free(f1);

	return rv;
}

int
ukfs_util_builddirs(struct ukfs *ukfs, const char *pathname, mode_t mode)
{

	return builddirs(pathname, mode, ukfs_mkdir, ukfs);
}
