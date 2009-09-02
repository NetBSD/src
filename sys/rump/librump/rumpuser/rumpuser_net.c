/*	$NetBSD: rumpuser_net.c,v 1.7 2009/09/02 19:02:51 pooka Exp $	*/

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: rumpuser_net.c,v 1.7 2009/09/02 19:02:51 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>

#include <rump/rumpuser.h>

#include "rumpuser_int.h"

int
rumpuser_net_socket(int domain, int type, int proto, int *error)
{

	DOCALL_KLOCK(int, (socket(domain, type, proto)));
}

int
rumpuser_net_sendmsg(int s, const struct msghdr *msg, int flags, int *error)
{

	DOCALL_KLOCK(ssize_t, (sendmsg(s, msg, flags)));
}

int
rumpuser_net_recvmsg(int s, struct msghdr *msg, int flags, int *error)
{

	DOCALL_KLOCK(ssize_t, (recvmsg(s, msg, flags)));
}

int
rumpuser_net_connect(int s, const struct sockaddr *name, int len, int *error)
{

	DOCALL_KLOCK(int, (connect(s, name, (socklen_t)len)));
}

int
rumpuser_net_bind(int s, const struct sockaddr *name, int len, int *error)
{

	DOCALL_KLOCK(int, (bind(s, name, (socklen_t)len)));
}

int
rumpuser_net_accept(int s, struct sockaddr *name, int *lenp, int *error)
{

	DOCALL_KLOCK(int, (accept(s, name, (socklen_t *)lenp)));
}

int
rumpuser_net_listen(int s, int backlog, int *error)
{

	DOCALL_KLOCK(int, (listen(s, backlog)));
}

int
rumpuser_net_getname(int s, struct sockaddr *so, int *lenp,
	enum rumpuser_getnametype which, int *error)
{
	socklen_t slen = *lenp;
	int rv;

	if (which == RUMPUSER_SOCKNAME)
		rv = getsockname(s, so, &slen);
	else
		rv = getpeername(s, so, &slen);

	*lenp = slen;
	if (rv == -1)
		*error = errno;
	else
		*error = 0;

	return rv;
}

int
rumpuser_net_setsockopt(int s, int level, int name,
	const void *data, int dlen, int *error)
{
	socklen_t slen = dlen;
	int rv;

	rv = setsockopt(s, level, name, data, slen);
	if (rv == -1)
		*error = errno;
	else
		*error = 0;
	return rv;
}
