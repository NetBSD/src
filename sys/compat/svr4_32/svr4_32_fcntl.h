/*	$NetBSD: svr4_32_fcntl.h,v 1.3.38.1 2008/05/18 12:33:28 yamt Exp $	 */

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

#ifndef	_SVR4_32_FCNTL_H_
#define	_SVR4_32_FCNTL_H_

#include <compat/svr4/svr4_types.h>
#include <compat/svr4_32/svr4_32_types.h>
#include <sys/fcntl.h>
#include <compat/svr4/svr4_fcntl.h>

struct svr4_32_flock_svr3 {
	short		l_type;
	short		l_whence;
	svr4_32_off_t	l_start;
	svr4_32_off_t	l_len;
	short		l_sysid;
	svr4_32_o_pid_t	l_pid;
};
typedef netbsd32_caddr_t svr4_32_flock_svr3p;

struct svr4_32_flock {
	short		l_type;
	short		l_whence;
	svr4_32_off_t	l_start;
	svr4_32_off_t	l_len;
	netbsd32_long	l_sysid;
	svr4_32_pid_t	l_pid;
	netbsd32_long	pad[4];
};
typedef netbsd32_caddr_t svr4_32_flockp;

struct svr4_32_flock64 {
	short		l_type;
	short		l_whence;
	svr4_32_off64_t	l_start;
	svr4_32_off64_t	l_len;
	netbsd32_long	l_sysid;
	svr4_32_pid_t	l_pid;
	netbsd32_long	pad[4];
};
typedef netbsd32_caddr_t svr4_32_flock64p;

#endif /* !_SVR4_32_FCNTL_H_ */
