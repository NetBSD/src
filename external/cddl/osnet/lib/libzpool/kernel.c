/*     $NetBSD: kernel.c,v 1.5.6.1 2014/05/22 14:01:24 yamt Exp $  */

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/cdefs.h>
__RCSID("$NetBSD: kernel.c,v 1.5.6.1 2014/05/22 14:01:24 yamt Exp $");

#include <sys/zfs_context.h>
#include <sys/sysctl.h>
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <zlib.h>
#include <sys/spa.h>
#include <sys/stat.h>
#include <sys/processor.h>
#include <sys/zmod.h>
#include <sys/utsname.h>

/*
 * Emulation of kernel services in userland.
 */

#ifdef XXXNETBSD
int hz = 119;	/* frequency when using gethrtime() >> 23 for lbolt */
#endif
int aok;
uint64_t physmem;
vnode_t *rootdir = (vnode_t *)0xabcd1234;
char hw_serial[11];
size_t pgsize;

struct utsname utsname = {
	"userland"
};

/* this only exists to have its address taken */
struct proc p0;

/*
 * =========================================================================
 * threads
 * =========================================================================
 */
/*ARGSUSED*/
kthread_t *
zk_thread_create(void (*func)(), void *arg)
{
	thread_t tid;

	VERIFY(thr_create(0, 0, (void *(*)(void *))func, arg, THR_DETACHED,
	    &tid) == 0);

	return ((void *)(uintptr_t)tid);
}

/*
 * =========================================================================
 * kstats
 * =========================================================================
 */
/*ARGSUSED*/
kstat_t *
kstat_create(char *module, int instance, char *name, char *class,
    uchar_t type, ulong_t ndata, uchar_t ks_flag)
{
	return (NULL);
}

/*ARGSUSED*/
void
kstat_install(kstat_t *ksp)
{}

/*ARGSUSED*/
void
kstat_delete(kstat_t *ksp)
{}

/*
 * =========================================================================
 * vnode operations
 * =========================================================================
 */
/*
 * Note: for the xxxat() versions of these functions, we assume that the
 * starting vp is always rootdir (which is true for spa_directory.c, the only
 * ZFS consumer of these interfaces).  We assert this is true, and then emulate
 * them by adding '/' in front of the path.
 */

/*ARGSUSED*/
int
vn_open(char *path, int x1, int flags, int mode, vnode_t **vpp, int x2, int x3)
{
	int fd;
	vnode_t *vp;
	int old_umask;
	char realpath[MAXPATHLEN];
	struct stat64 st;

	/*
	 * If we're accessing a real disk from userland, we need to use
	 * the character interface to avoid caching.  This is particularly
	 * important if we're trying to look at a real in-kernel storage
	 * pool from userland, e.g. via zdb, because otherwise we won't
	 * see the changes occurring under the segmap cache.
	 * On the other hand, the stupid character device returns zero
	 * for its size.  So -- gag -- we open the block device to get
	 * its size, and remember it for subsequent VOP_GETATTR().
	 */
	if (strncmp(path, "/dev/", 5) == 0) {
		char *dsk;
		fd = open64(path, O_RDONLY);
		if (fd == -1)
			return (errno);
		if (fstat64(fd, &st) == -1) {
			close(fd);
			return (errno);
		}
		close(fd);
		(void) sprintf(realpath, "%s", path);
		dsk = strstr(path, "/dsk/");
		if (dsk != NULL)
			(void) sprintf(realpath + (dsk - path) + 1, "r%s",
			    dsk + 1);
	} else {
		(void) sprintf(realpath, "%s", path);
		if (!(flags & FCREAT) && stat64(realpath, &st) == -1)
			return (errno);
	}

	if (flags & FCREAT)
		old_umask = umask(0);

	/*
	 * The construct 'flags - FREAD' conveniently maps combinations of
	 * FREAD and FWRITE to the corresponding O_RDONLY, O_WRONLY, and O_RDWR.
	 */
	fd = open64(realpath, flags - FREAD, mode);

	if (flags & FCREAT)
		(void) umask(old_umask);

	if (fd == -1)
		return (errno);

	if (fstat64(fd, &st) == -1) {
		close(fd);
		return (errno);
	}

	(void) fcntl(fd, F_SETFD, FD_CLOEXEC);

	*vpp = vp = umem_zalloc(sizeof (vnode_t), UMEM_NOFAIL);

	vp->v_fd = fd;
	if (S_ISCHR(st.st_mode)) {
#ifdef XXXAD
		ioctl(fd, DIOCGMEDIASIZE, &vp->v_size);
#endif
	} else
		vp->v_size = st.st_size;
	vp->v_path = spa_strdup(path);

	return (0);
}

