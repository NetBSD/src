/*	$NetBSD: pmap_pvt.c,v 1.3.16.2 2017/12/03 11:39:23 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pmap_pvt.c,v 1.3.16.2 2017/12/03 11:39:23 jdolecek Exp $");

#include <sys/kmem.h>
#include <sys/pserialize.h>

#include <uvm/uvm.h>
#include <uvm/pmap/pmap_pvt.h>

/*
 * unmanaged pv-tracked ranges
 *
 * This is a linear list for now because the only user are the DRM
 * graphics drivers, with a single tracked range per device, for the
 * graphics aperture, so there are expected to be few of them.
 *
 * This is used only after the VM system is initialized well enough
 * that we can use kmem_alloc.
 */

struct pv_track {
	paddr_t			pvt_start;
	psize_t			pvt_size;
	struct pv_track		*pvt_next;
	struct pmap_page	pvt_pages[];
};

static struct {
	kmutex_t	lock;
	pserialize_t	psz;
	struct pv_track	*list;
} pv_unmanaged __cacheline_aligned;

void
pmap_pv_init(void)
{

	mutex_init(&pv_unmanaged.lock, MUTEX_DEFAULT, IPL_NONE);
	pv_unmanaged.psz = pserialize_create();
	pv_unmanaged.list = NULL;
}

void
pmap_pv_track(paddr_t start, psize_t size)
{
	struct pv_track *pvt;
	size_t npages;

	KASSERT(start == trunc_page(start));
	KASSERT(size == trunc_page(size));

	/* We may sleep for allocation.  */
	ASSERT_SLEEPABLE();

	npages = size >> PAGE_SHIFT;
	pvt = kmem_zalloc(offsetof(struct pv_track, pvt_pages[npages]),
	    KM_SLEEP);
	pvt->pvt_start = start;
	pvt->pvt_size = size;

	mutex_enter(&pv_unmanaged.lock);
	pvt->pvt_next = pv_unmanaged.list;
	membar_producer();
	pv_unmanaged.list = pvt;
	mutex_exit(&pv_unmanaged.lock);
}

void
pmap_pv_untrack(paddr_t start, psize_t size)
{
	struct pv_track **pvtp, *pvt;
	size_t npages;

	KASSERT(start == trunc_page(start));
	KASSERT(size == trunc_page(size));

	/* We may sleep for pserialize_perform.  */
	ASSERT_SLEEPABLE();

	mutex_enter(&pv_unmanaged.lock);
	for (pvtp = &pv_unmanaged.list;
	     (pvt = *pvtp) != NULL;
	     pvtp = &pvt->pvt_next) {
		if (pvt->pvt_start != start)
			continue;
		if (pvt->pvt_size != size)
			panic("pmap_pv_untrack: pv-tracking at 0x%"PRIxPADDR
			    ": 0x%"PRIxPSIZE" bytes, not 0x%"PRIxPSIZE" bytes",
			    pvt->pvt_start, pvt->pvt_size, size);
		*pvtp = pvt->pvt_next;
		pserialize_perform(pv_unmanaged.psz);
		pvt->pvt_next = NULL;
		goto out;
	}
	panic("pmap_pv_untrack: pages not pv-tracked at 0x%"PRIxPADDR
	    " (0x%"PRIxPSIZE" bytes)",
	    start, size);
out:	mutex_exit(&pv_unmanaged.lock);

	npages = size >> PAGE_SHIFT;
	kmem_free(pvt, offsetof(struct pv_track, pvt_pages[npages]));
}

struct pmap_page *
pmap_pv_tracked(paddr_t pa)
{
	struct pv_track *pvt;
	size_t pgno;
	int s;

	KASSERT(pa == trunc_page(pa));

	s = pserialize_read_enter();
	for (pvt = pv_unmanaged.list; pvt != NULL; pvt = pvt->pvt_next) {
		membar_datadep_consumer();
		if ((pvt->pvt_start <= pa) &&
		    ((pa - pvt->pvt_start) < pvt->pvt_size))
			break;
	}
	pserialize_read_exit(s);

	if (pvt == NULL)
		return NULL;
	KASSERT(pvt->pvt_start <= pa);
	KASSERT((pa - pvt->pvt_start) < pvt->pvt_size);
	pgno = (pa - pvt->pvt_start) >> PAGE_SHIFT;
	return &pvt->pvt_pages[pgno];
}

