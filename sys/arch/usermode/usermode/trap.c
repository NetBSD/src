/* $NetBSD: trap.c,v 1.66.38.2 2018/06/25 07:25:46 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.66.38.2 2018/06/25 07:25:46 pgoyette Exp $");

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

/* define maximum signal number */
#ifndef NSIG
#define NSIG 64
#endif

/* forwards and externals */
void setup_signal_handlers(void);
void stop_all_signal_handlers(void);

static sigfunc_t pagefault;
static sigfunc_t illegal_instruction;
static sigfunc_t alarm;
static sigfunc_t sigio;
static sigfunc_t pass_on;

/* raw signal handlers */
static char    sig_stack[SIGSTKSZ];
static stack_t sigstk;
ucontext_t jump_ucp;

sigfunc_t *sig_funcs[NSIG];

/* segv, bus */
extern bool pmap_fault(pmap_t pmap, vaddr_t va, vm_prot_t *atype);

/* alarm */
void setup_clock_intr(void);
extern void clock_intr(void *priv);

extern int clock_running;
void *alrm_ih;

/* sigio handlers */
struct intr_handler {
	int (*func)(void *);
	void *arg;
};
#define SIGIO_MAX_HANDLERS	8
static struct intr_handler sigio_intr_handler[SIGIO_MAX_HANDLERS];

/* misc */
int astpending = 0;


/* XXX why is it here ? */
void
startlwp(void *arg)
{
	/* nothing here */
}


void
setup_signal_handlers(void)
{
	int i;

	/*
	 * Set up the alternative signal stack. This prevents signals to be
	 * pushed on the NetBSD/usermode userland's stack with all desastrous
	 * effects. Especially ld.so and friends have such tiny stacks that
	 * its not feasable.
	 */
	sigstk.ss_sp    = sig_stack;
	sigstk.ss_size  = SIGSTKSZ;
	sigstk.ss_flags = 0;
	if (thunk_sigaltstack(&sigstk, 0) < 0)
		panic("can't set alternate stacksize: %d",
		    thunk_geterrno());

	for (i = 0; i < NSIG; i++)
		sig_funcs[i] = NULL;

	/* HUP */
	/* INT */	/* ttycons ^C */
	/* QUIT */
	signal_intr_establish(SIGILL, illegal_instruction);
	/* TRAP */
	/* ABRT */
	/* SIGEMT */
	signal_intr_establish(SIGFPE, pass_on);
	/* KILL */
	signal_intr_establish(SIGBUS,  pagefault);
	signal_intr_establish(SIGSEGV, pagefault);
	/* SYS */
	/* PIPE */
	signal_intr_establish(SIGALRM, alarm);
	/* TERM */
	/* URG */
	/* STOP */
	/* TSTP */	/* ttycons ^Z */
	/* CONT */
	/* CHLD */
	/* GTTIN */
	/* TTOU */
	signal_intr_establish(SIGIO,   sigio);
	/* XCPU */
	/* XFSZ */
	/* VTALRM */
	/* PROF */
	/* WINCH */
	/* INFO */
	/* USR1 */
	/* USR2 */
	/* PWR */
}


/* XXX yes this is blunt */
void
stop_all_signal_handlers(void)
{
	int i;
	for (i = 0; i < NSIG; i++)
		if (sig_funcs[i])
			thunk_sigblock(i);
}


void
setup_clock_intr(void)
{
	/* setup soft interrupt handler */
	alrm_ih = softint_establish(SOFTINT_CLOCK,
		clock_intr, NULL);
}


/* ast and userret */
static void
ast(struct lwp *l)
{
	struct pcb *pcb;

	if (!astpending)
		return;

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
	if (curcpu()->ci_want_resched) {
		curcpu()->ci_want_resched = 0;
		preempt();
		/* returns here! */
	}
	KASSERT(l == curlwp); KASSERT(l);
	pcb = lwp_getpcb(l); KASSERT(pcb);
	mi_userret(l);
}


void
userret(struct lwp *l)
{
	/* invoke MI userret code */
	mi_userret(l);

	ast(l);
}


#ifdef DEBUG
/*
 * Uncomment the following if you want to receive information about what
 * triggered the fault. Mainly for debugging and porting purposes
 */
static void
print_mem_access_siginfo(int sig, siginfo_t *info, void *ctx,
	vaddr_t pc, vaddr_t va, vaddr_t sp)
{
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

#if 0
	thunk_printf("memaccess error, pc %p, va %p, sp %p\n",
		(void *) pc, (void *) va, (void *) sp);
#endif
}

