/*	$NetBSD: svr4_stat.h,v 1.7.8.1 2001/03/12 13:29:55 bouyer Exp $	 */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef	_SVR4_STAT_H_
#define	_SVR4_STAT_H_

#include <compat/svr4/svr4_types.h>
#include <sys/stat.h>

struct svr4_stat {
	svr4_o_dev_t	st_dev;
	svr4_o_ino_t	st_ino;
	svr4_o_mode_t	st_mode;
	svr4_o_nlink_t	st_nlink;
	svr4_o_uid_t	st_uid;
	svr4_o_gid_t	st_gid;
	svr4_o_dev_t	st_rdev;
	svr4_off32_t	st_size;
	svr4_time32_t	st_atim;
	svr4_time32_t	st_mtim;
	svr4_time32_t	st_ctim;
};

struct svr4_xstat {
	svr4_dev_t		st_dev;
#ifndef __LP64__
	long			st_pad1[3];
#endif
	svr4_ino_t		st_ino;
	svr4_mode_t		st_mode;
	svr4_nlink_t		st_nlink;
	svr4_uid_t		st_uid;
	svr4_gid_t		st_gid;
	svr4_dev_t		st_rdev;
#ifndef __LP64__
	long			st_pad2[2];
#endif
	svr4_off_t		st_size;
#ifndef __LP64__
	long			st_pad3;
#endif
	svr4_timestruc_t	st_atim;
	svr4_timestruc_t	st_mtim;
	svr4_timestruc_t	st_ctim;

	int			st_blksize;
	svr4_blkcnt_t		st_blocks;
	char			st_fstype[16];
#ifndef __LP64__
	long			st_pad4[8];
#endif
};

struct svr4_stat64 {
	svr4_dev_t		st_dev;
#ifndef __LP64__
	long			st_pad1[3];
#endif
	svr4_ino64_t		st_ino;
	svr4_mode_t		st_mode;
	svr4_nlink_t		st_nlink;
	svr4_uid_t		st_uid;
	svr4_gid_t		st_gid;
	svr4_dev_t		st_rdev;
#ifndef __LP64__
	long			st_pad2[2];
#endif
	svr4_off64_t		st_size;
	svr4_timestruc_t	st_atim;
	svr4_timestruc_t	st_mtim;
	svr4_timestruc_t	st_ctim;
	int			st_blksize;
	svr4_blkcnt64_t		st_blocks;
	char			st_fstype[16];
#ifndef __LP64__
	long			st_pad4[8];
#endif
};

#define	SVR4_PC_LINK_MAX		1
#define	SVR4_PC_MAX_CANON		2
#define	SVR4_PC_MAX_INPUT		3
#define	SVR4_PC_NAME_MAX		4
#define	SVR4_PC_PATH_MAX		5
#define	SVR4_PC_PIPE_BUF		6
#define	SVR4_PC_NO_TRUNC		7
#define	SVR4_PC_VDISABLE		8
#define	SVR4_PC_CHOWN_RESTRICTED	9
#define	SVR4_PC_ASYNC_IO		10
#define	SVR4_PC_PRIO_IO			11
#define	SVR4_PC_SYNC_IO			12
#define	SVR4_PC_FILESIZEBITS		67

#endif /* !_SVR4_STAT_H_ */
