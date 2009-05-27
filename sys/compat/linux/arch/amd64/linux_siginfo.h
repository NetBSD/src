/*	$NetBSD: linux_siginfo.h,v 1.5 2009/05/27 12:20:37 njoly Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_LINUX_SIGINFO_H
#define _AMD64_LINUX_SIGINFO_H

#define LINUX___ARCH_SI_PREAMBLE_SIZE (3 * sizeof(int))
#define LINUX_SI_MAX_SIZE 128
#define LINUX_SI_PAD_SIZE \
    ((LINUX_SI_MAX_SIZE - LINUX___ARCH_SI_PREAMBLE_SIZE) / sizeof(int))

typedef union linux_sigval {
	int sival_int;
	void *sival_ptr;
} linux_sigval_t;


struct linux_siginfo {
	int lsi_signo;
	int lsi_errno;
	int lsi_code;
	union {
		int _pad[LINUX_SI_PAD_SIZE];
		struct {
			linux_pid_t _pid;
			linux_uid_t _uid;
		} _kill;

		struct {
			linux_timer_t _tid;
			int _overrun;
			char _pad[sizeof(linux_uid_t) - sizeof(int)];
			linux_sigval_t _sigval;
			int _sys_private;
		} _timer;

		struct {
			linux_pid_t _pid;
			linux_uid_t _uid;
			int _status;
			linux_clock_t _utime;
			linux_clock_t _stime;
		} _sigchld;

		struct {
			void *_addr;
		} _sigfault;

		struct {
			long _band;
			int _fd;
		} _sigpoll;
	} _sifields;
} linux_siginfo_t;

#endif /* !_AMD64_LINUX_SIGINFO_H */
