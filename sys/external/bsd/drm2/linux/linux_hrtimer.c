/*	$NetBSD: linux_hrtimer.c,v 1.2 2021/12/19 11:53:09 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_hrtimer.c,v 1.2 2021/12/19 11:53:09 riastradh Exp $");

#include <sys/types.h>
#include <sys/callout.h>
#include <sys/kmem.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>

struct hrtimer_private {
	struct callout		ch;
	enum hrtimer_mode	mode;
	ktime_t			expires;
};

static void hrtimer_fire(void *);

void
hrtimer_init(struct hrtimer *hrt, clockid_t clkid, enum hrtimer_mode mode)
{
	struct hrtimer_private *H;

	KASSERTMSG(clkid == CLOCK_MONOTONIC, "clkid %d", clkid);

	H = hrt->hrt_private = kmem_zalloc(sizeof(*H), KM_SLEEP);

	callout_init(&H->ch, CALLOUT_MPSAFE);
	callout_setfunc(&H->ch, hrtimer_fire, H);
	H->mode = mode;
}

static void
_hrtimer_schedule(struct hrtimer *hrt)
{
	struct hrtimer_private *H = hrt->hrt_private;
	int delta;

	switch (H->mode) {
	case HRTIMER_MODE_ABS:
		panic("absolute hrtimer NYI");
		break;
	case HRTIMER_MODE_REL:
		delta = ktime_to_ms(H->expires);
		break;
	default:
		panic("invalid hrtimer mode %d", H->mode);
	}
	callout_schedule(&H->ch, delta);
}

static void
hrtimer_fire(void *cookie)
{
	struct hrtimer *hrt = cookie;
	struct hrtimer_private *H = hrt->hrt_private;

	switch ((*hrt->function)(hrt)) {
	case HRTIMER_RESTART:
		_hrtimer_schedule(hrt);
		break;
	case HRTIMER_NORESTART:
		break;
	}

	callout_ack(&H->ch);
}

void
hrtimer_set_expires(struct hrtimer *hrt, ktime_t expires)
{
	struct hrtimer_private *H = hrt->hrt_private;

	H->expires = expires;
}

void
hrtimer_add_expires_ns(struct hrtimer *hrt, uint64_t ns)
{
	struct hrtimer_private *H = hrt->hrt_private;

	H->expires = ktime_add_ns(H->expires, ns);
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
	struct hrtimer_private *H = hrt->hrt_private;

	H->expires = expires;
	(void)range_ns;
	H->mode = mode;
	_hrtimer_schedule(hrt);
}

int
hrtimer_cancel(struct hrtimer *hrt)
{
	struct hrtimer_private *H = hrt->hrt_private;
	bool active;

	/*
	 * Halt the callout and ascertain whether the hrtimer was
	 * active when we invoked hrtimer_cancel.
	 */
	if (callout_halt(&H->ch, NULL)) {
		/* Callout expired, meaning it was active.  */
		active = true;
	} else {
		/*
		 * Callout had not yet expired.  It will not expire
		 * now, so callout_pending is now stable and
		 * corresponds with whether the hrtimer was active or
		 * not.
		 */
		active = callout_pending(&H->ch);
	}
	return active;
}

void
hrtimer_destroy(struct hrtimer *hrt)
{
	struct hrtimer_private *H = hrt->hrt_private;

	callout_destroy(&H->ch);
	kmem_free(H, sizeof(*H));

	explicit_memset(hrt, 0, sizeof(*hrt)); /* paranoia */
}

bool
hrtimer_active(struct hrtimer *hrt)
{
	struct hrtimer_private *H = hrt->hrt_private;

	/*
	 * If the callout has been scheduled, but has not yet fired,
	 * then it is pending.
	 *
	 * If the callout has fired, but has not yet reached
	 * callout_ack, then it is invoking.
	 */
	return callout_pending(&H->ch) || callout_invoking(&H->ch);
}

uint64_t
hrtimer_forward(struct hrtimer *hrt, ktime_t now, ktime_t period)
{
	struct hrtimer_private *H = hrt->hrt_private;
	uint64_t now_ms, period_ms, expires_ms, nperiods;

	KASSERT(!callout_pending(&H->ch));

	/*
	 * Can't get better than 10ms precision (or ~1ms if you set
	 * HZ=1000) so not much point in doing this arithmetic at finer
	 * resolution than ms.
	 */
	now_ms = ktime_to_ms(now);
	period_ms = ktime_to_ms(period);
	expires_ms = ktime_to_ms(H->expires);

	/* If it hasn't yet expired, no overruns.  */
	if (now_ms < expires_ms)
		return 0;

	/* Advance it by as many periods as it should have fired.  */
	/* XXX fenceposts */
	nperiods = howmany(now_ms - expires_ms, period_ms);
	H->expires = ktime_add_ns(H->expires, 1000000*nperiods*period_ms);

	return nperiods;
}

uint64_t
hrtimer_forward_now(struct hrtimer *hrt, ktime_t period)
{

	return hrtimer_forward(hrt, ktime_get(), period);
}
