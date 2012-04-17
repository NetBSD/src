/*	$NetBSD: linux32_sched.h,v 1.1.8.2 2012/04/17 00:07:19 yamt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#ifndef _LINUX32_SCHED_H
#define	_LINUX32_SCHED_H

/*
 * Flags passed to the Linux __clone(2) system call.
 */
#define	LINUX32_CLONE_CSIGNAL	0x000000ff	/* signal to be sent at exit */
#define	LINUX32_CLONE_VM		0x00000100	/* share address space */
#define	LINUX32_CLONE_FS		0x00000200	/* share "file system" info */
#define	LINUX32_CLONE_FILES	0x00000400	/* share file descriptors */
#define	LINUX32_CLONE_SIGHAND	0x00000800	/* share signal actions */
#define	LINUX32_CLONE_PID		0x00001000	/* share process ID */
#define	LINUX32_CLONE_PTRACE	0x00002000	/* ptrace(2) continues on
						   child */
#define	LINUX32_CLONE_VFORK	0x00004000	/* parent blocks until child
						   exits */
#define LINUX32_CLONE_PARENT	0x00008000	/* want same parent as cloner */
#define LINUX32_CLONE_THREAD	0x00010000	/* same thread group */
#define LINUX32_CLONE_NEWNS	0x00020000	/* new namespace group */
#define LINUX32_CLONE_SYSVSEM	0x00040000	/* share SysV SEM_UNDO */
#define LINUX32_CLONE_SETTLS	0x00080000	/* create new TLS for child */
#define LINUX32_CLONE_PARENT_SETTID \
				0x00100000	/* set TID in the parent */
#define LINUX32_CLONE_CHILD_CLEARTID \
				0x00200000	/* clear TID in the child */
#define LINUX32_CLONE_DETACHED	0x00400000	/* unused */
#define LINUX32_CLONE_UNTRACED	0x00800000	/* set if parent cannot force CLONE_PTRACE */
#define LINUX32_CLONE_CHILD_SETTID \
				0x01000000	/* set TID in the child */
#define LINUX32_CLONE_STOPPED	0x02000000	/* start in stopped state */

struct linux32_sched_param {
	int	sched_priority;
};

#define LINUX32_SCHED_OTHER	0
#define LINUX32_SCHED_FIFO	1
#define LINUX32_SCHED_RR		2

struct linux32_timespec {
	linux32_time_t	tv_sec;		/* seconds */
	int		tv_nsec;	/* nanoseconds */
};

#define LINUX32_CLOCK_REALTIME		0
#define LINUX32_CLOCK_MONOTONIC		1
#define LINUX32_CLOCK_PROCESS_CPUTIME_ID	2
#define LINUX32_CLOCK_THREAD_CPUTIME_ID	3
#define LINUX32_CLOCK_REALTIME_HR		4
#define LINUX32_CLOCK_MONOTONIC_HR	5

int linux32_to_native_clockid(clockid_t *, clockid_t);
void native_to_linux32_timespec(struct linux32_timespec *, struct timespec *);
void linux32_to_native_timespec(struct timespec *, struct linux32_timespec *);

#endif /* _LINUX32_SCHED_H */
