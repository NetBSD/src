/*	$NetBSD: subr_kmem.c,v 1.2.2.2 2006/06/26 12:52:57 yamt Exp $	*/

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
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

/*
 * allocator of kernel wired memory.
 *
 * TODO:
 * -	worth to have "intrsafe" version?  maybe..
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kmem.c,v 1.2.2.2 2006/06/26 12:52:57 yamt Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <lib/libkern/libkern.h>

#define	KMEM_QUANTUM_SIZE	sizeof(void *)	/* XXX */

static vmem_t *kmem_arena;

static vmem_addr_t kmem_backend_alloc(vmem_t *, vmem_size_t, vmem_size_t *,
    vm_flag_t);
static void kmem_backend_free(vmem_t *, vmem_addr_t, vmem_size_t);

static inline vm_flag_t
kmf_to_vmf(km_flag_t kmflags)
{
	vm_flag_t vmflags;

	KASSERT((kmflags & (KM_SLEEP|KM_NOSLEEP)) != 0);
	KASSERT((~kmflags & (KM_SLEEP|KM_NOSLEEP)) != 0);

	vmflags = 0;
	if ((kmflags & KM_SLEEP) != 0) {
		vmflags |= VM_SLEEP;
	}
	if ((kmflags & KM_NOSLEEP) != 0) {
		vmflags |= VM_NOSLEEP;
	}

	return vmflags;
}

/* ---- kmem API */

/*
 * kmem_alloc: allocate wired memory.
 *
 * => must not be called from interrupt context.
 */

void *
kmem_alloc(size_t size, km_flag_t kmflags)
{

	return (void *)vmem_alloc(kmem_arena, size,
	    kmf_to_vmf(kmflags) | VM_INSTANTFIT);
}

/*
 * kmem_zalloc: allocate wired memory.
 *
 * => must not be called from interrupt context.
 */

void *
kmem_zalloc(size_t size, km_flag_t kmflags)
{
	void *p;

	p = kmem_alloc(size, kmflags);
	if (p != NULL) {
		memset(p, 0, size);
	}
	return p;
}

/*
 * kmem_free: free wired memory allocated by kmem_alloc.
 *
 * => must not be called from interrupt context.
 */

void
kmem_free(void *p, size_t size)
{

	vmem_free(kmem_arena, (vmem_addr_t)p, size);
}

void
kmem_init(void)
{

	kmem_arena = vmem_create("kmem", 0, 0, KMEM_QUANTUM_SIZE,
	    kmem_backend_alloc, kmem_backend_free, NULL, 0, VM_SLEEP);
}

size_t
kmem_roundup_size(size_t size)
{

	return vmem_roundup_size(kmem_arena, size);
}

/* ---- uvm glue */

#include <uvm/uvm_extern.h>

static vmem_addr_t
kmem_backend_alloc(vmem_t *dummy, vmem_size_t size, vmem_size_t *resultsize,
    vm_flag_t vmflags)
{
	uvm_flag_t uflags;

	KASSERT(dummy == NULL);
	KASSERT(size != 0);
	KASSERT((vmflags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	KASSERT((~vmflags & (VM_SLEEP|VM_NOSLEEP)) != 0);

	if ((vmflags & VM_NOSLEEP) != 0) {
		uflags = UVM_KMF_TRYLOCK | UVM_KMF_NOWAIT;
	} else {
		uflags = UVM_KMF_WAITVA;
	}
	*resultsize = size = round_page(size);
	return (vmem_addr_t)uvm_km_alloc(kernel_map, size, 0,
	    uflags | UVM_KMF_WIRED | UVM_KMF_CANFAIL);
}

static void
kmem_backend_free(vmem_t *dummy, vmem_addr_t addr, vmem_size_t size)
{

	KASSERT(dummy == NULL);
	KASSERT(addr != 0);
	KASSERT(size != 0);
	KASSERT(size == round_page(size));

	uvm_km_free(kernel_map, (vaddr_t)addr, size, UVM_KMF_WIRED);
}
