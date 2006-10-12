/*	$NetBSD: subr_kmem.c,v 1.10 2006/10/12 01:32:18 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_kmem.c,v 1.10 2006/10/12 01:32:18 christos Exp $");

#include <sys/param.h>
#include <sys/callback.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <lib/libkern/libkern.h>

#define	KMEM_QUANTUM_SIZE	(ALIGNBYTES + 1)

static vmem_t *kmem_arena;
static struct callback_entry kmem_kva_reclaim_entry;

#if defined(DEBUG)
static void kmem_poison_fill(void *, size_t);
static void kmem_poison_check(void *, size_t);
#else /* defined(DEBUG) */
#define	kmem_poison_fill(p, sz)		/* nothing */
#define	kmem_poison_check(p, sz)	/* nothing */
#endif /* defined(DEBUG) */

static vmem_addr_t kmem_backend_alloc(vmem_t *, vmem_size_t, vmem_size_t *,
    vm_flag_t);
static void kmem_backend_free(vmem_t *, vmem_addr_t, vmem_size_t);
static int kmem_kva_reclaim_callback(struct callback_entry *, void *, void *);

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
	void *p;

	p = (void *)vmem_alloc(kmem_arena, size,
	    kmf_to_vmf(kmflags) | VM_INSTANTFIT);
	kmem_poison_check(p, size);
	return p;
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

	kmem_poison_fill(p, size);
	vmem_free(kmem_arena, (vmem_addr_t)p, size);
}

void
kmem_init(void)
{

	kmem_arena = vmem_create("kmem", 0, 0, KMEM_QUANTUM_SIZE,
	    kmem_backend_alloc, kmem_backend_free, NULL,
	    KMEM_QUANTUM_SIZE * 32, VM_SLEEP);
	callback_register(&vm_map_to_kernel(kernel_map)->vmk_reclaim_callback,
	    &kmem_kva_reclaim_entry, kmem_arena, kmem_kva_reclaim_callback);
}

size_t
kmem_roundup_size(size_t size)
{

	return vmem_roundup_size(kmem_arena, size);
}

/* ---- uvm glue */

#include <uvm/uvm_extern.h>

static vmem_addr_t
kmem_backend_alloc(vmem_t *dummy __unused, vmem_size_t size,
    vmem_size_t *resultsize, vm_flag_t vmflags)
{
	uvm_flag_t uflags;
	vaddr_t va;

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
	va = uvm_km_alloc(kernel_map, size, 0,
	    uflags | UVM_KMF_WIRED | UVM_KMF_CANFAIL);
	kmem_poison_fill((void *)va, size);
	return (vmem_addr_t)va;
}

static void
kmem_backend_free(vmem_t *dummy __unused, vmem_addr_t addr, vmem_size_t size)
{

	KASSERT(dummy == NULL);
	KASSERT(addr != 0);
	KASSERT(size != 0);
	KASSERT(size == round_page(size));

	kmem_poison_check((void *)addr, size);
	uvm_km_free(kernel_map, (vaddr_t)addr, size, UVM_KMF_WIRED);
}

static int
kmem_kva_reclaim_callback(struct callback_entry *ce __unused, void *obj,
    void *arg __unused)
{
	vmem_t *vm = obj;

	vmem_reap(vm);
	return CALLBACK_CHAIN_CONTINUE;
}

/* ---- debug */

#if defined(DEBUG)

#if defined(_LP64)
#define	PRIME	0x9e37fffffffc0001UL
#else /* defined(_LP64) */
#define	PRIME	0x9e3779b1
#endif /* defined(_LP64) */

static inline uint8_t
kmem_poison_pattern(const void *p)
{

	return (uint8_t)((((uintptr_t)p) * PRIME)
	    >> ((sizeof(uintptr_t) - sizeof(uint8_t))) * CHAR_BIT);
}

static void
kmem_poison_fill(void *p, size_t sz)
{
	uint8_t *cp;
	const uint8_t *ep;

	cp = p;
	ep = cp + sz;
	while (cp < ep) {
		*cp = kmem_poison_pattern(cp);
		cp++;
	}
}

static void
kmem_poison_check(void *p, size_t sz)
{
	uint8_t *cp;
	const uint8_t *ep;

	cp = p;
	ep = cp + sz;
	while (cp < ep) {
		const uint8_t expected = kmem_poison_pattern(cp);

		if (*cp != expected) {
			panic("%s: %p: 0x%02x != 0x%02x\n",
			    __func__, cp, *cp, expected);
		}
		cp++;
	}
}

#endif /* defined(DEBUG) */
