/*	$NetBSD: irix_exec.c,v 1.10.4.4 2002/06/23 17:43:52 jdolecek Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: irix_exec.c,v 1.10.4.4 2002/06/23 17:43:52 jdolecek Exp $");

#ifndef ELFSIZE
#define ELFSIZE		32	/* XXX should die */
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/regnum.h>

#include <compat/common/compat_util.h>

#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_types.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_prctl.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_errno.h>

extern const int native_to_svr4_signo[];

static void setregs_n32 __P((struct proc *, struct exec_package *, u_long));
static void irix_e_proc_exec __P((struct proc *, struct exec_package *));
static void irix_e_proc_fork __P((struct proc *, struct proc *));
static void irix_e_proc_exit __P((struct proc *));
static void irix_e_proc_init __P((struct proc *, struct vmspace *));

extern struct sysent irix_sysent[];
extern const char * const irix_syscallnames[];

#ifndef __HAVE_SYSCALL_INTERN
void irix_syscall __P((void));
#else
void irix_syscall_intern __P((struct proc *));
#endif

const struct emul emul_irix_o32 = {
	"irix o32",
	"/emul/irix",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	native_to_irix_errno,
	IRIX_SYS_syscall,
	IRIX_SYS_MAXSYSCALL,
#endif
	irix_sysent,
#ifdef SYSCALL_DEBUG
	irix_syscallnames,
#else
	NULL,
#endif
	irix_sendsig,
	trapsignal,
	NULL,
	NULL,
	setregs,
	irix_e_proc_exec,
	irix_e_proc_fork,
	irix_e_proc_exit,
#ifdef __HAVE_SYSCALL_INTERN
	irix_syscall_intern,
#else
	irix_syscall,
#endif
};

const struct emul emul_irix_n32 = {
	"irix n32",
	"/emul/irix",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	native_to_irix_errno,
	IRIX_SYS_syscall,
	IRIX_SYS_MAXSYSCALL,
#endif
	irix_sysent,
#ifdef SYSCALL_DEBUG
	irix_syscallnames,
#else
	NULL,
#endif
	irix_sendsig,
	trapsignal,
	NULL,
	NULL,
	setregs_n32,
	irix_e_proc_exec,
	irix_e_proc_fork,
	irix_e_proc_exit,
#ifdef __HAVE_SYSCALL_INTERN
	irix_syscall_intern,
#else
	irix_syscall,
#endif
};

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
	const char *bp;
	int error;
	size_t len;

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
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path,
		    itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	*pos = ELF_NO_ADDR;
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
	const char *bp;
	int error;
	size_t len;

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
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path,
		    itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	*pos = ELF_NO_ADDR;
#ifdef DEBUG_IRIX
	printf("irix_probe_n32: returning 0\n");
	printf("epp->ep_vm_minaddr = 0x%lx\n", epp->ep_vm_minaddr);
#endif
	epp->ep_vm_minaddr = epp->ep_vm_minaddr & ~0xfUL;
	return 0;
}

/*
 * set registers on exec for N32 applications 
 */
void
setregs_n32(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct frame *f = (struct frame *)p->p_md.md_regs;
	
	/* Use regular setregs */
	setregs(p, pack, stack);

	/* Enable 64 bit instructions (eg: sd) */
	f->f_regs[SR] |= MIPS3_SR_UX; 

	return;
}

/*
 * per-process emuldata allocation
 */
static void
irix_e_proc_init(p, vmspace)
	struct proc *p;
	struct vmspace *vmspace;
{
	if (!p->p_emuldata)
		MALLOC(p->p_emuldata, void *, sizeof(struct irix_emuldata),
			M_EMULDATA, M_WAITOK | M_ZERO);
}  

/* 
 * exec() hook used to allocate per process structures
 */
static void
irix_e_proc_exec(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error;

	irix_e_proc_init(p, p->p_vmspace);

	/* Initialize the process private area (PRDA) */
	error = irix_prda_init(p);
#ifdef DEBUG_IRIX
	if (error != 0)
		printf("irix_e_proc_init(): PRDA map failed ");
#endif
}

/*
 * exit() hook used to free per process data structures
 */
static void
irix_e_proc_exit(p)
	struct proc *p;
{
	struct proc *pp;
	struct irix_emuldata *ied;

	LIST_FOREACH(pp, &allproc, p_list) {
		/* Select IRIX processes */
		if (irix_check_exec(pp) == 0)
			continue;

		ied = (struct irix_emuldata *)(pp->p_emuldata);
		if (ied->ied_pptr == p)
			psignal(pp, native_to_svr4_signo[SIGHUP]);
	}

	FREE(p->p_emuldata, M_EMULDATA);
	p->p_emuldata = NULL;
}

/*
 * fork() hook used to allocate per process structures
 */
static void
irix_e_proc_fork(p, parent)
        struct proc *p, *parent;
{
	struct irix_emuldata *ied1;
	struct irix_emuldata *ied2;

        p->p_emuldata = NULL;

	/* Use parent's vmspace because our vmspace may not be setup yet */
        irix_e_proc_init(p, parent->p_vmspace);

	ied1 = p->p_emuldata;
	ied2 = parent->p_emuldata;

	(void) memcpy(ied1, ied2, (unsigned)
	    ((caddr_t)&ied1->ied_endcopy - (caddr_t)&ied1->ied_startcopy));
}

/*
 * Return true if the given process is an IRIX process 
 */
int
irix_check_exec(p)
	struct proc *p;
{
	if (p->p_emul == &emul_irix_n32 ||
	    p->p_emul == &emul_irix_o32)
		return 1;
	return 0;
}