/*
 * Uncomment the following if you want to receive information about what
 * triggered the fault. Mainly for debugging and porting purposes
 */
static void
print_illegal_instruction_siginfo(int sig, siginfo_t *info, void *ctx,
	vaddr_t pc, vaddr_t va, vaddr_t sp)
{
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

#if 0
	thunk_printf("sigill\n");
#endif
}
#else /* DEBUG */
#define print_mem_access_siginfo(s, i, c, p, v, sp) {}
#define print_illegal_instruction_siginfo(s, i, c, p, v, sp) {}
#endif /* DEBUG */


static void
handle_signal(int sig, siginfo_t *info, void *ctx)
{
	sigfunc_t *f;
	ucontext_t *ucp = ctx;
	struct lwp *l;
	struct pcb *pcb;
	vaddr_t va, sp, pc, fp;
	long from_userland;

	if (sig == SIGBUS || sig == SIGSEGV || sig == SIGILL) {
		if (info->si_code == SI_NOINFO)
			panic("received signal %d with no info",
			    info->si_signo);
	}

	f = sig_funcs[sig];
	KASSERT(f);

	l = curlwp; KASSERT(l);
	pcb = lwp_getpcb(l); KASSERT(pcb);

	/* get address of possible faulted memory access and page aligne it */
	va = (vaddr_t) info->si_addr;
	va = trunc_page(va);

	/* get PC address of possibly faulted instruction */
	pc = md_get_pc(ctx);

	/* nest it on the stack */
	sp = md_get_sp(ctx);

	if (sig == SIGBUS || sig == SIGSEGV)
		print_mem_access_siginfo(sig, info, ctx, pc, va, sp);
	if (sig == SIGILL)
		print_illegal_instruction_siginfo(sig, info, ctx, pc, va, sp);

	/* currently running on the dedicated signal stack */

	/* if we're running on a userland stack, switch to the system stack */
	from_userland = 0;
	if ((sp < (vaddr_t) pcb->sys_stack) ||
	    (sp > (vaddr_t) pcb->sys_stack_top)) {
		sp = (vaddr_t) pcb->sys_stack_top - sizeof(register_t);
		fp = (vaddr_t) &pcb->pcb_userret_ucp;
		if (pc < kmem_user_end)
			from_userland = 1;
	} else {
		/* stack grows down */
		fp = sp - sizeof(ucontext_t) - sizeof(register_t); /* slack */
		sp = fp - sizeof(register_t);	/* slack */

		/* sanity check before copying */
		if (fp - 4*PAGE_SIZE < (vaddr_t) pcb->sys_stack)
			panic("%s: out of system stack", __func__);
	}

	memcpy((void *) fp, ucp, sizeof(ucontext_t));
	memcpy(&jump_ucp, ucp, sizeof(ucontext_t));

	/* create context */
	jump_ucp.uc_stack.ss_sp = (void *) pcb->sys_stack;
	jump_ucp.uc_stack.ss_size = sp - (vaddr_t) pcb->sys_stack;
	jump_ucp.uc_link = (void *) fp;	/* link to old frame on stack */

	/* prevent multiple nested SIGIOs */
	if (sig == SIGIO)
		thunk_sigfillset(&jump_ucp.uc_sigmask);
	else
		thunk_sigemptyset(&jump_ucp.uc_sigmask);
	jump_ucp.uc_flags = _UC_STACK | _UC_CPU | _UC_SIGMASK;

	thunk_makecontext(&jump_ucp,
			(void (*)(void)) f,
		4, info, (void *) from_userland, (void *) pc, (void *) va);

	/* switch to the new context on return from signal */
	thunk_setcontext(&jump_ucp);
//	memcpy(ctx, &pcb->pcb_ucp, sizeof(ucontext_t));
}


void
signal_intr_establish(int sig, sigfunc_t f)
{
	static struct sigaction sa;

	sig_funcs[sig] = f;

	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = (void *) handle_signal;
	thunk_sigfillset(&sa.sa_mask);
	if (thunk_sigaction(sig, &sa, NULL) == -1)
		panic("couldn't register SIG%d handler: %d", sig,
		    thunk_geterrno());
}


/*
 * Context for handing page faults from the sigsegv handler; check if its a
 * pmap reference fault or let uvm handle it.
 */
