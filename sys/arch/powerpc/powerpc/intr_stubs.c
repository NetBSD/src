/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#define	__INTR_PRIVATE
#define	__INTR_NOINLINE

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: intr_stubs.c,v 1.3.14.1 2014/08/20 00:03:20 tls Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>

static int
null_splraise(int ipl)
{
	int cpl = curcpu()->ci_cpl;
	curcpu()->ci_cpl = ipl;
	return cpl;
}

static void
null_splx(int ipl)
{
	curcpu()->ci_cpl = ipl;
}

static const struct intrsw null_intrsw = {
	.intrsw_splraise = null_splraise,
	.intrsw_splx = null_splx,
};

const struct intrsw *powerpc_intrsw = &null_intrsw;

#define	__stub	__section(".stub") __noprofile

void *intr_establish(int, int, int, int (*)(void *), void *) __stub;

void *
intr_establish(int irq, int ipl, int ist, int (*func)(void *), void *arg)
{
	return (*powerpc_intrsw->intrsw_establish)(irq, ipl, ist, func, arg);
}

void intr_disestablish(void *) __stub;

void
intr_disestablish(void *ih)
{
	(*powerpc_intrsw->intrsw_disestablish)(ih);
}

const char *intr_string(int, int, char *, size_t) __stub;

const char *
intr_string(int irq, int ist, char *buf, size_t len)
{
	return (*powerpc_intrsw->intrsw_string)(irq, ist, buf, len);
}

void spl0(void) __stub;

void
spl0(void)
{
	(*powerpc_intrsw->intrsw_spl0)();
}

int splraise(int) __stub;

int 
splraise(int ipl)
{
	return (*powerpc_intrsw->intrsw_splraise)(ipl);
}

/*
 * This is called by softint_cleanup and can't be a stub but it can call
 * a stub.
 */
int 
splhigh(void)
{
	return splraise(IPL_HIGH);
}

void splx(int) __stub;

void
splx(int ipl)
{
	return (*powerpc_intrsw->intrsw_splx)(ipl);
}

void softint_init_md(struct lwp *, u_int, uintptr_t *) __stub;

void
softint_init_md(struct lwp *l, u_int level, uintptr_t *machdep_p)
{
	(*powerpc_intrsw->intrsw_softint_init_md)(l, level, machdep_p);
}

void softint_trigger(uintptr_t) __stub;

void
softint_trigger(uintptr_t machdep)
{
	(*powerpc_intrsw->intrsw_softint_trigger)(machdep);
}

void intr_cpu_attach(struct cpu_info *) __stub;

void
intr_cpu_attach(struct cpu_info *ci)
{
	(*powerpc_intrsw->intrsw_cpu_attach)(ci);
}

void intr_cpu_hatch(struct cpu_info *) __stub;

void
intr_cpu_hatch(struct cpu_info *ci)
{
	(*powerpc_intrsw->intrsw_cpu_hatch)(ci);
}

void intr_init(void) __stub;

void
intr_init(void)
{
	(*powerpc_intrsw->intrsw_init)();
}

void intr_critintr(struct trapframe *) __stub;

void
intr_critintr(struct trapframe *tf)
{
	(*powerpc_intrsw->intrsw_critintr)(tf);
	
}

void intr_extintr(struct trapframe *) __stub;

void
intr_extintr(struct trapframe *tf)
{
	(*powerpc_intrsw->intrsw_extintr)(tf);
	
}

void intr_decrintr(struct trapframe *) __stub;

void
intr_decrintr(struct trapframe *tf)
{
	(*powerpc_intrsw->intrsw_decrintr)(tf);
	
}

void intr_fitintr(struct trapframe *) __stub;

void
intr_fitintr(struct trapframe *tf)
{
	(*powerpc_intrsw->intrsw_fitintr)(tf);
	
}

void intr_wdogintr(struct trapframe *) __stub;

void
intr_wdogintr(struct trapframe *tf)
{
	(*powerpc_intrsw->intrsw_wdogintr)(tf);
}

void cpu_send_ipi(cpuid_t, uint32_t) __stub;

void
cpu_send_ipi(cpuid_t id, uint32_t mask)
{
	(*powerpc_intrsw->intrsw_cpu_send_ipi)(id, mask);
}
