/*	$NetBSD: evutil.c,v 1.1.8.1 2009/08/04 18:32:09 snj Exp $	*/
/*
 * Copyright (c) 2007 Niels Provos <provos@citi.umich.edu>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include <stdio.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>

#include "evutil.h"
#include "log.h"

int
evutil_socketpair(int family, int type, int protocol, int fd[2])
{
	return socketpair(family, type, protocol, fd);
}

int
evutil_make_socket_nonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		event_warn("fcntl(O_NONBLOCK)");
		return -1;
	}

	return 0;
}

ev_int64_t
evutil_strtoll(const char *s, char **endptr, int base)
{
#ifdef HAVE_STRTOLL
	return (ev_int64_t)strtoll(s, endptr, base);
#elif SIZEOF_LONG == 8
	return (ev_int64_t)strtol(s, endptr, base);
#else
#error "I don't know how to parse 64-bit integers."
#endif
}
int
evutil_snprintf(char *buf, size_t buflen, const char *format, ...)
{
	int r;
	va_list ap;
	va_start(ap, format);
	r = evutil_vsnprintf(buf, buflen, format, ap);
	va_end(ap);
	return r;
}

int
evutil_vsnprintf(char *buf, size_t buflen, const char *format, va_list ap)
{
	int r = vsnprintf(buf, buflen, format, ap);
	buf[buflen-1] = '\0';
	return r;
}
