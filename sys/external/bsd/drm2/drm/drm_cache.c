/*	$NetBSD: drm_cache.c,v 1.7 2015/10/17 15:13:19 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_cache.c,v 1.7 2015/10/17 15:13:19 jmcneill Exp $");

#include <sys/types.h>
#include <sys/xcall.h>

#include <uvm/uvm_extern.h>

#include <linux/mm_types.h>

#include <drm/drmP.h>

static bool		drm_md_clflush_finegrained_p(void);
static void		drm_md_clflush_all(void);
static void		drm_md_clflush_page(struct page *);
static void		drm_md_clflush_virt_range(const void *, size_t);

void
drm_clflush_pages(struct page **pages, unsigned long npages)
{

	if (drm_md_clflush_finegrained_p()) {
		while (npages--)
			drm_md_clflush_page(pages[npages]);
	} else {
		drm_md_clflush_all();
	}
}

void
drm_clflush_pglist(struct pglist *list)
{

	if (drm_md_clflush_finegrained_p()) {
		struct vm_page *page;

		TAILQ_FOREACH(page, list, pageq.queue)
			drm_md_clflush_page(container_of(page, struct page,
				p_vmp));
	} else {
		drm_md_clflush_all();
	}
}

void
drm_clflush_page(struct page *page)
{

	if (drm_md_clflush_finegrained_p())
		drm_md_clflush_page(page);
	else
		drm_md_clflush_all();
}

void
drm_clflush_virt_range(const void *vaddr, size_t nbytes)
{

	if (drm_md_clflush_finegrained_p())
		drm_md_clflush_virt_range(vaddr, nbytes);
	else
		drm_md_clflush_all();
}

#if defined(__i386__) || defined(__x86_64__)

#include <machine/cpufunc.h>

static bool
drm_md_clflush_finegrained_p(void)
{
	return ISSET(cpu_info_primary.ci_feat_val[0], CPUID_CFLUSH);
}

static void
drm_x86_clflush(const void *vaddr)
{
	asm volatile ("clflush %0" : : "m" (*(const char *)vaddr));
}

static size_t
drm_x86_clflush_size(void)
{
	KASSERT(drm_md_clflush_finegrained_p());
	return cpu_info_primary.ci_cflush_lsize;
}

static void
drm_x86_clflush_xc(void *arg0 __unused, void *arg1 __unused)
{
	wbinvd();
}

static void
drm_md_clflush_all(void)
{
	xc_wait(xc_broadcast(0, &drm_x86_clflush_xc, NULL, NULL));
}

static void
drm_md_clflush_page(struct page *page)
{
	void *const vaddr = kmap_atomic(page);

	drm_md_clflush_virt_range(vaddr, PAGE_SIZE);

	kunmap_atomic(vaddr);
}

static void
drm_md_clflush_virt_range(const void *vaddr, size_t nbytes)
{
	const unsigned clflush_size = drm_x86_clflush_size();
	const vaddr_t va = (vaddr_t)vaddr;
	const char *const start = (const void *)rounddown(va, clflush_size);
	const char *const end = (const void *)roundup(va + nbytes,
	    clflush_size);
	const char *p;

	/* Support for CLFLUSH implies support for MFENCE.  */
	KASSERT(drm_md_clflush_finegrained_p());
	x86_mfence();
	for (p = start; p < end; p += clflush_size)
		drm_x86_clflush(p);
	x86_mfence();
}

#endif	/* defined(__i386__) || defined(__x86_64__) */
