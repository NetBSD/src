/*	$NetBSD: compat.c,v 1.2 2010/07/26 11:52:25 pooka Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat.c,v 1.2 2010/07/26 11:52:25 pooka Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/sched.h>
#include <sys/select.h>
#include <sys/syscallargs.h>
#include <sys/vnode.h>

#include <compat/sys/time.h>

#include <rump/rump.h>
#include <rump/rump_syscalls_compat.h>
#include <rump/rumpuser.h>

/* mostly from sys/compat/common/kern_time_50.c */

int
rump_sys_nb5_pollts(struct pollfd *fds, size_t nfds,
	const struct timespec *tsarg, const void *sigset)
{
	register_t retval;
	struct timespec	ats, *ts = NULL;
	struct timespec50 ats50;
	sigset_t	amask, *mask = NULL;
	int		error;

	rump_schedule();

	if (tsarg) {
		error = copyin(tsarg, &ats50, sizeof(ats50));
		if (error)
			return error;
		timespec50_to_timespec(&ats50, &ats);
		ts = &ats;
	}
	if (sigset) {
		error = copyin(sigset, &amask, sizeof(amask));
		if (error)
			return error;
		mask = &amask;
	}

	error = pollcommon(&retval, fds, nfds, ts, mask);

	rump_unschedule();

	if (error) {
		rumpuser_seterrno(error);
		retval = -1;
	}

	return retval;
}

int
rump_sys_nb5_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exfds,
	struct timeval *timeout)
{
	register_t retval;
	struct timespec ats, *ts = NULL;
	struct timeval50 atv50;
	int error;

	rump_schedule();

	if (timeout) {
		error = copyin(timeout, &atv50, sizeof(atv50));
		if (error)
			return error;
		ats.tv_sec = atv50.tv_sec;
		ats.tv_nsec = atv50.tv_usec * 1000;
		ts = &ats;
	}

	error = selcommon(&retval, nfds, readfds, writefds, exfds, ts, NULL);

	rump_unschedule();

	if (error) {
		rumpuser_seterrno(error);
		retval = -1;
	}

	return retval;
}
