/*	$NetBSD: irix_syssgi.c,v 1.9 2001/12/25 19:04:19 manu Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: irix_syssgi.c,v 1.9 2001/12/25 19:04:19 manu Exp $");

#include "opt_ddb.h"

#ifndef ELFSIZE
#define ELFSIZE 32
#endif

/* round up and down to page boundaries. Borrowed from sys/kern/exec_elf32.c */
#define ELF_ROUND(a, b)         (((a) + (b) - 1) & ~((b) - 1))
#define ELF_TRUNC(a, b)         ((a) & ~((b) - 1))

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/sysctl.h>
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <compat/svr4/svr4_types.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_syscallargs.h>
#include <compat/irix/irix_syssgi.h>

static int irix_syssgi_mapelf __P((int, Elf_Phdr *, int, 
    struct proc *, register_t *));
static int irix_syssgi_sysconf __P((int name, struct proc *, register_t *));

int
irix_sys_syssgi(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_syssgi_args /* {
		syscallarg(int) request;
		syscallarg(void *) arg1;
		syscallarg(void *) arg2;
		syscallarg(void *) arg3;
		syscallarg(void *) arg4;                                        
		syscallarg(void *) arg5;                                        
	} */ *uap = v;  
	int request = SCARG(uap, request);
	void *arg1, *arg2, *arg3; 

#ifdef DEBUG_IRIX
	printf("irix_sys_syssgi(): request = %d\n", request);
#endif
	switch(request) {
	case IRIX_SGI_SYSID:	/* Get HostID */
		*retval = (register_t)hostid;
		break;

	case IRIX_SGI_RDNAME: {	/* Read Processes' name */
		struct proc *tp;

		arg1 = SCARG(uap, arg1); /* PID of the process */
		arg2 = SCARG(uap, arg2); /* Adress of user buffer */
		arg3 = SCARG(uap, arg3); /* Length of user buffer */
		if (!uvm_useracc((caddr_t)arg2, (size_t)arg2, B_WRITE))
			return EACCES;

		tp = pfind((pid_t)arg1);
		if (tp == NULL || \
		    tp->p_psstr == NULL || \
		    tp->p_psstr->ps_argvstr == NULL || \
		    tp->p_psstr->ps_argvstr[0] == NULL) 
			return 0;

		*retval = (register_t)copyout(tp->p_psstr->ps_argvstr[0], 
		    (void *)arg2, (size_t)arg3);

		break;
	}
	case IRIX_SGI_TUNE:	/* Tune system variables */
	case IRIX_SGI_IDBG:	/* Kernel debugging */
	case IRIX_SGI_INVENT:	/* Get system inventory */
	case IRIX_SGI_SETLED:	/* Turn on/off LEDs */
	case IRIX_SGI_SETNVRAM:	/* Sets a NVRAM variable value */
	case IRIX_SGI_GETNVRAM:	/* Gets a NVRAM variable value */
		return EINVAL;
		break;

	case IRIX_SGI_ELFMAP:	/* Maps an ELF image */
		 arg1 = SCARG(uap, arg1); /* file descriptor  */
		 arg2 = SCARG(uap, arg2); /* ptr to ELF program header array */
		 arg3 = SCARG(uap, arg3); /* array's length */
		return irix_syssgi_mapelf((int)arg1, (Elf_Phdr *)arg2, 
		    (int)arg3, p, retval);
		break;

	case IRIX_SGI_USE_FP_BCOPY:	/* bcopy and bzero can use FPU or not */
		/* 
		 * Our kernel does not use FPU, hence we do nothing.
		 */
		break;

	case IRIX_SGI_TOSSTSAVE:	/* Kill saved pregions */
		/* 
		 * Our kernel does not seem to save such "pregions",
		 * therefore we do not have to do anything.
		 */
		break;

	case IRIX_SGI_SYSCONF:		/* POSIX sysconf */
		arg1 = SCARG(uap, arg1); /* system variable name */
		return irix_syssgi_sysconf((int)arg1, p, retval);	
		break;

	case IRIX_SGI_RXEV_GET:		/* Trusted IRIX call */
		/* Undocumented (?) and unimplemented */
		return EINVAL;
		break;

	default:
		printf("Warning: call to unimplemented syssgi() command %d\n",
		    request);
		    return EINVAL;
		break;
	}

	return 0;
}

