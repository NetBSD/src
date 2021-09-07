/*	$NetBSD: statvfs.h,v 1.4 2021/09/07 11:43:05 riastradh Exp $	 */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_COMPAT_SYS_STATVFS_H_
#define	_COMPAT_SYS_STATVFS_H_

#include <sys/statvfs.h>

struct statvfs90 {
	unsigned long	f_flag;		/* copy of mount exported flags */
	unsigned long	f_bsize;	/* file system block size */
	unsigned long	f_frsize;	/* fundamental file system block size */
	unsigned long	f_iosize;	/* optimal file system block size */

	/* The following are in units of f_frsize */
	fsblkcnt_t	f_blocks;	/* number of blocks in file system, */
	fsblkcnt_t	f_bfree;	/* free blocks avail in file system */
	fsblkcnt_t	f_bavail;	/* free blocks avail to non-root */
	fsblkcnt_t	f_bresvd;	/* blocks reserved for root */

	fsfilcnt_t	f_files;	/* total file nodes in file system */
	fsfilcnt_t	f_ffree;	/* free file nodes in file system */
	fsfilcnt_t	f_favail;	/* free file nodes avail to non-root */
	fsfilcnt_t	f_fresvd;	/* file nodes reserved for root */

	uint64_t  	f_syncreads;	/* count of sync reads since mount */
	uint64_t  	f_syncwrites;	/* count of sync writes since mount */

	uint64_t  	f_asyncreads;	/* count of async reads since mount */
	uint64_t  	f_asyncwrites;	/* count of async writes since mount */

	fsid_t		f_fsidx;	/* NetBSD compatible fsid */
	unsigned long	f_fsid;		/* Posix compatible fsid */
	unsigned long	f_namemax;	/* maximum filename length */
	uid_t		f_owner;	/* user that mounted the file system */

	uint32_t	f_spare[4];	/* spare space */

	char	f_fstypename[_VFS_NAMELEN]; /* fs type name */
	char	f_mntonname[_VFS_MNAMELEN];  /* directory on which mounted */
	char	f_mntfromname[_VFS_MNAMELEN];  /* mounted file system */
};

__BEGIN_DECLS
#ifndef _KERNEL
#include <string.h>
#endif

static __inline void
statvfs_to_statvfs90(const struct statvfs *s, struct statvfs90 *s90)
{

	memset(s90, 0, sizeof(*s90));

	s90->f_flag = s->f_flag;
	s90->f_bsize = s->f_bsize;
	s90->f_frsize = s->f_frsize;
	s90->f_iosize = s->f_iosize;

	s90->f_blocks = s->f_blocks;
	s90->f_bfree = s->f_bfree;
	s90->f_bavail = s->f_bavail;
	s90->f_bresvd = s->f_bresvd;

	s90->f_files = s->f_files;
	s90->f_ffree = s->f_ffree;
	s90->f_favail = s->f_favail;
	s90->f_fresvd = s->f_fresvd;

	s90->f_syncreads = s->f_syncreads;
	s90->f_syncwrites = s->f_syncwrites;

	s90->f_asyncreads = s->f_asyncreads;
	s90->f_asyncwrites = s->f_asyncwrites;

	s90->f_fsidx = s->f_fsidx;
	s90->f_fsid = s->f_fsid;
	s90->f_namemax = s->f_namemax;
	s90->f_owner = s->f_owner;

	memcpy(s90->f_fstypename, s->f_fstypename, sizeof(s90->f_fstypename));
	memcpy(s90->f_mntonname, s->f_mntonname, sizeof(s90->f_mntonname));
	memcpy(s90->f_mntfromname, s->f_mntfromname, sizeof(s90->f_mntfromname));
}

#ifdef _KERNEL
static __inline int
statvfs_to_statvfs90_copy(const void *vs, void *vs90, size_t l)
{
	struct statvfs90 *s90 = kmem_zalloc(sizeof(*s90), KM_SLEEP);
	int error;

	statvfs_to_statvfs90(vs, s90);
	error = copyout(s90, vs90, sizeof(*s90));
	kmem_free(s90, sizeof(*s90));

	return error;
}
#else

#ifdef __LIBC12_SOURCE__

int	__compat_statvfs(const char *__restrict, struct statvfs90 *__restrict);
int	__compat_statvfs1(const char *__restrict, struct statvfs90 *__restrict,
    int);

int	__compat_fstatvfs(int, struct statvfs90 *);
int	__compat_fstatvfs1(int, struct statvfs90 *, int);

int	__compat___getmntinfo13(struct statvfs90 **, int);

int	__compat___fhstatvfs40(const void *, size_t, struct statvfs90 *);
int	__compat___fhstatvfs140(const void *, size_t, struct statvfs90 *, int);

int	__compat_getvfsstat(struct statvfs90 *, size_t, int);

int	__statvfs90(const char *__restrict, struct statvfs *__restrict);
int	__statvfs190(const char *__restrict, struct statvfs *__restrict, int);

int	__fstatvfs90(int, struct statvfs *);
int	__fstatvfs190(int, struct statvfs *, int);

int	__fhstatvfs90(const void *, size_t, struct statvfs *);
int	__fhstatvfs190(const void *, size_t, struct statvfs *, int);

int	__getvfsstat90(struct statvfs *, size_t, int);

int	__getmntinfo90(struct statvfs **, int);

#endif /* __LIBC12_SOURCE__ */

#endif /* _KERNEL */

__END_DECLS

#endif /* !_COMPAT_SYS_STATVFS_H_ */
