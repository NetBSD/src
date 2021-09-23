/*	$NetBSD: linux_siginfo.h,v 1.1 2021/09/23 06:56:27 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _AARCH64_LINUX_SIGINFO_H
#define _AARCH64_LINUX_SIGINFO_H

#define LINUX___ARCH_SI_PREAMBLE_SIZE	(4 * sizeof(int))
#define LINUX_SI_MAX_SIZE		128
#define LINUX_SI_PAD_SIZE		\
	((LINUX_SI_MAX_SIZE - LINUX___ARCH_SI_PREAMBLE_SIZE) / sizeof(int))

typedef union linux_sigval {
	int sival_int;
	void *sival_ptr;
} linux_sigval_t;

typedef struct linux_siginfo {
	int lsi_signo;
	int lsi_errno;
	int lsi_code;
	union {
		int _pad[LINUX_SI_PAD_SIZE];

		/* kill() */
		struct {
			linux_pid_t _pid;
			linux_uid_t _uid;
		} _kill;

		/* POSIX.1b signals */
		struct {
			linux_pid_t _pid;
			linux_uid_t _uid;
			linux_sigval_t _sigval;
		} _rt;

		/* POSIX.1b timers */
		struct {
			linux_timer_t _tid;
			int _overrun;
			char _pad[sizeof(linux_uid_t) - sizeof(int)];
			linux_sigval_t _sigval;
			int _sys_private;
		} _timer;

		/* SIGCHLD */
		struct {
			linux_pid_t _pid;
			linux_uid_t _uid;
			int _status;
			linux_clock_t _utime;
			linux_clock_t _stime;
		} _sigchld;

		/* SIGPOLL */
		struct {
			long _band;
			int _fd;
		} _sigpoll;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGTRAP, SIGEMT */
		struct {
			void *_addr;
#define __ADDR_BND_PKEY_PAD	\
    (__alignof__(void *) < sizeof(short) ? sizeof(short) : __alignof__(void *))
			union {
				short _addr_lsb;
				struct {
					char _dummy_bnd[__ADDR_BND_PKEY_PAD];
					void *_lower;
					void *_upper;
				} _addr_bnd;
				struct {
					char _dummy_pkey[__ADDR_BND_PKEY_PAD];
					uint32_t _pkey;
				} _addr_pkey;
			};
		} _sigfault;
	} _sidata;
} linux_siginfo_t;

#endif /* !_AARCH64_LINUX_SIGINFO_H */