#define INSANE_DEBUG_IRIX
static int
irix_syssgi_mapelf(fd, ph, count, p, retval)
	int fd;
	Elf_Phdr *ph;
	int count;
	struct proc *p;
	register_t *retval;
{
	Elf_Phdr *kph;
	Elf_Phdr *pht;
	int error = 0;
	int i,j;
	int prot;
	int flags;
	u_long size;
	u_long uaddr, msize, psize, rm, rf;
	long diff, offset;
	struct file *fp;
	struct filedesc *fdp;
	struct exec_vmcmd_set vcset;
	struct exec_vmcmd *base_vcp = NULL;
	struct vnode *vp;

#ifdef INSANE_DEBUG_IRIX 
	printf("irix_syssgi_mapelf(): fd = %d, *ph = %p, count = %d\n",
	    fd, ph, count);
#endif
	/* Check that the program header array is readable by the process */
	if (!uvm_useracc((caddr_t)ph, sizeof(Elf_Phdr) * count, B_READ))
		return EACCES;

	kph = (Elf_Phdr *)malloc(sizeof(Elf_Phdr) * count,
	    M_TEMP, M_WAITOK);

	error = copyin(ph, kph, sizeof(Elf_Phdr) * count);
	if (error)
		goto bad;
	
	/* Check that each ELF sections is loadable */
	pht = kph;
	for (i = 0; i < count; i++) {
		if (pht->p_type != PT_LOAD) {
			error = ENOEXEC;
			goto bad;
		}
		pht++;
	}

	/* Find the file's vnode */
	fdp = p->p_fd;
	fp = fd_getfile(fdp, fd);
	if (fp == NULL) {
		error = EBADF;
		goto bad;
	}
	vp = (struct vnode *)fp->f_data;

	/* Map each sections. Borrowed from sys/kern/exec_elf32.c */
	FILE_USE(fp);
	pht = kph;
	for (i = 0; i < count; i++) {
#ifdef INSANE_DEBUG_IRIX
		printf("i = %d\n", i);
#endif
		vcset.evs_cnt = 0;
		vcset.evs_used = 0;

		if (pht->p_align > 1)
			uaddr = ELF_TRUNC(pht->p_vaddr, pht->p_align);
		else
			uaddr = pht->p_vaddr;
		diff = pht->p_vaddr - uaddr;

		prot |= (pht->p_flags & PF_R) ? VM_PROT_READ : 0;
		prot |= (pht->p_flags & PF_W) ? VM_PROT_WRITE : 0;
		prot |= (pht->p_flags & PF_X) ? VM_PROT_EXECUTE : 0;

		offset = pht->p_offset - diff;
		size = pht->p_filesz + diff;
		msize = pht->p_memsz + diff;

		if (pht->p_align >= PAGE_SIZE) {
			if ((pht->p_flags & PF_W) != 0) {
				/*
				 * Because the pagedvn pager can't handle zero 
				 * fill of the last data page if it's not 
				 * page aligned we map the last page readvn.
				 */
				psize = trunc_page(size);
			} else {
				psize = round_page(size);
			}
		} else {
			psize = size;
		}

		if (psize > 0) {
#ifdef INSANE_DEBUG_IRIX 
			printf("irix_mapelf(): psize > 0; NEW_VMCMD2\n");
			printf("psize = 0x%lx, uaddr = 0x%lx\n", psize, uaddr);
			printf("vp = %p, offset = 0x%lx\n", vp, offset);
			printf("pht->p_align = 0x%lx\n", (long)pht->p_align);
#endif
			NEW_VMCMD2(&vcset, pht->p_align < PAGE_SIZE ?
			    vmcmd_map_readvn : vmcmd_map_pagedvn, psize,
			    uaddr, vp, offset, prot, flags);
		}
		if (psize < size) {
#ifdef INSANE_DEBUG_IRIX 
			printf("irix_mapelf(): psize < size; NEW_VMCMD2\n");
#endif
			NEW_VMCMD2(&vcset, vmcmd_map_readvn, size - psize,
			    uaddr + psize, vp, offset + psize, prot,
			    psize > 0 ? flags & VMCMD_RELATIVE : flags);
		}

		/*
		 * Check if we need to extend the size of the segment
		 */
		rm = round_page(uaddr + msize);
		rf = round_page(uaddr + size);

		if (rm != rf) {
#ifdef INSANE_DEBUG_IRIX 
			printf("irix_mapelf(): rm != rf; NEW_VMCMD2\n");
#endif
			NEW_VMCMD2(&vcset, vmcmd_map_zero, rm - rf, rf, NULLVP,
			    0, prot, flags & VMCMD_RELATIVE);
			size = msize;
		}

		/* 
		 * Run the vmcmds
		 */
		for (j = 0; j < vcset.evs_used && !error; j++) {
			struct exec_vmcmd *vcp;

			vcp = &vcset.evs_cmds[j];
			if (vcp->ev_flags & VMCMD_RELATIVE) {
				vcp->ev_addr += base_vcp->ev_addr;
			}
#ifdef INSANE_DEBUG_IRIX
	printf("irix_syssgi_mapelf(): mapping i = %d, j = %d, *pht = %p\n",
	    i, j, pht);
	printf("uaddr = 0x%lx, size = 0x%lx, msize = 0x%lx\n",
	    uaddr, size, msize);
	printf("psize = 0x%lx, rm = 0x%lx, rf = 0x%lx\n",
	    psize, rm, rf);
#endif
			/* Sleeping here seems to trigger some bugs */
			/* (void)tsleep ((void *)&vcp, 0, NULL, 10); */

			error = (*vcp->ev_proc)(p, vcp);
			if (vcp->ev_flags & VMCMD_BASE)
			base_vcp = vcp;
		}
		pht++;
		kill_vmcmds(&vcset);
	}

	FILE_UNUSE(fp, p);
	
	*retval = (register_t)kph->p_vaddr;	
bad:
	free(kph, M_TEMP);
	return error;
}



