/*	$NetBSD: irix_mman.c,v 1.8 2003/01/22 12:58:22 rafal Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: irix_mman.c,v 1.8 2003/01/22 12:58:22 rafal Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/mount.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_syscallargs.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_mman.h>
#include <compat/irix/irix_prctl.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_syscallargs.h>

static int irix_mmap __P((struct lwp *, void *, size_t, int , 
		int, int, off_t, register_t *)); 

int
irix_sys_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_mmap_args /* {
		syscallarg(void *) addr;
		syscallarg(irix_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;                                             
		syscallarg(irix_off_t) pos; 
	} */ *uap = v;

	return irix_mmap(l, SCARG(uap, addr), SCARG(uap, len), 
	    SCARG(uap, prot), SCARG(uap, flags), SCARG(uap, fd), 
	    SCARG(uap, pos), retval);
}

int
irix_sys_mmap64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_mmap64_args /* {
		syscallarg(void *) addr;
		syscallarg(irix_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;                                             
		syscallarg(int) pad1;
		syscallarg(irix_off64_t) pos; 
	} */ *uap = v;

	return irix_mmap(l, SCARG(uap, addr), SCARG(uap, len), 
	    SCARG(uap, prot), SCARG(uap, flags), SCARG(uap, fd), 
	    SCARG(uap, pos), retval);
}

static int
irix_mmap(l, addr, len, prot, flags, fd, pos, retval)
	struct lwp *l;
	void *addr;
	size_t len;
	int prot;
	int flags;
	int fd;
	off_t pos;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct sys_mmap_args cup;
	int bsd_flags = 0;
	int error = 0;

#ifdef DEBUG_IRIX
	printf("irix_sys_mmap(): addr = %p, len = 0x%x, prot = 0x%x ",
	    addr, len, prot);
	printf("flags = 0x%x, fd = %d, pos = 0x%lx\n", flags, fd, (long)pos);
	    
#endif
	if (flags & IRIX_MAP_SHARED)
		bsd_flags |= MAP_SHARED;
	if (flags & IRIX_MAP_PRIVATE)
		bsd_flags |= MAP_PRIVATE;
	if (flags & IRIX_MAP_COPY)
		bsd_flags |= MAP_PRIVATE;

	/*
	 * Note about MAP_FIXED: IRIX's mmap(2) states that
	 * when MAP_FIXED is unset, range 0x30000000 to 0x40000000
	 * will not be used except if MAP_SGI_ANYADDR is set
	 * or if syssgi(SGI_UNSUPPORTED_MAP_RESERVED_RANGE) was 
	 * enabled. We do not emulate this behavior for now.
	 */
	if (flags & IRIX_MAP_FIXED)
		bsd_flags |= MAP_FIXED;
	if (flags & IRIX_MAP_RENAME)
		bsd_flags |= MAP_RENAME;
		
	if (flags & IRIX_MAP_AUTORESRV)
		printf("Warning: unsupported IRIX mmap() flag MAP_AUTORESV\n");
	if (flags & IRIX_MAP_TEXT)
		printf("Warning: unsupported IRIX mmap() flag MAP_TEXT\n");
	if (flags & IRIX_MAP_BRK)
		printf("Warning: unsupported IRIX mmap() flag MAP_BRK\n");
	if (flags & IRIX_MAP_PRIMARY)
		printf("Warning: unsupported IRIX mmap() flag MAP_PRIMARY\n");
	if (flags & IRIX_MAP_SGI_ANYADDR)
		printf("Warning: unsupported IRIX mmap() flag IRIX_MAP_SGI_ANYADDR\n");

	/* 
	 * When AUTOGROW is set and the mapping is bigger than
	 * the file, if pages beyond the end of file are touched,
	 * IRIX will increase the file size accordingly. We are
	 * not able to emulate this (yet), hence we immediatly
	 * grow the file to fit the mapping, before mapping it.
	 */
	if (flags & IRIX_MAP_AUTOGROW) {
		struct file *fp;
		struct vnode *vp;
		struct vattr vattr;
		
		/* getvnode does FILE_USE */
		if ((error = getvnode(p->p_fd, fd, &fp)) != 0)
			return error;

		if ((fp->f_flag & FWRITE) == 0) {
			error = EINVAL;
			goto out;
		}

		vp = (struct vnode *)fp->f_data;
		if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
			error = ESPIPE;
			goto out;
		}

		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}

		if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
			goto out;

		if (pos + len > vattr.va_size) {
			VATTR_NULL(&vattr);
			vattr.va_size = round_page(pos + len);

			VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

			error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);

			VOP_UNLOCK(vp, 0);
		}
out:
		FILE_UNUSE(fp, p);
		if (error)
			return error;
			
	}

	SCARG(&cup, addr) = addr;
	SCARG(&cup, len) = len;
	SCARG(&cup, prot) = prot;
	SCARG(&cup, flags) = bsd_flags;
	SCARG(&cup, fd) = fd;
	SCARG(&cup, pos) = pos;

	/* A private mapping that should not be visible to the share group */
	if (flags & IRIX_MAP_LOCAL) {
		if ((error = sys_mmap(l, &cup, retval)) != 0)
			return error;
		addr = (void *)*retval;
		irix_isrr_insert((vaddr_t)addr, len, IRIX_ISRR_PRIVATE, p);
		return 0;
	}
		
	IRIX_VM_SYNC(p, error = sys_mmap(l, &cup, retval));
	return error;
}


int 
irix_sys_munmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_munmap_args /* {
		syscallarg(void *) addr;
		syscallarg(size_t) len;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;

	IRIX_VM_SYNC(p, error = sys_munmap(l, v, retval));
	if (error == 0)
		irix_isrr_insert((vaddr_t)SCARG(uap, addr), 
		    SCARG(uap, len), IRIX_ISRR_SHARED, p);

	return error;
}

int 
irix_sys_break(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	int error;

	IRIX_VM_SYNC(p, error = svr4_sys_break(l, v, retval));
	return error;
}

#ifdef SYSVSHM 
int 
irix_sys_shmsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	int error;

	IRIX_VM_SYNC(p, error = svr4_sys_shmsys(l, v, retval));
	return error;
}
#endif

int 
irix_sys_mprotect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	int error;

	IRIX_VM_SYNC(p, error = sys_mprotect(l, v, retval));
	return error;
}
