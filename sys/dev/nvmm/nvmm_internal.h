/*	$NetBSD: nvmm_internal.h,v 1.6 2019/02/23 12:27:00 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#ifndef _NVMM_INTERNAL_H_
#define _NVMM_INTERNAL_H_

#define NVMM_MAX_MACHINES	128
#define NVMM_MAX_VCPUS		256
#define NVMM_MAX_SEGS		32
#define NVMM_MAX_RAM		(128ULL * (1 << 30))

struct nvmm_cpu {
	/* Shared. */
	bool present;
	nvmm_cpuid_t cpuid;
	kmutex_t lock;

	/* State buffer. */
	void *state;

	/* Last host CPU on which the VCPU ran. */
	int hcpu_last;

	/* Implementation-specific. */
	void *cpudata;
};

struct nvmm_seg {
	bool present;
	uintptr_t hva;
	size_t size;
	struct uvm_object *uobj;
};

struct nvmm_machine {
	bool present;
	nvmm_machid_t machid;
	pid_t procid;
	krwlock_t lock;

	/* Kernel */
	struct vmspace *vm;
	gpaddr_t gpa_begin;
	gpaddr_t gpa_end;

	/* Segments */
	struct nvmm_seg segs[NVMM_MAX_SEGS];

	/* CPU */
	struct nvmm_cpu cpus[NVMM_MAX_VCPUS];

	/* Implementation-specific */
	void *machdata;
};

struct nvmm_impl {
	bool (*ident)(void);
	void (*init)(void);
	void (*fini)(void);
	void (*capability)(struct nvmm_capability *);

	size_t conf_max;
	const size_t *conf_sizes;
	size_t state_size;

	void (*machine_create)(struct nvmm_machine *);
	void (*machine_destroy)(struct nvmm_machine *);
	int (*machine_configure)(struct nvmm_machine *, uint64_t, void *);

	int (*vcpu_create)(struct nvmm_machine *, struct nvmm_cpu *);
	void (*vcpu_destroy)(struct nvmm_machine *, struct nvmm_cpu *);
	void (*vcpu_setstate)(struct nvmm_cpu *, const void *, uint64_t);
	void (*vcpu_getstate)(struct nvmm_cpu *, void *, uint64_t);
	int (*vcpu_inject)(struct nvmm_machine *, struct nvmm_cpu *,
	    struct nvmm_event *);
	int (*vcpu_run)(struct nvmm_machine *, struct nvmm_cpu *,
	    struct nvmm_exit *);
};

int nvmm_vcpu_get(struct nvmm_machine *, nvmm_cpuid_t, struct nvmm_cpu **);
void nvmm_vcpu_put(struct nvmm_cpu *);

extern const struct nvmm_impl nvmm_x86_svm;
extern const struct nvmm_impl nvmm_x86_vmx;

#endif /* _NVMM_INTERNAL_H_ */
