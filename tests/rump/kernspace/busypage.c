/*	$NetBSD: busypage.c,v 1.5.42.1 2020/04/08 14:09:12 martin Exp $	*/

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: busypage.c,v 1.5.42.1 2020/04/08 14:09:12 martin Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

#include "kernspace.h"

static struct uvm_object *uobj;
static struct vm_page *testpg;
static kcondvar_t tcv;

static bool threadrun = false;

static void
thread(void *arg)
{

	mutex_enter(&testpg->interlock);
	threadrun = true;
	cv_signal(&tcv);
	mutex_exit(&testpg->interlock);

	rw_enter(uobj->vmobjlock, RW_READER);
	uvm_pagewait(testpg, uobj->vmobjlock, "tw");

	kthread_exit(0);
}

void
rumptest_busypage()
{
	struct lwp *newl;
	int rv;

	cv_init(&tcv, "napina");

	uobj = uao_create(1, 0);
	rw_enter(uobj->vmobjlock, RW_WRITER);
	testpg = uvm_pagealloc(uobj, 0, NULL, 0);
	rw_exit(uobj->vmobjlock);
	if (testpg == NULL)
		panic("couldn't create vm page");

	rv = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN | KTHREAD_MPSAFE, NULL,
	    thread, NULL, &newl, "jointest");
	if (rv)
		panic("thread creation failed: %d", rv);

	kpause("lolgic", false, mstohz(100), NULL);

	mutex_enter(&testpg->interlock);
	while (!threadrun)
		cv_wait(&tcv, &testpg->interlock);
	mutex_exit(&testpg->interlock);

	rw_enter(uobj->vmobjlock, RW_WRITER);
	uvm_page_unbusy(&testpg, 1);
	rw_exit(uobj->vmobjlock);

	rv = kthread_join(newl);
	if (rv)
		panic("thread join failed: %d", rv);

}
