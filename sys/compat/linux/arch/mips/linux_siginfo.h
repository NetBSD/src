/*	$NetBSD: linux_siginfo.h,v 1.2.6.2 2001/09/21 22:35:16 nathanw Exp $ */

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz and Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _MIPS_LINUX_SIGINFO_H
#define _MIPS_LINUX_SIGINFO_H

/* 
 * Everything is from Linux's include/asm-mips/siginfo.h 
 */
typedef union linux_sigval {
	int sival_int;
	void *sival_ptr;
} linux_sigval_t;

#define SI_MAX_SIZE	128
#define SI_PAD_SIZE	((SI_MAX_SIZE/sizeof(int)) - 3)

typedef struct linux_siginfo {
	int lsi_signo;
	int lsi_code;
	int lsi_errno;
	union {
		int _pad[SI_PAD_SIZE];

		/* kill() */
		struct {
			linux_pid_t	_pid;
			linux_uid_t	_uid;
		} _kill;

		/* SIGCHLD */
		struct {
			linux_pid_t	_pid;
			linux_uid_t	_uid;
			linux_clock_t _utime;
			int _status;
			linux_clock_t _stime;
		} _sigchld;

		/* IRIX SIGCHLD */
		struct {
			pid_t _pid;
			linux_clock_t _utime;
			int _status;
			linux_clock_t _stime;
		} _irix_sigchld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void *_addr;
		} _sigfault;

		/* SIGPOLL, SIGXFSZ (To do ...)  */
		struct {
			int _band;
			int _fd;
		} _sigpoll;

		/* POSIX.1b timers */
		struct {
			unsigned int _timer1;
			unsigned int _timer2;
		} _timer;

		/* POSIX.1b signals */
		struct {
			linux_pid_t	_pid;
			linux_uid_t	_uid;
			linux_sigval_t	_sigval;
		} _rt;

	} _sidata;	/* This is _sifields for Linux/mips */
} linux_siginfo_t;

#define lsi_pid _sidata._kill._pid
#define lsi_uid _sidata._kill._uid

/*
 * si_code values
 * Digital reserves positive values for kernel-generated signals.
 */
#define LINUX__SI_CODE(T,N)  ((T) << 16 | ((N) & 0xffff))

#define LINUX_SI_USER		0
#define LINUX_SI_KERNEL		0x80
#define LINUX_SI_QUEUE		-1
#define LINUX_SI_ASYNCIO	-2
#define LINUX_SI_TIMER		LINUX__SI_CODE(__SI_TIMER,-3)
#define LINUX_SI_MESGQ		-4
#define LINUX_SI_SIGIO  	-5

#define LINUX_SI_FROMUSER(siptr)	((siptr)->si_code <= 0)
#define LINUX_SI_FROMKERENL(siptr)	((siptr)->si_code > 0)


#endif /* !_MIPS_LINUX_SIGINFO_H */
