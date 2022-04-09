/*	$NetBSD: linux_stop_machine.c,v 1.4 2022/04/09 23:38:33 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: linux_stop_machine.c,v 1.4 2022/04/09 23:38:33 riastradh Exp $");

#include <sys/mutex.h> /* XXX work around cycle x86/mutex.h<->x86/intr.h */

#include <sys/atomic.h>
#include <sys/intr.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#include <linux/stop_machine.h>

struct stop_machine {
	int			(*callback)(void *);
	void			*cookie;
	volatile unsigned	ncpu;
	volatile bool		done;
};

static void
stop_machine_xcall(void *a, void *b)
{
	struct stop_machine *S = a;
	int s;

	/* Block all activity on this CPU.  */
	s = splhigh();

	/* Note that we're ready, and see whether we're the chosen one.  */
	membar_release();
	if (atomic_dec_uint_nv(&S->ncpu) != 0) {
		while (!atomic_load_acquire(&S->done))
			SPINLOCK_BACKOFF_HOOK;
		goto out;
	}
	membar_acquire();

	/* It's time.  Call the callback.  */
	S->callback(S->cookie);

	/* Notify everyone else that we're done.  */
	atomic_store_release(&S->done, true);

	/* Allow activity again.  */
out:	splx(s);
}

void
stop_machine(int (*callback)(void *), void *cookie, const struct kcpuset *cpus)
{
	struct stop_machine stop, *S = &stop;

	KASSERT(cpus == NULL);	/* not implemented */

	S->callback = callback;
	S->cookie = cookie;
	S->ncpu = ncpu;		/* XXX cpu hotplug */
	S->done = false;

	xc_wait(xc_broadcast(0, stop_machine_xcall, &S, NULL));
}
