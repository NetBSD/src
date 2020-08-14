/*	$NetBSD: uvm_page_status.c,v 1.6 2020/08/14 09:06:15 chs Exp $	*/

/*-
 * Copyright (c)2011 YAMAMOTO Takashi,
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_page_status.c,v 1.6 2020/08/14 09:06:15 chs Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

/*
 * page dirtiness status tracking
 *
 * separated from uvm_page.c mainly for rump
 */

/*
 * these constants are chosen to match so that we can convert between
 * them quickly.
 */

__CTASSERT(UVM_PAGE_STATUS_UNKNOWN == 0);
__CTASSERT(UVM_PAGE_STATUS_DIRTY == PG_DIRTY);
__CTASSERT(UVM_PAGE_STATUS_CLEAN == PG_CLEAN);

/*
 * uvm_pagegetdirty: return the dirtiness status (one of UVM_PAGE_STATUS_
 * values) of the page.
 *
 * called with the owner locked.
 */

unsigned int
uvm_pagegetdirty(struct vm_page *pg)
{
	struct uvm_object * const uobj __diagused = pg->uobject;

	KASSERT((~pg->flags & (PG_CLEAN|PG_DIRTY)) != 0);
	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(uobj == NULL || ((pg->flags & PG_CLEAN) == 0) ==
		uvm_obj_page_dirty_p(pg));
	return pg->flags & (PG_CLEAN|PG_DIRTY);
}

/*
 * uvm_pagemarkdirty: set the dirtiness status (one of UVM_PAGE_STATUS_ values)
 * of the page.
 *
 * called with the owner locked.
 *
 * update the radix tree tag for object-owned page.
 *
 * if new status is UVM_PAGE_STATUS_UNKNOWN, clear pmap-level dirty bit
 * so that later uvm_pagecheckdirty() can notice modifications on the page.
 */

void
uvm_pagemarkdirty(struct vm_page *pg, unsigned int newstatus)
{
	struct uvm_object * const uobj = pg->uobject;
	const unsigned int oldstatus = uvm_pagegetdirty(pg);
	enum cpu_count base;

	KASSERT((~newstatus & (PG_CLEAN|PG_DIRTY)) != 0);
	KASSERT((newstatus & ~(PG_CLEAN|PG_DIRTY)) == 0);
	KASSERT(uvm_page_owner_locked_p(pg, true));
	KASSERT(uobj == NULL || ((pg->flags & PG_CLEAN) == 0) ==
		uvm_obj_page_dirty_p(pg));

	if (oldstatus == newstatus) {
		return;
	}

	/*
	 * set UVM_PAGE_DIRTY_TAG tag unless known CLEAN so that putpages can
	 * find possibly-dirty pages quickly.
	 */

	if (uobj != NULL) {
		if (newstatus == UVM_PAGE_STATUS_CLEAN) {
			uvm_obj_page_clear_dirty(pg);
		} else if (oldstatus == UVM_PAGE_STATUS_CLEAN) {
			/*
			 * on first dirty page, mark the object dirty.
			 * for vnodes this inserts to the syncer worklist.
			 */
			if (uvm_obj_clean_p(uobj) &&
		            uobj->pgops->pgo_markdirty != NULL) {
				(*uobj->pgops->pgo_markdirty)(uobj);
			}
			uvm_obj_page_set_dirty(pg);
		}
	}
	if (newstatus == UVM_PAGE_STATUS_UNKNOWN) {
		/*
		 * start relying on pmap-level dirtiness tracking.
		 */
		pmap_clear_modify(pg);
	}
	pg->flags &= ~(PG_CLEAN|PG_DIRTY);
	pg->flags |= newstatus;
	KASSERT(uobj == NULL || ((pg->flags & PG_CLEAN) == 0) ==
		uvm_obj_page_dirty_p(pg));
	if ((pg->flags & PG_STAT) != 0) {
		if ((pg->flags & PG_SWAPBACKED) != 0) {
			base = CPU_COUNT_ANONUNKNOWN;
		} else {
			base = CPU_COUNT_FILEUNKNOWN;
		}
		kpreempt_disable();
		CPU_COUNT(base + oldstatus, -1);
		CPU_COUNT(base + newstatus, +1);
		kpreempt_enable();
	}
}

/*
 * uvm_pagecheckdirty: check if page is dirty, and remove its dirty bit.
 *
 * called with the owner locked.
 *
 * returns if the page was dirty.
 *
 * if protected is true, mark the page CLEAN.  otherwise, mark the page UNKNOWN.
 * ("mark" in the sense of uvm_pagemarkdirty().)
 */

bool
uvm_pagecheckdirty(struct vm_page *pg, bool pgprotected)
{
	const unsigned int oldstatus = uvm_pagegetdirty(pg);
	bool modified;

	KASSERT(uvm_page_owner_locked_p(pg, true));

	/*
	 * if pgprotected is true, mark the page CLEAN.
	 * otherwise mark the page UNKNOWN unless it's CLEAN.
	 *
	 * possible transitions:
	 *
	 *	CLEAN   -> CLEAN  , modified = false
	 *	UNKNOWN -> UNKNOWN, modified = true
	 *	UNKNOWN -> UNKNOWN, modified = false
	 *	UNKNOWN -> CLEAN  , modified = true
	 *	UNKNOWN -> CLEAN  , modified = false
	 *	DIRTY   -> UNKNOWN, modified = true
	 *	DIRTY   -> CLEAN  , modified = true
	 *
	 * pmap_clear_modify is necessary if either of
	 * oldstatus or newstatus is UVM_PAGE_STATUS_UNKNOWN.
	 */

	if (oldstatus == UVM_PAGE_STATUS_CLEAN) {
		modified = false;
	} else {
		const unsigned int newstatus = pgprotected ?
		    UVM_PAGE_STATUS_CLEAN : UVM_PAGE_STATUS_UNKNOWN;

		if (oldstatus == UVM_PAGE_STATUS_DIRTY) {
			modified = true;
			if (newstatus == UVM_PAGE_STATUS_UNKNOWN) {
				pmap_clear_modify(pg);
			}
		} else {
			KASSERT(oldstatus == UVM_PAGE_STATUS_UNKNOWN);
			modified = pmap_clear_modify(pg);
		}
		uvm_pagemarkdirty(pg, newstatus);
	}
	return modified;
}
