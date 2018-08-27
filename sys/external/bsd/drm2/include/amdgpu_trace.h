/*	$NetBSD: amdgpu_trace.h,v 1.1 2018/08/27 14:02:32 riastradh Exp $	*/

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

#ifndef	_AMDGPU_TRACE_H_
#define	_AMDGPU_TRACE_H_

static inline void
trace_amdgpu_bo_list_set(struct amdgpu_bo_list *list, struct amdgpu_bo *obj)
{
}

static inline void
trace_amdgpu_cs(struct amdgpu_cs_parser *parser, int i)
{
}

static inline void
trace_amdgpu_cs_ioctl(struct amdgpu_job *job)
{
}

static inline void
trace_amdgpu_semaphore_signale(u32 ring, struct amdgpu_semaphore *semaphore)
{
}

static inline void
trace_amdgpu_semaphore_wait(u32 ring, struct amdgpu_semaphore *semaphore)
{
}

static inline void
trace_amdgpu_sched_run_job(struct amdgpu_job *job)
{
}

static inline void
trace_amdgpu_bo_create(struct amdgpu_bo *bo)
{
}

static inline void
trace_amdgpu_vm_grab_id(unsigned vmid, u32 ring)
{
}

static inline void
trace_amdgpu_vm_flush(uint64_t pd_addr, u32 ring, unsigned vmid)
{
}

static inline void
trace_amdgpu_vm_set_page(uint64_t pe, uint64_t addr, unsigned count,
    uint32_t incr, uint32_t flags)
{
}

static inline void
trace_amdgpu_vm_bo_update(struct amdgpu_bo_va_mapping *mapping)
{
}

static inline void
trace_amdgpu_vm_bo_mapping(struct amdgpu_bo_va_mapping *mapping)
{
}

static inline bool
trace_amdgpu_vm_bo_mapping_enabled(void)
{
	return false;
}

static inline void
trace_amdgpu_vm_bo_map(struct amdgpu_bo_va *bo_va,
    struct amdgpu_bo_va_mapping *mapping)
{
}

static inline void
trace_amdgpu_vm_bo_unmap(struct amdgpu_bo_va *bo_va,
    struct amdgpu_bo_va_mapping *mapping)
{
}

#endif	/* _AMDGPU_TRACE_H_ */
