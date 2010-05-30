/*	$NetBSD: klock.c,v 1.2.2.2 2010/05/30 05:18:06 rmind Exp $	*/

/*
 * Copyright (c) 2007-2010 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: klock.c,v 1.2.2.2 2010/05/30 05:18:06 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * giant lock
 */

static volatile int lockcnt;

bool
rump_kernel_isbiglocked()
{

	return rumpuser_mutex_held(rump_giantlock) && lockcnt > 0;
}

void
rump_kernel_unlock_allbutone(int *countp)
{
	int minusone = lockcnt-1;

	KASSERT(rump_kernel_isbiglocked());
	if (minusone) {
		_kernel_unlock(minusone, countp);
	}
	KASSERT(lockcnt == 1);
	*countp = minusone;

	/*
	 * We drop lockcnt to 0 since rumpuser doesn't know that the
	 * kernel biglock is being used as the interlock for cv in
	 * tsleep.
	 */
	lockcnt = 0;
}

void
rump_kernel_ununlock_allbutone(int nlocks)
{

	KASSERT(rumpuser_mutex_held(rump_giantlock) && lockcnt == 0);
	lockcnt = 1;
	_kernel_lock(nlocks);
}

void
_kernel_lock(int nlocks)
{

	while (nlocks--) {
		if (!rumpuser_mutex_tryenter(rump_giantlock)) {
			struct lwp *l = curlwp;

			rump_unschedule_cpu1(l, NULL);
			rumpuser_mutex_enter_nowrap(rump_giantlock);
			rump_schedule_cpu(l);
		}
		lockcnt++;
	}
}

void
_kernel_unlock(int nlocks, int *countp)
{

	if (!rumpuser_mutex_held(rump_giantlock)) {
		KASSERT(nlocks == 0);
		if (countp)
			*countp = 0;
		return;
	}

	if (countp)
		*countp = lockcnt;
	if (nlocks == 0)
		nlocks = lockcnt;
	if (nlocks == -1) {
		KASSERT(lockcnt == 1);
		nlocks = 1;
	}
	KASSERT(nlocks <= lockcnt);
	while (nlocks--) {
		lockcnt--;
		rumpuser_mutex_exit(rump_giantlock);
	}
}

void
rump_user_unschedule(int nlocks, int *countp, void *interlock)
{

	_kernel_unlock(nlocks, countp);
	/*
	 * XXX: technically we should unschedule_cpu1() here, but that
	 * requires rump_intr_enter/exit to be implemented.
	 */
	rump_unschedule_cpu_interlock(curlwp, interlock);
}

void
rump_user_schedule(int nlocks, void *interlock)
{

	rump_schedule_cpu_interlock(curlwp, interlock);

	if (nlocks)
		_kernel_lock(nlocks);
}
