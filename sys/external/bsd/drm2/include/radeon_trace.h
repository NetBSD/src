/*	$NetBSD: radeon_trace.h,v 1.1 2014/07/16 20:59:58 riastradh Exp $	*/

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

#ifndef _RADEON_TRACE_H_
#define _RADEON_TRACE_H_

#include <sys/cdefs.h>

#include <sys/types.h>

struct drm_device;
struct radeon_bo_parser;
struct radeon_bo_va;
struct radeon_cs_parser;

static inline void
trace_radeon_bo_create(struct radeon_bo *bo __unused)
{
}

static inline void
trace_radeon_cs(struct radeon_cs_parser *p __unused)
{
}

static inline void
trace_radeon_fence_emit(struct drm_device *dev __unused, int ring __unused,
    uint32_t seqno __unused)
{
}

static inline void
trace_radeon_fence_wait_begin(struct drm_device *dev __unused,
    int ring __unused, uint32_t seqno __unused)
{
}

static inline void
trace_radeon_fence_wait_end(struct drm_device *dev __unused, int ring __unused,
    uint32_t seqno __unused)
{
}

static inline void
trace_radeon_semaphore_signale(int ring __unused,
    struct radeon_semaphore *sem __unused)
{
}

static inline void
trace_radeon_semaphore_wait(int ring __unused,
    struct radeon_semaphore *sem __unused)
{
}

static inline void
trace_radeon_vm_bo_update(struct radeon_bo_va *bo_va __unused)
{
}

static inline void
trace_radeon_vm_grab_id(unsigned vmid __unused, int ring __unused)
{
}

static inline void
trace_radeon_vm_set_page(uint64_t pe __unused, uint64_t addr __unused,
    unsigned count __unused, uint32_t incr __unused, uint32_t flags __unused)
{
}

#endif	/* _RADEON_TRACE_H_ */
