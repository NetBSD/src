/*	$NetBSD: kern_event_100.c,v 1.1 2023/07/28 18:19:00 christos Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_event_100.c,v 1.1 2023/07/28 18:19:00 christos Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_mod.h>
#include <compat/sys/event.h>

static const struct syscall_package kern_event_100_syscalls[] = {
	{ SYS_compat_100___kevent50, 0,
	    (sy_call_t *)compat_100_sys___kevent50 },
	{ 0, 0, NULL },
};

int
compat_100_sys___kevent50(struct lwp *l,
    const struct compat_100_sys___kevent50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct kevent100 *) changelist;
		syscallarg(size_t) nchanges;
		syscallarg(struct kevent100 *) eventlist;
		syscallarg(size_t) nevents;
		syscallarg(const struct timespec *) timeout;
	} */
	static const struct kevent_ops compat_100_kevent_ops = {
		.keo_private = NULL,
		.keo_fetch_timeout = copyin,
		.keo_fetch_changes = compat_100___kevent50_fetch_changes,
		.keo_put_events = compat_100___kevent50_put_events,
	};

	return kevent1(retval, SCARG(uap, fd),
	    (const struct kevent *)SCARG(uap, changelist), SCARG(uap, nchanges),
	    (struct kevent *)SCARG(uap, eventlist), SCARG(uap, nevents),
	    SCARG(uap, timeout), &compat_100_kevent_ops);
}

int
kern_event_100_init(void)
{

	return syscall_establish(NULL, kern_event_100_syscalls);
}

int
kern_event_100_fini(void)
{

	return syscall_disestablish(NULL, kern_event_100_syscalls);
}