int
vn_openat(char *path, int x1, int flags, int mode, vnode_t **vpp, int x2,
    int x3, vnode_t *startvp, int fd)
{
	char *realpath = umem_alloc(strlen(path) + 2, UMEM_NOFAIL);
	int ret;

	ASSERT(startvp == rootdir);
	(void) sprintf(realpath, "/%s", path);

	ret = vn_open(realpath, x1, flags, mode, vpp, x2, x3);

	umem_free(realpath, strlen(path) + 2);

	return (ret);
}

int
vn_getattr(vnode_t *vp, vattr_t *va)
{
	int fd;
	struct stat64 st;

	fd = vp->v_fd;

	if (fstat64(fd, &st) == -1)
		return (errno);

	vp->v_size = st.st_size;
	va->va_size = st.st_size;

	return 0;
}


/*ARGSUSED*/
int
vn_rdwr(int uio, vnode_t *vp, void *addr, ssize_t len, offset_t offset,
	int x1, int x2, rlim64_t x3, void *x4, ssize_t *residp)
{
	ssize_t iolen, split;

	if (uio == UIO_READ) {
		iolen = pread64(vp->v_fd, addr, len, offset);
	} else {
		/*
		 * To simulate partial disk writes, we split writes into two
		 * system calls so that the process can be killed in between.
		 */
		split = (len > 0 ? rand() % len : 0);
		iolen = pwrite64(vp->v_fd, addr, split, offset);
		iolen += pwrite64(vp->v_fd, (char *)addr + split,
		    len - split, offset + split);
	}

	if (iolen == -1)
		return (errno);
	if (residp)
		*residp = len - iolen;
	else if (iolen != len)
		return (EIO);
	return (0);
}

void
vn_close(vnode_t *vp)
{
	close(vp->v_fd);
	spa_strfree(vp->v_path);
	umem_free(vp, sizeof (vnode_t));
}

#ifdef ZFS_DEBUG

/*
 * =========================================================================
 * Figure out which debugging statements to print
 * =========================================================================
 */

static char *dprintf_string;
static int dprintf_print_all;

int
dprintf_find_string(const char *string)
{
	char *tmp_str = dprintf_string;
	int len = strlen(string);

	/*
	 * Find out if this is a string we want to print.
	 * String format: file1.c,function_name1,file2.c,file3.c
	 */

	while (tmp_str != NULL) {
		if (strncmp(tmp_str, string, len) == 0 &&
		    (tmp_str[len] == ',' || tmp_str[len] == '\0'))
			return (1);
		tmp_str = strchr(tmp_str, ',');
		if (tmp_str != NULL)
			tmp_str++; /* Get rid of , */
	}
	return (0);
}

void
dprintf_setup(int *argc, char **argv)
{
	int i, j;

	/*
	 * Debugging can be specified two ways: by setting the
	 * environment variable ZFS_DEBUG, or by including a
	 * "debug=..."  argument on the command line.  The command
	 * line setting overrides the environment variable.
	 */

	for (i = 1; i < *argc; i++) {
		int len = strlen("debug=");
		/* First look for a command line argument */
		if (strncmp("debug=", argv[i], len) == 0) {
			dprintf_string = argv[i] + len;
			/* Remove from args */
			for (j = i; j < *argc; j++)
				argv[j] = argv[j+1];
			argv[j] = NULL;
			(*argc)--;
		}
	}

	if (dprintf_string == NULL) {
		/* Look for ZFS_DEBUG environment variable */
		dprintf_string = getenv("ZFS_DEBUG");
	}

	/*
	 * Are we just turning on all debugging?
	 */
	if (dprintf_find_string("on"))
		dprintf_print_all = 1;
}

/*
 * =========================================================================
 * debug printfs
 * =========================================================================
 */
