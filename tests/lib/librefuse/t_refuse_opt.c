/*	$NetBSD: t_refuse_opt.c,v 1.1 2016/11/14 16:10:31 pho Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
 * PURPOSE ARE DISCLAIMED.	IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_refuse_opt.c,v 1.1 2016/11/14 16:10:31 pho Exp $");

#include <atf-c.h>

#include <fuse.h>

#include "../../h_macros.h"

ATF_TC(efuse_opt_add_arg);
ATF_TC_HEAD(efuse_opt_add_arg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check that fuse_opt_add_arg(3) works");
}

ATF_TC_BODY(efuse_opt_add_arg, tc)
{
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	RZ(fuse_opt_add_arg(&args, "foo"));
	RZ(fuse_opt_add_arg(&args, "bar"));

	ATF_REQUIRE_EQ(args.argc, 2);
	ATF_CHECK_STREQ(args.argv[0], "foo");
	ATF_CHECK_STREQ(args.argv[1], "bar");
	ATF_CHECK(args.allocated != 0);
}

ATF_TC(efuse_opt_insert_arg);
ATF_TC_HEAD(efuse_opt_insert_arg, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check that fuse_opt_insert_arg(3) works");
}

ATF_TC_BODY(efuse_opt_insert_arg, tc)
{
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	RZ(fuse_opt_insert_arg(&args, 0, "foo"));
	RZ(fuse_opt_insert_arg(&args, 0, "bar"));

	ATF_REQUIRE_EQ(args.argc, 2);
	ATF_CHECK_STREQ(args.argv[0], "bar");
	ATF_CHECK_STREQ(args.argv[1], "foo");
	ATF_CHECK(args.allocated != 0);
}

ATF_TC(efuse_opt_add_opt);
ATF_TC_HEAD(efuse_opt_add_opt, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check that fuse_opt_add_opt(3) works");
}

ATF_TC_BODY(efuse_opt_add_opt, tc)
{
	char* opt = NULL;

	atf_tc_expect_death("fuse_opt_add_opt(3) is not implemented yet");

	RZ(fuse_opt_add_opt(&opt, "foo"));
	ATF_CHECK_STREQ(opt, "foo");

	RZ(fuse_opt_add_opt(&opt, "b\\a,r"));
	ATF_CHECK_STREQ(opt, "foo,b\\a,r");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, efuse_opt_add_arg);
	ATF_TP_ADD_TC(tp, efuse_opt_insert_arg);
	ATF_TP_ADD_TC(tp, efuse_opt_add_opt);

	return atf_no_error();
}
