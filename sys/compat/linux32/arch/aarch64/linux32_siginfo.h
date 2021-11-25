/*	$NetBSD: linux32_siginfo.h,v 1.1 2021/11/25 03:08:04 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ryo Shimizu.
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

#ifndef _AARCH64_LINUX32_SIGINFO_H_
#define _AARCH64_LINUX32_SIGINFO_H_

#include <compat/linux32/common/linux32_sigevent.h>

#define SI_MAX_SIZE	128
#define SI_PAD_SIZE	((SI_MAX_SIZE/sizeof(int)) - 3)	/* 3=signo,errno,pad */

typedef struct linux32_siginfo {
	int lsi_signo;
	int lsi_errno;
	int lsi_code;
	union {
		int _pad[SI_PAD_SIZE];

		/* kill() */
		struct {
			linux32_pid_t	_pid;
			linux32_uid_t	_uid;
		} _kill;

		/* POSIX.1b timers */
		struct {
			int _tid;
			int _overrun;
			linux32_sigval_t _sigval;
			int _sys_private;
		} _timer;

		/* POSIX.1b signals */
		struct {
			linux32_pid_t _pid;
			linux32_uid_t _uid;
			linux32_sigval_t _sigval;
		} _rt;

		/* SIGCHLD */
		struct {
			linux32_pid_t _pid;
			linux32_uid_t _uid;
			int _status;
			linux32_clock_t _utime;
			linux32_clock_t _stime;
		} _sigchld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGTRAP, SIGEMT */
		struct {
			netbsd32_voidp _addr;
			/* ... */
		} _sigfault;

		/* SIGPOLL */
		struct {
			int _band;
			int _fd;
		} _sigpoll;

		/* SIGSYS */
		struct {
			netbsd32_voidp _call_addr;
			int _syscall;
			unsigned int _arch;
		} _sigsys;
	} _sidata;
} linux32_siginfo_t;

#endif /* _AARCH64_LINUX32_SIGINFO_H_ */
