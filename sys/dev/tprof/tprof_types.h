/*	$NetBSD: tprof_types.h,v 1.7 2023/04/11 10:07:12 msaitoh Exp $	*/

/*-
 * Copyright (c)2010,2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_TPROF_TPROF_TYPES_H_
#define _DEV_TPROF_TPROF_TYPES_H_

/*
 * definitions used by both kernel and userland
 */

#if defined(_KERNEL)
#include <sys/types.h>
#else
#include <stdint.h>
#endif

#define TPROF_MAXCOUNTERS	32
typedef uint32_t tprof_countermask_t;
#define TPROF_COUNTERMASK_ALL	__BITS(31, 0)

typedef struct {
	uint32_t s_pid;		/* process id */
	uint32_t s_lwpid;	/* lwp id */
	uint32_t s_cpuid;	/* cpu id */
	uint32_t s_flags;	/* flags and counterID */
#define TPROF_SAMPLE_INKERNEL	0x00000001 /* s_pc is in kernel address space */
#define	TPROF_SAMPLE_COUNTER_MASK 0xff000000 /* 0..(TPROF_MAXCOUNTERS-1) */
	uintptr_t s_pc;		/* program counter */
} tprof_sample_t;

typedef struct tprof_param {
	u_int p_counter;	/* 0..(TPROF_MAXCOUNTERS-1) */
	u_int p__unused;
	uint64_t p_event;	/* event class */
	uint64_t p_unit;	/* unit within the event class */
	uint64_t p_flags;
#define	TPROF_PARAM_KERN		0x1
#define	TPROF_PARAM_USER		0x2
#define	TPROF_PARAM_PROFILE		0x4
#define	TPROF_PARAM_VALUE2_MASK		__BITS(63, 60)
#define	TPROF_PARAM_VALUE2_SCALE	__SHIFTIN(1, TPROF_PARAM_VALUE2_MASK)
#define	TPROF_PARAM_VALUE2_TRIGGERCOUNT	__SHIFTIN(2, TPROF_PARAM_VALUE2_MASK)
	uint64_t p_value;	/* initial value */
	uint64_t p_value2;
	/*
	 * p_value2 is an optional value. (p_flags & TPROF_PARAM_VALUE2_MASK)
	 * determines the usage.
	 *
	 * TPROF_PARAM_VALUE2_SCALE:
	 *   Specify the counter speed as the reciprocal of the cycle counter
	 *   speed ratio. if the counter is N times slower than the cycle
	 *   counter, p_value2 is (0x1_0000_0000 / N). 0 is treated as 1.0.
	 * TPROF_PARAM_VALUE2_TRIGGERCOUNT:
	 *   When the event counter counts up p_value2, an interrupt for
	 *   profile is generated. 0 is treated as 1.
	 */
} tprof_param_t;

typedef struct tprof_counts {
	uint32_t c_cpu;				/* W */
	uint32_t c_ncounters;			/* R */
	tprof_countermask_t c_runningmask;	/* R */
	uint32_t c__unused;
	uint64_t c_count[TPROF_MAXCOUNTERS];	/* R */
} tprof_counts_t;

/* ti_ident */
#define	TPROF_IDENT_NONE		0x00
#define	TPROF_IDENT_INTEL_GENERIC	0x01
#define	TPROF_IDENT_AMD_GENERIC		0x02
#define	TPROF_IDENT_ARMV8_GENERIC	0x03
#define	TPROF_IDENT_ARMV7_GENERIC	0x04

#endif /* _DEV_TPROF_TPROF_TYPES_H_ */
