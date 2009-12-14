/*	$NetBSD: irix_types.h,v 1.19 2009/12/14 00:47:10 matt Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#ifndef _IRIX_TYPES_H_
#define _IRIX_TYPES_H_

#include <sys/types.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>

/* From IRIX's <limits.h> */
#define IRIX_PATH_MAX 1024
#define IRIX_NAME_MAX 255

/* From IRIX's <sys/signal.h> */
typedef struct {
	__uint32_t bits[4];
} irix_sigset_t;

/* From IRIX's <sys/types.h> */
typedef __int32_t irix_app32_int_t;
typedef __uint32_t irix_app32_ulong_t;
typedef __int32_t irix_app32_long_t;
typedef __uint64_t irix_app32_ulong_long_t;
typedef __int64_t irix_app32_long_long_t;
typedef __uint32_t irix_ino_t;
typedef __int32_t irix_off_t;
typedef __uint32_t irix_app32_ptr_t;
typedef __uint64_t irix_k_sigset_t;

#if _MIPS_SZLONG == 32
typedef unsigned long irix_mode_t;
typedef unsigned long irix_dev_t;
typedef long irix_uid_t;
typedef long irix_gid_t;
typedef unsigned long irix_nlink_t;
typedef long irix_pid_t;
typedef long irix_time_t;
typedef unsigned long irix_size_t;
#endif
#if _MIPS_SZLONG == 64
typedef __uint32_t irix_dev_t;
typedef __uint32_t irix_mode_t;
typedef __int32_t irix_uid_t;
typedef __int32_t irix_gid_t;
typedef __uint32_t irix_nlink_t;
typedef __int32_t irix_pid_t;
typedef __int32_t irix_time_t;
typedef __uint32_t irix_size_t;
#endif
typedef __int32_t irix_blkcnt_t;
typedef __uint64_t irix_ino64_t;
typedef __int64_t irix_off64_t;
typedef __int64_t irix_blkcnt64_t;

/* From IRIX's <sys/ktypes.h> */
typedef irix_app32_long_long_t irix_irix5_n32_off_t;
typedef irix_app32_long_t irix_irix5_uid_t;
typedef irix_app32_long_t irix_irix5_clock_t;
typedef irix_app32_long_t irix_irix5_pid_t;

/* From IRIX's <sys/ktime.h> */
typedef irix_app32_long_t irix_irix5_time_t;
typedef struct irix_irix5_timespec {
	irix_irix5_time_t tv_sec;
	irix_app32_long_t tv_nsec;
} irix_irix5_timespec_t;

/* From IRIX's <sys/timespec.h> */
typedef struct irix___timespec {
	irix_time_t tv_sec;
	long tv_nsec;
} irix_timespec_t;

/* From IRIX's <sys/stat.h> */
#define IRIX__R3_STAT_VER 1
#define IRIX__STAT_VER 2
#define IRIX__STAT64_VER 3
struct irix_stat {
	irix_dev_t	ist_dev;
	__uint32_t	ist_pad1[3];
	irix_ino_t	ist_ino;
	irix_mode_t	ist_mode;
	irix_nlink_t	ist_nlink;
	irix_uid_t	ist_uid;
	irix_gid_t	ist_gid;
	irix_dev_t	ist_rdev;
	__uint32_t	ist_pad2[2];
	irix_off_t	ist_size;
	__uint32_t	ist_pad3;
	irix_timespec_t	ist_atim;
	irix_timespec_t	ist_mtim;
	irix_timespec_t	ist_ctim;
	__int32_t	ist_blksize;
	irix_blkcnt_t	ist_blocks;
	char		ist_fstype[16];
	__int32_t	ist_projid;
	__uint32_t	ist_pad4[7];
};

struct irix_stat64 {
	irix_dev_t	ist_dev;
	__uint32_t	ist_pad1[3];
	irix_ino64_t	ist_ino;
	irix_mode_t	ist_mode;
	irix_nlink_t	ist_nlink;
	irix_uid_t	ist_uid;
	irix_gid_t	ist_gid;
	irix_dev_t	ist_rdev;
	__uint32_t	ist_pad2[2];
	irix_off64_t	ist_size;
	__uint32_t	ist_pad3;
	irix_timespec_t	ist_atim;
	irix_timespec_t	ist_mtim;
	irix_timespec_t	ist_ctim;
	__int32_t	ist_blksize;
	irix_blkcnt64_t	ist_blocks;
	char		ist_fstype[16];
	__int32_t	ist_projid;
	__uint32_t	ist_pad4[7];
};

/* From IRIX's <sys/mount.h> */
typedef struct irix_mountid {
	unsigned int ival[4];
} irix_mountid_t;

/* From IRIX's <sys/dirent.h> */
typedef struct irix_dirent {
	irix_ino_t	d_ino;
	irix_off_t	d_off;
	unsigned short	d_reclen;
	char		d_name[IRIX_NAME_MAX + 1];
} irix_dirent_t;

typedef struct irix_dirent64 {
	irix_ino64_t	d_ino;
	irix_off64_t	d_off;
	unsigned short	d_reclen;
	char		d_name[IRIX_NAME_MAX + 1];
} irix_dirent64_t;

/* From IRIX's <sys/utsname.h> */
#define IRIX_SYS_NMLN 257
struct irix_utsname {
	char sysname[IRIX_SYS_NMLN];
	char nodename[IRIX_SYS_NMLN];
	char release[IRIX_SYS_NMLN];
	char version[IRIX_SYS_NMLN];
	char machine[IRIX_SYS_NMLN];
	char m_type[IRIX_SYS_NMLN];
	char base_rel[IRIX_SYS_NMLN];
	char reserve5[IRIX_SYS_NMLN];
	char reserve4[IRIX_SYS_NMLN];
	char reserve3[IRIX_SYS_NMLN];
	char reserve2[IRIX_SYS_NMLN];
	char reserve1[IRIX_SYS_NMLN];
	char reserve0[IRIX_SYS_NMLN];
};


#endif /* _IRIX_TYPES_H_ */
