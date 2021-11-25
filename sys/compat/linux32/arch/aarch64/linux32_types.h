/*	$NetBSD: linux32_types.h,v 1.1 2021/11/25 03:08:04 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ryo Shimizu.
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

#ifndef _AARCH64_LINUX32_TYPES_H_
#define _AARCH64_LINUX32_TYPES_H_

typedef uint16_t linux32_uid_t;		/* arm: unsigned short */
typedef uint16_t linux32_gid_t;		/* arm: unsigned short */
typedef uint32_t linux32_ino_t;		/* arm: unsigned long */
typedef uint32_t linux32_size_t;	/* arm: size_t */
typedef int32_t linux32_time_t;		/* arm: long */
typedef int32_t linux32_clock_t;	/* arm: long */
typedef int32_t linux32_off_t;		/* arm: long */
typedef int32_t linux32_pid_t;		/* arm: int */

typedef netbsd32_pointer_t linux32_ulongp_t;
typedef netbsd32_pointer_t linux32_user_descp_t;

struct linux32_stat {
	uint16_t			lst_dev;
	uint16_t			pad1;
	linux32_ino_t			lst_ino;
	uint16_t			lst_mode;
	uint16_t			lst_nlink;
	linux32_uid_t			lst_uid;
	linux32_gid_t			lst_gid;
	uint16_t			lst_rdev;
	uint16_t			pad2;
	linux32_off_t			lst_size;
	linux32_size_t			lst_blksize;
	uint32_t			lst_blocks;
	linux32_time_t			lst_atime;
	uint32_t			unused1;
	linux32_time_t			lst_mtime;
	uint32_t			unused2;
	linux32_time_t			lst_ctime;
	uint32_t			unused3;
	uint32_t			unused4;
	uint32_t			unused5;
} __packed;

struct linux32_stat64 {
	unsigned long long		lst_dev;
#define LINUX32_STAT64_HAS_BROKEN_ST_INO
	unsigned long long		__lst_ino;
	uint32_t			lst_mode;
	uint32_t			lst_nlink;
	uint32_t			lst_uid;
	uint32_t			lst_gid;
	unsigned long long		lst_rdev;
	unsigned long long		__pad1;
	long long			lst_size;
	uint32_t			lst_blksize;
	uint32_t			__pad2;
	unsigned long long		lst_blocks;
#define LINUX32_STAT64_HAS_NSEC
	uint32_t			lst_atime;
	uint32_t			lst_atime_nsec;
	uint32_t			lst_mtime;
	uint32_t			lst_mtime_nsec;
	uint32_t			lst_ctime;
	uint32_t			lst_ctime_nsec;
	unsigned long long		lst_ino;
} __packed;

struct linux32_utimbuf {
	linux32_time_t		l_actime;
	linux32_time_t		l_modtime;
};

#endif /* _AARCH64_LINUX32_TYPES_H_ */
