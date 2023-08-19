/*	$NetBSD: h_inotify_single_file.c,v 1.1 2023/08/19 22:56:44 christos Exp $	*/

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
__RCSID("$NetBSD: h_inotify_single_file.c,v 1.1 2023/08/19 22:56:44 christos Exp $");

#include "h_linux.h"

#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_inotify.h>

#define	INOTIFY_ALL_FILE	(LINUX_IN_ATTRIB|LINUX_IN_CLOSE_NOWRITE \
				|LINUX_IN_OPEN|LINUX_IN_MOVE_SELF \
				|LINUX_IN_ACCESS|LINUX_IN_CLOSE_WRITE \
				|LINUX_IN_MODIFY|LINUX_IN_DELETE_SELF)

struct linux_inotify_event events[10];

void
_start(void)
{
	int fd, wd, targetfd, buf;

	RS(targetfd = open("test", LINUX_O_RDWR|LINUX_O_CREAT, 0644));
	RS(close(targetfd));

	RS(fd = syscall(LINUX_SYS_inotify_init));
	RS(wd = syscall(LINUX_SYS_inotify_add_watch, fd, (register_t)"test",
	    INOTIFY_ALL_FILE));

	/* Create some events. */
	RS(targetfd = open("test", LINUX_O_RDWR|LINUX_O_CREAT, 0644));
	RS(write(targetfd, &buf, sizeof(buf)));
	RS(read(targetfd, &buf, sizeof(buf)));
	RS(close(targetfd));
	RS(targetfd = open("test", LINUX_O_RDONLY|LINUX_O_CREAT, 0644));
	RS(close(targetfd));
	RS(rename("test", "test2"));
	RS(unlink("test2"));

	/* Get and check the events. */
	RS(read(fd, events, sizeof(events)));

	for (size_t i = 0; i < __arraycount(events); i++)
		REQUIRE(events[i].wd == wd && events[i].cookie == 0
		    && events[i].len == 0);

	REQUIRE(events[0].mask == LINUX_IN_OPEN);
	REQUIRE(events[1].mask == LINUX_IN_MODIFY);
	REQUIRE(events[2].mask == LINUX_IN_ACCESS);
	REQUIRE(events[3].mask == LINUX_IN_CLOSE_WRITE);
	REQUIRE(events[4].mask == LINUX_IN_OPEN);
	REQUIRE(events[5].mask == LINUX_IN_CLOSE_NOWRITE);
	REQUIRE(events[6].mask == LINUX_IN_MOVE_SELF);
	REQUIRE(events[7].mask == LINUX_IN_ATTRIB);
	REQUIRE(events[8].mask == LINUX_IN_DELETE_SELF);
	REQUIRE(events[9].mask == LINUX_IN_IGNORED);

	exit(0);
}
