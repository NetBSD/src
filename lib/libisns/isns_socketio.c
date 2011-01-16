/*	$NetBSD: isns_socketio.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

/*-
 * Copyright (c) 2004,2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
__RCSID("$NetBSD: isns_socketio.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $");


/*
 * isns_socketio.c
 */

#include "isns.h"
#include "isns_config.h"

#include <unistd.h>


/*
 * isns_socket_create()
 */
int
isns_socket_create(isns_socket_t *s, int domain, int type)
{
#if HAVE_WEPE
	return (wepe_sys_socket(domain, type, 0, s));
#else
	return *s = socket(domain, type, 0);
#endif
}

/*
 * isns_socket_connect()
 */
int
isns_socket_connect(isns_socket_t s, const struct sockaddr *name, socklen_t
    namelen)
{
#if HAVE_WEPE
	return (wepe_sys_connect(s, name, namelen));
#else
	return connect(s, name, namelen);
#endif
}

/*
 * isns_socket_close()
 */
int
isns_socket_close(isns_socket_t s)
{
#if HAVE_WEPE
	return wepe_sys_close(s);
#else
	return close(s);
#endif
}

/*
 * isns_socket_writev()
 */
ssize_t
isns_socket_writev(isns_socket_t s, const struct iovec *iov, int iovcnt)
{
	return isns_file_writev(s, iov, iovcnt);
}

/*
 * isns_socket_readv()
 */
ssize_t
isns_socket_readv(isns_socket_t s, const struct iovec *iov, int iovcnt)
{
	return isns_file_readv(s, iov, iovcnt);
}
