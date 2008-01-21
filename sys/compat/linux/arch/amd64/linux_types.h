/*	$NetBSD: linux_types.h,v 1.1.2.2 2008/01/21 09:41:10 yamt Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_LINUX_TYPES_H
#define _AMD64_LINUX_TYPES_H

typedef long linux_off_t;
typedef long linux_clock_t;
typedef long linux_time_t;
typedef unsigned int linux_uid_t;
typedef unsigned int linux_gid_t;
typedef int linux_pid_t;
typedef unsigned int linux_mode_t;
typedef unsigned long linux_nlink_t;
typedef unsigned long linux_dev_t;
typedef unsigned long linux_ino_t;
typedef int linux_timer_t;

#define LINUX_STAT64_HAS_NSEC	1
struct linux_stat64 {
	linux_dev_t	lst_dev;
	linux_ino_t	lst_ino;
	linux_nlink_t	lst_nlink;
	linux_mode_t	lst_mode;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	unsigned int	l__pad0;
	linux_dev_t	lst_rdev;
	long		lst_size;
	long		lst_blksize;
	long		lst_blocks;
	unsigned long	lst_atime;
	unsigned long	lst_atime_nsec;
	unsigned long	lst_mtime;
	unsigned long	lst_mtime_nsec;
	unsigned long	lst_ctime;
	unsigned long	lst_ctime_nsec;
	long	l__unused[3];
};

struct linux_stat {
	linux_dev_t	lst_dev;
	unsigned char	l__pad0[4];
	unsigned int	l__st_ino;
	unsigned int 	lst_nlink;
	linux_mode_t	lst_mode;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	linux_dev_t	lst_rdev;
	unsigned char	l__pad3[4];
	long long	lst_size;
	unsigned int	lst_blksize;
	long long	lst_blocks;
	unsigned 	lst_atime;
	unsigned 	lst_atime_nsec;
	unsigned 	lst_mtime;
	unsigned 	lst_mtime_nsec;
	unsigned 	lst_ctime;
	unsigned	lst_ctime_nsec;
	unsigned long long lst_ino;
} __packed;

#endif /* !_AMD64_LINUX_TYPES_H */
