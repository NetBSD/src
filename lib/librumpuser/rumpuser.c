/*	$NetBSD: rumpuser.c,v 1.15.4.1 2012/04/17 00:05:33 yamt Exp $	*/

/*
 * Copyright (c) 2007-2010 Antti Kantee.  All Rights Reserved.
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
#if !defined(lint)
__RCSID("$NetBSD: rumpuser.c,v 1.15.4.1 2012/04/17 00:05:33 yamt Exp $");
#endif /* !lint */

/* thank the maker for this */
#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64
#include <features.h>
#endif

#include <sys/param.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/uio.h>

#ifdef __NetBSD__
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/sysctl.h>
#endif

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rump/rumpuser.h>

#include "rumpuser_int.h"

int
rumpuser_getversion()
{

	return RUMPUSER_VERSION;
}

int
rumpuser_getfileinfo(const char *path, uint64_t *sizep, int *ftp, int *error)
{
	struct stat sb;
	uint64_t size;
	int needsdev = 0, rv = 0, ft;
	int fd = -1;

	if (stat(path, &sb) == -1) {
		seterror(errno);
		return -1;
	}

	switch (sb.st_mode & S_IFMT) {
	case S_IFDIR:
		ft = RUMPUSER_FT_DIR;
		break;
	case S_IFREG:
		ft = RUMPUSER_FT_REG;
		break;
	case S_IFBLK:
		ft = RUMPUSER_FT_BLK;
		needsdev = 1;
		break;
	case S_IFCHR:
		ft = RUMPUSER_FT_CHR;
		needsdev = 1;
		break;
	default:
		ft = RUMPUSER_FT_OTHER;
		break;
	}

	if (!needsdev) {
		size = sb.st_size;
	} else if (sizep) {
		/*
		 * Welcome to the jungle.  Of course querying the kernel
		 * for a device partition size is supposed to be far from
		 * trivial.  On NetBSD we use ioctl.  On $other platform
		 * we have a problem.  We try "the lseek trick" and just
		 * fail if that fails.  Platform specific code can later
		 * be written here if appropriate.
		 *
		 * On NetBSD we hope and pray that for block devices nobody
		 * else is holding them open, because otherwise the kernel
		 * will not permit us to open it.  Thankfully, this is
		 * usually called only in bootstrap and then we can
		 * forget about it.
		 */
#ifndef __NetBSD__
		off_t off;

		fd = open(path, O_RDONLY);
		if (fd == -1) {
			seterror(errno);
			rv = -1;
			goto out;
		}

		off = lseek(fd, 0, SEEK_END);
		if (off != 0) {
			size = off;
			goto out;
		}
		fprintf(stderr, "error: device size query not implemented on "
		    "this platform\n");
		seterror(EOPNOTSUPP);
		rv = -1;
		goto out;
#else
		struct disklabel lab;
		struct partition *parta;
		struct dkwedge_info dkw;

		fd = open(path, O_RDONLY);
		if (fd == -1) {
			seterror(errno);
			rv = -1;
			goto out;
		}

		if (ioctl(fd, DIOCGDINFO, &lab) == 0) {
			parta = &lab.d_partitions[DISKPART(sb.st_rdev)];
			size = (uint64_t)lab.d_secsize * parta->p_size;
			goto out;
		}

		if (ioctl(fd, DIOCGWEDGEINFO, &dkw) == 0) {
			/*
			 * XXX: should use DIOCGDISKINFO to query
			 * sector size, but that requires proplib,
			 * so just don't bother for now.  it's nice
			 * that something as difficult as figuring out
			 * a partition's size has been made so easy.
			 */
			size = dkw.dkw_size << DEV_BSHIFT;
			goto out;
		}

		seterror(errno);
		rv = -1;
#endif /* __NetBSD__ */
	}

 out:
	if (rv == 0 && sizep)
		*sizep = size;
	if (rv == 0 && ftp)
		*ftp = ft;
	if (fd != -1)
		close(fd);

	return rv;
}

int
rumpuser_nanosleep(uint64_t *sec, uint64_t *nsec, int *error)
{
	struct timespec rqt, rmt;
	int rv;

	/*LINTED*/
	rqt.tv_sec = *sec;
	/*LINTED*/
	rqt.tv_nsec = *nsec;

	KLOCK_WRAP(rv = nanosleep(&rqt, &rmt));
	if (rv == -1)
		seterror(errno);

	*sec = rmt.tv_sec;
	*nsec = rmt.tv_nsec;

	return rv;
}

