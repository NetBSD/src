/*	$NetBSD: adjtime.c,v 1.1 2001/09/16 07:41:57 manu Exp $ */

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.      
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "namespace.h"
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/clockctl.h>
 
#ifdef __weak_alias
__weak_alias(adjtime,_adjtime)
#endif 

int
adjtime(delta, olddelta)
	const struct timeval *delta;
	struct timeval *olddelta;
{
	struct clockctl_adjtime_args args;
	int error;
	int fd;
	quad_t q;
	int rv;

	/* 
	 * Root always uses the adjtime syscall
	 */
	if (geteuid() == 0)
		goto try_syscall;

	/* 
	 * Try to use /dev/clockctl, and revert to 
	 * adjtime syscall if it fails.
	 */
	fd = open(_PATH_CLOCKCTL, O_WRONLY, 0);
	if (fd == -1)
		goto try_syscall;

	(void)memcpy(&args.delta, delta, sizeof(delta));
	error = ioctl(fd, CLOCKCTL_ADJTIME, &args);
	(void)close(fd);
	if (!error && olddelta) {
		(void)memcpy(olddelta, &args.olddelta, sizeof(olddelta));
		return 0;
	}

try_syscall:
	q = __syscall((quad_t)SYS_adjtime, delta, olddelta);
	if (/* LINTED constant */ sizeof (quad_t) == sizeof (register_t) ||
		 /* LINTED constant */ BYTE_ORDER == LITTLE_ENDIAN)
		rv = (int)q;
	else
		rv = (int)((u_quad_t)q >> 32); 
	return rv;
}
