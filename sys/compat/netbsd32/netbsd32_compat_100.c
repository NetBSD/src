/*	$NetBSD: netbsd32_compat_100.c,v 1.3 2023/07/29 12:48:15 rin Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_100.c,v 1.3 2023/07/29 12:48:15 rin Exp $");

#include <sys/types.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/module.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_event.h>

static void
compat_100_netbsd32_to_kevent(const struct netbsd32_kevent100 *ke32,
    struct kevent *ke)
{

	memset(ke, 0, sizeof(*ke));
	ke->ident = ke32->ident;
	ke->filter = ke32->filter;
	ke->flags = ke32->flags;
	ke->fflags = ke32->fflags;
	ke->data = ke32->data;
	ke->udata = NETBSD32PTR64(ke32->udata);
}

static void
compat_100_netbsd32_from_kevent(const struct kevent *ke,
    struct netbsd32_kevent100 *ke32)
{

	memset(ke32, 0, sizeof(*ke32));
	ke32->ident = ke->ident;
	ke32->filter = ke->filter;
	ke32->flags = ke->flags;
	ke32->fflags = ke->fflags;
	ke32->data = ke->data;
	NETBSD32PTR32(ke32->udata, ke->udata);
}

int
compat_100_netbsd32_kevent_fetch_changes(void *ctx,
    const struct kevent *changelist, struct kevent *changes, size_t index,
    int n)
{
	const struct netbsd32_kevent100 *src =
	    (const struct netbsd32_kevent100 *)changelist;
	struct netbsd32_kevent100 *ke32, *changes32 = ctx;
	int error, i;

	error = copyin(src + index, changes32, n * sizeof(*changes32));
	if (error)
		return error;
	for (i = 0, ke32 = changes32; i < n; i++, ke32++, changes++)
		compat_100_netbsd32_to_kevent(ke32, changes);
	return 0;
}

int
compat_100_netbsd32_kevent_put_events(void *ctx, struct kevent *events,
    struct kevent *eventlist, size_t index, int n)
{
	struct netbsd32_kevent100 *ke32, *events32 = ctx;
	int i;

	for (i = 0, ke32 = events32; i < n; i++, ke32++, events++)
		compat_100_netbsd32_from_kevent(events, ke32);
	ke32 = ((struct netbsd32_kevent100 *)eventlist) + index;
	return copyout(events32, ke32, n * sizeof(*events32));
}

int
compat_100_netbsd32___kevent50(struct lwp *l,
    const struct compat_100_netbsd32___kevent50_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_kevent100p_t) changelist;
		syscallarg(netbsd32_size_t) nchanges;
		syscallarg(netbsd32_kevent100p_t) eventlist;
		syscallarg(netbsd32_size_t) nevents;
		syscallarg(netbsd32_timespecp_t) timeout;
	} */
	struct kevent_ops netbsd32_kevent_ops = {
		.keo_fetch_timeout = netbsd32_kevent_fetch_timeout,
		.keo_fetch_changes = compat_100_netbsd32_kevent_fetch_changes,
		.keo_put_events = compat_100_netbsd32_kevent_put_events,
	};

	return netbsd32_kevent1(retval, SCARG(uap, fd),
	    (netbsd32_keventp_t)SCARG(uap, changelist), SCARG(uap, nchanges),
	    (netbsd32_keventp_t)SCARG(uap, eventlist), SCARG(uap, nevents),
	    SCARG(uap, timeout), &netbsd32_kevent_ops);
}

static struct syscall_package compat_netbsd32_100_syscalls[] = {
	{ NETBSD32_SYS_compat_100_netbsd32___kevent50, 0,
	    (sy_call_t *)compat_100_netbsd32___kevent50 },
	{ 0, 0, NULL },
};

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_100, "compat_netbsd32,compat_100");

static int
compat_netbsd32_100_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return syscall_establish(&emul_netbsd32,
		    compat_netbsd32_100_syscalls);

	case MODULE_CMD_FINI:
		return syscall_disestablish(&emul_netbsd32,
		    compat_netbsd32_100_syscalls);

	default:
		return ENOTTY;
	}
}
