/*	$NetBSD: linux_types.h,v 1.1 2021/09/23 06:56:27 ryo Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _AARCH64_LINUX_TYPES_H
#define _AARCH64_LINUX_TYPES_H

typedef long linux_off_t;
typedef long linux_clock_t;
typedef long linux_time_t;
typedef long linux_suseconds_t;
typedef unsigned int linux_uid_t;
typedef unsigned int linux_gid_t;
typedef int linux_pid_t;
typedef unsigned int linux_mode_t;
typedef unsigned int linux_nlink_t;
typedef unsigned long linux_dev_t;
typedef unsigned long linux_ino_t;
typedef int linux_timer_t;

/* From linux_termios.h */
typedef unsigned char linux_cc_t;
typedef unsigned int linux_speed_t;
typedef unsigned int linux_tcflag_t;

struct linux_stat {
	linux_dev_t	lst_dev;
	linux_ino_t	lst_ino;
	linux_mode_t	lst_mode;
	linux_nlink_t	lst_nlink;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	linux_dev_t	lst_rdev;
	unsigned long	__pad1;
	long		lst_size;
	int		lst_blksize;
	int		__pad2;
	long		lst_blocks;
	long		lst_atime;
	unsigned long	lst_atime_nsec;
	long		lst_mtime;
	unsigned long	lst_mtime_nsec;
	long		lst_ctime;
	unsigned long	lst_ctime_nsec;
	unsigned int	__unused4;
	unsigned int	__unused5;
};

struct linux_stat64 {
	linux_dev_t	lst_dev;
	linux_ino_t	lst_ino;
	linux_mode_t	lst_mode;
	linux_nlink_t	lst_nlink;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	linux_dev_t	lst_rdev;
	unsigned long	__pad1;
	long		lst_size;
	int		lst_blksize;
	int		__pad2;
	long		lst_blocks;
	long		lst_atime;
	unsigned long	lst_atime_nsec;
	long		lst_mtime;
	unsigned long	lst_mtime_nsec;
	long		lst_ctime;
	unsigned long	lst_ctime_nsec;
	unsigned int	__unused4;
	unsigned int	__unused5;
};

#endif /* !_AARCH4_LINUX_TYPES_H */
