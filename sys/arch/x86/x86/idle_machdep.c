/*	$NetBSD: idle_machdep.c,v 1.2.2.2 2007/07/11 20:03:20 mjf Exp $	*/

/*-
 * Copyright (c)2002, 2006, 2007 YAMAMOTO Takashi,
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: idle_machdep.c,v 1.2.2.2 2007/07/11 20:03:20 mjf Exp $");

#include <sys/param.h>
#include <sys/cpu.h>

#include <machine/cpufunc.h>

#if defined(I686_CPU) || defined(__x86_64__)

static void
monitor(const void *addr)
{
	uint32_t hint = 0;
	uint32_t ext = 0;

	__asm __volatile (".byte 0x0f, 0x01, 0xc8" /* monitor */
	    :: "a"(addr), "c"(ext), "d"(hint));
}

static void
mwait(void)
{
	uint32_t hint = 0;
	uint32_t ext = 0;

	__asm __volatile (".byte 0x0f, 0x01, 0xc9" /* mwait */
	    :: "a"(hint), "c"(ext));
}

static void
cpu_idle_mwait(struct cpu_info *ci)
{

	monitor(&ci->ci_want_resched);
	if (__predict_false(ci->ci_want_resched)) {
		return;
	}
	mwait();
	ci->ci_want_resched = 0;
}

#endif /* defined(I686_CPU) || defined(__x86_64__) */

static void
cpu_idle_halt(struct cpu_info *ci)
{

	disable_intr();
	__insn_barrier();
	if (!__predict_false(ci->ci_want_resched)) {
		__asm __volatile ("sti; hlt");
	} else {
		enable_intr();
	}
	ci->ci_want_resched = 0;
}

void
cpu_idle(void)
{
	struct cpu_info *ci = curcpu();

#if defined(I686_CPU) || defined(__x86_64__)
	if ((ci->ci_feature2_flags & CPUID2_MONITOR) != 0) {
		cpu_idle_mwait(ci);
	} else {
		cpu_idle_halt(ci);
	}
#else /* defined(I686_CPU) || defined(__x86_64__) */
	cpu_idle_halt(ci);
#endif /* defined(I686_CPU) || defined(__x86_64__) */
}
