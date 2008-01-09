/*	$NetBSD: linux32_types.h,v 1.3.10.1 2008/01/09 01:51:20 matt Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
#ifndef _I386_LINUX32_TYPES_H
#define _I386_LINUX32_TYPES_H

typedef unsigned short linux32_uid_t;
typedef unsigned short linux32_gid_t;
typedef int linux32_pid_t;
typedef int32_t linux32_clock_t;
typedef int32_t linux32_time_t;
typedef int32_t linux32_off_t;

#define LINUX32_STAT64_HAS_NSEC   1
struct linux32_stat64 {
	unsigned long long lst_dev;
	unsigned char __pad0[4];
#define LINUX32_STAT64_HAS_BROKEN_ST_INO  1
	unsigned int __lst_ino;

	unsigned int lst_mode;
	unsigned int lst_nlink;
	unsigned int lst_uid;
	unsigned int lst_gid;

	unsigned long long lst_rdev;
	unsigned char __pad3[4];
	long long   lst_size;
	unsigned int lst_blksize;
	long long lst_blocks;  /* Number 512-byte blocks allocated. */
	unsigned lst_atime;
	unsigned lst_atime_nsec;
	unsigned lst_mtime;
	unsigned lst_mtime_nsec;
	unsigned lst_ctime;
	unsigned lst_ctime_nsec;
	unsigned long long lst_ino;
} __packed;

struct linux32_stat {
	u_int16_t	lst_dev;
	u_int16_t	__pad1;
	u_int32_t	lst_ino;
	u_int16_t	lst_mode;
	u_int16_t	lst_nlink;
	u_int16_t	lst_uid;
	u_int16_t	lst_gid;
	u_int16_t	lst_rdev;
	u_int16_t	__pad2;
	u_int32_t	lst_size;
	u_int32_t	lst_blksize;
	u_int32_t	lst_blocks;
	u_int32_t	lst_atime;
	u_int32_t	lst_atime_nsec;
	u_int32_t	lst_mtime;
	u_int32_t	lst_mtime_nsec;
	u_int32_t	lst_ctime;
	u_int32_t	lst_ctime_nsec;
};

struct linux32_utimbuf {
	linux32_time_t l_actime;
	linux32_time_t l_modtime;
};

#endif /* _I386_LINUX32_TYPES_H */
