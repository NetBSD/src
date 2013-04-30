/*	$NetBSD: rumpcomp_user.c,v 1.5 2013/04/30 12:39:21 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _KERNEL
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <poll.h>

#include <rump/rumpuser_component.h>

#include "rumpcomp_user.h"

#define seterror(_v_) if ((_v_) == -1) rv = errno; else rv = 0;

int
rumpcomp_sockin_socket(int domain, int type, int proto, int *s)
{
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();
	*s = socket(domain, type, proto);
	seterror(*s);
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_sendmsg(int s, const struct msghdr *msg, int flags, size_t *snd)
{
	void *cookie;
	ssize_t nn;
	int rv;

	cookie = rumpuser_component_unschedule();
	nn = sendmsg(s, msg, flags);
	seterror(nn);
	*snd = (size_t)nn;
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_recvmsg(int s, struct msghdr *msg, int flags, size_t *rcv)
{
	void *cookie;
	ssize_t nn;
	int rv;

	cookie = rumpuser_component_unschedule();
	nn = recvmsg(s, msg, flags);
	seterror(nn);
	*rcv = (size_t)nn;
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_connect(int s, const struct sockaddr *name, int len)
{
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();
	rv = connect(s, name, (socklen_t)len);
	seterror(rv);
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_bind(int s, const struct sockaddr *name, int len)
{
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();
	rv = bind(s, name, (socklen_t)len);
	seterror(rv);
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_accept(int s, struct sockaddr *name, int *lenp, int *s2)
{
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();
	*s2 = accept(s, name, (socklen_t *)lenp);
	seterror(*s2);
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_listen(int s, int backlog)
{
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();
	rv = listen(s, backlog);
	seterror(rv);
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_getname(int s, struct sockaddr *so, int *lenp,
	enum rumpcomp_sockin_getnametype which)
{
	socklen_t slen = *lenp;
	int rv;

	if (which == RUMPCOMP_SOCKIN_SOCKNAME)
		rv = getsockname(s, so, &slen);
	else
		rv = getpeername(s, so, &slen);
	seterror(rv);
	*lenp = slen;

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_setsockopt(int s, int level, int name,
	const void *data, int dlen)
{
	socklen_t slen = dlen;
	int rv;

	rv = setsockopt(s, level, name, data, slen);
	seterror(rv);

	return rumpuser_component_errtrans(rv);
}

int
rumpcomp_sockin_poll(struct pollfd *fds, int nfds, int timeout, int *nready)
{
	void *cookie;
	int rv;

	cookie = rumpuser_component_unschedule();
	*nready = poll(fds, (nfds_t)nfds, timeout);
	seterror(*nready);
	rumpuser_component_schedule(cookie);

	return rumpuser_component_errtrans(rv);
}
#endif
