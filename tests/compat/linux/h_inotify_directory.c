/*	$NetBSD: h_inotify_directory.c,v 1.1 2023/08/19 22:56:44 christos Exp $	*/

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
__RCSID("$NetBSD: h_inotify_directory.c,v 1.1 2023/08/19 22:56:44 christos Exp $");

#include "h_linux.h"

#include <sys/null.h>

#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_inotify.h>

#define	INOTIFY_ALL_DIRECTORY	(LINUX_IN_ATTRIB|LINUX_IN_CREATE \
				|LINUX_IN_MOVE_SELF|LINUX_IN_MOVED_FROM \
				|LINUX_IN_MOVED_TO|LINUX_IN_DELETE \
				|LINUX_IN_DELETE_SELF)

char buf[8192];

struct {
	uint32_t	mask;
	bool		cookie;
	char		name[16];
} target_events[] = {
	{ .mask = LINUX_IN_CREATE,	.cookie = 0,	.name = "test", },
	{ .mask = LINUX_IN_MOVED_FROM,	.cookie = 1,	.name = "test", },
	{ .mask = LINUX_IN_MOVED_TO,	.cookie = 1,	.name = "test2", },
	{ .mask = LINUX_IN_DELETE,	.cookie = 0,	.name = "test2", },
	{ .mask = LINUX_IN_MOVE_SELF,	.cookie = 0,	.name = "", },
	{ .mask = LINUX_IN_DELETE_SELF,	.cookie = 0,	.name = "", },
	{ .mask = LINUX_IN_IGNORED,	.cookie = 0,	.name = "", },
};

void
_start(void)
{
	int fd, wd, targetfd;
	char *cur_buf;
	struct linux_inotify_event *cur_ie;

	RS(mkdir("test", 0644));

	RS(fd = syscall(LINUX_SYS_inotify_init));
	RS(wd = syscall(LINUX_SYS_inotify_add_watch, fd, (register_t)"test",
	    INOTIFY_ALL_DIRECTORY));

	/* Create some events. */
	RS(targetfd = open("test/test", LINUX_O_RDWR|LINUX_O_CREAT, 0644));
	RS(write(targetfd, &targetfd, sizeof(targetfd)));
	RS(close(targetfd));
	RS(rename("test/test", "test/test2"));
	RS(unlink("test/test2"));
	RS(rename("test", "test2"));
	RS(rmdir("test2"));

	/* Check the events. */
	RS(read(fd, buf, sizeof(buf)));
	cur_buf = buf;
	for (size_t i = 0; i < __arraycount(target_events); i++) {
		cur_ie = (struct linux_inotify_event *)cur_buf;

		REQUIRE(cur_ie->wd == wd);
		REQUIRE(cur_ie->mask == target_events[i].mask);

		if (target_events[i].cookie)
			REQUIRE(cur_ie->cookie != 0);
		else
			REQUIRE(cur_ie->cookie == 0);

		if (target_events[i].name[0] != '\0') {
			REQUIRE(cur_ie->len > strlen(target_events[i].name));
			REQUIRE(strcmp(cur_ie->name, target_events[i].name) == 0);
		} else
			REQUIRE(cur_ie->len == 0);

		cur_buf += sizeof(struct linux_inotify_event) + cur_ie->len;
	}

	exit(0);
}
