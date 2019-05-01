/*	$NetBSD: cpufunc.h,v 1.26 2019/05/01 15:17:49 maxv Exp $	*/

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

static inline void
x86_pause(void)
{
	asm volatile ("pause");
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

static inline uint64_t
rdtsc(void)
{
	uint32_t low, high;

	asm volatile (
		"rdtsc"
		: "=a" (low), "=d" (high)
		:
	);

	return (low | ((uint64_t)high << 32));
}

#ifndef XEN
void	x86_hotpatch(uint32_t, const uint8_t *, size_t);
void	x86_patch_window_open(u_long *, u_long *);
void	x86_patch_window_close(u_long, u_long);
void	x86_patch(bool);
#endif

void	x86_monitor(const void *, uint32_t, uint32_t);
void	x86_mwait(uint32_t, uint32_t);
/* x86_cpuid2() writes four 32bit values, %eax, %ebx, %ecx and %edx */
#define	x86_cpuid(a,b)	x86_cpuid2((a),0,(b))
void	x86_cpuid2(uint32_t, uint32_t, uint32_t *);

/* -------------------------------------------------------------------------- */

void	lidt(struct region_descriptor *);
void	lldt(u_short);
void	ltr(u_short);

void	lcr0(u_long);
u_long	rcr0(void);
void	lcr2(vaddr_t);
vaddr_t	rcr2(void);
void	lcr3(vaddr_t);
vaddr_t	rcr3(void);
void	lcr4(vaddr_t);
vaddr_t	rcr4(void);
void	lcr8(vaddr_t);
vaddr_t	rcr8(void);

register_t	rdr0(void);
void		ldr0(register_t);
register_t	rdr1(void);
void		ldr1(register_t);
register_t	rdr2(void);
void		ldr2(register_t);
register_t	rdr3(void);
void		ldr3(register_t);
register_t	rdr6(void);
void		ldr6(register_t);
register_t	rdr7(void);
void		ldr7(register_t);

u_int	x86_getss(void);
void	setds(int);
void	setes(int);
void	setfs(int);
void	setusergs(int);

/* -------------------------------------------------------------------------- */

union savefpu;
void	fnclex(void);
void	fninit(void);
void	fnsave(union savefpu *);
void	fnstcw(uint16_t *);
uint16_t fngetsw(void);
void	fnstsw(uint16_t *);
void	frstor(const union savefpu *);
void	clts(void);
void	stts(void);
void	fxsave(union savefpu *);
void	fxrstor(const union savefpu *);

void	x86_ldmxcsr(const uint32_t *);
void	x86_stmxcsr(uint32_t *);
void	fldummy(void);

static inline uint64_t
rdxcr(uint32_t xcr)
{
	uint32_t low, high;

	asm volatile (
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
	asm volatile (
		"xsetbv"
		:
		: "a" (low), "d" (high), "c" (xcr)
	);
}

void	xrstor(const union savefpu *, uint64_t);
void	xsave(union savefpu *, uint64_t);
void	xsaveopt(union savefpu *, uint64_t);

/* -------------------------------------------------------------------------- */

/* Use read_psl, write_psl when saving and restoring interrupt state. */
void	x86_disable_intr(void);
void	x86_enable_intr(void);
u_long	x86_read_psl(void);
void	x86_write_psl(u_long);

/* Use read_flags, write_flags to adjust other members of %eflags. */
u_long	x86_read_flags(void);
void	x86_write_flags(u_long);

void	x86_reset(void);

/* -------------------------------------------------------------------------- */

/* 
 * Some of the undocumented AMD64 MSRs need a 'passcode' to access.
 *
 * See LinuxBIOSv2: src/cpu/amd/model_fxx/model_fxx_init.c
 */

#define	OPTERON_MSR_PASSCODE	0x9c5a203aU

static inline uint64_t
rdmsr(u_int msr)
{
	uint32_t low, high;

	asm volatile (
		"rdmsr"
		: "=a" (low), "=d" (high)
		: "c" (msr)
	);

	return (low | ((uint64_t)high << 32));
}

uint64_t	rdmsr_locked(u_int);
int		rdmsr_safe(u_int, uint64_t *);

static inline void
wrmsr(u_int msr, uint64_t val)
{
	uint32_t low, high;

	low = val;
	high = val >> 32;
	asm volatile (
		"wrmsr"
		:
		: "a" (low), "d" (high), "c" (msr)
	);
}

void		wrmsr_locked(u_int, uint64_t);

#endif /* _KERNEL */

#endif /* !_X86_CPUFUNC_H_ */
