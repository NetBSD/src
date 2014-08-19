/*	$NetBSD: linux_types.h,v 1.6.24.1 2014/08/20 00:03:31 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

#ifndef _ALPHA_LINUX_TYPES_H
#define _ALPAH_LINUX_TYPES_H

typedef unsigned int linux_uid_t;
typedef unsigned int linux_gid_t;
typedef unsigned int linux_dev_t;
typedef unsigned int linux_ino_t;
typedef unsigned int linux_mode_t;
typedef unsigned int linux_nlink_t;
typedef long linux_time_t;
typedef long linux_suseconds_t;
typedef long linux_clock_t;
typedef long linux_off_t;
typedef int linux_pid_t;

/* From linux_termios.h */
typedef unsigned char linux_cc_t;
typedef	unsigned int  linux_speed_t;
typedef unsigned int  linux_tcflag_t;

struct linux_stat {
	linux_dev_t		lst_dev;
	linux_ino_t		lst_ino;
	linux_mode_t		lst_mode;
	linux_nlink_t		lst_nlink;
	linux_uid_t		lst_uid;
	linux_gid_t		lst_gid;
	linux_dev_t		lst_rdev;
	linux_off_t		lst_size;
	linux_time_t		lst_atime;	/* Note: Linux uses	*/
	linux_time_t		lst_mtime;	/*       unsigned long	*/
	linux_time_t		lst_ctime;	/*       for these	*/
	unsigned int		lst_blksize;
	int			lst_blocks;
	unsigned int		lst_flags;	/* unused */
	unsigned int		lst_gen;	/* unused */
};

/* The stat64 structure increases the size of dev_t, blkcnt_t, adds
   nanosecond resolution times, and padding for expansion.  */
#define	LINUX_STAT64_HAS_NSEC	1
struct linux_stat64 {
	unsigned long		lst_dev;
	unsigned long		lst_ino;
	unsigned long		lst_rdev;
	long			lst_size;
	unsigned long		lst_blocks;

	unsigned int		lst_mode;
	unsigned int		lst_uid;
	unsigned int		lst_gid;
	unsigned int		lst_blksize;
	unsigned int		lst_nlink;
	unsigned int		__pad0;

	unsigned long		lst_atime;
	unsigned long		lst_atime_nsec; 
	unsigned long		lst_mtime;
	unsigned long		lst_mtime_nsec;
	unsigned long		lst_ctime;
	unsigned long		lst_ctime_nsec;
	long			__pad1[3];
};

#endif /* !_ALPHA_LINUX_TYPES_H */
