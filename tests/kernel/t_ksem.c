/* $NetBSD: t_ksem.c,v 1.1 2019/02/03 03:20:24 thorpej Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_ksem.c,v 1.1 2019/02/03 03:20:24 thorpej Exp $");

#include <sys/mman.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <atf-c.h>

#define	_LIBC
#include <sys/ksem.h>

ATF_TC(close_on_unnamed);
ATF_TC_HEAD(close_on_unnamed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct error on close of unnamed semaphore");
}
ATF_TC_BODY(close_on_unnamed, tc)
{
	intptr_t ksem;

	/* _ksem_close() is invalid on unnamed semaphore. */
	ksem = 0;
	ATF_REQUIRE_EQ(_ksem_init(0, &ksem), 0);
	ATF_REQUIRE(_ksem_close(ksem) == -1 && errno == EINVAL);
	ATF_REQUIRE_EQ(_ksem_destroy(ksem), 0);
}

ATF_TC(close_on_unnamed_pshared);
ATF_TC_HEAD(close_on_unnamed_pshared, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct error on close of unnamed pshared semaphore");
}
ATF_TC_BODY(close_on_unnamed_pshared, tc)
{
	intptr_t ksem;

	/* Similar, but lifecycle of pshared is slightly different. */
	ksem = KSEM_PSHARED;
	ATF_REQUIRE_EQ(_ksem_init(0, &ksem), 0);
	ATF_REQUIRE(_ksem_close(ksem) == -1 && errno == EINVAL);
	ATF_REQUIRE_EQ(_ksem_destroy(ksem), 0);
}

ATF_TC_WITH_CLEANUP(destroy_on_named);
ATF_TC_HEAD(destroy_on_named, tc)
{
	 atf_tc_set_md_var(tc, "descr",
	     "test for correct error on destroy of named semaphore");
}
ATF_TC_BODY(destroy_on_named, tc)
{
	intptr_t ksem;

	/* Exercise open-unlinked semaphore lifecycle. */
	ATF_REQUIRE_EQ(_ksem_open("/ksem_x", O_CREAT | O_EXCL, 0644, 0, &ksem),
	    0);
	ATF_REQUIRE(_ksem_destroy(ksem) == -1 && errno == EINVAL);
	ATF_REQUIRE_EQ(_ksem_close(ksem), 0);
	ATF_REQUIRE_EQ(_ksem_unlink("/ksem_x"), 0);
}
ATF_TC_CLEANUP(destroy_on_named, tc)
{
	(void)_ksem_unlink("/ksem_x");
}

ATF_TC_WITH_CLEANUP(open_unlinked_lifecycle);
ATF_TC_HEAD(open_unlinked_lifecycle, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Exercises lifecycle of open-unlined semaphores");
}
ATF_TC_BODY(open_unlinked_lifecycle, tc)
{
	intptr_t ksem, ksem1;
	int val;

	/* Exercise open-unlinked semaphore lifecycle. */
	ATF_REQUIRE_EQ(_ksem_open("/ksem_b", O_CREAT | O_EXCL, 0644, 0, &ksem),
	    0);
	ATF_REQUIRE_EQ(_ksem_unlink("/ksem_b"), 0);
	val = 255;
	ATF_REQUIRE(_ksem_getvalue(ksem, &val) == 0 && val == 0);

	ATF_REQUIRE_EQ(_ksem_open("/ksem_b", O_CREAT | O_EXCL, 0644, 1, &ksem1),
	    0);
	ATF_REQUIRE_EQ(_ksem_unlink("/ksem_b"), 0);
	val = 255;
	ATF_REQUIRE(_ksem_getvalue(ksem1, &val) == 0 && val == 1);

	ATF_REQUIRE_EQ(_ksem_close(ksem), 0);
	ATF_REQUIRE_EQ(_ksem_close(ksem1), 0);
}
ATF_TC_CLEANUP(open_unlinked_lifecycle, tc)
{
	(void)_ksem_unlink("/ksem_a");
	(void)_ksem_unlink("/ksem_b");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, close_on_unnamed);
	ATF_TP_ADD_TC(tp, close_on_unnamed_pshared);
	ATF_TP_ADD_TC(tp, destroy_on_named);
	ATF_TP_ADD_TC(tp, open_unlinked_lifecycle);

	return atf_no_error();
}
