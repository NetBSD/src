/*	$NetBSD: siginfo.h,v 1.9 2019/11/18 12:06:26 rin Exp $	 */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#ifndef	_COMPAT_SYS_SIGINFO_H_
#define	_COMPAT_SYS_SIGINFO_H_

#ifdef _KERNEL

/* Avoids circular dependency with machine/netbsd32_machdep.h */
#if defined(__x86_64__) || (defined(__arm__) && defined(__ARM_EABI__))
#define NETBSD32_SIGINFO_UINT64_ALIGN __attribute__((__aligned__(4)))
#else
#define NETBSD32_SIGINFO_UINT64_ALIGN __attribute__((__aligned__(8)))
#endif
typedef uint64_t netbsd32_siginfo_uint64 NETBSD32_SIGINFO_UINT64_ALIGN;
#undef NETBSD32_SIGINFO_UINT64_ALIGN

typedef union sigval32 {
	int sival_int;
	uint32_t sival_ptr;
} sigval32_t;

struct __ksiginfo32 {
	int	_signo;
	int	_code;
	int	_errno;

	union {
		struct {
			pid_t _pid;
			uid_t _uid;
			sigval32_t _value;
		} _rt;

		struct {
			pid_t _pid;
			uid_t _uid;
			int _status;
			clock_t _utime;
			clock_t _stime;
		} _child;

		struct {
			uint32_t _addr;
			int _trap;
		} _fault;

		struct {
			int32_t _band;
			int _fd;
		} _poll;

		struct {
			int	_sysnum;
			int	_retval[2];
			int	_error;
			netbsd32_siginfo_uint64 _args[8]; /* SYS_MAXSYSARGS */
		} _syscall;

		struct {
			int	_pe_report_event;
			union {
				pid_t	_pe_other_pid;
				lwpid_t	_pe_lwp;
			} _option;
		} _ptrace_state;
	} _reason;
};

typedef union siginfo32 {
	char	si_pad[128];
	struct __ksiginfo32 _info;
} siginfo32_t;

#endif /* _KERNEL */

#endif /* !_COMPAT_SYS_SIGINFO_H_ */
