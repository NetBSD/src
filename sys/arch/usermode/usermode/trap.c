/* $NetBSD: trap.c,v 1.57 2012/01/14 21:45:28 reinoud Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.57 2012/01/14 21:45:28 reinoud Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/userret.h>
#include <sys/kauth.h>
#include <sys/errno.h>

#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/machdep.h>
#include <machine/intr.h>
#include <machine/thunk.h>


/* forwards and externals */
void setup_signal_handlers(void);
void stop_all_signal_handlers(void);
void userret(struct lwp *l);

static void mem_access_handler(int sig, siginfo_t *info, void *ctx);
static void illegal_instruction_handler(int sig, siginfo_t *info, void *ctx);
extern int errno;

static void pagefault(vaddr_t pc, vaddr_t va);
static void illegal_instruction(void);

bool pmap_fault(pmap_t pmap, vaddr_t va, vm_prot_t *atype);

static stack_t sigstk;

int astpending;

void
startlwp(void *arg)
{
	/* nothing here */
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

	/* SIGBUS and SIGSEGV need to be reentrant hence the SA_NODEFER */
	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
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


void
stop_all_signal_handlers(void)
{
	thunk_sigblock(SIGALRM);
	thunk_sigblock(SIGIO);
	thunk_sigblock(SIGILL);
	thunk_sigblock(SIGSEGV);
	thunk_sigblock(SIGBUS);
}


/* signal handler switching to a pagefault context */
static void
mem_access_handler(int sig, siginfo_t *info, void *ctx)
{
	ucontext_t *ucp = ctx;
	struct lwp *l;
	struct pcb *pcb;
	vaddr_t va, sp, pc, fp;

	assert((info->si_signo == SIGSEGV) || (info->si_signo == SIGBUS));

	if (info->si_code == SI_NOINFO)
		panic("received signal %d with no info",
		    info->si_signo);

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

	/* setup for pagefault context */
	sp = md_get_sp(ctx);

#if 0
	printf("memaccess error, pc %p, va %p, "
		"sys_stack %p, sp %p, stack top %p\n",
		(void *) pc, (void *) va,
		(void *) pcb->sys_stack, (void *) sp,
		(void *) pcb->sys_stack_top);
#endif

	/* if we're running on a stack of our own, use the system stack */
	if ((sp < (vaddr_t) pcb->sys_stack) || (sp > (vaddr_t) pcb->sys_stack_top)) {
		sp = (vaddr_t) pcb->sys_stack_top - sizeof(register_t);
		fp = (vaddr_t) &pcb->pcb_userret_ucp;
	} else {
		/* stack grows down */
		fp = sp - sizeof(ucontext_t) - sizeof(register_t); /* slack */
		sp = fp - sizeof(register_t);	/* slack */

		/* sanity check before copying */
		if (fp - 2*PAGE_SIZE < (vaddr_t) pcb->sys_stack)
			panic("%s: out of system stack", __func__);
	}

	memcpy((void *) fp, ucp, sizeof(ucontext_t));

	/* create context for pagefault */
	pcb->pcb_ucp.uc_stack.ss_sp = (void *) pcb->sys_stack;
	pcb->pcb_ucp.uc_stack.ss_size = sp - (vaddr_t) pcb->sys_stack;
	pcb->pcb_ucp.uc_link = (void *) fp;	/* link to old frame on stack */

	pcb->pcb_ucp.uc_flags = _UC_STACK | _UC_CPU;
	thunk_makecontext(&pcb->pcb_ucp, (void (*)(void)) pagefault,
		2, (void *) pc, (void *) va, NULL);

	/* switch to the new pagefault entry on return from signal */
	memcpy(ctx, &pcb->pcb_ucp, sizeof(ucontext_t));
}


/* ast and userret */
void
userret(struct lwp *l)
{
	struct pcb *pcb;
	ucontext_t ucp, *nucp;
	vaddr_t pc;
	
	KASSERT(l);

	/* are we going back to userland? */
	pcb = lwp_getpcb(l);
	KASSERT(pcb);

	/* where are we going back to ? */
	thunk_getcontext(&ucp);
	nucp = (ucontext_t *) ucp.uc_link;
	pc = md_get_pc(nucp);

	if (pc >= kmem_k_start)
		return;

	/* ok, going to userland, proceed! */
	if (astpending) {
		astpending = 0;

		curcpu()->ci_data.cpu_ntrap++;
#if 0
		/* profiling */
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}
#endif
		/* allow a forced task switch */
		if (l->l_cpu->ci_want_resched)
			preempt();
	}

	/* invoke MI userret code */
	mi_userret(l);
}

