/*	$NetBSD: linux_types.h,v 1.33 2021/11/25 02:27:08 ryo Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_types.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_types.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_types.h>
#elif defined(__aarch64__)
#include <compat/linux/arch/aarch64/linux_types.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_types.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_types.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_types.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_types.h>
#else
typedef unsigned long linux_clock_t;
typedef long linux_time_t;
typedef long linux_suseconds_t;
#endif

typedef unsigned short linux_gid16_t;
typedef unsigned short linux_uid16_t;
typedef unsigned short linux_umode_t;

/*
 * From Linux include/asm-.../posix_types.h
 */
typedef struct {
	int val[2];
} linux_fsid_t;

/*
 * Structure for uname(2)
 */
struct linux_utsname {
	char l_sysname[65];
	char l_nodename[65];
	char l_release[65];
	char l_version[65];
	char l_machine[65];
	char l_domainname[65];
};

extern char linux_sysname[];
extern char linux_release[];
extern char linux_version[];
extern char linux_machine[];

struct linux_tms {
	linux_clock_t ltms_utime;
	linux_clock_t ltms_stime;
	linux_clock_t ltms_cutime;
	linux_clock_t ltms_cstime;
};

struct linux_timeval {
	linux_time_t tv_sec;
	linux_suseconds_t tv_usec;
};

struct linux_utimbuf {
	linux_time_t l_actime;
	linux_time_t l_modtime;
};

struct linux___sysctl {
	int          *name;
	int           nlen;
	void         *oldval;
	size_t       *oldlenp;
	void         *newval;
	size_t        newlen;
	unsigned long __unused0[4];
};

#include <compat/linux/common/linux_siginfo.h>

/*
 * Generic statfs structure from Linux include/asm-generic/statfs.h
 * Linux/x86_64 uses different (fully 64bit) struct statfs.
 */
#ifdef LINUX_STATFS_64BIT
struct linux_statfs {
	long		l_ftype;
	long		l_fbsize;
	long		l_fblocks;
	long		l_fbfree;
	long		l_fbavail;
	long		l_ffiles;
	long		l_fffree;
	linux_fsid_t	l_ffsid;
	long		l_fnamelen;
	long		l_ffrsize;
	long		l_fspare[5];
};

struct linux_statfs64 {
	long		l_ftype;
	long		l_fbsize;
	long		l_fblocks;
	long		l_fbfree;
	long		l_fbavail;
	long		l_ffiles;
	long		l_fffree;
	linux_fsid_t	l_ffsid;
	long		l_fnamelen;
	long		l_ffrsize;
	long		l_fspare[5];
};
#else /* !LINUX_STATFS_64BIT */
struct linux_statfs {
	u_int32_t	l_ftype;
	u_int32_t	l_fbsize;
	u_int32_t	l_fblocks;
	u_int32_t	l_fbfree;
	u_int32_t	l_fbavail;
	u_int32_t	l_ffiles;
	u_int32_t	l_fffree;
	linux_fsid_t	l_ffsid;
	u_int32_t	l_fnamelen;
	u_int32_t	l_ffrsize;
	u_int32_t	l_fspare[5];
};

struct linux_statfs64 {
	u_int32_t	l_ftype;
	u_int32_t	l_fbsize;
	u_int64_t	l_fblocks;
	u_int64_t	l_fbfree;
	u_int64_t	l_fbavail;
	u_int64_t	l_ffiles;
	u_int64_t	l_fffree;
	linux_fsid_t	l_ffsid;
	u_int32_t	l_fnamelen;
	u_int32_t	l_ffrsize;
	u_int32_t	l_fspare[5];
};
#endif /* !LINUX_STATFS_64BIT */

struct linux_statx_timestamp {
	int64_t tv_sec;
	uint32_t tv_nsec;
	int32_t __reserved;
};

#define STATX_TYPE		0x00000001
#define STATX_MODE		0x00000002
#define STATX_NLINK		0x00000004
#define STATX_UID		0x00000008
#define STATX_GID		0x00000010
#define STATX_ATIME		0x00000020
#define STATX_MTIME		0x00000040
#define STATX_CTIME		0x00000080
#define STATX_INO		0x00000100
#define STATX_SIZE		0x00000200
#define STATX_BLOCKS		0x00000400
#define STATX_BASIC_STATS	0x000007ff
#define STATX_BTIME		0x00000800
#define STATX_MNT_ID		0x00001000
#define STATX_ALL		0x00000fff
#define STATX__RESERVED		0x80000000

#define STATX_ATTR_COMPRESSED	0x00000004
#define STATX_ATTR_IMMUTABLE	0x00000010
#define STATX_ATTR_APPEND	0x00000020
#define STATX_ATTR_NODUMP	0x00000040
#define STATX_ATTR_ENCRYPTED	0x00000800
#define STATX_ATTR_AUTOMOUNT	0x00001000
#define STATX_ATTR_MOUNT_ROOT	0x00002000
#define STATX_ATTR_VERITY	0x00100000
#define STATX_ATTR_DAX		0x00200000

struct linux_statx {
	uint32_t stx_mask;
	uint32_t stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint16_t __pad1;
	uint64_t stx_ino;
	uint64_t stx_size;
	uint64_t stx_blocks;
	uint64_t stx_attributes_mask;
	struct linux_statx_timestamp stx_atime;
	struct linux_statx_timestamp stx_btime;
	struct linux_statx_timestamp stx_ctime;
	struct linux_statx_timestamp stx_mtime;
	uint32_t stx_rdev_major;
	uint32_t stx_rdev_minor;
	uint32_t stx_dev_major;
	uint32_t stx_dev_minor;
	uint64_t stx_mnt_id;
	uint64_t __spare2;
	uint64_t __spare3[12];
} __packed;

#endif /* !_LINUX_TYPES_H */
