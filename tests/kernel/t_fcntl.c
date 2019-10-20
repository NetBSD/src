/*	$NetBSD: t_fcntl.c,v 1.2 2019/10/20 16:02:11 christos Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/param.h>
#include <sys/types.h>
#include <atf-c.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ATF_TC(getpath);
ATF_TC_HEAD(getpath, tc)
{  

	atf_tc_set_md_var(tc, "descr", "Checks fcntl(2) F_GETPATH");
}

static const struct {
	const char *name;
	int rv;
} files[] = {
	{ "/bin/ls", 0 },
	{ "/bin/sh", 0 },
	{ "/dev/zero", 0 },
	{ "/dev/null", 0 },
	{ "/sbin/chown", 0 },
	{ "/", ENOENT },
};

ATF_TC_BODY(getpath, tc)
{
	char path[MAXPATHLEN];
	int fd, rv;

	for (size_t i = 0; i < __arraycount(files); i++) {
		fd = open(files[i].name, O_RDONLY|O_NOFOLLOW);
		ATF_REQUIRE_MSG(fd != -1, "Cannot open `%s'", files[i].name);
		rv = fcntl(fd, F_GETPATH, path);
		if (files[i].rv) {
			ATF_REQUIRE_MSG(errno == files[i].rv,
			    "Unexpected error %d != %d for `%s'", errno,
			    files[i].rv, files[i].name); 
		} else {
			ATF_REQUIRE_MSG(rv != -1,
			    "Can't get path for `%s' (%s)", files[i].name,
			    strerror(errno));
			ATF_REQUIRE_MSG(strcmp(files[i].name, path) == 0,
			    "Bad name `%s' != `%s'", path, files[i].name);
		close(fd);
		}
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, getpath); 

	return atf_no_error();
}
