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

__KERNEL_RCSID(0, "$NetBSD: booke_stubs.c,v 1.2 2011/01/18 01:02:52 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <powerpc/instr.h>
#include <powerpc/booke/cpuvar.h>

#define	__stub	__section(".stub")

void tlb_set_asid(uint32_t) __stub;

void
tlb_set_asid(uint32_t asid)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_set_asid)(asid);
}

uint32_t tlb_get_asid(void) __stub;

uint32_t
tlb_get_asid(void)
{
	return (*cpu_md_ops.md_tlb_ops->md_tlb_get_asid)();
}

void tlb_invalidate_all(void) __stub;

void
tlb_invalidate_all(void)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_invalidate_all)();
}

void tlb_invalidate_globals(void) __stub;

void
tlb_invalidate_globals(void)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_invalidate_globals)();
}

void tlb_invalidate_asids(uint32_t, uint32_t) __stub;

void
tlb_invalidate_asids(uint32_t asid_lo, uint32_t asid_hi)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_invalidate_asids)(asid_lo, asid_hi);
}

void tlb_invalidate_addr(vaddr_t, uint32_t) __stub;

void
tlb_invalidate_addr(vaddr_t va, uint32_t asid)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_invalidate_addr)(va, asid);
}

bool tlb_update_addr(vaddr_t, uint32_t, uint32_t, bool) __stub;

bool
tlb_update_addr(vaddr_t va, uint32_t asid, uint32_t pte, bool insert_p)
{
	return (*cpu_md_ops.md_tlb_ops->md_tlb_update_addr)(va, asid, pte, insert_p);
}

void tlb_read_entry(size_t, struct tlbmask *) __stub;

void
tlb_read_entry(size_t pos, struct tlbmask *tlb)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_read_entry)(pos, tlb);
}

u_int tlb_record_asids(u_long *, uint32_t) __stub;

u_int
tlb_record_asids(u_long *bitmap, uint32_t start)
{
	return (*cpu_md_ops.md_tlb_ops->md_tlb_record_asids)(bitmap, start);
}

void *tlb_mapiodev(paddr_t, psize_t) __stub;

void *
tlb_mapiodev(paddr_t pa, psize_t len)
{
	return (*cpu_md_ops.md_tlb_ops->md_tlb_mapiodev)(pa, len);
}

void tlb_unmapiodev(vaddr_t, vsize_t) __stub;

void
tlb_unmapiodev(vaddr_t va, vsize_t len)
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_unmapiodev)(va, len);
}

int tlb_ioreserve(vaddr_t, vsize_t, uint32_t) __stub;

int
tlb_ioreserve(vaddr_t va, vsize_t len, uint32_t pte)
{
	return (*cpu_md_ops.md_tlb_ops->md_tlb_ioreserve)(va, len, pte);
}

int tlb_iorelease(vaddr_t) __stub;

int
tlb_iorelease(vaddr_t va)
{
	return (*cpu_md_ops.md_tlb_ops->md_tlb_iorelease)(va);
}

void tlb_dump(void (*)(const char *, ...)) __stub;

void
tlb_dump(void (*pr)(const char *, ...))
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_dump)(pr);
}

void tlb_walk(void *, bool (*)(void *, vaddr_t, uint32_t, uint32_t))
    __stub;

void
tlb_walk(void *ctx, bool (*func)(void *, vaddr_t, uint32_t, uint32_t))
{
	(*cpu_md_ops.md_tlb_ops->md_tlb_walk)(ctx, func);
}

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

const char *intr_string(int, int) __stub;

const char *
intr_string(int irq, int ist)
{
	return (*powerpc_intrsw->intrsw_string)(irq, ist);
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

#if 0
int splhigh(void) __stub;
#endif

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

void intr_cpu_init(struct cpu_info *) __stub;

void
intr_cpu_init(struct cpu_info *ci)
{
	(*powerpc_intrsw->intrsw_cpu_init)(ci);
}

void intr_init(void) __stub;

void
intr_init(void)
{
	(*powerpc_intrsw->intrsw_init)();
}

void
booke_fixup_stubs(void)
{
	extern uint32_t _ftext[];
	extern uint32_t _etext[];

	powerpc_fixup_stubs(_ftext, _etext);
}