void
__dprintf(const char *file, const char *func, int line, const char *fmt, ...)
{
	const char *newfile;
	va_list adx;

	/*
	 * Get rid of annoying "../common/" prefix to filename.
	 */
	newfile = strrchr(file, '/');
	if (newfile != NULL) {
		newfile = newfile + 1; /* Get rid of leading / */
	} else {
		newfile = file;
	}

	if (dprintf_print_all ||
	    dprintf_find_string(newfile) ||
	    dprintf_find_string(func)) {
		/* Print out just the function name if requested */
		flockfile(stdout);
		if (dprintf_find_string("pid"))
			(void) printf("%d ", getpid());
		if (dprintf_find_string("tid"))
			(void) printf("%u ", thr_self());
#if 0
		if (dprintf_find_string("cpu"))
			(void) printf("%u ", getcpuid());
#endif
		if (dprintf_find_string("time"))
			(void) printf("%llu ", gethrtime());
		if (dprintf_find_string("long"))
			(void) printf("%s, line %d: ", newfile, line);
		(void) printf("%s: ", func);
		va_start(adx, fmt);
		(void) vprintf(fmt, adx);
		va_end(adx);
		funlockfile(stdout);
	}
}

#endif /* ZFS_DEBUG */

/*
 * =========================================================================
 * cmn_err() and panic()
 * =========================================================================
 */
static char ce_prefix[CE_IGNORE][10] = { "", "NOTICE: ", "WARNING: ", "" };
static char ce_suffix[CE_IGNORE][2] = { "", "\n", "\n", "" };

void
vpanic(const char *fmt, va_list adx)
{
	(void) fprintf(stderr, "error: ");
	(void) vfprintf(stderr, fmt, adx);
	(void) fprintf(stderr, "\n");

	abort();	/* think of it as a "user-level crash dump" */
}

void
panic(const char *fmt, ...)
{
	va_list adx;

	va_start(adx, fmt);
	vpanic(fmt, adx);
	va_end(adx);
}

void
vcmn_err(int ce, const char *fmt, va_list adx)
{
	if (ce == CE_PANIC)
		vpanic(fmt, adx);
	if (ce != CE_NOTE) {	/* suppress noise in userland stress testing */
		(void) fprintf(stderr, "%s", ce_prefix[ce]);
		(void) vfprintf(stderr, fmt, adx);
		(void) fprintf(stderr, "%s", ce_suffix[ce]);
	}
}

/*PRINTFLIKE2*/
void
cmn_err(int ce, const char *fmt, ...)
{
	va_list adx;

	va_start(adx, fmt);
	vcmn_err(ce, fmt, adx);
	va_end(adx);
}

/*
 * =========================================================================
 * kobj interfaces
 * =========================================================================
 */
struct _buf *
kobj_open_file(char *name)
{
	struct _buf *file;
	vnode_t *vp;

	/* set vp as the _fd field of the file */
	if (vn_openat(name, UIO_SYSSPACE, FREAD, 0, &vp, 0, 0, rootdir, 0) != 0)
		return ((void *)-1UL);

	file = umem_zalloc(sizeof (struct _buf), UMEM_NOFAIL);
	file->_fd = (intptr_t)vp;
	return (file);
}

int
kobj_read_file(struct _buf *file, char *buf, unsigned size, unsigned off)
{
	ssize_t resid;

	vn_rdwr(UIO_READ, (vnode_t *)file->_fd, buf, size, (offset_t)off,
	    UIO_SYSSPACE, 0, 0, 0, &resid);

	return (size - resid);
}

void
kobj_close_file(struct _buf *file)
{
	vn_close((vnode_t *)file->_fd);
	umem_free(file, sizeof (struct _buf));
}

int
kobj_get_filesize(struct _buf *file, uint64_t *size)
{
	struct stat64 st;
	vnode_t *vp = (vnode_t *)file->_fd;

	if (fstat64(vp->v_fd, &st) == -1) {
		vn_close(vp);
		return (errno);
	}
	*size = st.st_size;
	return (0);
}

/*
 * =========================================================================
 * misc routines
 * =========================================================================
 */

void
xdelay(clock_t ticks)
{
	poll(0, 0, ticks * (1000 / hz));
}

#if 0
/*
 * Find highest one bit set.
 *	Returns bit number + 1 of highest bit that is set, otherwise returns 0.
 * High order bit is 31 (or 63 in _LP64 kernel).
 */
int
highbit(ulong_t i)
{
	register int h = 1;

	if (i == 0)
		return (0);
#ifdef _LP64
	if (i & 0xffffffff00000000ul) {
		h += 32; i >>= 32;
	}
#endif
	if (i & 0xffff0000) {
		h += 16; i >>= 16;
	}
	if (i & 0xff00) {
		h += 8; i >>= 8;
	}
	if (i & 0xf0) {
		h += 4; i >>= 4;
	}
	if (i & 0xc) {
		h += 2; i >>= 2;
	}
	if (i & 0x2) {
		h += 1;
	}
	return (h);
}
#endif

