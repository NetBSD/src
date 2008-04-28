/*	$NetBSD: svr4_32_stat.h,v 1.4 2008/04/28 20:23:46 martin Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_SVR4_32_STAT_H_
#define	_SVR4_32_STAT_H_

#include <compat/svr4_32/svr4_32_types.h>
#include <sys/stat.h>
#include <compat/svr4/svr4_stat.h>

struct svr4_32_stat {
	svr4_32_o_dev_t		st_dev;
	svr4_32_o_ino_t		st_ino;
	svr4_32_o_mode_t	st_mode;
	svr4_32_o_nlink_t	st_nlink;
	svr4_32_o_uid_t		st_uid;
	svr4_32_o_gid_t		st_gid;
	svr4_32_o_dev_t		st_rdev;
	svr4_32_off_t		st_size;
	svr4_32_time_t		st_atim;
	svr4_32_time_t		st_mtim;
	svr4_32_time_t		st_ctim;
};

struct svr4_32_xstat {
	svr4_32_dev_t		st_dev;
	netbsd32_long		st_pad1[3];
	svr4_32_ino_t		st_ino;
	svr4_32_mode_t		st_mode;
	svr4_32_nlink_t		st_nlink;
	svr4_32_uid_t		st_uid;
	svr4_32_gid_t		st_gid;
	svr4_32_dev_t		st_rdev;
	netbsd32_long		st_pad2[2];
	svr4_32_off_t		st_size;
	netbsd32_long		st_pad3;
	svr4_32_timestruc_t	st_atim;
	svr4_32_timestruc_t	st_mtim;
	svr4_32_timestruc_t	st_ctim;
	netbsd32_long		st_blksize;
	svr4_32_blkcnt_t	st_blocks;
	char			st_fstype[16];
	netbsd32_long		st_pad4[8];
};
typedef netbsd32_caddr_t svr4_32_xstatp;

struct svr4_32_stat64 {
	svr4_32_dev_t		st_dev;
	netbsd32_long		st_pad1[3];
	svr4_32_ino64_t		st_ino;
	svr4_32_mode_t		st_mode;
	svr4_32_nlink_t		st_nlink;
	svr4_32_uid_t		st_uid;
	svr4_32_gid_t		st_gid;
	svr4_32_dev_t		st_rdev;
	netbsd32_long		st_pad2[2];
	svr4_32_off64_t		st_size;
	svr4_32_timestruc_t	st_atim;
	svr4_32_timestruc_t	st_mtim;
	svr4_32_timestruc_t	st_ctim;
	netbsd32_long		st_blksize;
	svr4_32_blkcnt64_t	st_blocks;
	char			st_fstype[16];
	netbsd32_long		st_pad4[8];
};
typedef netbsd32_caddr_t svr4_32_xstat64p;

#endif /* !_SVR4_32_STAT_H_ */
