/*	$NetBSD: uvm_init.c,v 1.41.4.1 2011/11/20 10:52:33 yamt Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from: Id: uvm_init.c,v 1.1.2.3 1998/02/06 05:15:27 chs Exp
 */

/*
 * uvm_init.c: init the vm system.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_init.c,v 1.41.4.1 2011/11/20 10:52:33 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/debug.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/resourcevar.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/vnode.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_readahead.h>

/*
 * struct uvm: we store most global vars in this structure to make them
 * easier to spot...
 */

struct uvm uvm;		/* decl */
struct uvmexp uvmexp;	/* decl */
struct uvm_object *uvm_kernel_object;

#if defined(__uvmexp_pagesize)
int *uvmexp_pagesize = &uvmexp.pagesize;
int *uvmexp_pagemask = &uvmexp.pagemask;
int *uvmexp_pageshift = &uvmexp.pageshift;
#endif

kmutex_t uvm_pageqlock;
kmutex_t uvm_fpageqlock;
kmutex_t uvm_kentry_lock;
kmutex_t uvm_swap_data_lock;

/*
 * uvm_init: init the VM system.   called from kern/init_main.c.
 */

void
uvm_init(void)
{
	vaddr_t kvm_start, kvm_end;

	/*
	 * step 0: ensure that the hardware set the page size
	 */

	if (uvmexp.pagesize == 0) {
		panic("uvm_init: page size not set");
	}

	/*
	 * step 1: zero the uvm structure
	 */

	memset(&uvm, 0, sizeof(uvm));
	averunnable.fscale = FSCALE;
	uvm_amap_init();

	/*
	 * step 2: init the page sub-system.  this includes allocating the
	 * vm_page structures, and setting up all the page queues (and
	 * locks).  available memory will be put in the "free" queue.
	 * kvm_start and kvm_end will be set to the area of kernel virtual
	 * memory which is available for general use.
	 */

	uvm_page_init(&kvm_start, &kvm_end);

	/*
	 * step 3: init the map sub-system.  allocates the static pool of
	 * vm_map_entry structures that are used for "special" kernel maps
	 * (e.g. kernel_map, kmem_map, etc...).
	 */

	uvm_map_init();

	/*
	 * step 4: setup the kernel's virtual memory data structures.  this
	 * includes setting up the kernel_map/kernel_object.
	 */

	uvm_km_init(kvm_start, kvm_end);

	/*
	 * step 5: init the pmap module.   the pmap module is free to allocate
	 * memory for its private use (e.g. pvlists).
	 */

	pmap_init();

	/*
	 * step 6: init the kernel memory allocator.   after this call the
	 * kernel memory allocator (malloc) can be used. this includes
	 * setting up the kmem_map.
	 */

	kmeminit();

#ifdef DEBUG
	debug_init();
#endif

	/*
	 * step 7: init all pagers and the pager_map.
	 */

	uvm_pager_init();

	/*
	 * Initialize pools.  This must be done before anyone manipulates
	 * any vm_maps because we use a pool for some map entry structures.
	 */

	pool_subsystem_init();

	/*
	 * init slab memory allocator kmem(9).
	 */

	kmem_init();

	/*
	 * Initialize lock caches.
	 */

	mutex_obj_init();
	rw_obj_init();

	/*
	 * Initialize the uvm_loan() facility.
	 * REQUIRE: mutex_obj_init
	 */

	uvm_loan_init();

	/*
	 * init emap subsystem.
	 */

	uvm_emap_sysinit();

	/*
	 * the VM system is now up!  now that kmem is up we can resize the
	 * <obj,off> => <page> hash table for general use and enable paging
	 * of kernel objects.
	 */

	uao_create(VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS,
	    UAO_FLAG_KERNSWAP);

	uvmpdpol_reinit();

	/*
	 * init anonymous memory systems
	 */

	uvm_anon_init();

	uvm_uarea_init();

	/*
	 * init readahead module
	 */

	uvm_ra_init();
}
