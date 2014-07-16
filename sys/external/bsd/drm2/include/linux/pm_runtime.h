/*	$NetBSD: pm_runtime.h,v 1.2 2014/07/16 20:59:58 riastradh Exp $	*/

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

#ifndef _LINUX_PM_RUNTIME_H_
#define _LINUX_PM_RUNTIME_H_

struct device;

static inline void
pm_runtime_allow(struct device *dev __unused)
{
}

static inline void
pm_runtime_disable(struct device *dev __unused)
{
}

static inline int
pm_runtime_get_sync(struct device *dev __unused)
{
	return 0;
}

static inline void
pm_runtime_mark_last_busy(struct device *dev __unused)
{
}

static inline void
pm_runtime_put_autosuspend(struct device *dev __unused)
{
}

static inline void
pm_runtime_set_active(struct device *dev __unused)
{
}

static inline void
pm_runtime_set_autosuspend_delay(struct device *dev __unused,
    unsigned ms __unused)
{
}

static inline void
pm_runtime_use_autosuspend(struct device *dev __unused)
{
}

#endif  /* _LINUX_PM_RUNTIME_H_ */