void *
rumpuser_malloc(size_t howmuch, int alignment)
{
	void *mem;
	int rv;

	if (alignment == 0)
		alignment = sizeof(void *);

	rv = posix_memalign(&mem, (size_t)alignment, howmuch);
	if (__predict_false(rv != 0)) {
		if (rv == EINVAL) {
			printf("rumpuser_malloc: invalid alignment %d\n",
			    alignment);
			abort();
		}
		mem = NULL;
	}

	return mem;
}

void *
rumpuser_realloc(void *ptr, size_t howmuch)
{

	return realloc(ptr, howmuch);
}

void
rumpuser_free(void *ptr)
{

	free(ptr);
}

void *
rumpuser_anonmmap(void *prefaddr, size_t size, int alignbit,
	int exec, int *error)
{
	void *rv;
	int prot;

	prot = PROT_READ|PROT_WRITE;
	if (exec)
		prot |= PROT_EXEC;
	/* XXX: MAP_ALIGNED() is not portable */
	rv = mmap(prefaddr, size, prot,
	    MAP_ANON | MAP_ALIGNED(alignbit), -1, 0);
	if (rv == MAP_FAILED) {
		seterror(errno);
		return NULL;
	}
	return rv;
}

void
rumpuser_unmap(void *addr, size_t len)
{
	int rv;

	rv = munmap(addr, len);
	assert(rv == 0);
}

void *
rumpuser_filemmap(int fd, off_t offset, size_t len, int flags, int *error)
{
	void *rv;
	int mmflags, prot;

	if (flags & RUMPUSER_FILEMMAP_TRUNCATE)
		ftruncate(fd, offset + len);

	mmflags = MAP_FILE;
	if (flags & RUMPUSER_FILEMMAP_SHARED)
		mmflags |= MAP_SHARED;
	else
		mmflags |= MAP_PRIVATE;

	prot = 0;
	if (flags & RUMPUSER_FILEMMAP_READ)
		prot |= PROT_READ;
	if (flags & RUMPUSER_FILEMMAP_WRITE)
		prot |= PROT_WRITE;

	rv = mmap(NULL, len, PROT_READ|PROT_WRITE, mmflags, fd, offset);
	if (rv == MAP_FAILED) {
		seterror(errno);
		return NULL;
	}

	seterror(0);
	return rv;
}

int
rumpuser_memsync(void *addr, size_t len, int *error)
{

	DOCALL_KLOCK(int, (msync(addr, len, MS_SYNC)));
}

int
rumpuser_open(const char *path, int flags, int *error)
{

	DOCALL(int, (open(path, flags, 0644)));
}

int
rumpuser_ioctl(int fd, u_long cmd, void *data, int *error)
{

	DOCALL_KLOCK(int, (ioctl(fd, cmd, data)));
}

int
rumpuser_close(int fd, int *error)
{

	DOCALL(int, close(fd));
}

int
rumpuser_fsync(int fd, int *error)
{

	DOCALL_KLOCK(int, fsync(fd));
}

ssize_t
rumpuser_read(int fd, void *data, size_t size, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = read(fd, data, size));
	if (rv == -1)
		seterror(errno);

	return rv;
}

ssize_t
rumpuser_pread(int fd, void *data, size_t size, off_t offset, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = pread(fd, data, size, offset));
	if (rv == -1)
		seterror(errno);

	return rv;
}

void
rumpuser_read_bio(int fd, void *data, size_t size, off_t offset,
	rump_biodone_fn biodone, void *biodonecookie)
{
	ssize_t rv;
	int error = 0;

	rv = rumpuser_pread(fd, data, size, offset, &error);
	/* check against <0 instead of ==-1 to get typing below right */
	if (rv < 0)
		rv = 0;

	/* LINTED: see above */
	biodone(biodonecookie, rv, error);
}

ssize_t
rumpuser_write(int fd, const void *data, size_t size, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = write(fd, data, size));
	if (rv == -1)
		seterror(errno);

	return rv;
}

ssize_t
rumpuser_pwrite(int fd, const void *data, size_t size, off_t offset, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = pwrite(fd, data, size, offset));
	if (rv == -1)
		seterror(errno);

	return rv;
}

void
rumpuser_write_bio(int fd, const void *data, size_t size, off_t offset,
	rump_biodone_fn biodone, void *biodonecookie)
{
	ssize_t rv;
	int error = 0;

	rv = rumpuser_pwrite(fd, data, size, offset, &error);
	/* check against <0 instead of ==-1 to get typing below right */
	if (rv < 0)
		rv = 0;

	/* LINTED: see above */
	biodone(biodonecookie, rv, error);
}

