/*	$NetBSD: mm.h,v 1.9 2018/08/27 13:44:54 riastradh Exp $	*/

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

#ifndef _LINUX_MM_H_
#define _LINUX_MM_H_

#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <asm/page.h>
#include <linux/shrinker.h>

struct file;

/* XXX Ugh bletch!  Whattakludge!  Linux's sense is reversed...  */
#undef	PAGE_MASK
#define	PAGE_MASK	(~(PAGE_SIZE-1))

#define	PAGE_ALIGN(x)		(((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))
#define	offset_in_page(x)	((x) & (PAGE_SIZE-1))

struct sysinfo {
	unsigned long totalram;
	unsigned long totalhigh;
	uint32_t mem_unit;
};

static inline void
si_meminfo(struct sysinfo *si)
{

	si->totalram = uvmexp.npages;
	si->totalhigh = kernel_map->size >> PAGE_SHIFT;
	si->mem_unit = PAGE_SIZE;
	/* XXX Fill in more as needed.  */
}

static inline unsigned long
vm_mmap(struct file *file __unused, unsigned long base __unused,
    unsigned long size __unused, unsigned long prot __unused,
    unsigned long flags __unused, unsigned long token __unused)
{

	return -ENODEV;
}

static inline unsigned long
get_num_physpages(void)
{
	return uvmexp.npages;
}

/*
 * XXX Requires that kmalloc in <linux/slab.h> and vmalloc in
 * <linux/vmalloc.h> both use malloc(9).  If you change either of
 * those, be sure to update this.
 */
static inline void
kvfree(void *ptr)
{

	if (ptr != NULL)
		free(ptr, M_TEMP);
}

static inline void
set_page_dirty(struct page *page)
{

	page->p_vmp.flags &= ~PG_CLEAN;
}

#endif  /* _LINUX_MM_H_ */
