/*	$NetBSD: irix_exec_elf32.c,v 1.8 2003/08/06 01:04:44 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_exec_elf32.c,v 1.8 2003/08/06 01:04:44 manu Exp $");

#ifndef ELFSIZE
#define ELFSIZE		32	/* XXX should die */
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/null.h>
#include <sys/malloc.h>
#include <sys/syslimits.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/errno.h>
#include <sys/namei.h>	/* Used for irix_load_addr */
#include <sys/vnode.h>	/* Used for irix_load_addr */

#include <machine/vmparam.h>

#include <uvm/uvm_extern.h>

#include <compat/common/compat_util.h>

#include <compat/irix/irix_exec.h>

static int irix_load_addr(struct proc *, char *, vaddr_t *);

/*
 * IRIX o32 ABI probe function
 */
int
ELFNAME2(irix,probe_o32)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp; 
	vaddr_t *pos; 
{
	int error;

#ifdef DEBUG_IRIX
	printf("irix_probe_o32()\n");
#endif
	if ((((Elf_Ehdr *)epp->ep_hdr)->e_flags & IRIX_EF_IRIX_ABI_MASK) !=
	    IRIX_EF_IRIX_ABIO32)
		return error;

	if (itp[0]) {
		/* o32 binaries use /lib/libc.so.1 */
		if (strncmp(itp, "/lib/libc.so", 12) && 
		    strncmp(itp, "/usr/lib/libc.so", 16))
			return ENOEXEC;
		if ((error = emul_find_interp(p, epp->ep_esch->es_emul->e_path,
		    itp)))
			return error;
		if ((error = irix_load_addr(p, itp, pos)) != 0)
			return error;
	} else {
		*pos = ELF_NO_ADDR;
	}
#ifdef DEBUG_IRIX
	printf("irix_probe_o32: returning 0\n");
	printf("epp->ep_vm_minaddr = 0x%lx\n", epp->ep_vm_minaddr);
#endif
	epp->ep_vm_minaddr = epp->ep_vm_minaddr & ~0xfUL;
	return 0;
}

/*
 * IRIX n32 ABI probe function
 */
int
ELFNAME2(irix,probe_n32)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp; 
	vaddr_t *pos; 
{
	int error;

#ifdef DEBUG_IRIX
	printf("irix_probe_n32()\n");
#endif
	if ((((Elf_Ehdr *)epp->ep_hdr)->e_flags & IRIX_EF_IRIX_ABI_MASK) !=
	    IRIX_EF_IRIX_ABIN32)
		return error;

	if (itp[0]) {
		/* n32 binaries use /lib32/libc.so.1 */
		if (strncmp(itp, "/lib32/libc.so", 14) &&
		    strncmp(itp, "/usr/lib32/libc.so", 18))
			return ENOEXEC;
		if ((error = emul_find_interp(p, epp->ep_esch->es_emul->e_path,
		    itp)))
			return error;
	}
	*pos = ELF_NO_ADDR;
#ifdef DEBUG_IRIX
	printf("irix_probe_n32: returning 0\n");
	printf("epp->ep_vm_minaddr = 0x%lx\n", epp->ep_vm_minaddr);
#endif
	epp->ep_vm_minaddr = epp->ep_vm_minaddr & ~0xfUL;
	return 0;
}

int
ELFNAME2(irix,copyargs)(p, pack, arginfo, stackp, argp)
	struct proc *p;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	char **cpp, *dp, *sp;
	size_t len;
	void *nullp;
	long argc, envc;
	AuxInfo ai[IRIX_ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;
	int error;

	/*
	 * IRIX seems to expect argc and **argv to be aligned on a 
	 * 16 bytes boundary. It seems there is no other way of
	 * getting **argv aligned than duplicating and customizing
	 * the code that sets up the stack in copyargs():
	 */
#ifdef DEBUG_IRIX
	printf("irix_elf32_copyargs(): *stackp = %p\n", *stackp);
#endif
	/* 
	 * This is borrowed from sys/kern/kern_exec.c:copyargs()
	 */
	cpp = (char **)*stackp;
	nullp = NULL;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_es->es_arglen);

	/* Align **argv on a 16 bytes boundary */
	dp = (char *)(((unsigned long)dp + 0xf) & ~0xfUL);

	sp = argp;

	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len) {
#ifdef DEBUG_IRIX
	printf("irix_elf32_copyargs(): argc = %d, cpp = %p, dp = %p\n", (int)argc, cpp, dp);
#endif
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0 ||
		    (error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0)
			return error;
	}

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0 ||
		    (error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0)
			return error;

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	*stackp = (char *)cpp;

	/*
	 * This is borrowed from sys/kern/exec_elf32.c:elf32_copyargs
	 */
	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if ((error = copyout(ai, *stackp, len)) != 0)
		return error;
/* 	*stackp += len; */
#ifdef DEBUG_IRIX
	printf("*stackp = %p\n", *stackp);
#endif
	return 0;
}

/* 
 * Find the load address of the intepreter. Most of the code is 
 * borrowed from sys/kern/exec_elf32.c:load_file(). 
 * There is nothing specific to IRIX here, it may move to 
 * sys/compat/compat_util.c if another emulation needs it.
 */
#define MAXPHNUM	50
static int
irix_load_addr(p, itp, pos)
	struct proc *p;
	char *itp;
	vaddr_t *pos;
{
	int error = 0;
	int i;
	struct nameidata nd;
	struct vnode *vp;
	Elf_Ehdr eh;
	Elf_Phdr *ph = NULL;
	u_long phsize;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, itp, p);
	if ((error = namei(&nd)) != 0)
		return ELFDEFNNAME(NO_ADDR);
	vp = nd.ni_vp;

	/* Check this is a regular file */
	if (vp->v_type != VREG)  {
		VOP_UNLOCK(vp, 0);
		error = EACCES;
		goto bad;
	}

	VOP_UNLOCK(vp, 0);

	if ((error = exec_read_from(p, vp, 0, &eh, sizeof(eh))) != 0)
		goto bad;

	if ((error = ELFNAME(check_header)(&eh, ET_DYN)) != 0)
		goto bad;

	if (eh.e_phnum > MAXPHNUM)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	if ((error = exec_read_from(p, vp, eh.e_phoff, ph, phsize)) != 0)
		goto bad;

	/* Look for the first loadable section */
	for (i = 0; i < eh.e_phnum; i++)
		if (ph[i].p_type == PT_LOAD)
			break;
	if (i == eh.e_phnum) {
		error = ENOEXEC;
		goto bad;
	}

	*pos = ph[i].p_vaddr;
#ifdef DEBUG_IRIX
	printf("irix_load_addr: file %s, addr %p\n", itp, (void *)*pos);
#endif
bad:
	if (ph != NULL)
		free(ph, M_TEMP);
	vrele(vp);
	return error;
}
