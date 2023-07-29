/*	$NetBSD: event.h,v 1.4 2023/07/29 11:58:53 rin Exp $	*/

/*-
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/sys/sys/event.h,v 1.12 2001/02/24 01:44:03 jlemon Exp $
 */

#ifndef _COMPAT_SYS_EVENT_H_
#define	_COMPAT_SYS_EVENT_H_

#include <sys/cdefs.h>
struct timespec;

#ifdef _KERNEL
#include <lib/libkern/libkern.h>
#else
#include <string.h>
#endif

struct kevent100 {
	uintptr_t	ident;		/* identifier for this event */
	uint32_t	filter;		/* filter for event */
	uint32_t	flags;		/* action flags for kqueue */
	uint32_t	fflags;		/* filter flag value */
	int64_t		data;		/* filter data value */
	void		*udata;		/* opaque user data identifier */
};

static __inline void
kevent100_to_kevent(const struct kevent100 *kev100, struct kevent *kev)
{
	memset(kev, 0, sizeof(*kev));
	memcpy(kev, kev100, sizeof(*kev100));
}

static __inline void
kevent_to_kevent100(const struct kevent *kev, struct kevent100 *kev100)
{
	memcpy(kev100, kev, sizeof(*kev100));
}

#ifdef _KERNEL
static __inline int
compat_100___kevent50_fetch_changes(void *ctx, const struct kevent *changelist,
    struct kevent *changes, size_t index, int n)
{
	int error, i;
	struct kevent100 *buf;
	const size_t buf_size = sizeof(*buf) * n;
	const struct kevent100 *changelist100 = (const struct kevent100 *)changelist;

	KASSERT(n >= 0);

	buf = kmem_alloc(buf_size, KM_SLEEP);

	error = copyin(changelist100 + index, buf, buf_size);
	if (error != 0)
		goto leave;

	for (i = 0; i < n; i++)
		kevent100_to_kevent(buf + i, changes + i);

leave:
	kmem_free(buf, buf_size);
	return error;
}

static __inline int
compat_100___kevent50_put_events(void *ctx, struct kevent *events,
    struct kevent *eventlist, size_t index, int n)
{
	int error, i;
        struct kevent100 *buf;
	const size_t buf_size = sizeof(*buf) * n;
	struct kevent100 *eventlist100 = (struct kevent100 *)eventlist;

	KASSERT(n >= 0);

	buf = kmem_alloc(buf_size, KM_SLEEP);

	for (i = 0; i < n; i++)
	        kevent_to_kevent100(events + i, buf + i);

	error = copyout(buf, eventlist100 + index, buf_size);

	kmem_free(buf, buf_size);
	return error;
}
#endif /* _KERNEL */

__BEGIN_DECLS
int	kevent(int, const struct kevent100 *, size_t, struct kevent100 *,
    size_t, const struct timespec50 *);
int	__kevent50(int, const struct kevent100 *, size_t, struct kevent100 *,
    size_t, const struct timespec *);
int	__kevent100(int, const struct kevent *, size_t, struct kevent *,
    size_t, const struct timespec *);
__END_DECLS

#endif /* !_COMPAT_SYS_EVENT_H_ */
