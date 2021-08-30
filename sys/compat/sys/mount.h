/*	$NetBSD: mount.h,v 1.15 2021/08/30 08:40:00 riastradh Exp $	*/

/*
 * Copyright (c) 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mount.h	8.21 (Berkeley) 5/20/95
 */

#ifndef _COMPAT_SYS_MOUNT_H_
#define _COMPAT_SYS_MOUNT_H_

#ifdef _KERNEL_OPT
#include "opt_compat_43.h"
#endif

#define MFSNAMELEN	16

struct statfs12 {
	short	f_type;			/* type of file system */
	u_short	f_oflags;		/* deprecated copy of mount flags */
	long	f_bsize;		/* fundamental file system block size */
	long	f_iosize;		/* optimal transfer block size */
	long	f_blocks;		/* total data blocks in file system */
	long	f_bfree;		/* free blocks in fs */
	long	f_bavail;		/* free blocks avail to non-superuser */
	long	f_files;		/* total file nodes in file system */
	long	f_ffree;		/* free file nodes in fs */
	fsid_t	f_fsid;			/* file system id */
	uid_t	f_owner;		/* user that mounted the file system */
	long	f_flags;		/* copy of mount flags */
	long	f_syncwrites;		/* count of sync writes since mount */
	long	f_asyncwrites;		/* count of async writes since mount */
	long	f_spare[1];		/* spare for later */
	char	f_fstypename[MFSNAMELEN]; /* fs type name */
	char	f_mntonname[MNAMELEN];	  /* directory on which mounted */
	char	f_mntfromname[MNAMELEN];  /* mounted file system */
};

#ifndef _KERNEL
#include <string.h>
#endif

/*
 * Operations supported on mounted file system.
 */
/*
 * Convert from a new statvfs to an old statfs structure.
 */

#define MOUNTNO_NONE	0
#define MOUNTNO_UFS	1		/* UNIX "Fast" Filesystem */
#define MOUNTNO_NFS	2		/* Network Filesystem */
#define MOUNTNO_MFS	3		/* Memory Filesystem */
#define MOUNTNO_MSDOS	4		/* MSDOS Filesystem */
#define MOUNTNO_CD9660	5		/* iso9660 cdrom */
#define MOUNTNO_FDESC	6		/* /dev/fd filesystem */
#define MOUNTNO_KERNFS	7		/* kernel variable filesystem */ 
#define MOUNTNO_DEVFS	8		/* device node filesystem */
#define MOUNTNO_AFS	9		/* AFS 3.x */

static const struct {
	const char *name;
	const int value;
} __nv[] = {
	{ MOUNT_UFS, MOUNTNO_UFS },
	{ MOUNT_NFS, MOUNTNO_NFS },
	{ MOUNT_MFS, MOUNTNO_MFS },
	{ MOUNT_MSDOS, MOUNTNO_MSDOS },
	{ MOUNT_CD9660, MOUNTNO_CD9660 },
	{ MOUNT_FDESC, MOUNTNO_FDESC },
	{ MOUNT_KERNFS, MOUNTNO_KERNFS },
	{ MOUNT_AFS, MOUNTNO_AFS },
};

