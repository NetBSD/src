/*	$NetBSD: irix_syssgi.c,v 1.14 2002/02/02 19:27:18 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_syssgi.c,v 1.14 2002/02/02 19:27:18 manu Exp $");

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
#include <sys/file.h>
#include <sys/sysctl.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <compat/svr4/svr4_types.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_syscallargs.h>
#include <compat/irix/irix_syssgi.h>

/* In sys/kern/exec_elf32.c */
void	ELFNAME(load_psection)(struct exec_vmcmd_set *, struct vnode *,
	    const Elf_Phdr *, Elf_Addr *, u_long *, int *, int);

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

	case IRIX_SGI_GETPGID: {	/* Get parent process GID */
		struct proc *tp;

		arg1 = SCARG(uap, arg1); /* Process group GID */ 
		if (arg1 == 0)
			tp = p;
		else
			tp = pfind((pid_t)arg1);

		if (tp == NULL || tp->p_pgrp == NULL)
			return 0;

		*retval = (register_t)tp->p_pgid;

		return 0;
	}

	case IRIX_SGI_RDNAME: {	/* Read Processes' name */
		struct proc *tp;

		arg1 = SCARG(uap, arg1); /* PID of the process */
		arg2 = SCARG(uap, arg2); /* Address of user buffer */
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
		return 0;
		break;

	default:
		printf("Warning: call to unimplemented syssgi() command %d\n",
		    request);
		    return EINVAL;
		break;
	}

	return 0;
}

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
	Elf_Addr uaddr;
	struct file *fp;
	struct filedesc *fdp;
	struct exec_vmcmd_set vcset;
	struct exec_vmcmd *base_vcp = NULL;
	struct vnode *vp;
	struct vm_map_entry *ret;
	struct exec_vmcmd *vcp;
	Elf_Addr relocation = IRIX_MAPELF_RELOCATE;

	vcset.evs_cnt = 0;
	vcset.evs_used = 0;

	/* Check that the program header array is writable by the process */
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
#ifdef DEBUG_IRIX_MAPELF
		printf("mapelf: section %d: type=%d offset=0x%x vaddr=%p paddr=%p filesz=0x%08x memsz=0x%08x flags=0x%08lx align=0x%08x\n", i, pht->p_type, pht->p_offset, (void *)pht->p_vaddr, (void *)pht->p_paddr, pht->p_filesz, pht->p_memsz, (long)pht->p_flags, pht->p_align);
#endif
		pht++;
	}

	/* Find the file's vnode */
	fdp = p->p_fd;
	fp = fd_getfile(fdp, fd);
	if (fp == NULL) {
		error = EBADF;
		goto bad;
	}
	FILE_USE(fp);
	vp = (struct vnode *)fp->f_data;

	/* Load the sections */
	pht = kph;
	for (i = 0; i < count; i++) {
retry:
#ifdef DEBUG_IRIX_MAPELF
		printf("mapelf: loading section %d, addr=%p, len=0x%08lx\n", 
		    i, (void *)pht->p_vaddr, (long)pht->p_memsz);
#endif
		kill_vmcmds(&vcset);
		uaddr = pht->p_vaddr;
		size = 0;
		prot = 0;
		flags = VMCMD_BASE;

		ELFNAME(load_psection)(&vcset, vp, pht, &uaddr, 
		    &size, &prot, flags);

#ifdef DEBUG_IRIX_MAPELF
		printf("mapelf: vmcmd to run = %d\n", vcset.evs_used);
#endif
		for (j = 0; j < vcset.evs_used && !error; j++) {
			vcp = &vcset.evs_cmds[j];
			if (vcp->ev_flags & VMCMD_RELATIVE) {
				if (base_vcp == NULL)
					panic("irix_syssgi_mapelf():  bad vmcmd base\n");
				   
				vcp->ev_addr += base_vcp->ev_addr;
			}
#ifdef DEBUG_IRIX_MAPELF
			printf("mapelf: attempt to run vmcmd %d (addr %p)\n",
			    j, (void *)vcp->ev_addr);
#endif
			error = (*vcp->ev_proc)(p, vcp);
			if (error) {
				/* 
				 * Section load failed, probably because the 
				 * requested area was already allocated. We 
				 * try to relocate the section to a free place.
				 */
				relocation = (relocation & ~(pht->p_align - 1))
				    + pht->p_align;
				ret = uvm_map_findspace(&p->p_vmspace->vm_map, 
				    relocation, vcp->ev_len, 
				    (void *)&pht->p_vaddr, 
				    NULL, 0, pht->p_align, 0);
				if (ret == NULL) {
#ifdef DEBUG_IRIX_MAPELF
					printf("mapelf: failure\n");
#endif
					goto bad;
				}

				/* 
				 * XXX this is a hack to get libX11 laoding 
				 *
				 * We have to use it since uvm_map_findspace()
				 * insist on proposing addresses that are
				 * pht->p_align far away from the first 
				 * addresses availlable after "relocation".
				 * It seems to be a NetBSD round up vs IRIX 
				 * round down issue.
				 */
				if (relocation == ((u_long)pht->p_vaddr 
				    - pht->p_align)) {
					pht->p_vaddr = (Elf_Addr)relocation;
#ifdef DEBUG_IRIX_MAPELF
					printf("mapelf: relocation hack\n");
#endif
				}
#ifdef DEBUG_IRIX_MAPELF
				printf("mapelf: relocating at %p\n", 
				    (void *)pht->p_vaddr);
#endif
				relocation = pht->p_vaddr + vcp->ev_len;
				error = 0;
				goto retry;
			}
#ifdef DEBUG_IRIX_MAPELF
			printf("mapelf: success\n");
#endif
		}
		pht++;
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