static int
irix_syssgi_sysconf(name, p, retval)
	int name;
	struct proc *p;
	register_t *retval;
{
	int error = 0;
	int mib[2], value;
	int len = sizeof(value);

	switch (name) {
	case IRIX_SC_ARG_MAX:
		mib[0] = CTL_KERN;
		mib[1] = KERN_ARGMAX;
		break;
	case IRIX_SC_CHILD_MAX:
		mib[0] = CTL_KERN;
		mib[1] = KERN_MAXPROC;
		break;
	case IRIX_SC_CLK_TCK:
		*retval = hz;
		return 0;
		break;
	case IRIX_SC_NGROUPS_MAX:
		mib[0] = CTL_KERN;
		mib[1] = KERN_NGROUPS;
		break;
	case IRIX_SC_OPEN_MAX:
		mib[0] = CTL_KERN;
		mib[1] = KERN_MAXFILES;
		break;
	case IRIX_SC_JOB_CONTROL:
		*retval = 1;
		return 0;
		break;
	case IRIX_SC_SAVED_IDS:
		*retval = 1;
		return 0;
		break;
	/* Trusted IRIX capabilities are unsupported */
	case IRIX_SC_ACL:	/* ACcess Lists */
	case IRIX_SC_AUDIT:	/* Audit */
	case IRIX_SC_INF:	/* Information labels */
	case IRIX_SC_MAC:	/* Mandatory Access Control */
	case IRIX_SC_CAP:	/* Capabilities */
		*retval = 0;
		return 0;
		break;
	case IRIX_SC_PAGESIZE:
		mib[0] = CTL_HW;
		mib[1] = HW_PAGESIZE;
		break;

	case IRIX_SC_PASS_MAX:
	case IRIX_SC_VERSION:
	default:
		printf("Warning: syssgi(SYSCONF) unsupported variable %d\n",
		    name);
		    return EINVAL;
		break;
	}

	error = kern_sysctl(mib, 2, &value, &len, NULL, 0, p);
	if (error == 0)
		*retval = (register_t)value;
	return error;
}
