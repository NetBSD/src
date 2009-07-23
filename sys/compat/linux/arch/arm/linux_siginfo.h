/*	$NetBSD: linux_siginfo.h,v 1.1.138.1 2009/07/23 23:31:39 jym Exp $	*/

/*-
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

#ifndef _ARM_LINUX_SIGINFO_H
#define _ARM_LINUX_SIGINFO_H

typedef union linux_sigval {
	int	sival_int;
	void	*sival_ptr;
} linux_sigval_t;

#define SI_MAX_SIZE	128
#define SI_PAD_SIZE	((SI_MAX_SIZE/sizeof(int)) - 3)

typedef struct linux_siginfo {
	int	lsi_signo;
	int	lsi_errno;
	int	lsi_code;
	union {
		int	_pad[SI_PAD_SIZE];

		/* kill() */
		struct {
			linux_pid_t	_pid;
			linux_uid_t	_uid;
		} _kill;

		/* POSIX.1b signals */
		struct {
			linux_pid_t	_pid;
			linux_uid_t	_uid;
			linux_sigval_t	_sigval;
		} _rt;

		/* POSIX.1b timers */
		struct {
			unsigned int	_timer1;
			unsigned int	_timer2;
		} _timer;

		/* SIGCHLD */
		struct {
			linux_pid_t	_pid;
			linux_uid_t	_uid;
			int		_status;
			linux_clock_t	_utime;
			linux_clock_t	_stime;
		} _sigchld;

		/* SIGPOLL */
		struct {
			int		_band;
			int		_fd;
		} _sigpoll;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void		*_addr;
		} _sigfault;
	} _sidata;
} linux_siginfo_t;

#endif /* !_ARM_LINUX_SIGINFO_H */
