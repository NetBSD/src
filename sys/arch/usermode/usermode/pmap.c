/* $NetBSD: pmap.c,v 1.1.6.2 2008/01/21 09:39:57 yamt Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.1.6.2 2008/01/21 09:39:57 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/buf.h>

#include <uvm/uvm_page.h>
#include <uvm/uvm_pmap.h>

#include "opt_memsize.h"

struct pmap	pmap_kernel_store;
static uint8_t	pmap_memory[1024*MEMSIZE] __attribute__((__aligned__));
static vaddr_t	virtual_avail, virtual_end;
static vaddr_t	pmap_maxkvaddr;

void		pmap_bootstrap(void);

void
pmap_bootstrap(void)
{
#if 0
	vsize_t bufsz;
#endif

	virtual_avail = (vaddr_t)pmap_memory;
	virtual_end = virtual_avail + sizeof(pmap_memory);

	uvm_page_physload(atop(virtual_avail),
	    atop(virtual_end),
	    atop(virtual_avail + PAGE_SIZE),
	    atop(virtual_end), 
	    VM_FREELIST_DEFAULT);

	virtual_avail += PAGE_SIZE;

	pmap_maxkvaddr = VM_MIN_KERNEL_ADDRESS;
#if 0
	kmeminit_nkmempages();
	bufsz = buf_memcalc();
	if (buf_setvalimit(bufsz))
		panic("pmap_bootstrap: buf_setvalimit failed\n");
#endif
}

void
pmap_init(void)
{
}

pmap_t
pmap_kernel(void)
{
	return &pmap_kernel_store;
}

void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = virtual_avail;
	*vendp = virtual_end;
}

/* XXXJDM */
static struct pmap_list {
	bool used;
	struct pmap pmap;
} pmap_list[128];

pmap_t
pmap_create(void)
{
	int i;

	for (i = 0; i < __arraycount(pmap_list); i++)
		if (pmap_list[i].used == false) {
			pmap_list[i].used = true;
			return &pmap_list[i].pmap;
		}

	printf("pmap_create: out of space\n");
	return NULL;
}

void
pmap_destroy(pmap_t pmap)
{
	int i;

	for (i = 0; i < __arraycount(pmap_list); i++)
		if (pmap == &pmap_list[i].pmap) {
			pmap_list[i].used = false;
			break;
		}
}

void
pmap_reference(pmap_t pmap)
{
}

long
pmap_resident_count(pmap_t pmap)
{
	printf("pmap_resident_count\n");
	return 1;
}

long
pmap_wired_count(pmap_t pmap)
{
	printf("pmap_wired_count\n");
	return 1;
}

int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	return 0;
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
}

void
pmap_remove_all(pmap_t pmap)
{
}

void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
}

void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
}

bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	/* XXXJDM */
	*pap = va;
	return true;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
}

void
pmap_copy(pmap_t dst_map, pmap_t src_map, vaddr_t dst_addr, vsize_t len,
    vaddr_t src_addr)
{
}

void
pmap_collect(pmap_t pmap)
{
}

void
pmap_update(pmap_t pmap)
{
}

void
pmap_activate(struct lwp *l)
{
}

void
pmap_deactivate(struct lwp *l)
{
}

void
pmap_zero_page(paddr_t pa)
{
}

void
pmap_copy_page(paddr_t src, paddr_t dst)
{
}

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
}

bool
pmap_clear_modify(struct vm_page *pg)
{
	return true;
}

bool
pmap_clear_reference(struct vm_page *pg)
{
	return true;
}

bool
pmap_is_modified(struct vm_page *pg)
{
	return false;
}

bool
pmap_is_referenced(struct vm_page *pg)
{
	return false;
}

paddr_t
pmap_phys_address(paddr_t cookie)
{
	return ptoa(cookie);
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	pmap_maxkvaddr = maxkvaddr;
	return maxkvaddr;
}
