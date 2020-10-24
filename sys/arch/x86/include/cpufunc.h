/*	$NetBSD: cpufunc.h,v 1.42 2020/10/24 07:14:29 mgorny Exp $	*/

/*
 * Copyright (c) 1998, 2007, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Andrew Doran.
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

#ifndef _X86_CPUFUNC_H_
#define	_X86_CPUFUNC_H_

/*
 * Functions to provide access to x86-specific instructions.
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/segments.h>
#include <machine/specialreg.h>

#ifdef _KERNEL
#if defined(_KERNEL_OPT)
#include "opt_xen.h"
#endif

static inline void
x86_pause(void)
{
	__asm volatile ("pause");
}

void	x86_lfence(void);
void	x86_sfence(void);
void	x86_mfence(void);
void	x86_flush(void);
void	x86_hlt(void);
void	x86_stihlt(void);
void	tlbflush(void);
void	tlbflushg(void);
void	invlpg(vaddr_t);
void	wbinvd(void);
void	breakpoint(void);

#define INVPCID_ADDRESS		0
#define INVPCID_CONTEXT		1
#define INVPCID_ALL		2
#define INVPCID_ALL_NONGLOBAL	3

static inline void
invpcid(register_t op, uint64_t pcid, vaddr_t va)
{
	struct {
		uint64_t pcid;
		uint64_t addr;
	} desc = {
		.pcid = pcid,
		.addr = va
	};

	__asm volatile (
		"invpcid %[desc],%[op]"
		:
		: [desc] "m" (desc), [op] "r" (op)
		: "memory"
	);
}

extern uint64_t (*rdtsc)(void);

#define _SERIALIZE_lfence	__asm volatile ("lfence")
#define _SERIALIZE_mfence	__asm volatile ("mfence")
#define _SERIALIZE_cpuid	__asm volatile ("xor %%eax, %%eax;cpuid" ::: \
	    "eax", "ebx", "ecx", "edx");

#define RDTSCFUNC(fence)			\
static inline uint64_t				\
rdtsc_##fence(void)				\
{						\
	uint32_t low, high;			\
						\
	_SERIALIZE_##fence;			\
	__asm volatile (			\
		"rdtsc"				\
		: "=a" (low), "=d" (high)	\
		:				\
	);					\
						\
	return (low | ((uint64_t)high << 32));	\
}

RDTSCFUNC(lfence)
RDTSCFUNC(mfence)
RDTSCFUNC(cpuid)

#undef _SERIALIZE_LFENCE
#undef _SERIALIZE_MFENCE
#undef _SERIALIZE_CPUID


#ifndef XENPV
struct x86_hotpatch_source {
	uint8_t *saddr;
	uint8_t *eaddr;
};

struct x86_hotpatch_descriptor {
	uint8_t name;
	uint8_t nsrc;
	const struct x86_hotpatch_source *srcs[];
};

void	x86_hotpatch(uint8_t, uint8_t);
void	x86_patch(bool);
#endif

void	x86_monitor(const void *, uint32_t, uint32_t);
void	x86_mwait(uint32_t, uint32_t);

static inline void
x86_cpuid2(uint32_t eax, uint32_t ecx, uint32_t *regs)
{
	uint32_t ebx, edx;

	__asm volatile (
		"cpuid"
		: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
		: "a" (eax), "c" (ecx)
	);

	regs[0] = eax;
	regs[1] = ebx;
	regs[2] = ecx;
	regs[3] = edx;
}
#define x86_cpuid(a,b)	x86_cpuid2((a), 0, (b))

/* -------------------------------------------------------------------------- */

void	lidt(struct region_descriptor *);
void	lldt(u_short);
void	ltr(u_short);

static inline uint16_t
x86_getss(void)
{
	uint16_t val;

	__asm volatile (
		"mov	%%ss,%[val]"
		: [val] "=r" (val)
		:
	);
	return val;
}

static inline void
setds(uint16_t val)
{
	__asm volatile (
		"mov	%[val],%%ds"
		:
		: [val] "r" (val)
	);
}

static inline void
setes(uint16_t val)
{
	__asm volatile (
		"mov	%[val],%%es"
		:
		: [val] "r" (val)
	);
}

static inline void
setfs(uint16_t val)
{
	__asm volatile (
		"mov	%[val],%%fs"
		:
		: [val] "r" (val)
	);
}

void	setusergs(int);

/* -------------------------------------------------------------------------- */

