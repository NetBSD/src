/*	$NetBSD: fstypes.h,v 1.5 2005/06/23 01:59:31 christos Exp $	*/

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

#ifndef _SYS_FSTYPES_H_
#define _SYS_FSTYPES_H_

typedef struct { int32_t __fsid_val[2]; } fsid_t; /* file system id type */

/*
 * File identifier.
 * These are unique per filesystem on a single machine.
 */
#define	_VFS_MAXFIDSZ	16

struct fid {
	unsigned short	fid_len;		/* length of data in bytes */
	unsigned short	fid_reserved;		/* force longword alignment */
	char		fid_data[_VFS_MAXFIDSZ];/* data (variable length) */
};

#if defined(_NETBSD_SOURCE)
#define	VFS_MAXFIDSZ	_VFS_MAXFIDSZ
#endif

/*
 * Generic file handle
 */
struct fhandle {
	fsid_t	fh_fsid;	/* File system id of mount point */
	struct	fid fh_fid;	/* File sys specific id */
};
typedef struct fhandle	fhandle_t;

/*
 * Mount flags.  XXX BEWARE: these are not in numerical order!
 *
 * Unmount uses MNT_FORCE flag.
 *
 * Note that all mount flags are listed here.  if you need to add one, take
 * one of the __MNT_UNUSED flags.
 */

#define	__MNT_UNUSED4	0x00200000
#define	__MNT_UNUSED5	0x01000000
#define	__MNT_UNUSED6	0x02000000

#define	MNT_RDONLY	0x00000001	/* read only filesystem */
#define	MNT_SYNCHRONOUS	0x00000002	/* file system written synchronously */
#define	MNT_NOEXEC	0x00000004	/* can't exec from filesystem */
#define	MNT_NOSUID	0x00000008	/* don't honor setuid bits on fs */
#define	MNT_NODEV	0x00000010	/* don't interpret special files */
#define	MNT_UNION	0x00000020	/* union with underlying filesystem */
#define	MNT_ASYNC	0x00000040	/* file system written asynchronously */
#define	MNT_NOCOREDUMP	0x00008000	/* don't write core dumps to this FS */
#define MNT_IGNORE	0x00100000	/* don't show entry in df */
#define	MNT_MAGICLINKS	0x00800000	/* interpret symlinks for magic names */
#define MNT_NOATIME	0x04000000	/* Never update access times in fs */
#define MNT_SYMPERM	0x20000000	/* recognize symlink permission */
#define MNT_NODEVMTIME	0x40000000	/* Never update mod times for devs */
#define MNT_SOFTDEP	0x80000000	/* Use soft dependencies */

#define __MNT_BASIC_FLAGS \
	{ MNT_RDONLY,		0,	"read-only" }, \
	{ MNT_SYNCHRONOUS,	0,	"synchronous" }, \
	{ MNT_NOEXEC,		0,	"noexec" }, \
	{ MNT_NOSUID,		0,	"nosuid" }, \
	{ MNT_NODEV,		0,	"nodev" }, \
	{ MNT_UNION,		0,	"union" }, \
	{ MNT_ASYNC,		0,	"asynchronous" }, \
	{ MNT_NOCOREDUMP,	0,	"nocoredump" }, \
	{ MNT_IGNORE,		0,	"hidden" }, \
	{ MNT_NOATIME,		0,	"noatime" }, \
	{ MNT_SYMPERM,		0,	"symperm" }, \
	{ MNT_NODEVMTIME,	0,	"nodevmtime" }, \
	{ MNT_MAGICLINKS,	0,	"magiclinks" }, \
	{ MNT_SOFTDEP,		0,	"soft dependencies" },

/*
 * exported mount flags.
 */
#define	MNT_EXRDONLY	0x00000080	/* exported read only */
#define	MNT_EXPORTED	0x00000100	/* file system is exported */
#define	MNT_DEFEXPORTED	0x00000200	/* exported to the world */
#define	MNT_EXPORTANON	0x00000400	/* use anon uid mapping for everyone */
#define	MNT_EXKERB	0x00000800	/* exported with Kerberos uid mapping */
#define MNT_EXNORESPORT	0x08000000	/* don't enforce reserved ports (NFS) */
#define MNT_EXPUBLIC	0x10000000	/* public export (WebNFS) */

#define __MNT_EXPORTED_FLAGS \
	{ MNT_EXRDONLY,		1,	"exported read-only" }, \
	{ MNT_EXPORTED,		0,	"NFS exported" }, \
	{ MNT_DEFEXPORTED,	1,	"exported to the world" }, \
	{ MNT_EXPORTANON,	1,	"anon uid mapping" }, \
	{ MNT_EXKERB,		1,	"kerberos uid mapping" }, \
	{ MNT_EXNORESPORT,	0,	"non-reserved ports" }, \
	{ MNT_EXPUBLIC,		0,	"WebNFS exports" },

/*
 * Flags set by internal operations.
 */
#define	MNT_LOCAL	0x00001000	/* filesystem is stored locally */
#define	MNT_QUOTA	0x00002000	/* quotas are enabled on filesystem */
#define	MNT_ROOTFS	0x00004000	/* identifies the root filesystem */

