/*	$NetBSD: hrtimer.h,v 1.6 2021/12/19 11:53:09 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_HRTIMER_H_
#define _LINUX_HRTIMER_H_

#include <sys/types.h>

#include <sys/callout.h>

#include <linux/ktime.h>
#include <linux/timer.h>

struct hrtimer {
	enum hrtimer_restart (*function)(struct hrtimer *);

	struct hrtimer_private	*hrt_private;
};

enum hrtimer_mode {
	HRTIMER_MODE_ABS,
	HRTIMER_MODE_REL,
	HRTIMER_MODE_REL_PINNED,
};

enum hrtimer_restart {
	HRTIMER_NORESTART,
	HRTIMER_RESTART,
};

#define	hrtimer_active		linux_hrtimer_active
#define	hrtimer_add_expires_ns	linux_hrtimer_add_expires_ns
#define	hrtimer_cancel		linux_hrtimer_cancel
#define	hrtimer_destroy		linux_hrtimer_destroy
#define	hrtimer_forward		linux_hrtimer_forward
#define	hrtimer_forward_now	linux_hrtimer_forward_now
#define	hrtimer_init		linux_hrtimer_init
#define	hrtimer_set_expires	linux_hrtimer_set_expiresp
#define	hrtimer_start		linux_hrtimer_start
#define	hrtimer_start_range_ns	linux_hrtimer_start_range_ns

void hrtimer_init(struct hrtimer *, clockid_t, enum hrtimer_mode);
void hrtimer_set_expires(struct hrtimer *, ktime_t);
void hrtimer_add_expires_ns(struct hrtimer *, uint64_t);
void hrtimer_start(struct hrtimer *, ktime_t, enum hrtimer_mode);
void hrtimer_start_range_ns(struct hrtimer *, ktime_t, uint64_t,
    enum hrtimer_mode);
int hrtimer_cancel(struct hrtimer *);
void hrtimer_destroy(struct hrtimer *);
bool hrtimer_active(struct hrtimer *);
uint64_t hrtimer_forward(struct hrtimer *, ktime_t, ktime_t);
uint64_t hrtimer_forward_now(struct hrtimer *, ktime_t);

#endif  /* _LINUX_HRTIMER_H_ */
