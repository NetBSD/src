/*	$NetBSD: linux_types.h,v 1.1 1998/12/15 19:25:41 itohy Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _M68K_LINUX_TYPES_H
#define _M68K_LINUX_TYPES_H

typedef struct {
	long	val[2];
} linux_fsid_t;

typedef unsigned short linux_uid_t;
typedef unsigned short linux_gid_t;
typedef unsigned short linux_dev_t;
typedef unsigned long linux_ino_t;
typedef unsigned short linux_mode_t;
typedef unsigned short linux_nlink_t;
typedef long linux_time_t;
typedef long linux_clock_t;
typedef long linux_off_t;
typedef int linux_pid_t;

/* From linux_termios.h */
typedef unsigned char linux_cc_t;
typedef unsigned int linux_speed_t;
typedef unsigned int linux_tcflag_t;

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
	long		l_fspare[6];
};

struct linux_stat {
	linux_dev_t		lst_dev;
	unsigned short		pad1;
	linux_ino_t		lst_ino;
	linux_mode_t		lst_mode;
	linux_nlink_t		lst_nlink;
	linux_uid_t		lst_uid;
	linux_gid_t		lst_gid;
	linux_dev_t		lst_rdev;
	unsigned short		pad2;
	linux_off_t		lst_size;
	unsigned long		lst_blksize;
	unsigned long		lst_blocks;
	linux_time_t		lst_atime;
	unsigned long		unused1;
	linux_time_t		lst_mtime;
	unsigned long		unused2;
	linux_time_t		lst_ctime;
	unsigned long		unused3;
	unsigned long		unused4;
	unsigned long		unused5;
};

#endif /* !_M68K_LINUX_TYPES_H */
