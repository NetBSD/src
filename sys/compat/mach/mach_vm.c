/*	$NetBSD: mach_vm.c,v 1.10 2002/11/17 18:39:48 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_vm.c,v 1.10 2002/11/17 18:39:48 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_prot.h>
#include <uvm/uvm_map.h>
 
#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_vm.h> 
#include <compat/mach/mach_errno.h> 
#include <compat/mach/mach_syscallargs.h>

int
mach_vm_map(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_vm_map_request_t req;
	mach_vm_map_reply_t rep;
	struct sys_mmap_args cup;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	DPRINTF(("mach_vm_map(addr = %p, size = 0x%08x);\n",
	    (void *)req.req_address, req.req_size));

#if 1
	/* XXX Darwin fails on mapping a page at address 0 */
	if (req.req_address == 0)
		return MACH_MSG_ERROR(msgh, &req, &rep, ENOMEM);
#endif

	bzero(&rep, sizeof(rep));

	SCARG(&cup, addr) = (void *)req.req_address;
	SCARG(&cup, len) = req.req_size;
	SCARG(&cup, prot) = PROT_READ | PROT_WRITE;
	SCARG(&cup, flags) = MAP_ANON | MAP_FIXED;
	SCARG(&cup, fd) = -1;
	SCARG(&cup, pos) = 0;
	
	if ((error = sys_mmap(p, &cup, &rep.rep_retval)) != 0)
		return MACH_MSG_ERROR(msgh, &req, &rep, error);

	rep.rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}

int
mach_vm_allocate(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_vm_allocate_request_t req;
	mach_vm_allocate_reply_t rep;
	struct sys_mmap_args cup;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	DPRINTF(("mach_vm_allocate(addr = %p, size = 0x%08x);\n",
	    (void *)req.req_address, req.req_size));

	bzero(&rep, sizeof(rep));

	SCARG(&cup, addr) = (caddr_t)req.req_address;
	SCARG(&cup, len) = req.req_size;
	SCARG(&cup, prot) = PROT_READ | PROT_WRITE;
	SCARG(&cup, flags) = MAP_ANON;
	if (req.req_flags)
		SCARG(&cup, flags) |= MAP_FIXED;
	SCARG(&cup, fd) = -1;
	SCARG(&cup, pos) = 0;

	if ((error = sys_mmap(p, &cup, &rep.rep_address)) != 0)
		return MACH_MSG_ERROR(msgh, &req, &rep, error);

	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}
int
mach_vm_deallocate(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_vm_deallocate_request_t req;
	mach_vm_deallocate_reply_t rep;
	struct sys_munmap_args cup;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	DPRINTF(("mach_vm_deallocate(addr = %p, size = 0x%08x);\n",
	    (void *)req.req_address, req.req_size));

	bzero(&rep, sizeof(rep));

	SCARG(&cup, addr) = (caddr_t)req.req_address;
	SCARG(&cup, len) = req.req_size;

	if ((error = sys_munmap(p, &cup, &rep.rep_retval)) != 0)
		return MACH_MSG_ERROR(msgh, &req, &rep, error);

	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}


/* XXX The findspace argument is not handled correctly */
int
mach_sys_map_fd(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct mach_sys_map_fd_args /* {
		syscallarg(int) fd;
		syscallarg(mach_vm_offset_t) offset;
		syscallarg(mach_vm_offset_t *) va;
		syscallarg(mach_boolean_t) findspace;
		syscallarg(mach_vm_size_t) size;
	} */ *uap = v;
	struct file *fp; 
	struct filedesc *fdp;
	struct vnode *vp;
	struct exec_vmcmd evc;
	struct vm_map_entry *ret;
	void *va;
	int error;

	if ((error = copyin(SCARG(uap, va), (void *)&va, sizeof(va))) != 0)
		return error;

	fdp = p->p_fd;
	fp = fd_getfile(fdp, SCARG(uap, fd));
	if (fp == NULL)
		return EBADF;

	FILE_USE(fp);
	vp = (struct vnode *)fp->f_data;
	vref(vp);

	DPRINTF(("vm_map_fd: addr = %p len = 0x%08x\n", va, SCARG(uap, size)));
	bzero(&evc, sizeof(evc));
	evc.ev_addr = (u_long)va;
	evc.ev_len = SCARG(uap, size);
	evc.ev_prot = VM_PROT_ALL;
	evc.ev_flags = 0;
	evc.ev_proc = vmcmd_map_readvn;
	evc.ev_offset = SCARG(uap, offset);
	evc.ev_vp = vp;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if ((error = (*evc.ev_proc)(p, &evc)) != 0) {
		VOP_UNLOCK(vp, 0);

		DPRINTF(("mach_sys_map_fd: mapping at %p failed\n", va));

		if (SCARG(uap, findspace) == 0)
			goto bad2;

		vm_map_lock(&p->p_vmspace->vm_map);
		if ((ret = uvm_map_findspace(&p->p_vmspace->vm_map,
		    0x8000, evc.ev_len, (vaddr_t *)&evc.ev_addr,
		    NULL, 0, PAGE_SIZE, 0)) == NULL) {
			vm_map_unlock(&p->p_vmspace->vm_map);
			goto bad2;
		}
		vm_map_unlock(&p->p_vmspace->vm_map);

		va = (void *)evc.ev_addr;

		bzero(&evc, sizeof(evc));
		evc.ev_addr = (u_long)va;
		evc.ev_len = SCARG(uap, size);
		evc.ev_prot = VM_PROT_ALL;
		evc.ev_flags = 0;
		evc.ev_proc = vmcmd_map_readvn;
		evc.ev_offset = SCARG(uap, offset);
		evc.ev_vp = vp;

		DPRINTF(("mach_sys_map_fd: trying at %p\n", va));
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = (*evc.ev_proc)(p, &evc)) != 0)
			goto bad1;
	}

	vput(vp);
	FILE_UNUSE(fp, p);
	DPRINTF(("mach_sya_map_fd: mapping at %p\n", (void *)evc.ev_addr));

	va = (mach_vm_offset_t *)evc.ev_addr;

	if ((error = copyout((void *)&va, SCARG(uap, va), sizeof(va))) != 0)
		return error;
	
	return 0;

bad1:
	VOP_UNLOCK(vp, 0);
bad2:	
	vrele(vp);
	FILE_UNUSE(fp, p);
	DPRINTF(("mach_sys_map_fd: mapping at %p failed, error = %d\n", 
	    (void *)evc.ev_addr, error));
	return error;
}
 
