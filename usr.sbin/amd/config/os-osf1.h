/*
 * OSF/1 definitions for Amd (automounter)
 *
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Id: os-osf1.h,v 1.1 1993/11/27 21:19:13 mycroft Exp $
 *
 *	%W% (Berkeley) %G%
 */

/*
 * Does the compiler grok void *
 */
#define	VOIDP

/*
 * Which version of the Sun RPC library we are using
 * This is the implementation release number, not
 * the protocol revision number.
 */
#define	RPC_4

/*
 * Which version of the NFS interface are we using.
 * This is the implementation release number, not
 * the protocol revision number.
 */
#define	NFS_44

/*
 * Support NFS over TCP?
 */
/*#define HAS_TCP_NFS*/

/*
 * Does this OS have NDBM support?
 */
#define OS_HAS_NDBM

/*
 * The mount table is obtained from the kernel
 */
#undef	UPDATE_MTAB

/*
 * No mntent info on OSF/1
 */
#undef	MNTENT_HDR

/*
 * Name of filesystem types
 */
#define	MOUNT_TYPE_NFS	MOUNT_NFS
#define	MOUNT_TYPE_UFS	MOUNT_UFS
#undef MTAB_TYPE_UFS
#define	MTAB_TYPE_UFS	"ufs"
#define	MTAB_TYPE_MFS	"mfs"

/*
 * How to unmount filesystems
 */
#undef UNMOUNT_TRAP
#undef NEED_UMOUNT_FS
#define NEED_UMOUNT_OSF
#undef UMOUNT_FS
#define	UMOUNT_FS(dir)	umount_fs(dir)

/*
 * Byte ordering
 */
#ifndef BYTE_ORDER
#include <machine/endian.h>
#endif /* BYTE_ORDER */

#undef ARCH_ENDIAN
#if BYTE_ORDER == LITTLE_ENDIAN
#define ARCH_ENDIAN "little"
#else
#if BYTE_ORDER == BIG_ENDIAN
#define ARCH_ENDIAN "big"
#else
XXX - Probably no hope of running Amd on this machine!
#endif /* BIG */
#endif /* LITTLE */

/*
 * Miscellaneous OSF/1 bits
 */
/*#define MISC_RPC*/
#define	NEED_MNTOPT_PARSER
#define	SHORT_MOUNT_NAME

#define	MNTMAXSTR       128

#define	MNTTYPE_UFS	"ufs"		/* Un*x file system */
#define	MNTTYPE_NFS	"nfs"		/* network file system */
#define	MNTTYPE_MFS	"mfs"		/* memory file system */
#define	MNTTYPE_IGNORE	"ignore"	/* No type specified, ignore this entry */

#define	MNTOPT_SOFT	"soft"		/* soft mount */
#define	MNTOPT_INTR	"intr"		/* interrupts allowed */
#define MNTOPT_NOCONN	"noconn"	/* accept any responder */
        
struct mntent {
	char	*mnt_fsname;	/* name of mounted file system */
	char	*mnt_dir;	/* file system path prefix */
	char	*mnt_type;	/* MNTTYPE_* */
	char	*mnt_opts;	/* MNTOPT* */
	int	mnt_freq;	/* dump frequency, in days */
	int	mnt_passno;	/* pass number on parallel fsck */
};

/*
 * Type of a file handle
 */
#undef NFS_FH_TYPE
#define	NFS_FH_TYPE	nfsv2fh_t *

/*
 * How to get a mount list
 */
#undef	READ_MTAB_FROM_FILE
#define	READ_MTAB_BSD_STYLE

/*
 * The data for the mount syscall needs the path in addition to the
 * host name since that is the only source of information about the
 * mounted filesystem.
 */
#define	NFS_ARGS_NEEDS_PATH

/*
 * 4.4 has RE support built in
 */
#undef RE_HDR
#define RE_HDR <regexp.h>
