/* $NetBSD: trap.c,v 1.53 2012/01/06 20:39:42 reinoud Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@netbsd.org>
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.53 2012/01/06 20:39:42 reinoud Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/userret.h>
#include <sys/errno.h>

#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/machdep.h>
#include <machine/intr.h>
#include <machine/thunk.h>


//#include <machine/ctlreg.h>
//#include <machine/trap.h>
//#include <machine/instr.h>
//#include <machine/userret.h>

/* forwards and externals */
void setup_signal_handlers(void);
static void mem_access_handler(int sig, siginfo_t *info, void *ctx);
static void illegal_instruction_handler(int sig, siginfo_t *info, void *ctx);
extern int errno;

bool pmap_fault(pmap_t pmap, vaddr_t va, vm_prot_t *atype);

static stack_t sigstk;

void
startlwp(void *arg)
{
}

void
setup_signal_handlers(void)
{
	static struct sigaction sa;

	if ((sigstk.ss_sp = thunk_malloc(SIGSTKSZ)) == NULL)
		panic("can't allocate signal stack space\n");
	sigstk.ss_size  = SIGSTKSZ;
	sigstk.ss_flags = 0;
	if (thunk_sigaltstack(&sigstk, 0) < 0)
		panic("can't set alternate stacksize: %d",
		    thunk_geterrno());

	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = mem_access_handler;
	thunk_sigemptyset(&sa.sa_mask);
	if (thunk_sigaction(SIGSEGV, &sa, NULL) == -1)
		panic("couldn't register SIGSEGV handler: %d",
		    thunk_geterrno());
	if (thunk_sigaction(SIGBUS, &sa, NULL) == -1)
		panic("couldn't register SIGBUS handler: %d", thunk_geterrno());

	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = illegal_instruction_handler;
	thunk_sigemptyset(&sa.sa_mask);
	if (thunk_sigaction(SIGILL, &sa, NULL) == -1)
		panic("couldn't register SIGILL handler: %d", thunk_geterrno());

	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = sigio_signal_handler;
	thunk_sigemptyset(&sa.sa_mask);
	if (thunk_sigaction(SIGIO, &sa, NULL) == -1)
		panic("couldn't register SIGIO handler: %d", thunk_geterrno());
}


static void
mem_access_handler(int sig, siginfo_t *info, void *ctx)
{
	ucontext_t *uct = ctx;
	struct lwp *l;
	struct pcb *pcb;
	vaddr_t va, pc;

	assert((info->si_signo == SIGSEGV) || (info->si_signo == SIGBUS));

	if (info->si_code == SI_NOINFO)
		panic("received signal %d with no info",
		    info->si_signo);

//printf("sigsegv\n");
#if 0
	va = (vaddr_t) info->si_addr;
	thunk_printf_debug("mem trap lwp = %p pid = %d lid = %d, va = %p\n",
	    curlwp,
	    curlwp->l_proc->p_pid,
	    curlwp->l_lid,
	    (void *) va);
#endif
#if 0
	thunk_printf_debug("SIGSEGV or SIGBUS!\n");
	thunk_printf_debug("\tsi_signo = %d\n", info->si_signo);
	thunk_printf_debug("\tsi_errno = %d\n", info->si_errno);
	thunk_printf_debug("\tsi_code  = %d\n", info->si_code);
	if (info->si_code == SEGV_MAPERR)
		thunk_printf_debug("\t\tSEGV_MAPERR\n");
	if (info->si_code == SEGV_ACCERR)
		thunk_printf_debug("\t\tSEGV_ACCERR\n");
	if (info->si_code == BUS_ADRALN)
		thunk_printf_debug("\t\tBUS_ADRALN\n");
	if (info->si_code == BUS_ADRERR)
		thunk_printf_debug("\t\tBUS_ADRERR\n");
	if (info->si_code == BUS_OBJERR)
		thunk_printf_debug("\t\tBUS_OBJERR\n");
	thunk_printf_debug("\tsi_addr = %p\n", info->si_addr);
	thunk_printf_debug("\tsi_trap = %d\n", info->si_trap);
#endif

	l = curlwp;
	pcb = lwp_getpcb(l);

	/* get address of faulted memory access and make it page aligned */
	va = (vaddr_t) info->si_addr;
	va = trunc_page(va);

	/* get PC address of faulted memory instruction */
	pc = md_get_pc(ctx);

#if 0	/* disabled for now, these checks need to move */
#ifdef DIAGNOSTIC
	/* sanity */
	if ((va < VM_MIN_ADDRESS) || (va >= VM_MAX_KERNEL_ADDRESS))
		panic("peeing outside the box! (va=%p)", (void *)va);

	/* extra debug for now -> should issue signal */
	if (va == 0)
		panic("NULL deref\n");
#endif
#endif

//printf("memaccess error : va = %p\n", (void *) va);
	/* copy this state to return to */
	memcpy(&pcb->pcb_trapret_ucp, uct, sizeof(ucontext_t));

	/* remember our parameters */
//	assert((void *) pcb->pcb_fault_addr == NULL);
	pcb->pcb_fault_addr = va;
	pcb->pcb_fault_pc   = pc;

	/* switch to the pagefault entry on return from signal */
	memcpy(uct, &pcb->pcb_pagefault_ucp, sizeof(ucontext_t));
}


