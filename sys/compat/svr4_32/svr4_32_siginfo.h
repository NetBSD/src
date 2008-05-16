/*	$NetBSD: svr4_32_siginfo.h,v 1.3.40.1 2008/05/16 02:23:48 yamt Exp $	 */

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

#ifndef	_SVR4_32_SIGINFO_H_
#define	_SVR4_32_SIGINFO_H_

#include <compat/svr4/svr4_siginfo.h>

typedef union svr4_32_siginfo {
	char	si_pad[128];	/* Total size; for future expansion */
	struct {
		int					_signo;
		int					_code;
		int					_errno;
		union {
			struct {
				svr4_32_pid_t		_pid;
				svr4_32_clock_t		_utime;
				int			_status;
				svr4_32_clock_t		_stime;
			} _child;

			struct {
				netbsd32_caddr_t	_addr;
				int			_trap;
			} _fault;
		} _reason;
	} _info;
} svr4_32_siginfo_t;
typedef netbsd32_caddr_t svr4_32_siginfo_tp;

#endif /* !_SVR4_32_SIGINFO_H_ */