#define FUNC_CR(crnum)					\
	static inline void lcr##crnum(register_t val)	\
	{						\
		__asm volatile (				\
			"mov	%[val],%%cr" #crnum	\
			:				\
			: [val] "r" (val)		\
			: "memory"			\
		);					\
	}						\
	static inline register_t rcr##crnum(void)	\
	{						\
		register_t val;				\
		__asm volatile (				\
			"mov	%%cr" #crnum ",%[val]"	\
			: [val] "=r" (val)		\
			:				\
		);					\
		return val;				\
	}

#define PROTO_CR(crnum)					\
	void lcr##crnum(register_t);			\
	register_t rcr##crnum(void);

#ifndef XENPV
FUNC_CR(0)
FUNC_CR(2)
FUNC_CR(3)
#else
PROTO_CR(0)
PROTO_CR(2)
PROTO_CR(3)
#endif

FUNC_CR(4)
FUNC_CR(8)

/* -------------------------------------------------------------------------- */

#define FUNC_DR(drnum)					\
	static inline void ldr##drnum(register_t val)	\
	{						\
		__asm volatile (				\
			"mov	%[val],%%dr" #drnum	\
			:				\
			: [val] "r" (val)		\
		);					\
	}						\
	static inline register_t rdr##drnum(void)	\
	{						\
		register_t val;				\
		__asm volatile (				\
			"mov	%%dr" #drnum ",%[val]"	\
			: [val] "=r" (val)		\
			:				\
		);					\
		return val;				\
	}

#define PROTO_DR(drnum)					\
	register_t rdr##drnum(void);			\
	void ldr##drnum(register_t);

#ifndef XENPV
FUNC_DR(0)
FUNC_DR(1)
FUNC_DR(2)
FUNC_DR(3)
FUNC_DR(6)
FUNC_DR(7)
#else
PROTO_DR(0)
PROTO_DR(1)
PROTO_DR(2)
PROTO_DR(3)
PROTO_DR(6)
PROTO_DR(7)
#endif

/* -------------------------------------------------------------------------- */

union savefpu;

static inline void
fninit(void)
{
	__asm volatile ("fninit" ::: "memory");
}

static inline void
fnclex(void)
{
	__asm volatile ("fnclex");
}

static inline void
fnstcw(uint16_t *val)
{
	__asm volatile (
		"fnstcw	%[val]"
		: [val] "=m" (*val)
		:
	);
}

static inline void
fnstsw(uint16_t *val)
{
	__asm volatile (
		"fnstsw	%[val]"
		: [val] "=m" (*val)
		:
	);
}

static inline void
clts(void)
{
	__asm volatile ("clts" ::: "memory");
}

void	stts(void);

static inline void
x86_stmxcsr(uint32_t *val)
{
	__asm volatile (
		"stmxcsr %[val]"
		: [val] "=m" (*val)
		:
	);
}

static inline void
x86_ldmxcsr(uint32_t *val)
{
	__asm volatile (
		"ldmxcsr %[val]"
		:
		: [val] "m" (*val)
	);
}

void	fldummy(void);

static inline uint64_t
rdxcr(uint32_t xcr)
{
	uint32_t low, high;

	__asm volatile (
		"xgetbv"
		: "=a" (low), "=d" (high)
		: "c" (xcr)
	);

	return (low | ((uint64_t)high << 32));
}

static inline void
wrxcr(uint32_t xcr, uint64_t val)
{
	uint32_t low, high;

	low = val;
	high = val >> 32;
	__asm volatile (
		"xsetbv"
		:
		: "a" (low), "d" (high), "c" (xcr)
	);
}

static inline void
fnsave(void *addr)
{
	uint8_t *area = addr;

	__asm volatile (
		"fnsave	%[area]"
		: [area] "=m" (*area)
		:
		: "memory"
	);
}

static inline void
frstor(const void *addr)
{
	const uint8_t *area = addr;

	__asm volatile (
		"frstor	%[area]"
		:
		: [area] "m" (*area)
		: "memory"
	);
}

static inline void
fxsave(void *addr)
{
	uint8_t *area = addr;

	__asm volatile (
		"fxsave	%[area]"
		: [area] "=m" (*area)
		:
		: "memory"
	);
}

static inline void
fxrstor(const void *addr)
{
	const uint8_t *area = addr;

	__asm volatile (
		"fxrstor %[area]"
		:
		: [area] "m" (*area)
		: "memory"
	);
}

static inline void
xsave(void *addr, uint64_t mask)
{
	uint8_t *area = addr;
	uint32_t low, high;

	low = mask;
	high = mask >> 32;
	__asm volatile (
		"xsave	%[area]"
		: [area] "=m" (*area)
		: "a" (low), "d" (high)
		: "memory"
	);
}

