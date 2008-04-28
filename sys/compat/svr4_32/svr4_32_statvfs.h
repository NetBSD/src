/*	$NetBSD: svr4_32_statvfs.h,v 1.2 2008/04/28 20:23:46 martin Exp $	 */

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

#ifndef	_SVR4_32_STATVFS_H_
#define	_SVR4_32_STATVFS_H_

#include <compat/svr4/svr4_statvfs.h>

typedef struct svr4_32_statvfs {
	netbsd32_u_long		f_bsize;
	netbsd32_u_long		f_frsize;
	svr4_32_fsblkcnt_t	f_blocks;
	svr4_32_fsblkcnt_t	f_bfree;
	svr4_32_fsblkcnt_t	f_bavail;
	svr4_32_fsblkcnt_t	f_files;
	svr4_32_fsblkcnt_t	f_ffree;
	svr4_32_fsblkcnt_t	f_favail;
	netbsd32_u_long		f_fsid;
	char			f_basetype[16];
	netbsd32_u_long		f_flag;
	netbsd32_u_long		f_namemax;
	char			f_fstr[32];
	netbsd32_u_long		f_filler[16];
} svr4_32_statvfs_t;

typedef struct svr4_32_statvfs64 {
	netbsd32_u_long		f_bsize;
	netbsd32_u_long		f_frsize;
	svr4_fsblkcnt64_t	f_blocks;
	svr4_fsblkcnt64_t	f_bfree;
	svr4_fsblkcnt64_t	f_bavail;
	svr4_fsblkcnt64_t	f_files;
	svr4_fsblkcnt64_t	f_ffree;
	svr4_fsblkcnt64_t	f_favail;
	netbsd32_u_long		f_fsid;
	char			f_basetype[16];
	netbsd32_u_long		f_flag;
	netbsd32_u_long		f_namemax;
	char			f_fstr[32];
	netbsd32_u_long		f_filler[16];
} svr4_32_statvfs64_t;

#endif /* !_SVR4_32_STATVFS_H_ */
