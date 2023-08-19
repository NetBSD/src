/*	$NetBSD: h_inotify_watch_change.c,v 1.1 2023/08/19 22:56:44 christos Exp $	*/

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
__RCSID("$NetBSD: h_inotify_watch_change.c,v 1.1 2023/08/19 22:56:44 christos Exp $");

#include "h_linux.h"

#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_inotify.h>

struct linux_inotify_event events[2];

void
_start(void)
{
	int fd, wd, targetfd;
	ssize_t nread;

	RS(targetfd = open("test", LINUX_O_RDWR|LINUX_O_CREAT, 0644));
	RS(close(targetfd));

	RS(fd = syscall(LINUX_SYS_inotify_init));
	RS(wd = syscall(LINUX_SYS_inotify_add_watch, fd, (register_t)"test",
            LINUX_IN_CLOSE_NOWRITE));

	/* We should only get the close event. */
	RS(targetfd = open("test", LINUX_O_RDONLY|LINUX_O_CREAT, 0644));
	RS(close(targetfd));

	RS(nread = read(fd, events, sizeof(events)));
	REQUIRE(nread == sizeof(events[0]));
	REQUIRE(events[0].mask == LINUX_IN_CLOSE_NOWRITE);

	/* Change the watch descriptor. */
	RS(wd = syscall(LINUX_SYS_inotify_add_watch, fd, (register_t)"test",
	    LINUX_IN_OPEN));

	/* We should only get the open event. */
	RS(targetfd = open("test", LINUX_O_RDONLY|LINUX_O_CREAT, 0644));
	RS(close(targetfd));

	RS(nread = read(fd, events, sizeof(events)));
	REQUIRE(nread == sizeof(events[0]));
	REQUIRE(events[0].mask == LINUX_IN_OPEN);

	/* Add to the watch descriptor. */
	RS(wd = syscall(LINUX_SYS_inotify_add_watch, fd, (register_t)"test",
	    LINUX_IN_CLOSE_NOWRITE|LINUX_IN_MASK_ADD));

	/* Now we should get both the open and the close. */
	RS(targetfd = open("test", LINUX_O_RDONLY|LINUX_O_CREAT, 0644));
	RS(close(targetfd));

	RS(nread = read(fd, events, sizeof(events)));
	REQUIRE(nread == 2 * sizeof(events[0]));
	REQUIRE(events[0].mask == LINUX_IN_OPEN);
	REQUIRE(events[1].mask == LINUX_IN_CLOSE_NOWRITE);

	exit(0);
}
