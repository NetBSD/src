/*	$NetBSD: rumpuser.c,v 1.43 2013/04/29 17:31:05 pooka Exp $	*/

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

#include "rumpuser_port.h"

#if !defined(lint)
__RCSID("$NetBSD: rumpuser.c,v 1.43 2013/04/29 17:31:05 pooka Exp $");
#endif /* !lint */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>

#ifdef __NetBSD__
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/sysctl.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
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

struct rumpuser_hyperup rumpuser__hyp;

int
rumpuser_init(int version, const struct rumpuser_hyperup *hyp)
{

	if (version != RUMPUSER_VERSION) {
		fprintf(stderr, "rumpuser mismatch, kern: %d, hypervisor %d\n",
		    version, RUMPUSER_VERSION);
		return 1;
	}

#ifdef RUMPUSER_USE_DEVRANDOM
	uint32_t rv;
	int fd;

	if ((fd = open("/dev/urandom", O_RDONLY)) == -1) {
		srandom(time(NULL));
	} else {
		if (read(fd, &rv, sizeof(rv)) != sizeof(rv))
			srandom(time(NULL));
		else
			srandom(rv);
		close(fd);
	}
#endif

	rumpuser__thrinit();
	rumpuser__hyp = *hyp;

	return 0;
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

/*ARGSUSED1*/
void
rumpuser_free(void *ptr, size_t size)
{

	free(ptr);
}

void *
rumpuser_anonmmap(void *prefaddr, size_t size, int alignbit,
	int exec, int *error)
{
	void *rv;
	int prot;

#ifndef MAP_ALIGNED
#define MAP_ALIGNED(a) 0
	if (alignbit)
		fprintf(stderr, "rumpuser_anonmmap: warning, requested "
		    "alignment not supported by hypervisor\n");
#endif

	prot = PROT_READ|PROT_WRITE;
	if (exec)
		prot |= PROT_EXEC;
	rv = mmap(prefaddr, size, prot,
	    MAP_PRIVATE | MAP_ANON | MAP_ALIGNED(alignbit), -1, 0);
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

int
rumpuser_open(const char *path, int ruflags, int *error)
{
	int flags;

	switch (ruflags & RUMPUSER_OPEN_ACCMODE) {
	case RUMPUSER_OPEN_RDONLY:
		flags = O_RDONLY;
		break;
	case RUMPUSER_OPEN_WRONLY:
		flags = O_WRONLY;
		break;
	case RUMPUSER_OPEN_RDWR:
		flags = O_RDWR;
		break;
	default:
		*error = EINVAL;
		return -1;
	}

#define TESTSET(_ru_, _h_) if (ruflags & _ru_) flags |= _h_;
	TESTSET(RUMPUSER_OPEN_CREATE, O_CREAT);
	TESTSET(RUMPUSER_OPEN_EXCL, O_EXCL);
#undef TESTSET

	DOCALL_KLOCK(int, (open(path, flags, 0644)));
}

int
rumpuser_close(int fd, int *error)
{
	int nlocks;

	rumpkern_unsched(&nlocks, NULL);
	fsync(fd);
	close(fd);
	rumpkern_sched(nlocks, NULL);

	return 0;
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
rumpuser_clock_gettime(uint64_t *sec, uint64_t *nsec, enum rumpclock rclk)
{
	struct timespec ts;
	clockid_t clk;
	int rv;

	switch (rclk) {
	case RUMPUSER_CLOCK_RELWALL:
		clk = CLOCK_REALTIME;
		break;
	case RUMPUSER_CLOCK_ABSMONO:
#ifdef HAVE_CLOCK_NANOSLEEP
		clk = CLOCK_MONOTONIC;
#else
		clk = CLOCK_REALTIME;
#endif
		break;
	default:
		abort();
	}

	rv = clock_gettime(clk, &ts);
	if (rv == -1) {
		return errno;
	}
	*sec = ts.tv_sec;
	*nsec = ts.tv_nsec;

	return 0;
}

int
rumpuser_clock_sleep(uint64_t sec, uint64_t nsec, enum rumpclock clk)
{
	struct timespec rqt, rmt;
	int nlocks;
	int rv;

	rumpkern_unsched(&nlocks, NULL);

	/*LINTED*/
	rqt.tv_sec = sec;
	/*LINTED*/
	rqt.tv_nsec = nsec;

	switch (clk) {
	case RUMPUSER_CLOCK_RELWALL:
		do {
			rv = nanosleep(&rqt, &rmt);
			rqt = rmt;
		} while (rv == -1 && errno == EINTR);
		if (rv == -1) {
			rv = errno;
		}
		break;
	case RUMPUSER_CLOCK_ABSMONO:
		do {
#ifdef HAVE_CLOCK_NANOSLEEP
			rv = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
			    &rqt, NULL);
#else
			/* le/la/der/die/das sigh. timevalspec tailspin */
			struct timespec ts, tsr;
			clock_gettime(CLOCK_REALTIME, &ts);
			if (ts.tv_sec == rqt.tv_sec ?
			    ts.tv_nsec > rqt.tv_nsec : ts.tv_sec > rqt.tv_sec) {
				rv = 0;
			} else {
				tsr.tv_sec = rqt.tv_sec - ts.tv_sec;
				tsr.tv_nsec = rqt.tv_nsec - ts.tv_nsec;
				if (tsr.tv_nsec < 0) {
					tsr.tv_sec--;
					tsr.tv_nsec += 1000*1000*1000;
				}
				rv = nanosleep(&tsr, NULL);
			}
#endif
		} while (rv == -1 && errno == EINTR);
		if (rv == -1) {
			rv = errno;
		}
		break;
	default:
		abort();
	}

	rumpkern_sched(nlocks, NULL);
	return rv;
}

static int
gethostncpu(void)
{
	int ncpu = 1;

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
	size_t sz = sizeof(ncpu);

	sysctlbyname("hw.ncpu", &ncpu, &sz, NULL, 0);
#elif defined(__linux__) || defined(__CYGWIN__)
	FILE *fp;
	char *line = NULL;
	size_t n = 0;

	/* If anyone knows a better way, I'm all ears */
	if ((fp = fopen("/proc/cpuinfo", "r")) != NULL) {
		ncpu = 0;
		while (getline(&line, &n, fp) != -1) {
			if (strncmp(line,
			    "processor", sizeof("processor")-1) == 0)
			    	ncpu++;
		}
		if (ncpu == 0)
			ncpu = 1;
		free(line);
		fclose(fp);
	}
#elif __sun__
	/* XXX: this is just a rough estimate ... */
	ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	
	return ncpu;
}

int
rumpuser_getparam(const char *name, void *buf, size_t blen)
{

	if (strcmp(name, RUMPUSER_PARAM_NCPU) == 0) {
		int ncpu;

		if (getenv_r("RUMP_NCPU", buf, blen) == -1) {
			ncpu = gethostncpu();
			snprintf(buf, blen, "%d", ncpu);
		}
		return 0;
	} else if (strcmp(name, RUMPUSER_PARAM_HOSTNAME) == 0) {
		char tmp[MAXHOSTNAMELEN];

		if (gethostname(tmp, sizeof(tmp)) == -1) {
			snprintf(buf, blen, "rump-%05d", (int)getpid());
		} else {
			snprintf(buf, blen, "rump-%05d.%s",
			    (int)getpid(), tmp);
		}
		return 0;
	} else if (*name == '_') {
		return EINVAL;
	} else {
		if (getenv_r(name, buf, blen) == -1)
			return errno;
		else
			return 0;
	}
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

size_t
rumpuser_getrandom(void *buf, size_t buflen, int flags)
{
	size_t origlen = buflen;
	uint32_t *p = buf;
	uint32_t tmp;
	int chunk;

	do {
		chunk = buflen < 4 ? buflen : 4; /* portable MIN ... */
		tmp = RUMPUSER_RANDOM();
		memcpy(p, &tmp, chunk);
		p++;
		buflen -= chunk;
	} while (chunk);

	return origlen;
}
