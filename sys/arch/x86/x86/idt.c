/*	$NetBSD: idt.c,v 1.16 2022/02/13 19:21:21 riastradh Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility NASA Ames Research Center, and by Andrew Doran.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: idt.c,v 1.16 2022/02/13 19:21:21 riastradh Exp $");

#include "opt_pcpu_idt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>

#include <machine/segments.h>

/*
 * XEN PV and native have a different idea of what idt entries should
 * look like.
 */

/* Normalise across XEN PV and native */
#if defined(XENPV)

void
set_idtgate(struct trap_info *xen_idd, void *function, int ist,
	    int type, int dpl, int sel)
{
	/*
	 * Find the page boundary in which the descriptor resides.
	 * We make an assumption here, that the descriptor is part of
	 * a table (array), which fits in a page and is page aligned.
	 *
	 * This assumption is from the usecases at early startup in
	 * machine/machdep.c
	 *
	 * Thus this function may not work in the "general" case of a
	 * randomly located idt entry template (for eg:).
	 */

	vaddr_t xen_idt_vaddr = ((vaddr_t) xen_idd) & ~PAGE_MASK;

	//kpreempt_disable();
#if defined(__x86_64__)
	/* Make it writeable, so we can update the values. */
	pmap_changeprot_local(xen_idt_vaddr, VM_PROT_READ | VM_PROT_WRITE);
#endif /* __x86_64 */
	xen_idd->cs = sel;
	xen_idd->address = (unsigned long) function;
	xen_idd->flags = dpl;

	/*
	 * Again we make the assumption that the descriptor is
	 * implicitly part of an idt, which we infer as
	 * xen_idt_vaddr. (See above).
	 */
	xen_idd->vector = xen_idd - (struct trap_info *)xen_idt_vaddr;

	/* Back to read-only, as it should be. */
#if defined(__x86_64__)
	pmap_changeprot_local(xen_idt_vaddr, VM_PROT_READ);
#endif /* __x86_64 */
	//kpreempt_enable();
}
void
unset_idtgate(struct trap_info *xen_idd)
{
#if defined(__x86_64__)
	vaddr_t xen_idt_vaddr = ((vaddr_t) xen_idd) & ~PAGE_MASK;

	/* Make it writeable, so we can update the values. */
	pmap_changeprot_local(xen_idt_vaddr, VM_PROT_READ | VM_PROT_WRITE);
#endif /* __x86_64 */

	/* Zero it */
	memset(xen_idd, 0, sizeof (*xen_idd));

#if defined(__x86_64__)
	/* Back to read-only, as it should be. */
	pmap_changeprot_local(xen_idt_vaddr, VM_PROT_READ);
#endif /* __x86_64 */
}
#else /* XENPV */
void
set_idtgate(struct gate_descriptor *idd, void *function, int ist, int type, int dpl, int sel)
{
	setgate(idd, function, ist, type, dpl,	sel);
}
void
unset_idtgate(struct gate_descriptor *idd)
{
	unsetgate(idd);
}
#endif /* XENPV */

/*
 * Allocate an IDT vector slot within the given range.
 * cpu_lock will be held unless single threaded during early boot.
 */
int
idt_vec_alloc(struct idt_vec *iv, int low, int high)
{
	int vec;
	char *idt_allocmap = iv->iv_allocmap;

	KASSERT(mutex_owned(&cpu_lock) || !mp_online);

	if (low < 0 || high >= __arraycount(iv->iv_allocmap))
		return -1;

	for (vec = low; vec <= high; vec++) {
		/* pairs with atomic_store_release in idt_vec_free */
		if (atomic_load_acquire(&idt_allocmap[vec]) == 0) {
			/*
			 * No ordering needed here (`relaxed') because
			 * access to free entries is serialized by
			 * cpu_lock or single-threaded operation.
			 */
			atomic_store_relaxed(&idt_allocmap[vec], 1);
			return vec;
		}
	}

	return -1;
}

void
idt_vec_reserve(struct idt_vec *iv, int vec)
{
	int result;

	KASSERT(mutex_owned(&cpu_lock) || !mp_online);

	result = idt_vec_alloc(iv, vec, vec);
	if (result < 0) {
		panic("%s: failed to reserve vec %d", __func__, vec);
	}
}

void
idt_vec_set(struct idt_vec *iv, int vec, void (*function)(void))
{
	idt_descriptor_t *idt;
	char *idt_allocmap __diagused = iv->iv_allocmap;

	KASSERT(atomic_load_relaxed(&idt_allocmap[vec]) == 1);

	idt = iv->iv_idt;
	set_idtgate(&idt[vec], function, 0, SDT_SYS386IGT, SEL_KPL,
	       GSEL(GCODE_SEL, SEL_KPL));
}

/*
 * Free IDT vector.  No locking required as release is atomic.
 */
void
idt_vec_free(struct idt_vec *iv, int vec)
{
	idt_descriptor_t *idt;
	char *idt_allocmap = iv->iv_allocmap;

	KASSERT(atomic_load_relaxed(&idt_allocmap[vec]) == 1);

	idt = iv->iv_idt;
	unset_idtgate(&idt[vec]);
	/* pairs with atomic_load_acquire in idt_vec_alloc */
	atomic_store_release(&idt_allocmap[vec], 0);
}

bool
idt_vec_is_pcpu(void)
{

#ifdef PCPU_IDT
	return true;
#else
	return false;
#endif
}

struct idt_vec *
idt_vec_ref(struct idt_vec *iv)
{
	if (idt_vec_is_pcpu())
		return iv;

	return &(cpu_info_primary.ci_idtvec);
}
