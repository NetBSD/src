/* $NetBSD: t_write.c,v 1.1 2011/10/15 06:50:52 jruoho Exp $ */

/*-
 * Copyright (c) 2001, 2008 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_write.c,v 1.1 2011/10/15 06:50:52 jruoho Exp $");

#include <sys/uio.h>
#include <sys/syslimits.h>

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

ATF_TC(writev_iovmax);
ATF_TC_HEAD(writev_iovmax, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr",
	    "Checks that file descriptor is properly FILE_UNUSE()d "
	    "when iovcnt is greater than IOV_MAX");
}

ATF_TC_BODY(writev_iovmax, tc)
{
	ssize_t retval;

	(void)printf("Calling writev(2, NULL, IOV_MAX + 1)...\n");

	errno = 0;
	retval = writev(2, NULL, IOV_MAX + 1);

	ATF_REQUIRE_EQ_MSG(retval, -1, "got: %zd", retval);
	ATF_REQUIRE_EQ_MSG(errno, EINVAL, "got: %s", strerror(errno));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, writev_iovmax);

	return atf_no_error();
}
