/*	$NetBSD: linux_gfp.c,v 1.2 2014/03/18 18:20:43 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: linux_gfp.c,v 1.2 2014/03/18 18:20:43 riastradh Exp $");

#include <sys/types.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/mm_types.h>

struct page *
alloc_page(gfp_t gfp)
{
	paddr_t low = 0;
	paddr_t high = ~(paddr_t)0;
	struct pglist pglist;
	struct vm_page *vm_page;
	int error;

	if (ISSET(gfp, __GFP_DMA32))
		high = 0xffffffff;

	error = uvm_pglistalloc(PAGE_SIZE, low, high, PAGE_SIZE, PAGE_SIZE,
	    &pglist, 1, ISSET(gfp, __GFP_WAIT));
	if (error)
		return NULL;

	vm_page = TAILQ_FIRST(&pglist);
	TAILQ_REMOVE(&pglist, vm_page, pageq.queue);	/* paranoia */
	KASSERT(TAILQ_EMPTY(&pglist));

	return container_of(vm_page, struct page, p_vmp);
}

void
__free_page(struct page *page)
{
	struct pglist pglist = TAILQ_HEAD_INITIALIZER(pglist);

	TAILQ_INSERT_TAIL(&pglist, &page->p_vmp, pageq.queue);

	uvm_pglistfree(&pglist);
}
