#undef DEBUG_DARWIN
#undef DEBUG_MACH
/*	$NetBSD: darwin_mman.c,v 1.6 2003/01/22 17:47:03 christos Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_mman.c,v 1.6 2003/01/22 17:47:03 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/exec.h>

#include <sys/syscallargs.h>

#include <compat/common/compat_file.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_syscallargs.h>

int
darwin_sys_load_shared_file(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_load_shared_file_args /* {
		syscallarg(char *) filename;
		syscallarg(caddr_t) addr;
		syscallarg(u_long) len;
		syscallarg(caddr_t *) base;
		syscallarg(int) count:
		syscallarg(mach_sf_mapping_t *) mappings;
		syscallarg(int *) flags;
	} */ *uap = v;
	struct file *fp;
	struct filedesc *fdp;
	struct vnode *vp;
	vaddr_t base;
	struct proc *p = l->l_proc;
	int flags;
	char filename[MAXPATHLEN + 1];
	mach_sf_mapping_t *mapp;
	size_t maplen;
	struct sys_open_args open_cup;
	struct sys_close_args close_cup;
	int fd;
	int i;
	int error;
	vaddr_t max_addr, addr;
	size_t len;
	vaddr_t uaddr;
	int need_relocation;
	struct exec_vmcmd evc;

	if ((error = copyin(SCARG(uap, filename), &filename, MAXPATHLEN)) != 0)
		return error;

	if ((error = copyin(SCARG(uap, base), &base, sizeof(base))) != 0)
		return error;

	if ((error = copyin(SCARG(uap, flags), &flags, sizeof(base))) != 0)
		return error;

#ifdef DEBUG_DARWIN
	DPRINTF(("darwin_sys_load_shared_file: filename = %p ", 
	    SCARG(uap, filename)));
	DPRINTF(("addr = %p len = 0x%08lx base = %p ", 
	    SCARG(uap, addr), SCARG(uap, len), SCARG(uap, base)));
	DPRINTF(("count = %d mappings = %p flags = %p ", 
	    SCARG(uap, count), SCARG(uap, mappings), SCARG(uap, flags)));
	DPRINTF(("*base = 0x%08lx *flags = %d filename=`%s'\n",
	    base, flags, filename));
#endif

	SCARG(&open_cup, path) = SCARG(uap, filename);
	SCARG(&open_cup, flags) = O_RDONLY;
	SCARG(&open_cup, mode) = 0;
	if ((error = bsd_sys_open(l, &open_cup, &fd)) != 0)
		return error;
	
	fdp = p->p_fd;
	fp = fd_getfile(fdp, fd);
	if (fp == NULL) {
		error = EBADF;
		goto bad3;
	}
	FILE_USE(fp);
	vp = (struct vnode *)fp->f_data;
	vref(vp);

	/* XXX maximum count ? */
	maplen = sizeof(*mapp) * SCARG(uap, count);
	if (maplen > PAGE_SIZE) {
		error = ENOMEM;
		goto bad3;
	}
	mapp = malloc(sizeof(*mapp) * SCARG(uap, count), M_TEMP, M_WAITOK);

	if ((error = copyin(SCARG(uap, mappings), mapp, maplen)) != 0)
		goto bad2;
	
#ifdef DEBUG_DARWIN
	for (i = 0; i < SCARG(uap, count); i++) {
		DPRINTF(("mapp[%d].mapping_offset = 0x%08lx\n", 
		    i, mapp[i].mapping_offset));
		DPRINTF(("mapp[%d].size = 0x%08x\n", i, mapp[i].size));
		DPRINTF(("mapp[%d].file_offset = 0x%08lx\n", 
		    i, mapp[i].file_offset));
		DPRINTF(("mapp[%d].protection = %d\n", 
		    i, mapp[i].protection));
		DPRINTF(("mapp[%d].cksum = %ld\n", i, mapp[i].cksum));
	}
#endif

	/* Check if we can load at the default addresses */
	need_relocation = 0;
	vm_map_lock(&p->p_vmspace->vm_map);
	for (i = 0; i < SCARG(uap, count); i++) 
		if ((uvm_map_findspace(&p->p_vmspace->vm_map,
		    base + mapp[i].mapping_offset, mapp[i].size, 
		    &uaddr, NULL, 0, 0, UVM_FLAG_FIXED)) == NULL)
			need_relocation = 1;	
	vm_map_unlock(&p->p_vmspace->vm_map);

	/* If we cannot, we need a relocation */
	if (need_relocation) {
		DPRINTF(("Relocating\n"));
		/* Compute the length of the region enclosing all sections */
		max_addr = 0; 
		for (i = 0; i < SCARG(uap, count); i++) {
			addr = (vaddr_t)(mapp[i].mapping_offset 
			    + base + mapp[i].size);
			if (addr > max_addr)
				max_addr = addr;
		}
		len = max_addr - base;
		DPRINTF(("base = 0x%08lx max_addr = 0x%08lx len = 0x%08x\n",
		    base, max_addr, len));

		/* Find some place to map this region */
		vm_map_lock(&p->p_vmspace->vm_map);
		if ((uvm_map_findspace(&p->p_vmspace->vm_map, base, 
		    len, &uaddr, NULL, 0, PAGE_SIZE, 0)) == NULL) {
			DPRINTF(("Impossible to find some space\n"));
			vm_map_unlock(&p->p_vmspace->vm_map);
			error = ENOMEM;
			goto bad2;
		}
		vm_map_unlock(&p->p_vmspace->vm_map);

		/* Update the base address */
		base = uaddr;
		DPRINTF(("New base address: base = 0x%08lx\n", base));
	}
		
	/* Do the actual mapping */
	for (i = 0; i < SCARG(uap, count); i++) {
		bzero(&evc, sizeof(evc));
		evc.ev_addr = base + mapp[i].mapping_offset;
		evc.ev_len = mapp[i].size;
		evc.ev_prot = mapp[i].protection & VM_PROT_ALL;
		evc.ev_flags = 0;
		if (mapp[i].protection & MACH_VM_PROT_ZF)
			evc.ev_proc = vmcmd_map_zero;
		else
			evc.ev_proc = vmcmd_map_readvn;
		evc.ev_offset = mapp[i].file_offset;
		evc.ev_vp = vp;

		DPRINTF(("map section %d: start = 0x%08lx, len = 0x%08lx\n",
		    i, evc.ev_addr, evc.ev_len));

		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = (*evc.ev_proc)(p, &evc)) != 0) {
			VOP_UNLOCK(vp, 0);
			DPRINTF(("Failed\n"));
			goto bad2;
		}
		VOP_UNLOCK(vp, 0);
		DPRINTF(("Success\n"));
	}
bad2:
	free(mapp, M_TEMP);
bad3:
	vrele(vp);
	FILE_UNUSE(fp, p);
	SCARG(&close_cup, fd) = fd;
	if ((error = sys_close(l, &close_cup, retval)) != 0)
		return error;

	if ((error = copyout(&base, SCARG(uap, base), sizeof(base))) != 0)
		return error;

	if ((error = copyout(&flags, SCARG(uap, flags), sizeof(base))) != 0)
		return error;

	return error;
}
