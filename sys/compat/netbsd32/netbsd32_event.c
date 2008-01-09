/*	$NetBSD: netbsd32_event.c,v 1.2.48.2 2008/01/09 01:51:36 matt Exp $	*/

/*
 *  Copyright (c) 2005 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_event.c,v 1.2.48.2 2008/01/09 01:51:36 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

static int
netbsd32_kevent_fetch_timeout(const void *src, void *dest, size_t length)
{
	struct netbsd32_timespec ts32;
	int error;

	KASSERT(length == sizeof(struct timespec));

	error = copyin(src, &ts32, sizeof(ts32));
	if (error)
		return error;
	netbsd32_to_timespec(&ts32, (struct timespec *)dest);
	return 0;
}

static int
netbsd32_kevent_fetch_changes(void *private, const struct kevent *changelist,
    struct kevent *changes, size_t index, int n)
{
	const struct netbsd32_kevent *src =
	    (const struct netbsd32_kevent *)changelist;
	struct netbsd32_kevent *kev32, *changes32 = private;
	int error, i;

	error = copyin(src + index, changes32, n * sizeof(*changes32));
	if (error)
		return error;
	for (i = 0, kev32 = changes32; i < n; i++, kev32++, changes++)
		netbsd32_to_kevent(kev32, changes);
	return 0;
}

static int
netbsd32_kevent_put_events(void *private, struct kevent *events,
    struct kevent *eventlist, size_t index, int n)
{
	struct netbsd32_kevent *kev32, *events32 = private;
	int i;

	for (i = 0, kev32 = events32; i < n; i++, kev32++, events++)
		netbsd32_from_kevent(events, kev32);
	kev32 = (struct netbsd32_kevent *)eventlist;
	return  copyout(events32, kev32, n * sizeof(*events32));
}

int
netbsd32_kevent(struct lwp *l, const struct netbsd32_kevent_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_keventp_t) changelist;
		syscallarg(netbsd32_size_t) nchanges;
		syscallarg(netbsd32_keventp_t) eventlist;
		syscallarg(netbsd32_size_t) nevents;
		syscallarg(netbsd32_timespecp_t) timeout;
	} */
	int error;
	size_t maxalloc, nchanges, nevents;
	struct kevent_ops netbsd32_kevent_ops = {
		keo_fetch_timeout: netbsd32_kevent_fetch_timeout,
		keo_fetch_changes: netbsd32_kevent_fetch_changes,
		keo_put_events: netbsd32_kevent_put_events,
	};

	nchanges = SCARG(uap, nchanges);
	nevents = SCARG(uap, nevents);
	maxalloc = MIN(KQ_NEVENTS, MAX(nchanges, nevents));
	netbsd32_kevent_ops.keo_private =
	    malloc(maxalloc * sizeof(struct netbsd32_kevent), M_TEMP,
	    M_WAITOK);

	error = kevent1(l, retval, SCARG(uap, fd),
	    NETBSD32PTR64(SCARG(uap, changelist)), nchanges,
	    NETBSD32PTR64(SCARG(uap, eventlist)), nevents,
	    NETBSD32PTR64(SCARG(uap, timeout)), &netbsd32_kevent_ops);

	free(netbsd32_kevent_ops.keo_private, M_TEMP);
	return error;
}
