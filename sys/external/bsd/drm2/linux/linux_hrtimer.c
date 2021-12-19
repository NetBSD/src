/*	$NetBSD: linux_hrtimer.c,v 1.3 2021/12/19 11:55:47 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_hrtimer.c,v 1.3 2021/12/19 11:55:47 riastradh Exp $");

#include <sys/types.h>
#include <sys/callout.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>

static void hrtimer_fire(void *);

void
hrtimer_init(struct hrtimer *hrt, clockid_t clkid, enum hrtimer_mode mode)
{

	KASSERTMSG(clkid == CLOCK_MONOTONIC, "clkid %d", clkid);

	callout_init(&hrt->hrt_ch, CALLOUT_MPSAFE);
	callout_setfunc(&hrt->hrt_ch, hrtimer_fire, hrt);
	hrt->hrt_mode = mode;
}

static void
_hrtimer_schedule(struct hrtimer *hrt)
{
	int delta;

	switch (hrt->hrt_mode) {
	case HRTIMER_MODE_ABS:
		panic("absolute hrtimer NYI");
		break;
	case HRTIMER_MODE_REL:
		delta = ktime_to_ms(hrt->hrt_expires);
		break;
	default:
		panic("invalid hrtimer mode %d", hrt->hrt_mode);
	}
	callout_schedule(&hrt->hrt_ch, delta);
}

static void
hrtimer_fire(void *cookie)
{
	struct hrtimer *hrt = cookie;

	switch ((*hrt->function)(hrt)) {
	case HRTIMER_RESTART:
		_hrtimer_schedule(hrt);
		break;
	case HRTIMER_NORESTART:
		break;
	}

	callout_ack(&hrt->hrt_ch);
}

void
hrtimer_set_expires(struct hrtimer *hrt, ktime_t expires)
{

	hrt->hrt_expires = expires;
}

void
hrtimer_add_expires_ns(struct hrtimer *hrt, uint64_t ns)
{

	hrt->hrt_expires = ktime_add_ns(hrt->hrt_expires, ns);
}

void
hrtimer_start(struct hrtimer *hrt, ktime_t expires, enum hrtimer_mode mode)
{

	hrtimer_start_range_ns(hrt, expires, 0, mode);
}

void
hrtimer_start_range_ns(struct hrtimer *hrt, ktime_t expires, uint64_t range_ns,
    enum hrtimer_mode mode)
{

	hrt->hrt_expires = expires;
	(void)range_ns;
	hrt->hrt_mode = mode;
	_hrtimer_schedule(hrt);
}

int
hrtimer_cancel(struct hrtimer *hrt)
{
	bool active;

	/*
	 * Halt the callout and ascertain whether the hrtimer was
	 * active when we invoked hrtimer_cancel.
	 */
	if (callout_halt(&hrt->hrt_ch, NULL)) {
		/* Callout expired, meaning it was active.  */
		active = true;
	} else {
		/*
		 * Callout had not yet expired.  It will not expire
		 * now, so callout_pending is now stable and
		 * corresponds with whether the hrtimer was active or
		 * not.
		 */
		active = callout_pending(&hrt->hrt_ch);
	}
	return active;
}

bool
hrtimer_active(struct hrtimer *hrt)
{

	/*
	 * If the callout has been scheduled, but has not yet fired,
	 * then it is pending.
	 *
	 * If the callout has fired, but has not yet reached
	 * callout_ack, then it is invoking.
	 */
	return callout_pending(&hrt->hrt_ch) || callout_invoking(&hrt->hrt_ch);
}

uint64_t
hrtimer_forward(struct hrtimer *hrt, ktime_t now, ktime_t period)
{
	uint64_t now_ms, period_ms, expires_ms, nperiods;

	KASSERT(!callout_pending(&hrt->hrt_ch));

	/*
	 * Can't get better than 10ms precision (or ~1ms if you set
	 * HZ=1000) so not much point in doing this arithmetic at finer
	 * resolution than ms.
	 */
	now_ms = ktime_to_ms(now);
	period_ms = ktime_to_ms(period);
	expires_ms = ktime_to_ms(hrt->hrt_expires);

	/* If it hasn't yet expired, no overruns.  */
	if (now_ms < expires_ms)
		return 0;

	/* Advance it by as many periods as it should have fired.  */
	/* XXX fenceposts */
	nperiods = howmany(now_ms - expires_ms, period_ms);
	hrt->hrt_expires = ktime_add_ns(hrt->hrt_expires,
	    1000000*nperiods*period_ms);

	return nperiods;
}

uint64_t
hrtimer_forward_now(struct hrtimer *hrt, ktime_t period)
{

	return hrtimer_forward(hrt, ktime_get(), period);
}
