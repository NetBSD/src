/*	$NetBSD: nvmm.h,v 1.1 2018/11/10 09:28:56 maxv Exp $	*/

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

#ifndef _LIBNVMM_H_
#define _LIBNVMM_H_

#include <stdint.h>
#include <stdbool.h>

#include <dev/nvmm/nvmm.h>
#include <dev/nvmm/nvmm_ioctl.h>
#ifdef __x86_64__
#include <dev/nvmm/x86/nvmm_x86.h>
#endif

struct nvmm_area {
	gpaddr_t gpa;
	uintptr_t hva;
	size_t size;
};

struct nvmm_machine {
	nvmm_machid_t machid;
	struct nvmm_area *areas;
	size_t nareas;
};

struct nvmm_io {
	uint64_t port;
	bool in;
	size_t size;
	uint8_t data[8];
};

struct nvmm_mem {
	gvaddr_t gva;
	gpaddr_t gpa;
	bool write;
	size_t size;
	uint8_t data[8];
};

#define NVMM_PROT_READ		0x01
#define NVMM_PROT_WRITE		0x02
#define NVMM_PROT_EXEC		0x04
#define NVMM_PROT_USER		0x08
#define NVMM_PROT_ALL		0x0F
typedef uint64_t nvmm_prot_t;

int nvmm_capability(struct nvmm_capability *);

int nvmm_machine_create(struct nvmm_machine *);
int nvmm_machine_destroy(struct nvmm_machine *);
int nvmm_machine_configure(struct nvmm_machine *, uint64_t, void *);

int nvmm_vcpu_create(struct nvmm_machine *, nvmm_cpuid_t);
int nvmm_vcpu_destroy(struct nvmm_machine *, nvmm_cpuid_t);
int nvmm_vcpu_setstate(struct nvmm_machine *, nvmm_cpuid_t, void *, uint64_t);
int nvmm_vcpu_getstate(struct nvmm_machine *, nvmm_cpuid_t, void *, uint64_t);
int nvmm_vcpu_inject(struct nvmm_machine *, nvmm_cpuid_t, struct nvmm_event *);
int nvmm_vcpu_run(struct nvmm_machine *, nvmm_cpuid_t, struct nvmm_exit *);

int nvmm_gpa_map(struct nvmm_machine *, uintptr_t, gpaddr_t, size_t, int);
int nvmm_gpa_unmap(struct nvmm_machine *, uintptr_t, gpaddr_t, size_t);

int nvmm_gva_to_gpa(struct nvmm_machine *, nvmm_cpuid_t, gvaddr_t, gpaddr_t *,
    nvmm_prot_t *);
int nvmm_gpa_to_hva(struct nvmm_machine *, gpaddr_t, uintptr_t *);

int nvmm_assist_io(struct nvmm_machine *, nvmm_cpuid_t, struct nvmm_exit *,
    void (*)(struct nvmm_io *));
int nvmm_assist_mem(struct nvmm_machine *, nvmm_cpuid_t, struct nvmm_exit *,
    void (*)(struct nvmm_mem *));

#endif /* _LIBNVMM_H_ */
