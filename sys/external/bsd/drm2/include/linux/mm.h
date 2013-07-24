/*	$NetBSD: mm.h,v 1.1.2.2 2013/07/24 02:04:31 riastradh Exp $	*/

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

#include <uvm/uvm_extern.h>

#define	PAGE_ALIGN(x)	round_page(x)

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
	struct vnode *vnode;
	vaddr_t addr = 0;
	int error;

	/*
	 * Cargo-culted from sys_mmap.  Various conditions kasserted
	 * rather than checked for expedience and safey.
	 */

	KASSERT(base == 0);
	KASSERT(prot == (PROT_READ | PROT_WRITE));
	KASSERT(flags == MAP_SHARED);

	KASSERT(file->f_type == DTYPE_VNODE);
	vnode = file->f_data;

	KASSERT(vnode->v_type == VCHR);
	KASSERT((file->f_flag & (FREAD | FWRITE)) == (FREAD | FWRITE));

	{
		struct vattr va;

		vn_lock(vnode, (LK_SHARED | LK_RETRY));
		error = VOP_GETATTR(vnode, &va, kauth_cred_get());
		VOP_UNLOCK(vnode);
		if (error)
			goto out;
		/* XXX kassert?  */
		if ((va.va_flags & (SF_SNAPSHOT | IMMUTABLE | APPEND)) != 0) {
			error = EACCES;
			goto out;
		}
	}

	/* XXX pax_mprotect?  pax_aslr?  */

	error = uvm_mmap(&curproc->p_vmspace->vm_map, &addr, size,
	    (VM_PROT_READ | VM_PROT_WRITE), (VM_PROT_READ | VM_PROT_WRITE),
	    MAP_SHARED, vnode, base,
	    curproc->p_rlimit[RLIMIT_MEMLOCK].rlim_cur);
	if (error)
		goto out;

	KASSERT(addr <= -1024UL); /* XXX Kludgerosity!  */

out:	/* XXX errno NetBSD->Linux (kludgerific) */
	return (error? (-error) : (unsigned long)addr);
}

#endif  /* _LINUX_MM_H_ */
