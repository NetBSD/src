/*	$NetBSD: t_dummy.c,v 1.1 2016/11/16 21:36:23 kamil Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_dummy.c,v 1.1 2016/11/16 21:36:23 kamil Exp $");

#include <pthread.h>
#include <pthread_dbg.h>

#include <atf-c.h>

static int
dummy1_proc_read(void *arg, caddr_t addr, void *buf, size_t size)
{
	return TD_ERR_ERR;
}

static int
dummy1_proc_write(void *arg, caddr_t addr, void *buf, size_t size)
{
	return TD_ERR_ERR;
}

static int
dummy1_proc_lookup(void *arg, const char *sym, caddr_t *addr)
{
	return TD_ERR_ERR;
}

static int
dummy1_proc_regsize(void *arg, int regset, size_t *size)
{
	return TD_ERR_ERR;
}
 
static int
dummy1_proc_getregs(void *arg, int regset, int lwp, void *buf)   
{
	return TD_ERR_ERR;
}

static int
dummy1_proc_setregs(void *arg, int regset, int lwp, void *buf)
{
	return TD_ERR_ERR;
}


ATF_TC(dummy1);
ATF_TC_HEAD(dummy1, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Asserts that dummy lookup functions stops td_open()");
}

ATF_TC_BODY(dummy1, tc)
{

	struct td_proc_callbacks_t dummy1_callbacks;
	td_proc_t *main_ta;

	dummy1_callbacks.proc_read	= dummy1_proc_read;
	dummy1_callbacks.proc_write	= dummy1_proc_write;
	dummy1_callbacks.proc_lookup	= dummy1_proc_lookup;
	dummy1_callbacks.proc_regsize	= dummy1_proc_regsize;
	dummy1_callbacks.proc_getregs	= dummy1_proc_getregs;
	dummy1_callbacks.proc_setregs	= dummy1_proc_setregs;

	ATF_REQUIRE(td_open(&dummy1_callbacks, NULL, &main_ta) == TD_ERR_ERR);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, dummy1);

	return atf_no_error();
}