static __inline void
statvfs_to_statfs12(const struct statvfs *fs, struct statfs12 *s12)
{
	size_t i = 0;

	memset(s12, 0, sizeof(*s12));

	s12->f_type = 0;
	s12->f_oflags = (short)fs->f_flag;

	for (i = 0; i < sizeof(__nv) / sizeof(__nv[0]); i++) {
		if (strcmp(__nv[i].name, fs->f_fstypename) == 0) {
			s12->f_type = __nv[i].value;
			break;
		}
	}
#define __STATFSCLAMP(a)	(long)(((a) & ~LONG_MAX) ? LONG_MAX : (a))
	s12->f_bsize = __STATFSCLAMP(fs->f_frsize);
	s12->f_iosize = __STATFSCLAMP(fs->f_iosize);
	s12->f_blocks = __STATFSCLAMP(fs->f_blocks);
	s12->f_bfree = __STATFSCLAMP(fs->f_bfree);
	if (fs->f_bfree > fs->f_bresvd)
		s12->f_bavail = __STATFSCLAMP(fs->f_bfree - fs->f_bresvd);
	else
		s12->f_bavail = -__STATFSCLAMP(fs->f_bresvd - fs->f_bfree);
	s12->f_files = __STATFSCLAMP(fs->f_files);
	s12->f_ffree = __STATFSCLAMP(fs->f_ffree);
	s12->f_fsid = fs->f_fsidx;
	s12->f_owner = fs->f_owner;
	s12->f_flags = (long)fs->f_flag;
	s12->f_syncwrites = __STATFSCLAMP(fs->f_syncwrites);
	s12->f_asyncwrites = __STATFSCLAMP(fs->f_asyncwrites);
	memcpy(s12->f_fstypename, fs->f_fstypename, sizeof(s12->f_fstypename));
	memcpy(s12->f_mntonname, fs->f_mntonname, sizeof(s12->f_mntonname));
	memcpy(s12->f_mntfromname, fs->f_mntfromname,
	    sizeof(s12->f_mntfromname));
}

#ifdef _KERNEL
static __inline int
statvfs_to_statfs12_copy(const void *vs, void *vs12, size_t l)
{
	struct statfs12 *s12 = kmem_zalloc(sizeof(*s12), KM_SLEEP);
	int error;

	statvfs_to_statfs12(vs, s12);
	error = copyout(s12, vs12, sizeof(*s12));
	kmem_free(s12, sizeof(*s12));

	return error;
}

/*
 * Filesystem configuration information. Not used by NetBSD, but
 * defined here to provide a compatible sysctl interface to Lite2.
 */
struct vfsconf {
	struct	vfsops *vfc_vfsops;	/* filesystem operations vector */
	char	vfc_name[MFSNAMELEN]; 	/* filesystem type name */
	int	vfc_typenum;		/* historic filesystem type number */
	int  	vfc_refcount;		/* number mounted of this type */
	int	vfc_flags;		/* permanent flags */
	int	(*vfc_mountroot)(void);	/* if != NULL, routine to mount root */
	struct	vfsconf *vfc_next; 	/* next in list */
};

/* Old, fixed size filehandle structures (used upto (including) 3.x) */
struct compat_30_fid {
	unsigned short	fid_len;
	unsigned short	fid_reserved;
	char		fid_data[16];
};
struct compat_30_fhandle {
	fsid_t	fh_fsid;
	struct compat_30_fid fh_fid;
};

#else

__BEGIN_DECLS
int	__compat_fstatfs(int, struct statfs12 *) __dso_hidden;
int	__compat_getfsstat(struct statfs12 *, long, int) __dso_hidden;
int	__compat_statfs(const char *, struct statfs12 *) __dso_hidden;
int	__compat_getmntinfo(struct statfs12 **, int) __dso_hidden;
#if defined(_NETBSD_SOURCE)
struct compat_30_fhandle;
int	__compat_fhstatfs(const struct compat_30_fhandle *, struct statfs12 *)
    __dso_hidden;
struct stat13;
int	__compat_fhstat(const struct compat_30_fhandle *, struct stat13 *)
    __dso_hidden;
struct stat30;
int	__compat___fhstat30(const struct compat_30_fhandle *, struct stat30 *)
    __dso_hidden;
int	__compat___fhstat40(const void *, size_t, struct stat30 *) __dso_hidden;
struct stat;
int	__fhstat50(const void *, size_t, struct stat *);
#endif /* _NETBSD_SOURCE */
__END_DECLS

#endif /* _KERNEL */

#endif /* !_COMPAT_SYS_MOUNT_H_ */
