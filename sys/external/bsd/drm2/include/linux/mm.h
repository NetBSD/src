/*	$NetBSD: mm.h,v 1.3.6.1 2015/04/06 15:18:17 skrll Exp $	*/

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

#include <sys/kauth.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_extern.h>

#include <asm/page.h>

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

/*
 * ###################################################################
 * ############### XXX THIS NEEDS SERIOUS SCRUTINY XXX ###############
 * ###################################################################
 */

/*
 * XXX unsigned long is a loser but will probably work accidentally.
 * XXX struct file might not map quite right between Linux and NetBSD.
 * XXX This is large enough it should take its own file.
 */

static inline unsigned long
vm_mmap(struct file *file, unsigned long base, unsigned long size,
    unsigned long prot, unsigned long flags, unsigned long token)
{
	struct vnode *vp;
	void *addr;
	int error;

	/*
	 * Cargo-culted from sys_mmap.  Various conditions kasserted
	 * rather than checked for expedience and safey.
	 */

	KASSERT(base == 0);
	KASSERT(prot == (PROT_READ | PROT_WRITE));
	KASSERT(flags == MAP_SHARED);

	KASSERT(file->f_type == DTYPE_VNODE);
	vp = file->f_data;

	KASSERT(vp->v_type == VCHR);
	KASSERT((file->f_flag & (FREAD | FWRITE)) == (FREAD | FWRITE));

	/* XXX pax_mprotect?  pax_aslr?  */

	addr = NULL;
	error = uvm_mmap_dev(curproc, &addr, size, vp->v_rdev, (off_t)base);
	if (error)
		goto out;

	KASSERT((uintptr_t)addr <= -1024UL); /* XXX Kludgerosity!  */

out:	/* XXX errno NetBSD->Linux (kludgerific) */
	return (error? (-error) : (unsigned long)addr);
}

#endif  /* _LINUX_MM_H_ */
