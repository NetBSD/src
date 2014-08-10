/*	$NetBSD: vmalloc.h,v 1.2.2.1 2014/08/10 06:55:39 tls Exp $	*/

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

#ifndef _LINUX_VMALLOC_H_
#define _LINUX_VMALLOC_H_

#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <linux/mm_types.h>

static inline void *
vmalloc_user(unsigned long size)
{
	return malloc(size, M_TEMP, (M_WAITOK | M_ZERO));
}

static inline void *
vzalloc(unsigned long size)
{
	return malloc(size, M_TEMP, (M_WAITOK | M_ZERO));
}

static inline void
vfree(void *ptr)
{
	return free(ptr, M_TEMP);
}

#define	PAGE_KERNEL	0	/* XXX pgprot */

static inline void *
vmap(struct page **pages, unsigned npages, unsigned long flags __unused,
    pgprot_t prot __unused)
{
	vaddr_t va;

	/* XXX Sleazy cast should be OK here.  */
	__CTASSERT(sizeof(*pages[0]) == sizeof(struct vm_page));
	va = uvm_pagermapin((struct vm_page **)pages, npages, 0);
	if (va == 0)
		return NULL;

	return (void *)va;
}

static inline void
vunmap(void *ptr, unsigned npages)
{

	uvm_pagermapout((vaddr_t)ptr, npages);
}

#endif  /* _LINUX_VMALLOC_H_ */
