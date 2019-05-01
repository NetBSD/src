/*	$NetBSD: nvmm.h,v 1.9 2019/05/01 09:20:21 maxv Exp $	*/

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

#ifndef _NVMM_H_
#define _NVMM_H_

#include <sys/types.h>

#ifndef _KERNEL
#include <stdbool.h>
#endif

typedef uint64_t	gpaddr_t;
typedef uint64_t	gvaddr_t;

typedef uint32_t	nvmm_machid_t;
typedef uint32_t	nvmm_cpuid_t;

#ifdef __x86_64__
#include <dev/nvmm/x86/nvmm_x86.h>
#endif

#define NVMM_EXIT_NONE		0x0000000000000000ULL
#define NVMM_EXIT_MEMORY	0x0000000000000001ULL
#define NVMM_EXIT_IO		0x0000000000000002ULL
#define NVMM_EXIT_MSR		0x0000000000000003ULL /* x86 only? */
#define NVMM_EXIT_INT_READY	0x0000000000000004ULL
#define NVMM_EXIT_NMI_READY	0x0000000000000005ULL
#define NVMM_EXIT_HALTED	0x0000000000000006ULL
#define NVMM_EXIT_SHUTDOWN	0x0000000000000007ULL
/* Range 0x1000-0x10000 is MD. */
#define NVMM_EXIT_INVALID	0xFFFFFFFFFFFFFFFFULL

struct nvmm_exit {
	uint64_t reason;
	union nvmm_exit_md u;
	uint64_t exitstate[8];
};

enum nvmm_event_type {
	NVMM_EVENT_INTERRUPT_HW,
	NVMM_EVENT_INTERRUPT_SW,
	NVMM_EVENT_EXCEPTION
};

struct nvmm_event {
	enum nvmm_event_type type;
	uint64_t vector;
	union {
		/* NVMM_EVENT_INTERRUPT_HW */
		uint8_t prio;

		/* NVMM_EVENT_EXCEPTION */
		uint64_t error;
	} u;
};

#define NVMM_CAPABILITY_VERSION		1

struct nvmm_capability {
	uint64_t version;
	uint64_t state_size;
	uint64_t max_machines;
	uint64_t max_vcpus;
	uint64_t max_ram;
	struct nvmm_cap_md arch;
};

struct nvmm_comm_page {
	/* State. */
	uint64_t state_wanted;
	uint64_t state_cached;
	uint64_t state_commit;
	struct nvmm_vcpu_state state;

	/* Event. */
	bool event_commit;
	struct nvmm_event event;
};

/*
 * Bits 20:27 -> machid
 * Bits 12:19 -> cpuid
 */
#define NVMM_COMM_OFF(machid, cpuid)	\
	((((uint64_t)machid & 0xFFULL) << 20) | (((uint64_t)cpuid & 0xFFULL) << 12))

#define NVMM_COMM_MACHID(off)		\
	((off >> 20) & 0xFF)

#define NVMM_COMM_CPUID(off)		\
	((off >> 12) & 0xFF)

#endif
