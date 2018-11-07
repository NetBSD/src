/*	$NetBSD: nvmm.h,v 1.1 2018/11/07 07:43:08 maxv Exp $	*/

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

enum nvmm_exit_reason {
	NVMM_EXIT_NONE		= 0x0000000000000000,

	/* General. */
	NVMM_EXIT_MEMORY	= 0x0000000000000001,
	NVMM_EXIT_IO		= 0x0000000000000002,
	NVMM_EXIT_MSR		= 0x0000000000000003,
	NVMM_EXIT_INT_READY	= 0x0000000000000004,
	NVMM_EXIT_NMI_READY	= 0x0000000000000005,
	NVMM_EXIT_SHUTDOWN	= 0x0000000000000006,

	/* Instructions (x86). */
	NVMM_EXIT_HLT		= 0x0000000000001000,
	NVMM_EXIT_MONITOR	= 0x0000000000001001,
	NVMM_EXIT_MWAIT		= 0x0000000000001002,
	NVMM_EXIT_MWAIT_COND	= 0x0000000000001003,

	NVMM_EXIT_INVALID	= 0xFFFFFFFFFFFFFFFF
};

enum nvmm_exit_memory_perm {
	NVMM_EXIT_MEMORY_READ,
	NVMM_EXIT_MEMORY_WRITE,
	NVMM_EXIT_MEMORY_EXEC
};

struct nvmm_exit_memory {
	enum nvmm_exit_memory_perm perm;
	gpaddr_t gpa;
	uint8_t inst_len;
	uint8_t inst_bytes[15];
	uint64_t npc;
};

enum nvmm_exit_io_type {
	NVMM_EXIT_IO_IN,
	NVMM_EXIT_IO_OUT
};

struct nvmm_exit_io {
	enum nvmm_exit_io_type type;
	uint16_t port;
	int seg;
	uint8_t address_size;
	uint8_t operand_size;
	bool rep;
	bool str;
	uint64_t npc;
};

enum nvmm_exit_msr_type {
	NVMM_EXIT_MSR_RDMSR,
	NVMM_EXIT_MSR_WRMSR
};

struct nvmm_exit_msr {
	enum nvmm_exit_msr_type type;
	uint64_t msr;
	uint64_t val;
	uint64_t npc;
};

struct nvmm_exit {
	enum nvmm_exit_reason reason;
	union {
		struct nvmm_exit_memory mem;
		struct nvmm_exit_io io;
		struct nvmm_exit_msr msr;
	} u;
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
	union {
		struct {
			uint64_t xcr0_mask;
			uint64_t mxcsr_mask;
			uint64_t conf_cpuid_maxops;
		} x86;
		uint64_t rsvd[8];
	} u;
};

#endif