static void
illegal_instruction_handler(int sig, siginfo_t *info, void *ctx)
{
	ucontext_t *uct = ctx;
	struct lwp *l;
	struct pcb *pcb;

	assert(info->si_signo == SIGILL);
#if 0
	sigset_t ss;
	thunk_sigemptyset(&ss);
	thunk_sigaddset(&ss, SIGALRM);
	thunk_sigaddset(&ss, SIGILL);
	thunk_sigprocmask(SIG_UNBLOCK, &ss, NULL);
#endif

#if 0
	printf("\nillegal instruction trap lwp = %p pid = %d lid = %d, va = %p\n",
	    curlwp,
	    curlwp->l_proc->p_pid,
	    curlwp->l_lid,
	    (void *) info->si_addr);
#endif
#if 0
	thunk_printf("SIGILL!\n");
	thunk_printf("\tsi_signo = %d\n", info->si_signo);
	thunk_printf("\tsi_errno = %d\n", info->si_errno);
	thunk_printf("\tsi_code  = %d\n", info->si_code);
	if (info->si_code == ILL_ILLOPC)
		thunk_printf("\t\tIllegal opcode");
	if (info->si_code == ILL_ILLOPN)
		thunk_printf("\t\tIllegal operand");
	if (info->si_code == ILL_ILLADR)
		thunk_printf("\t\tIllegal addressing mode");
	if (info->si_code == ILL_ILLTRP)
		thunk_printf("\t\tIllegal trap");
	if (info->si_code == ILL_PRVOPC)
		thunk_printf("\t\tPrivileged opcode");
	if (info->si_code == ILL_PRVREG)
		thunk_printf("\t\tPrivileged register");
	if (info->si_code == ILL_COPROC)
		thunk_printf("\t\tCoprocessor error");
	if (info->si_code == ILL_BADSTK)
		thunk_printf("\t\tInternal stack error");
	thunk_printf("\tsi_addr = %p\n", info->si_addr);
	thunk_printf("\tsi_trap = %d\n", info->si_trap);

	thunk_printf("%p : ", info->si_addr);
	for (int i = 0; i < 10; i++)
		thunk_printf("%02x ", *((uint8_t *) info->si_addr + i));
	thunk_printf("\n");
#endif

	l = curlwp;
	pcb = lwp_getpcb(l);

	/* copy this state to return to */
	memcpy(&pcb->pcb_userret_ucp, uct, sizeof(ucontext_t));

	/* if its a syscall ... */
	if (md_syscall_check_opcode(uct)) {
		/* switch to the syscall entry on return from signal */
		memcpy(uct, &pcb->pcb_syscall_ucp, sizeof(ucontext_t));
		return;
	}

	panic("should deliver a trap to the process : illegal instruction "
		"encountered\n");
}


/*
 * Entry point from the segv handler; check if its a pmap reference fault or
 * let uvm handle it.
 */
void
pagefault(void)
{
	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;
	struct vmspace *vm;
	struct vm_map *vm_map;
	vm_prot_t atype;
	vaddr_t va, pc;
	void *onfault;
	int from_kernel, lwp_errno, rv;

	l = curlwp;
	pcb = lwp_getpcb(l);
	p = l->l_proc;
	vm = p->p_vmspace;
	va = pcb->pcb_fault_addr;
	pc = pcb->pcb_fault_pc;

	lwp_errno = thunk_geterrno();

	vm_map = &vm->vm_map;
	from_kernel = (pc >= kmem_k_start);
	if (from_kernel && (va >= VM_MIN_KERNEL_ADDRESS))
		vm_map = kernel_map;

	thunk_printf_debug("pagefault : pc %p, va %p\n", (void *) pc, (void *) va);

	/* can pmap handle it? on its own? (r/m) */
	onfault = pcb->pcb_onfault;
	rv = 0;
	if (!pmap_fault(vm_map->pmap, va, &atype)) {
		thunk_printf_debug("pmap fault couldn't handle it! : "
			"derived atype %d\n", atype);

		/* extra debug for now */
		if (pcb == 0)
			panic("NULL pcb!\n");

		pcb->pcb_onfault = NULL;
		rv = uvm_fault(vm_map, va, atype);
		pcb->pcb_onfault = onfault;
	}

	if (rv) {
		thunk_printf_debug("uvm_fault returned error %d\n", rv);

		/* something got wrong */
		if (from_kernel) {
			/* copyin / copyout */
			if (!onfault)
				panic("kernel fault");
			panic("%s: can't call onfault yet\n", __func__);
			/* jump to given onfault */
			// tf = &kernel_tf;
			// memset(tf, 0, sizeof(struct trapframe));
			// tf->tf_pc = onfault;
			// tf->tf_io[0] = (rv == EACCES) ? EFAULT : rv;
			return;
		}
		panic("%s: should deliver a trap to the process for va %p", __func__, (void *) va);
		/* XXX HOWTO see arm/arm/syscall.c illegal instruction signal */
	}

	thunk_seterrno(lwp_errno);
	pcb->pcb_errno = lwp_errno;
}

stack_t *
usermode_signal_stack(void)
{
	return &sigstk;
}
