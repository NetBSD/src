/*	$NetBSD: shmem_fs.h,v 1.4 2021/12/19 12:13:08 riastradh Exp $	*/

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

#ifndef _LINUX_SHMEM_FS_H_
#define _LINUX_SHMEM_FS_H_

#include <sys/types.h>
#include <sys/param.h>

#include <sys/rwlock.h>

#include <lib/libkern/libkern.h>

#include <uvm/uvm_extern.h>

#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/mm_types.h>

static inline struct page *
shmem_read_mapping_page_gfp(struct uvm_object *uobj, voff_t i, gfp_t gfp)
{
	struct vm_page *vm_page;
	int error;

	error = uvm_obj_wirepages(uobj, i*PAGE_SIZE, (i + 1)*PAGE_SIZE, NULL);
	if (error)
		return ERR_PTR(-error); /* XXX errno NetBSD->Linux */

	rw_enter(uobj->vmobjlock, RW_READER);
	vm_page = uvm_pagelookup(uobj, i*PAGE_SIZE);
	rw_exit(uobj->vmobjlock);

	KASSERT(vm_page);
	return container_of(vm_page, struct page, p_vmp);
}

static inline struct page *
shmem_read_mapping_page(struct uvm_object *uobj, voff_t i)
{
	return shmem_read_mapping_page_gfp(uobj, i, GFP_KERNEL);
}

static inline void
shmem_truncate_range(struct uvm_object *uobj, voff_t start, voff_t end)
{
	int flags = PGO_FREE;

	if (start == 0 && end == -1) {
		flags |= PGO_ALLPAGES;
	} else {
		KASSERT(0 <= start);
		KASSERT(start <= end);
	}

	rw_enter(uobj->vmobjlock, RW_WRITER);
	(*uobj->pgops->pgo_put)(uobj, start, end, flags);
}

#endif  /* _LINUX_SHMEM_FS_H_ */
