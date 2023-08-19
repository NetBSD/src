/*	$NetBSD: h_inotify_init.c,v 1.1 2023/08/19 22:56:44 christos Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Theodore Preduta.
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
__RCSID("$NetBSD: h_inotify_init.c,v 1.1 2023/08/19 22:56:44 christos Exp $");

#include "h_linux.h"

#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_inotify.h>

void
_start(void)
{
	int fd;

	/* Check that none of CLOEXEC or NONBLOCK are set. */
	RS(fd = syscall(LINUX_SYS_inotify_init));
	REQUIRE(fcntl(fd, LINUX_F_GETFD) == 0);
	REQUIRE((fcntl(fd, LINUX_F_GETFL) & LINUX_O_NONBLOCK) == 0);
	RS(close(fd));

	/* Check that only NONBLOCK is set. */
	RS(fd = syscall(LINUX_SYS_inotify_init1, LINUX_IN_NONBLOCK));
	REQUIRE(fcntl(fd, LINUX_F_GETFD) == 0);
	REQUIRE((fcntl(fd, LINUX_F_GETFL) & LINUX_O_NONBLOCK) != 0);
	RS(close(fd));

	/* Check that only CLOEXEC is set. */
	RS(fd = syscall(LINUX_SYS_inotify_init1, LINUX_IN_CLOEXEC));
	REQUIRE(fcntl(fd, LINUX_F_GETFD) != 0);
	REQUIRE((fcntl(fd, LINUX_F_GETFL) & LINUX_O_NONBLOCK) == 0);
	RS(close(fd));

	exit(0);
}
