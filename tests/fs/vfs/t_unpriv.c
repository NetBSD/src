/*	$NetBSD: t_unpriv.c,v 1.4 2012/01/31 19:02:49 njoly Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <atf-c.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rump.h>

#include "../common/h_fsmacros.h"
#include "../../h_macros.h"

#define USES_OWNER							 \
	if (FSTYPE_MSDOS(tc))						 \
	    atf_tc_skip("owner not supported by file system")

static void
owner(const atf_tc_t *tc, const char *mp)
{

	USES_OWNER;

	FSTEST_ENTER();

	rump_pub_lwproc_rfork(RUMP_RFCFDG);
	if (rump_sys_setuid(1) == -1)
		atf_tc_fail_errno("setuid");
        if (rump_sys_chown(".", 1, -1) != -1 || errno != EPERM)
		atf_tc_fail_errno("chown");
        if (rump_sys_chmod(".", 0000) != -1 || errno != EPERM) 
                atf_tc_fail_errno("chmod");
	rump_pub_lwproc_releaselwp();

	if (rump_sys_chown(".", 1, -1) == -1)
		atf_tc_fail_errno("chown");

	rump_pub_lwproc_rfork(RUMP_RFCFDG);
	if (rump_sys_setuid(1) == -1)
		atf_tc_fail_errno("setuid");
        if (rump_sys_chown(".", 1, -1) == -1)
		atf_tc_fail_errno("chown");
        if (rump_sys_chmod(".", 0000) == -1) 
                atf_tc_fail_errno("chmod");
	rump_pub_lwproc_releaselwp();

	FSTEST_EXIT();
}


ATF_TC_FSAPPLY(owner, "owner unprivileged checks");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(owner);

	return atf_no_error();
}
