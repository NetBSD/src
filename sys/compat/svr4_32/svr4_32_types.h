/*	$NetBSD: svr4_32_types.h,v 1.4.2.1 2007/07/15 13:27:20 ad Exp $	 */

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

#ifndef	_SVR4_32_TYPES_H_
#define	_SVR4_32_TYPES_H_

#include <compat/netbsd32/netbsd32.h>
#include <compat/svr4/svr4_types.h>

typedef u_quad_t	 svr4_32_ino64_t;
typedef quad_t		 svr4_32_off64_t;
typedef quad_t		 svr4_32_blkcnt64_t;
typedef u_quad_t	 svr4_32_fsblkcnt64_t;

typedef netbsd32_long  	 svr4_32_off_t;
typedef netbsd32_u_long	 svr4_32_dev_t;
typedef netbsd32_u_long	 svr4_32_ino_t;
typedef netbsd32_u_long	 svr4_32_mode_t;
typedef netbsd32_u_long	 svr4_32_nlink_t;
typedef netbsd32_long	 svr4_32_uid_t;
typedef netbsd32_long	 svr4_32_gid_t;
typedef netbsd32_long	 svr4_32_daddr_t;
typedef netbsd32_long	 svr4_32_pid_t;
typedef netbsd32_long	 svr4_32_time_t;
typedef netbsd32_long	 svr4_32_blkcnt_t;
typedef netbsd32_u_long	 svr4_32_fsblkcnt_t;
typedef netbsd32_caddr_t svr4_32_caddr_t;
typedef u_int		 svr4_32_size_t;

typedef short		 svr4_32_o_dev_t;
typedef short		 svr4_32_o_pid_t;
typedef u_short		 svr4_32_o_ino_t;
typedef u_short		 svr4_32_o_mode_t;
typedef short		 svr4_32_o_nlink_t;
typedef u_short		 svr4_32_o_uid_t;
typedef u_short		 svr4_32_o_gid_t;
typedef netbsd32_long	 svr4_32_clock_t;
typedef int		 svr4_32_key_t;

typedef struct netbsd32_timespec  svr4_32_timestruc_t;

/* Pointer types used by svr4_32_syscallargs.h */
#define PTR	typedef netbsd32_pointer_t
PTR svr4_32_utimbufp;
PTR svr4_32_tms_tp;
PTR svr4_32_strbuf_tp;
PTR svr4_32_stat_tp;
PTR svr4_32_sigaltstack_tp;
PTR svr4_32_sigaction_tp;
PTR svr4_32_statvfs_tp;
PTR svr4_32_xstat_tp;
PTR svr4_32_rlimit_tp;
PTR svr4_32_lwpid_tp;
PTR svr4_32_aclent_tp;
PTR svr4_32_dirent64_tp;
PTR svr4_32_stat64_tp;
PTR svr4_32_statvfs64_tp;
PTR svr4_32_rlimit64_tp;
PTR svr4_32_statp;
PTR svr4_32_utsnamep;
PTR svr4_32_time_tp;
#undef PTR

#define	svr4_32_omajor(x)	((int32_t)((((x) & 0x7f00) >> 8)))
#define	svr4_32_ominor(x)	((int32_t)((((x) & 0x00ff) >> 0)))
#define	svr4_32_omakedev(x,y)	((svr4_32_o_dev_t)((((x) << 8) & 0x7f00) | \
						(((y) << 0) & 0x00ff)))
#define svr4_32_to_bsd_odev_t(d)	makedev(svr4_32_omajor(d), svr4_32_ominor(d))
#define bsd_to_svr4_32_odev_t(d)	svr4_32_omakedev(major(d), minor(d))

#define	svr4_32_major(x)	((int32_t)((((x) & 0xfffc0000) >> 18)))
#define	svr4_32_minor(x)	((int32_t)((((x) & 0x0003ffff) >>  0)))
#define	svr4_32_makedev(x,y)	((svr4_32_dev_t)((((x) << 18) & 0xfffc0000) | \
					      (((y) <<  0) & 0x0003ffff)))
#define svr4_32_to_bsd_dev_t(d)	makedev(svr4_32_major(d), svr4_32_minor(d))
#define bsd_to_svr4_32_dev_t(d)	svr4_32_makedev(major(d), minor(d))

#endif /* !_SVR4_32_TYPES_H_ */