static void
pagefault(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
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
	from_kernel = (pc >= kmem_k_start) && (!from_userland);
	if (from_kernel && (va >= VM_MIN_KERNEL_ADDRESS))
		vm_map = kernel_map;

#if 0
	thunk_printf("%s: l %p, pcb %p\n", __func__, l, pcb);
	thunk_printf("\tpc %p, va %p\n", (void *) pc, (void *) va);
#endif

	/* can pmap handle it? on its own? (r/m) emulation */
	if (pmap_fault(vm_map->pmap, va, &atype)) {
		/* no use doing anything else here */
		goto out_quick;
	}

	/* ask UVM */
#if 0
thunk_printf("%s: l %p, pcb %p, ", __func__, l, pcb);
thunk_printf("pc %p, va %p ", (void *) pc, (void *) va);
thunk_printf("derived atype %d\n", atype);
#endif
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

	if (from_kernel) {
		thunk_printf("%s: uvm fault %d, pc %p, va %p, from_kernel %d\n",
			__func__, error, (void *) pc, (void *) va, from_kernel);
		panic("Unhandled page fault in kernel mode");
	}

	/* send signal */
	/* something got wrong */
	thunk_printf_debug("%s: uvm fault %d, pc %p, va %p, from_kernel %d\n",
		__func__, error, (void *) pc, (void *) va, from_kernel);

	thunk_printf_debug("giving signal to userland\n");

	KASSERT(from_userland);
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = info->si_signo;
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
	if (from_userland)
		userret(l);
out_quick:
	thunk_seterrno(lwp_errno);
	pcb->pcb_errno = lwp_errno;
}


/*
 * handle an illegal instruction.
 *
 * arguments 'pc' and 'va' are ignored here
 */
static void
illegal_instruction(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
{
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;
	ksiginfo_t ksi;

//	thunk_printf("%s: l %p, pcb %p\n", __func__, l, pcb);

	KASSERT(from_userland);

	/* if its a syscall ... */
	if (md_syscall_check_opcode(ucp)) {
		syscall();
		userret(l);
		return;
	}

	thunk_printf("%s: giving SIGILL (TRAP)\n", __func__);

	KASSERT(from_userland);
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


/*
 * handle pass to userland signals
 *
 * arguments other than the origional siginfo_t are not used
 */
static void
pass_on(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
{
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;
	ksiginfo_t ksi;

	KASSERT(from_userland);
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = info->si_signo;
	ksi.ksi_trap  = 0;	/* XXX ? */
	ksi.ksi_errno = info->si_errno;
	ksi.ksi_code  = info->si_code;
	ksi.ksi_addr  = (void *) md_get_pc(ucp); /* only relyable source */

	trapsignal(l, &ksi);
	userret(l);
}


/*
 * handle alarm, a clock ticker.
 *
 * arguments 'pc' and 'va' are ignored here
 */
static void
alarm(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
{
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l); KASSERT(pcb);

	if (!clock_running)
		return;
//	thunk_printf("%s: l %p, pcb %p\n", __func__, l, pcb);

	softint_schedule(alrm_ih);

	KASSERT(l == curlwp);
	if (from_userland)
		userret(l);
}


/*
 * handle sigio, a mux for all io operations.
 *
 * arguments 'pc' and 'va' are ignored here
 */
static void
sigio(siginfo_t *info, vaddr_t from_userland, vaddr_t pc, vaddr_t va)
{
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l); KASSERT(pcb);
	struct intr_handler *sih;
	unsigned int n, pass;

//	thunk_printf("%s: l %p, pcb %p\n", __func__, l, pcb);
	for (pass = 0; pass < 2; pass++) {
		for (n = 0; n < SIGIO_MAX_HANDLERS; n++) {
			sih = &sigio_intr_handler[n];
			if (sih->func)
				sih->func(sih->arg);
		}
	}

	KASSERT(l == curlwp);
	if (from_userland)
		userret(l);		/* or ast? */
}


/* sigio register function */
void *
sigio_intr_establish(int (*func)(void *), void *arg)
{
	struct intr_handler *sih;
	unsigned int n;

	for (n = 0; n < SIGIO_MAX_HANDLERS; n++) {
		sih = &sigio_intr_handler[n];
		if (sih->func == NULL) {
			sih->func = func;
			sih->arg = arg;
			return sih;
		}
	}

	panic("increase SIGIO_MAX_HANDLERS");
}

