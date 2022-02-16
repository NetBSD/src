/*	$NetBSD: pmap_kernel.c,v 1.13 2022/02/16 23:31:13 riastradh Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: pmap_kernel.c,v 1.13 2022/02/16 23:31:13 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_pmap.h"
#endif

#include <sys/param.h>
#include <uvm/uvm_extern.h>

#ifdef ALTIVEC
int pmap_use_altivec;
#endif
volatile struct pteg *pmap_pteg_table;
unsigned int pmap_pteg_cnt;
unsigned int pmap_pteg_mask;

struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;

u_int
powerpc_mmap_flags(paddr_t pa)
{
	u_int flags = PMAP_NOCACHE;

	if (pa & POWERPC_MMAP_FLAG_PREFETCHABLE)
		flags |= PMAP_MD_PREFETCHABLE;
	if (pa & POWERPC_MMAP_FLAG_CACHEABLE)
		flags &= ~PMAP_NOCACHE;
	return flags;
}

#ifdef PMAP_NEEDS_FIXUP
#include <powerpc/instr.h>

const struct pmap_ops *pmapops;

#define	__stub	__section(".stub") __noprofile

int	pmap_pte_spill(struct pmap *, vaddr_t, bool)		__stub;
void	pmap_real_memory(paddr_t *, psize_t *)			__stub;
void	pmap_init(void)						__stub;
void	pmap_virtual_space(vaddr_t *, vaddr_t *)		__stub;
pmap_t	pmap_create(void)					__stub;
void	pmap_reference(pmap_t)					__stub;
void	pmap_destroy(pmap_t)					__stub;
void	pmap_copy(pmap_t, pmap_t, vaddr_t, vsize_t, vaddr_t)	__stub;
void	pmap_update(pmap_t)					__stub;
int	pmap_enter(pmap_t, vaddr_t, paddr_t, vm_prot_t, u_int)	__stub;
void	pmap_remove(pmap_t, vaddr_t, vaddr_t)			__stub;
void	pmap_kenter_pa(vaddr_t, paddr_t, vm_prot_t, u_int)	__stub;
void	pmap_kremove(vaddr_t, vsize_t)				__stub;
bool	pmap_extract(pmap_t, vaddr_t, paddr_t *)		__stub;

void	pmap_protect(pmap_t, vaddr_t, vaddr_t, vm_prot_t)	__stub;
void	pmap_unwire(pmap_t, vaddr_t)				__stub;
void	pmap_page_protect(struct vm_page *, vm_prot_t)		__stub;
bool	pmap_query_bit(struct vm_page *, int)			__stub;
bool	pmap_clear_bit(struct vm_page *, int)			__stub;

void	pmap_activate(struct lwp *)				__stub;
void	pmap_deactivate(struct lwp *)				__stub;

void	pmap_pinit(pmap_t)					__stub;
void	pmap_procwr(struct proc *, vaddr_t, size_t)		__stub;

#if defined(DEBUG) || defined(PMAPCHECK) || defined(DDB)
void	pmap_pte_print(volatile struct pte *)			__stub;
void	pmap_pteg_check(void)					__stub;
void	pmap_print_mmuregs(void)				__stub;
void	pmap_print_pte(pmap_t, vaddr_t)				__stub;
void	pmap_pteg_dist(void)					__stub;
#endif
#if defined(DEBUG) || defined(PMAPCHECK)
void	pmap_pvo_verify(void)					__stub;
#endif
vaddr_t	pmap_steal_memory(vsize_t, vaddr_t *, vaddr_t *)	__stub;
void	pmap_bootstrap(paddr_t, paddr_t)			__stub;
void	pmap_bootstrap1(paddr_t, paddr_t)			__stub;
void	pmap_bootstrap2(void)					__stub;

int
pmap_pte_spill(struct pmap *pm, vaddr_t va, bool exec)
{
	return (*pmapops->pmapop_pte_spill)(pm, va, exec);
}

void
pmap_real_memory(paddr_t *start, psize_t *size)
{
	(*pmapops->pmapop_real_memory)(start, size);
}

void
pmap_init(void)
{
	(*pmapops->pmapop_init)();
}

void
pmap_virtual_space(vaddr_t *startp, vaddr_t *endp)
{
	(*pmapops->pmapop_virtual_space)(startp, endp);
}

pmap_t
pmap_create(void)
{
	return (*pmapops->pmapop_create)();
}

void
pmap_reference(pmap_t pm)
{
	(*pmapops->pmapop_reference)(pm);
}

void
pmap_destroy(pmap_t pm)
{
	(*pmapops->pmapop_destroy)(pm);
}

void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_va, vsize_t len,
	vaddr_t src_va)
{
	(*pmapops->pmapop_copy)(dst_pmap, src_pmap, dst_va, len, src_va);
}

void
pmap_update(pmap_t pm)
{
	(*pmapops->pmapop_update)(pm);
}

int
pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	return (*pmapops->pmapop_enter)(pm, va, pa, prot, flags);
}

void
pmap_remove(pmap_t pm, vaddr_t start, vaddr_t end)
{
	(*pmapops->pmapop_remove)(pm, start, end);
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	(*pmapops->pmapop_kenter_pa)(va, pa, prot, flags);
}

void
pmap_kremove(vaddr_t start, vsize_t end)
{
	(*pmapops->pmapop_kremove)(start, end);
}

bool
pmap_extract(pmap_t pm, vaddr_t va, paddr_t *pap)
{
	return (*pmapops->pmapop_extract)(pm, va, pap);
}

void
pmap_protect(pmap_t pm, vaddr_t start, vaddr_t end, vm_prot_t prot)
{
	(*pmapops->pmapop_protect)(pm, start, end, prot);
}

void
pmap_unwire(pmap_t pm, vaddr_t va)
{
	(*pmapops->pmapop_unwire)(pm, va);
}

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	(*pmapops->pmapop_page_protect)(pg, prot);
}

void
pmap_pv_protect(paddr_t pa, vm_prot_t prot)
{
	(*pmapops->pmapop_pv_protect)(pa, prot);
}

bool
pmap_query_bit(struct vm_page *pg, int ptebit)
{
	return (*pmapops->pmapop_query_bit)(pg, ptebit);
}

bool
pmap_clear_bit(struct vm_page *pg, int ptebit)
{
	return (*pmapops->pmapop_clear_bit)(pg, ptebit);
}

void
pmap_activate(struct lwp *l)
{
	(*pmapops->pmapop_activate)(l);
}

void
pmap_deactivate(struct lwp *l)
{
	(*pmapops->pmapop_deactivate)(l);
}

void
pmap_pinit(pmap_t pm)
{
	(*pmapops->pmapop_pinit)(pm);
}

void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	(*pmapops->pmapop_procwr)(p, va, len);
}

#if defined(DEBUG) || defined(PMAPCHECK) || defined(DDB)
void
pmap_pte_print(volatile struct pte *ptep)
{
	(*pmapops->pmapop_pte_print)(ptep);
}

void
pmap_pteg_check(void)
{
	(*pmapops->pmapop_pteg_check)();
}

void
pmap_print_mmuregs(void)
{
	(*pmapops->pmapop_print_mmuregs)();
}

void
pmap_print_pte(pmap_t pm, vaddr_t va)
{
	(*pmapops->pmapop_print_pte)(pm, va);
}

void
pmap_pteg_dist(void)
{
	(*pmapops->pmapop_pteg_dist)();
}
#endif

#if defined(DEBUG) || defined(PMAPCHECK)
void
pmap_pvo_verify(void)
{
	(*pmapops->pmapop_pvo_verify)();
}
#endif

vaddr_t
pmap_steal_memory(vsize_t vsize, vaddr_t *vstartp, vaddr_t *vendp)
{
	return (*pmapops->pmapop_steal_memory)(vsize, vstartp, vendp);
}

void
pmap_bootstrap(paddr_t startkernel, paddr_t endkernel)
{
	(*pmapops->pmapop_bootstrap)(startkernel, endkernel);
}

void
pmap_bootstrap1(paddr_t startkernel, paddr_t endkernel)
{
	(*pmapops->pmapop_bootstrap1)(startkernel, endkernel);
}

void
pmap_bootstrap2(void)
{
	(*pmapops->pmapop_bootstrap2)();
}
#endif