#define __MNT_INTERNAL_FLAGS \
	{ MNT_LOCAL,		0,	"local" }, \
	{ MNT_QUOTA,		0,	"with quotas" }, \
	{ MNT_ROOTFS,		1,	"root file system" },

/*
 * Mask of flags that are visible to statvfs()
 */
#define	MNT_VISFLAGMASK	( \
     MNT_RDONLY | \
     MNT_SYNCHRONOUS | \
     MNT_NOEXEC | \
     MNT_NOSUID | \
     MNT_NODEV | \
     MNT_UNION | \
     MNT_ASYNC | \
     MNT_NOCOREDUMP | \
     MNT_IGNORE | \
     MNT_MAGICLINKS | \
     MNT_NOATIME | \
     MNT_SYMPERM | \
     MNT_NODEVMTIME | \
     MNT_SOFTDEP | \
     MNT_EXRDONLY | \
     MNT_EXPORTED | \
     MNT_DEFEXPORTED | \
     MNT_EXPORTANON | \
     MNT_EXKERB | \
     MNT_EXNORESPORT | \
     MNT_EXPUBLIC | \
     MNT_LOCAL | \
     MNT_QUOTA | \
     MNT_ROOTFS)

/*
 * External filesystem control flags.
 */
#define	MNT_UPDATE	0x00010000	/* not a real mount, just an update */
#define	MNT_DELEXPORT	0x00020000	/* delete export host lists */
#define	MNT_RELOAD	0x00040000	/* reload filesystem data */
#define	MNT_FORCE	0x00080000	/* force unmount or readonly change */
#define	MNT_GETARGS	0x00400000	/* retrieve file system specific args */

#define __MNT_EXTERNAL_FLAGS \
	{ MNT_UPDATE,		1,	"being updated" }, \
	{ MNT_DELEXPORT,	1,	"delete export list" }, \
	{ MNT_RELOAD,		1,	"reload filesystem data" }, \
	{ MNT_FORCE,		1,	"force unmount or readonly change" }, \
	{ MNT_GETARGS,		1,	"retrieve mount arguments" },

/*
 * Internal filesystem control flags.
 * These are set in struct mount mnt_iflag.
 *
 * IMNT_UNMOUNT locks the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define	IMNT_GONE	0x00000001	/* filesystem is gone.. */
#define	IMNT_UNMOUNT	0x00000002	/* unmount in progress */
#define	IMNT_WANTRDWR	0x00000004	/* upgrade to read/write requested */
#define	IMNT_SUSPEND	0x00000008	/* request upper write suspension */
#define	IMNT_SUSPENDLOW	0x00000010	/* request lower write suspension */
#define	IMNT_SUSPENDED	0x00000020	/* write operations are suspended */
#define	IMNT_DTYPE	0x00000040	/* returns d_type fields */

#define __MNT_FLAGS \
	__MNT_BASIC_FLAGS \
	__MNT_EXPORTED_FLAGS \
	__MNT_INTERNAL_FLAGS \
	__MNT_EXTERNAL_FLAGS

#define __MNT_FLAG_BITS \
	"\20" \
	"\40MNT_SOFTDEP" \
	"\37MNT_NODEVMTIME" \
	"\36MNT_SYMPERM" \
	"\35MNT_EXPUBLIC" \
	"\34MNT_EXNORESPORT" \
	"\33MNT_NOATIME" \
	"\32MNT_UNUSED6" \
	"\31MNT_UNUSED5" \
	"\30MNT_MAGICLINKS" \
	"\27MNT_GETARGS" \
	"\26MNT_UNUSED4" \
	"\25MNT_IGNORE" \
	"\24MNT_FORCE" \
	"\23MNT_RELOAD" \
	"\22MNT_DELEXPORT" \
	"\21MNT_UPDATE" \
	"\20MNT_NOCOREDUMP" \
	"\17MNT_ROOTFS" \
	"\16MNT_QUOTA" \
	"\15MNT_LOCAL" \
	"\14MNT_EXKERB" \
	"\13MNT_EXPORTANON" \
	"\12MNT_DEFEXPORTED" \
	"\11MNT_EXPORTED" \
	"\10MNT_EXRDONLY" \
	"\07MNT_ASYNC" \
	"\06MNT_UNION" \
	"\05MNT_NODEV" \
	"\04MNT_NOSUID" \
	"\03MNT_NOEXEC" \
	"\02MNT_SYNCHRONOUS" \
	"\01MNT_RDONLY"

#define __IMNT_FLAG_BITS \
	"\20" \
	"\07IMNT_DTYPE" \
	"\06IMNT_SUSPENDED" \
	"\05IMNT_SUSPENDLOW" \
	"\04IMNT_SUSPEND" \
	"\03IMNT_WANTRDWR" \
	"\02IMNT_UNMOUNT" \
	"\01IMNT_GONE"

/*
 * Flags for various system call interfaces.
 *
 * waitfor flags to vfs_sync() and getvfsstat()
 */
#define MNT_WAIT	1	/* synchronously wait for I/O to complete */
#define MNT_NOWAIT	2	/* start all I/O, but do not wait for it */
#define MNT_LAZY 	3	/* push data not written by filesystem syncer */
#endif /* _SYS_FSTYPES_H_ */
