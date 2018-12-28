/*	$NetBSD: t_threadpool.c,v 1.2 2018/12/28 19:54:36 thorpej Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/sysctl.h>

#include <rump/rump.h>

#include <atf-c.h>

#include "h_macros.h"
#include "../kernspace/kernspace.h"

ATF_TC(threadpool_unbound_lifecycle);
ATF_TC_HEAD(threadpool_unbound_lifecycle, tc)
{

	atf_tc_set_md_var(tc, "descr", "Tests unbound threadpool lifecycle");
}

ATF_TC_BODY(threadpool_unbound_lifecycle, tc)
{

	rump_init();

	rump_schedule();
	rumptest_threadpool_unbound_lifecycle(); /* panics if fails */
	rump_unschedule();
}

ATF_TC(threadpool_percpu_lifecycle);
ATF_TC_HEAD(threadpool_percpu_lifecycle, tc)
{

	atf_tc_set_md_var(tc, "descr", "Tests percpu threadpool lifecycle");
}

ATF_TC_BODY(threadpool_percpu_lifecycle, tc)
{

	rump_init();

	rump_schedule();
	rumptest_threadpool_percpu_lifecycle(); /* panics if fails */
	rump_unschedule();
}

ATF_TC(threadpool_unbound_schedule);
ATF_TC_HEAD(threadpool_unbound_schedule, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Tests scheduling on unbound threadpools");
}

ATF_TC_BODY(threadpool_unbound_schedule, tc)
{

	rump_init();

	rump_schedule();
	rumptest_threadpool_unbound_schedule(); /* panics if fails */
	rump_unschedule();
}

ATF_TC(threadpool_percpu_schedule);
ATF_TC_HEAD(threadpool_percpu_schedule, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Tests scheduling on percpu threadpools");
}

ATF_TC_BODY(threadpool_percpu_schedule, tc)
{

	rump_init();

	rump_schedule();
	rumptest_threadpool_percpu_schedule(); /* panics if fails */
	rump_unschedule();
}

ATF_TC(threadpool_job_cancel);
ATF_TC_HEAD(threadpool_job_cancel, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Tests synchronizing with job cancellation");
}

ATF_TC_BODY(threadpool_job_cancel, tc)
{

	rump_init();

	rump_schedule();
	rumptest_threadpool_job_cancel(); /* panics if fails */
	rump_unschedule();
}

ATF_TC(threadpool_job_cancelthrash);
ATF_TC_HEAD(threadpool_job_cancelthrash, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Tests thrashing job scheduling / cancellation");
}

ATF_TC_BODY(threadpool_job_cancelthrash, tc)
{

	rump_init();

	rump_schedule();
	rumptest_threadpool_job_cancelthrash(); /* panics if fails */
	rump_unschedule();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, threadpool_unbound_lifecycle);
	ATF_TP_ADD_TC(tp, threadpool_percpu_lifecycle);
	ATF_TP_ADD_TC(tp, threadpool_unbound_schedule);
	ATF_TP_ADD_TC(tp, threadpool_percpu_schedule);
	ATF_TP_ADD_TC(tp, threadpool_job_cancel);
	ATF_TP_ADD_TC(tp, threadpool_job_cancelthrash);

	return atf_no_error();
}