/* signal handler switching to a illegal instruction context */
static void
illegal_instruction_handler(int sig, siginfo_t *info, void *ctx)
{
	ucontext_t *ucp = ctx;
	struct lwp *l;
	struct pcb *pcb;
	vaddr_t sp, fp;

	assert(info->si_signo == SIGILL);
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

	/* setup for illegal_instruction context */
	sp = md_get_sp(ctx);

	/* if we're running on a stack of our own, use the system stack */
	if ((sp < (vaddr_t) pcb->sys_stack) ||
			(sp > (vaddr_t) pcb->sys_stack_top)) {
		sp = (vaddr_t) pcb->sys_stack_top - sizeof(register_t);
		fp = (vaddr_t) &pcb->pcb_userret_ucp;
	} else {
		panic("illegal instruction inside kernel?");
#if 0
		/* stack grows down */
		fp = sp - sizeof(ucontext_t) - sizeof(register_t); /* slack */
		sp = fp - sizeof(register_t);	/* slack */

		/* sanity check before copying */
		if (fp - 2*PAGE_SIZE < (vaddr_t) pcb->sys_stack)
			panic("%s: out of system stack", __func__);
#endif
	}

	memcpy((void *) fp, ucp, sizeof(ucontext_t));

	/* create context for illegal instruction */
	pcb->pcb_ucp.uc_stack.ss_sp = (void *) pcb->sys_stack;
	pcb->pcb_ucp.uc_stack.ss_size = sp - (vaddr_t) pcb->sys_stack;
	pcb->pcb_ucp.uc_link = (void *) fp;	/* link to old frame on stack */

	pcb->pcb_ucp.uc_flags = _UC_STACK | _UC_CPU;
	thunk_makecontext(&pcb->pcb_ucp, (void (*)(void)) illegal_instruction,
		0, NULL, NULL, NULL);

	/* switch to the new illegal instruction entry on return from signal */
	memcpy(ctx, &pcb->pcb_ucp, sizeof(ucontext_t));
}


/*
 * Context for handing page faults from the sigsegv handler; check if its a
 * pmap reference fault or let uvm handle it.
 */
static void
pagefault(vaddr_t pc, vaddr_t va)
{
	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;
	struct vmspace *vm;
	struct vm_map *vm_map;
	vm_prot_t atype;
	void *onfault;
	int from_kernel, lwp_errno, error;
	ksiginfo_t ksi;

	l = curlwp; KASSERT(l);
	pcb = lwp_getpcb(l);
	p = l->l_proc;
	vm = p->p_vmspace;

	lwp_errno = thunk_geterrno();

	vm_map = &vm->vm_map;
	from_kernel = (pc >= kmem_k_start);
	if (from_kernel && (va >= VM_MIN_KERNEL_ADDRESS))
		vm_map = kernel_map;

#if 0
	thunk_printf("pagefault : pc %p, va %p\n",
		(void *) pc, (void *) va);
#endif

	/* can pmap handle it? on its own? (r/m) emulation */
	if (pmap_fault(vm_map->pmap, va, &atype)) {
//		thunk_printf("pagefault leave (pmap)\n");
		/* no use doing anything else here */
		goto out_quick;
	}

	/* ask UVM */
	thunk_printf_debug("pmap fault couldn't handle it! : "
		"derived atype %d\n", atype);

	onfault = pcb->pcb_onfault;
	pcb->pcb_onfault = NULL;
	error = uvm_fault(vm_map, va, atype);
	pcb->pcb_onfault = onfault;

	if (vm_map != kernel_map) {
		if (error == 0)
			uvm_grow(l->l_proc, va);
	}
	if (error == EACCES)
		error = EFAULT;

	/* if uvm handled it, return */
	if (error == 0) {
//		thunk_printf("pagefault leave (uvm)\n");
		goto out;
	}

	/* something got wrong */
	thunk_printf("%s: uvm fault %d, pc %p, from_kernel %d\n",
		__func__, error, (void *) pc, from_kernel);

	/* check if its from copyin/copyout */
	if (onfault) {
		panic("%s: can't call onfault yet\n", __func__);
		/* XXX implement me ? */
		/* jump to given onfault */
		// tf = &kernel_tf;
		// memset(tf, 0, sizeof(struct trapframe));
		// tf->tf_pc = onfault;
		// tf->tf_io[0] = (rv == EACCES) ? EFAULT : rv;
		goto out;
	}

	if (from_kernel)
		panic("Unhandled page fault in kernel mode");

	/* send signal */
	thunk_printf("giving signal to userland\n");

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGSEGV;
	ksi.ksi_trap = 0;	/* XXX */
	ksi.ksi_code = (error == EPERM) ? SEGV_ACCERR : SEGV_MAPERR;
	ksi.ksi_addr = (void *) va;

	if (error == ENOMEM) {
		printf("UVM: pid %d.%d (%s), uid %d killed: "
		    "out of swap\n",
		    p->p_pid, l->l_lid, p->p_comm,
		    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1);
		ksi.ksi_signo = SIGKILL;
	}

#if 0
	p->p_emul->e_trapsignal(l, &ksi);
#else
	trapsignal(l, &ksi);
#endif

//	thunk_printf("pagefault leave\n");
out:
	userret(l);
out_quick:
	thunk_seterrno(lwp_errno);
	pcb->pcb_errno = lwp_errno;
}

/*
 * Context for handing illegal instruction from the sigill handler
 */
static void
illegal_instruction(void)
{
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;
	ksiginfo_t ksi;

//	thunk_printf("illegal instruction\n");
	/* if its a syscall ... */
	if (md_syscall_check_opcode(ucp)) {
		syscall();
//		thunk_printf("illegal instruction leave\n");
		userret(l);
		return;
	}

	thunk_printf("%s: giving SIGILL (TRAP)\n", __func__);

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGILL;
	ksi.ksi_trap  = 0;	/* XXX */
	ksi.ksi_errno = 0; // info->si_errno;
	ksi.ksi_code  = 0; // info->si_code;
	ksi.ksi_addr  = (void *) md_get_pc(ucp); /* only relyable source */

#if 0
	p->p_emul->e_trapsignal(l, &ksi);
#else
	trapsignal(l, &ksi);
#endif
	userret(l);
}

