/*	$NetBSD: uvm_object.c,v 1.4 2008/01/02 11:49:18 ad Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

/*
 * uvm_object.c: operate with memory objects
 *
 * TODO:
 *  1. Support PG_RELEASED-using objects
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_object.c,v 1.4 2008/01/02 11:49:18 ad Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/lock.h>

#include <uvm/uvm.h>

/* We will fetch this page count per step */
#define	FETCH_PAGECOUNT	16

/*
 * uobj_wirepages: wire the pages of entire uobj
 *
 * => NOTE: this function should only be used for types of objects
 *  where PG_RELEASED flag is never set (aobj objects)
 * => caller must pass page-aligned start and end values
 */

int
uobj_wirepages(struct uvm_object *uobj, off_t start, off_t end)
{
	int i, npages, error;
	struct vm_page *pgs[FETCH_PAGECOUNT], *pg = NULL;
	off_t offset = start, left;

	left = (end - start) >> PAGE_SHIFT;

	mutex_enter(&uobj->vmobjlock);
	while (left) {

		npages = MIN(FETCH_PAGECOUNT, left);

		/* Get the pages */
		memset(pgs, 0, sizeof(pgs));
		error = (*uobj->pgops->pgo_get)(uobj, offset, pgs, &npages, 0,
			VM_PROT_READ | VM_PROT_WRITE, UVM_ADV_SEQUENTIAL,
			PGO_ALLPAGES | PGO_SYNCIO);

		if (error)
			goto error;

		mutex_enter(&uobj->vmobjlock);
		for (i = 0; i < npages; i++) {

			KASSERT(pgs[i] != NULL);
			KASSERT(!(pgs[i]->flags & PG_RELEASED));

			/*
			 * Loan break
			 */
			if (pgs[i]->loan_count) {
				while (pgs[i]->loan_count) {
					pg = uvm_loanbreak(pgs[i]);
					if (!pg) {
						mutex_exit(&uobj->vmobjlock);
						uvm_wait("uobjwirepg");
						mutex_enter(&uobj->vmobjlock);
						continue;
					}
				}
				pgs[i] = pg;
			}

			if (pgs[i]->pqflags & PQ_AOBJ) {
				pgs[i]->flags &= ~(PG_CLEAN);
				uao_dropswap(uobj, i);
			}
		}

		/* Wire the pages */
		mutex_enter(&uvm_pageqlock);
		for (i = 0; i < npages; i++) {
			uvm_pagewire(pgs[i]);
		}
		mutex_exit(&uvm_pageqlock);

		/* Unbusy the pages */
		uvm_page_unbusy(pgs, npages);

		left -= npages;
		offset += npages << PAGE_SHIFT;
	}
	mutex_exit(&uobj->vmobjlock);

	return 0;

error:
	/* Unwire the pages which has been wired */
	uobj_unwirepages(uobj, start, offset);

	return error;
}

/*
 * uobj_unwirepages: unwire the pages of entire uobj
 *
 * => NOTE: this function should only be used for types of objects
 *  where PG_RELEASED flag is never set
 * => caller must pass page-aligned start and end values
 */

void
uobj_unwirepages(struct uvm_object *uobj, off_t start, off_t end)
{
	struct vm_page *pg;
	off_t offset;

	mutex_enter(&uobj->vmobjlock);
	mutex_enter(&uvm_pageqlock);
	for (offset = start; offset < end; offset += PAGE_SIZE) {
		pg = uvm_pagelookup(uobj, offset);

		KASSERT(pg != NULL);
		KASSERT(!(pg->flags & PG_RELEASED));

		uvm_pageunwire(pg);
	}
	mutex_exit(&uvm_pageqlock);
	mutex_exit(&uobj->vmobjlock);
}
