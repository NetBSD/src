/*	$NetBSD: t_io.c,v 1.2 2010/11/11 16:03:55 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/stat.h>
#include <sys/statvfs.h>

#include <atf-c.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rump.h>

#include "../common/h_fsmacros.h"
#include "../../h_macros.h"

static void
holywrite(const atf_tc_t *tc, const char *mp)
{
	char buf[1024];
	char *b2, *b3;
	size_t therange = getpagesize()+1;
	int fd;

	RL(rump_sys_chdir(mp));

	RL(fd = rump_sys_open("file", O_RDWR|O_CREAT|O_TRUNC, 0666));

	memset(buf, 'A', sizeof(buf));
	RL(rump_sys_pwrite(fd, buf, 1, getpagesize()));

	memset(buf, 'B', sizeof(buf));
	RL(rump_sys_pwrite(fd, buf, 2, 0xfff));

	REQUIRE_LIBC(b2 = malloc(2 * getpagesize()), NULL);
	REQUIRE_LIBC(b3 = malloc(2 * getpagesize()), NULL);

	RL(rump_sys_pread(fd, b2, therange, 0));

	memset(b3, 0, therange);
	memset(b3 + getpagesize() - 1, 'B', 2);

	ATF_REQUIRE_EQ(memcmp(b2, b3, therange), 0);

	rump_sys_close(fd);
	rump_sys_chdir("/");
}

ATF_TC_FSAPPLY(holywrite, "create a sparse file and fill hole");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(holywrite);

	return atf_no_error();
}
