/*	$NetBSD: pmap_devmap.h,v 1.1 2023/04/20 08:28:02 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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


#ifndef	_UVM_PMAP_DEVMAP_H_
#define	_UVM_PMAP_DEVMAP_H_

typedef struct pmap_devmap {
	vaddr_t		pd_va;		/* virtual address */
	paddr_t		pd_pa;		/* physical address */
	psize_t		pd_size;	/* size of region */
	vm_prot_t	pd_prot;	/* protection code */
	u_int		pd_flags;	/* flags for pmap_kenter_{pa,range} */
} pmap_devmap_t;


bool pmap_devmap_bootstrapped_p(void);
void pmap_devmap_register(const struct pmap_devmap *);
void pmap_devmap_bootstrap(vaddr_t, const struct pmap_devmap *);
vaddr_t pmap_devmap_root(void);
const struct pmap_devmap *pmap_devmap_find_pa(paddr_t, psize_t);
const struct pmap_devmap *pmap_devmap_find_va(vaddr_t, vsize_t);

#define	DEVMAP_ENTRY_FLAGS(va, pa, sz, fl)			\
{								\
	.pd_va = DEVMAP_ALIGN(va),				\
	.pd_pa = DEVMAP_ALIGN(pa),				\
	.pd_size = DEVMAP_SIZE(sz),				\
	.pd_prot = VM_PROT_READ | VM_PROT_WRITE,		\
	.pd_flags = (fl),					\
}

#define	DEVMAP_ENTRY(va, pa, sz)				\
	DEVMAP_ENTRY_FLAGS(va, pa, sz, DEVMAP_FLAGS)

#define	DEVMAP_ENTRY_END	{ 0 }

#endif	/* _UVM_PMAP_DEVMAP_H_ */
