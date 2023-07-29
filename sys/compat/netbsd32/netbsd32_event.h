/*	$NetBSD: netbsd32_event.h,v 1.2 2023/07/29 12:48:15 rin Exp $	*/

/*
 * Copyright (c) 2023 NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_NETBSD32_EVENT_H_
#define	_NETBSD32_EVENT_H_

#include <compat/netbsd32/netbsd32.h>

/* netbsd32_event.c */
int netbsd32_kevent_fetch_timeout(const void *, void *, size_t);
int netbsd32_kevent1(register_t *, int, const netbsd32_kevent100p_t,
    netbsd32_size_t, netbsd32_kevent100p_t, netbsd32_size_t,
    netbsd32_timespecp_t, struct kevent_ops *);

/* netbsd32_compat_100.c */
int compat_100_netbsd32_kevent_fetch_changes(void *, const struct kevent *,
    struct kevent *, size_t, int);
int compat_100_netbsd32_kevent_put_events(void *, struct kevent *,
    struct kevent *, size_t, int);

#endif /* !_NETBSD32_EVENT_H_ */
