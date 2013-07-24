/*	$NetBSD: timer.h,v 1.1.2.1 2013/07/24 01:54:04 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_TIMER_H_
#define _LINUX_TIMER_H_

#include <sys/callout.h>

struct timer_list {
	struct callout tl_callout;
};

static inline void
setup_timer(struct timer_list *timer, void (*fn)(unsigned long),
    unsigned long arg)
{

	callout_init(&timer->tl_callout, 0);

	/* XXX Super-sketchy casts!  */
	callout_setfunc(&timer->tl_callout, (void (*)(void *))fn,
	    (void *)(uintptr_t)arg);
}

static inline void
mod_timer(struct timer_list *timer, unsigned long ticks)
{
	callout_schedule(&timer->tl_callout, ticks);
}

static inline void
del_timer_sync(struct timer_list *timer)
{
	callout_halt(&timer->tl_callout, NULL);
	callout_destroy(&timer->tl_callout);
}

#endif  /* _LINUX_TIMER_H_ */