static inline void
xsaveopt(void *addr, uint64_t mask)
{
	uint8_t *area = addr;
	uint32_t low, high;

	low = mask;
	high = mask >> 32;
	__asm volatile (
		"xsaveopt %[area]"
		: [area] "=m" (*area)
		: "a" (low), "d" (high)
		: "memory"
	);
}

static inline void
xrstor(const void *addr, uint64_t mask)
{
	const uint8_t *area = addr;
	uint32_t low, high;

	low = mask;
	high = mask >> 32;
	__asm volatile (
		"xrstor %[area]"
		:
		: [area] "m" (*area), "a" (low), "d" (high)
		: "memory"
	);
}

#ifdef __x86_64__
static inline void
fxsave64(void *addr)
{
	uint8_t *area = addr;

	__asm volatile (
		"fxsave64	%[area]"
		: [area] "=m" (*area)
		:
		: "memory"
	);
}

static inline void
fxrstor64(const void *addr)
{
	const uint8_t *area = addr;

	__asm volatile (
		"fxrstor64 %[area]"
		:
		: [area] "m" (*area)
		: "memory"
	);
}

static inline void
xsave64(void *addr, uint64_t mask)
{
	uint8_t *area = addr;
	uint32_t low, high;

	low = mask;
	high = mask >> 32;
	__asm volatile (
		"xsave64	%[area]"
		: [area] "=m" (*area)
		: "a" (low), "d" (high)
		: "memory"
	);
}

static inline void
xsaveopt64(void *addr, uint64_t mask)
{
	uint8_t *area = addr;
	uint32_t low, high;

	low = mask;
	high = mask >> 32;
	__asm volatile (
		"xsaveopt64 %[area]"
		: [area] "=m" (*area)
		: "a" (low), "d" (high)
		: "memory"
	);
}

static inline void
xrstor64(const void *addr, uint64_t mask)
{
	const uint8_t *area = addr;
	uint32_t low, high;

	low = mask;
	high = mask >> 32;
	__asm volatile (
		"xrstor64 %[area]"
		:
		: [area] "m" (*area), "a" (low), "d" (high)
		: "memory"
	);
}
#endif

/* -------------------------------------------------------------------------- */

#ifdef XENPV
void x86_disable_intr(void);
void x86_enable_intr(void);
#else
static inline void
x86_disable_intr(void)
{
	__asm volatile ("cli" ::: "memory");
}

static inline void
x86_enable_intr(void)
{
	__asm volatile ("sti" ::: "memory");
}
#endif /* XENPV */

/* Use read_psl, write_psl when saving and restoring interrupt state. */
u_long	x86_read_psl(void);
void	x86_write_psl(u_long);

/* Use read_flags, write_flags to adjust other members of %eflags. */
u_long	x86_read_flags(void);
void	x86_write_flags(u_long);

void	x86_reset(void);

/* -------------------------------------------------------------------------- */

/* 
 * Some of the undocumented AMD64 MSRs need a 'passcode' to access.
 * See LinuxBIOSv2: src/cpu/amd/model_fxx/model_fxx_init.c
 */
#define	OPTERON_MSR_PASSCODE	0x9c5a203aU

static inline uint64_t
rdmsr(u_int msr)
{
	uint32_t low, high;

	__asm volatile (
		"rdmsr"
		: "=a" (low), "=d" (high)
		: "c" (msr)
	);

	return (low | ((uint64_t)high << 32));
}

static inline uint64_t
rdmsr_locked(u_int msr)
{
	uint32_t low, high, pass = OPTERON_MSR_PASSCODE;

	__asm volatile (
		"rdmsr"
		: "=a" (low), "=d" (high)
		: "c" (msr), "D" (pass)
	);

	return (low | ((uint64_t)high << 32));
}

int	rdmsr_safe(u_int, uint64_t *);

static inline void
wrmsr(u_int msr, uint64_t val)
{
	uint32_t low, high;

	low = val;
	high = val >> 32;
	__asm volatile (
		"wrmsr"
		:
		: "a" (low), "d" (high), "c" (msr)
		: "memory"
	);
}

static inline void
wrmsr_locked(u_int msr, uint64_t val)
{
	uint32_t low, high, pass = OPTERON_MSR_PASSCODE;

	low = val;
	high = val >> 32;
	__asm volatile (
		"wrmsr"
		:
		: "a" (low), "d" (high), "c" (msr), "D" (pass)
		: "memory"
	);
}

#endif /* _KERNEL */

#endif /* !_X86_CPUFUNC_H_ */
