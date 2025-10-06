/*	$NetBSD: t_pthread_weak_threaded.c,v 1.1 2025/10/06 13:16:44 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_pthread_weak_threaded.c,v 1.1 2025/10/06 13:16:44 riastradh Exp $");

#include <atf-c.h>

#include "h_pthread_weak.h"

ATF_TC(mutex);
ATF_TC_HEAD(mutex, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test mutex usage in library with _NETBSD_PTHREAD_CREATE_WEAK");
}
ATF_TC_BODY(mutex, tc)
{
	test_mutex();
}

ATF_TC(thread_creation);
ATF_TC_HEAD(thread_creation, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test pthread_create via library in threaded application");
}
ATF_TC_BODY(thread_creation, tc)
{
	test_thread_creation();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, mutex);
	ATF_TP_ADD_TC(tp, thread_creation);

	return atf_no_error();
}