ssize_t
rumpuser_readv(int fd, const struct rumpuser_iovec *riov, int iovcnt,
	int *error)
{
	struct iovec *iovp;
	ssize_t rv;
	int i;

	iovp = malloc(iovcnt * sizeof(struct iovec));
	if (iovp == NULL) {
		seterror(ENOMEM);
		return -1;
	}
	for (i = 0; i < iovcnt; i++) {
		iovp[i].iov_base = riov[i].iov_base;
		/*LINTED*/
		iovp[i].iov_len = riov[i].iov_len;
	}

	KLOCK_WRAP(rv = readv(fd, iovp, iovcnt));
	if (rv == -1)
		seterror(errno);
	free(iovp);

	return rv;
}

ssize_t
rumpuser_writev(int fd, const struct rumpuser_iovec *riov, int iovcnt,
	int *error)
{
	struct iovec *iovp;
	ssize_t rv;
	int i;

	iovp = malloc(iovcnt * sizeof(struct iovec));
	if (iovp == NULL) {
		seterror(ENOMEM);
		return -1;
	}
	for (i = 0; i < iovcnt; i++) {
		iovp[i].iov_base = riov[i].iov_base;
		/*LINTED*/
		iovp[i].iov_len = riov[i].iov_len;
	}

	KLOCK_WRAP(rv = writev(fd, iovp, iovcnt));
	if (rv == -1)
		seterror(errno);
	free(iovp);

	return rv;
}

int
rumpuser_gettime(uint64_t *sec, uint64_t *nsec, int *error)
{
	struct timeval tv;
	int rv;

	rv = gettimeofday(&tv, NULL);
	if (rv == -1) {
		seterror(errno);
		return rv;
	}

	*sec = tv.tv_sec;
	*nsec = tv.tv_usec * 1000;

	return 0;
}

int
rumpuser_getenv(const char *name, char *buf, size_t blen, int *error)
{

	DOCALL(int, getenv_r(name, buf, blen));
}

int
rumpuser_gethostname(char *name, size_t namelen, int *error)
{
	char tmp[MAXHOSTNAMELEN];

	if (gethostname(tmp, sizeof(tmp)) == -1) {
		snprintf(name, namelen, "rump-%05d.rumpdomain", getpid());
	} else {
		snprintf(name, namelen, "rump-%05d.%s.rumpdomain",
		    getpid(), tmp);
	}

	*error = 0;
	return 0;
}

int
rumpuser_poll(struct pollfd *fds, int nfds, int timeout, int *error)
{

	DOCALL_KLOCK(int, (poll(fds, (nfds_t)nfds, timeout)));
}

int
rumpuser_putchar(int c, int *error)
{

	DOCALL(int, (putchar(c)));
}

void
rumpuser_exit(int rv)
{

	if (rv == RUMPUSER_PANIC)
		abort();
	else
		exit(rv);
}

void
rumpuser_seterrno(int error)
{

	errno = error;
}

int
rumpuser_writewatchfile_setup(int kq, int fd, intptr_t opaque, int *error)
{
	struct kevent kev;

	if (kq == -1) {
		kq = kqueue();
		if (kq == -1) {
			seterror(errno);
			return -1;
		}
	}

	EV_SET(&kev, fd, EVFILT_VNODE, EV_ADD|EV_ENABLE|EV_CLEAR,
	    NOTE_WRITE, 0, opaque);
	if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1) {
		seterror(errno);
		return -1;
	}

	return kq;
}

int
rumpuser_writewatchfile_wait(int kq, intptr_t *opaque, int *error)
{
	struct kevent kev;
	int rv;

 again:
	KLOCK_WRAP(rv = kevent(kq, NULL, 0, &kev, 1, NULL));
	if (rv == -1) {
		if (errno == EINTR)
			goto again;
		seterror(errno);
		return -1;
	}

	if (opaque)
		*opaque = kev.udata;
	return rv;
}

/*
 * This is meant for safe debugging prints from the kernel.
 */
int
rumpuser_dprintf(const char *format, ...)
{
	va_list ap;
	int rv;

	va_start(ap, format);
	rv = vfprintf(stderr, format, ap);
	va_end(ap);

	return rv;
}

int
rumpuser_kill(int64_t pid, int sig, int *error)
{

#ifdef __NetBSD__
	if (pid == RUMPUSER_PID_SELF) {
		DOCALL(int, raise(sig));
	} else {
		DOCALL(int, kill((pid_t)pid, sig));
	}
#else
	/* XXXfixme: signal numbers may not match on non-NetBSD */
	seterror(EOPNOTSUPP);
	return -1;
#endif
}

int
rumpuser_getnhostcpu(void)
{
	int ncpu;
	size_t sz = sizeof(ncpu);

#ifdef __NetBSD__
	if (sysctlbyname("hw.ncpu", &ncpu, &sz, NULL, 0) == -1)
		return 1;
	return ncpu;
#else
	return 1;
#endif
}

uint32_t
rumpuser_arc4random(void)
{
	return arc4random();
}