static int
random_get_bytes_common(uint8_t *ptr, size_t len, char *devname)
{
	int fd = open(devname, O_RDONLY);
	size_t resid = len;
	ssize_t bytes;

	ASSERT(fd != -1);

	while (resid != 0) {
		bytes = read(fd, ptr, resid);
		ASSERT(bytes >= 0);
		ptr += bytes;
		resid -= bytes;
	}

	close(fd);

	return (0);
}

int
random_get_bytes(uint8_t *ptr, size_t len)
{
	return (random_get_bytes_common(ptr, len, "/dev/random"));
}

int
random_get_pseudo_bytes(uint8_t *ptr, size_t len)
{
	return (random_get_bytes_common(ptr, len, "/dev/urandom"));
}

int
ddi_strtoul(const char *hw_serial, char **nptr, int base, unsigned long *result)
{
	char *end;

	*result = strtoul(hw_serial, &end, base);
	if (*result == 0)
		return (errno);
	return (0);
}

/*
 * =========================================================================
 * kernel emulation setup & teardown
 * =========================================================================
 */
static int
umem_out_of_memory(void)
{
	char errmsg[] = "out of memory -- generating core dump\n";

	write(fileno(stderr), errmsg, sizeof (errmsg));
	abort();
	return (0);
}

void
kernel_init(int mode)
{
	umem_nofail_callback(umem_out_of_memory);
	uint64_t physmem;
	size_t len = sizeof(physmem);
	static int mib[2] = { CTL_HW, HW_USERMEM64 };

	if (sysctl(mib, sizeof(mib), &physmem, &len, NULL, 0) != 0) {
		len = 1048576 * 128;
	}

	pgsize = sysconf(_SC_PAGE_SIZE);
	dprintf("physmem = %llu pages (%.2f GB)\n",
	    physmem / pgsize, (double)physmem / (1ULL << 30));

	snprintf(hw_serial, sizeof (hw_serial), "%ld", gethostid());

	system_taskq_init();

	spa_init(mode);
}

void
kernel_fini(void)
{
	spa_fini();
}

int
z_uncompress(void *dst, size_t *dstlen, const void *src, size_t srclen)
{
	int ret;
	uLongf len = *dstlen;

	if ((ret = uncompress(dst, &len, src, srclen)) == Z_OK)
		*dstlen = (size_t)len;

	return (ret);
}

int
z_compress_level(void *dst, size_t *dstlen, const void *src, size_t srclen,
    int level)
{
	int ret;
	uLongf len = *dstlen;

	if ((ret = compress2(dst, &len, src, srclen, level)) == Z_OK)
		*dstlen = (size_t)len;

	return (ret);
}

uid_t
crgetuid(cred_t *cr)
{
	return (0);
}

gid_t
crgetgid(cred_t *cr)
{
	return (0);
}

int
crgetngroups(cred_t *cr)
{
	return (0);
}

gid_t *
crgetgroups(cred_t *cr)
{
	return (NULL);
}

int
zfs_secpolicy_snapshot_perms(const char *name, cred_t *cr)
{
	return (0);
}

int
zfs_secpolicy_rename_perms(const char *from, const char *to, cred_t *cr)
{
	return (0);
}

int
zfs_secpolicy_destroy_perms(const char *name, cred_t *cr)
{
	return (0);
}

ksiddomain_t *
ksid_lookupdomain(const char *dom)
{
	ksiddomain_t *kd;

	kd = umem_zalloc(sizeof (ksiddomain_t), UMEM_NOFAIL);
	kd->kd_name = spa_strdup(dom);
	return (kd);
}

void
ksiddomain_rele(ksiddomain_t *ksid)
{
	spa_strfree(ksid->kd_name);
	umem_free(ksid, sizeof (ksiddomain_t));
}

size_t
ptob(size_t npg)
{

	return npg * pgsize;
}

void
print_timestamp(int fmt)
{

	return;
}

/*
 * Do not change the length of the returned string; it must be freed
 * with strfree().
 */
char *
kmem_asprintf(const char *fmt, ...)
{
	int size;
	va_list adx;
	char *buf;

	va_start(adx, fmt);
	size = vsnprintf(NULL, 0, fmt, adx) + 1;
	va_end(adx);

	buf = kmem_alloc(size, KM_SLEEP);

	va_start(adx, fmt);
	size = vsnprintf(buf, size, fmt, adx);
	va_end(adx);

	return (buf);
}
