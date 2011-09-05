/* $NetBSD: trap.c,v 1.27 2011/09/05 18:50:34 reinoud Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.27 2011/09/05 18:50:34 reinoud Exp $");

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

//static int debug_fh;
void
setup_signal_handlers(void)
{
	static struct sigaction sa;

	if ((sigstk.ss_sp = thunk_malloc(SIGSTKSZ)) == NULL)
		panic("can't allocate signal stack space\n");
	sigstk.ss_size  = SIGSTKSZ;
	sigstk.ss_flags = 0;
	if (thunk_sigaltstack(&sigstk, 0) < 0)
		panic("can't set alternate stacksize : %d",
		    thunk_geterrno());

	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = mem_access_handler;
	thunk_sigemptyset(&sa.sa_mask);
	thunk_sigaddset(&sa.sa_mask, SIGALRM);
	if (thunk_sigaction(SIGSEGV, &sa, NULL) == -1)
		panic("couldn't register SIGSEGV handler : %d",
		    thunk_geterrno());
	if (thunk_sigaction(SIGBUS, &sa, NULL) == -1)
		panic("couldn't register SIGBUS handler : %d", thunk_geterrno());

	sa.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = illegal_instruction_handler;
	thunk_sigemptyset(&sa.sa_mask);
	thunk_sigaddset(&sa.sa_mask, SIGALRM);
	if (thunk_sigaction(SIGILL, &sa, NULL) == -1)
		panic("couldn't register SIGILL handler : %d", thunk_geterrno());

//	debug_fh = thunk_open("/usr/sources/debug", O_RDWR | O_TRUNC | O_CREAT, 0666);
}

static struct trapframe kernel_tf;

static void
mem_access_handler(int sig, siginfo_t *info, void *ctx)
{
	static volatile int recurse = 0;
	static vaddr_t old_va, old_old_va = 0;

	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;
	struct vmspace *vm;
	struct vm_map *vm_map;
	struct trapframe *tf;
	vm_prot_t atype;
	vaddr_t va;
	void *onfault;
	int kmem, lwp_errno, rv;

	recurse++;
	if (recurse > 1)
		printf("enter trap recursion level %d\n", recurse);
	if ((info->si_signo == SIGSEGV) || (info->si_signo == SIGBUS)) {
		l = curlwp;
		p = l->l_proc;
		pcb = lwp_getpcb(l);
		onfault = pcb->pcb_onfault;
		vm = p->p_vmspace;

		lwp_errno = pcb->pcb_errno = thunk_geterrno();
#if 0
		va = (vaddr_t) info->si_addr;
		printf("mem trap lwp = %p pid = %d lid = %d, va = %p\n",
		    curlwp,
		    curlwp->l_proc->p_pid,
		    curlwp->l_lid,
		    (void *) va);
#endif
#if 0
		printf("SIGSEGV or SIGBUS!\n");
		printf("\tsi_signo = %d\n", info->si_signo);
		printf("\tsi_errno = %d\n", info->si_errno);
		printf("\tsi_code  = %d\n", info->si_code);
		if (info->si_code == SEGV_MAPERR)
			printf("\t\tSEGV_MAPERR\n");
		if (info->si_code == SEGV_ACCERR)
			printf("\t\tSEGV_ACCERR\n");
		if (info->si_code == BUS_ADRALN)
			printf("\t\tBUS_ADRALN\n");
		if (info->si_code == BUS_ADRERR)
			printf("\t\tBUS_ADRERR\n");
		if (info->si_code == BUS_OBJERR)
			printf("\t\tBUS_OBJERR\n");
		printf("\tsi_addr = %p\n", info->si_addr);
		printf("\tsi_trap = %d\n", info->si_trap);
#endif

		if (info->si_code == SI_NOINFO)
			panic("received signal %d with no info",
			    info->si_signo);

		va = (vaddr_t) info->si_addr;
		va = trunc_page(va);

		kmem = 1;
		vm_map = kernel_map;
		if ((va >= VM_MIN_ADDRESS) && (va < VM_MAXUSER_ADDRESS)) {
			kmem = 0;
			vm_map = &vm->vm_map;
		}

		/* can pmap handle it? on its own? (r/m) */
		rv = 0;
		if (!pmap_fault(vm_map->pmap, va, &atype)) {
			aprint_debug("pmap fault couldn't handle it! : "
				"derived atype %d\n", atype);

			/* extra debug for now */
			if (pcb == 0)
				panic("NULL pcb!\n");

			pcb->pcb_onfault = NULL;
			rv = uvm_fault(vm_map, va, atype);
			pcb->pcb_onfault = onfault;
		}

		/* sanity */
		if ((va < VM_MIN_ADDRESS) || (va >= VM_MAX_ADDRESS))
			panic("peeing outside the box! (va=%p)", (void *)va);

		/* extra debug for now */
		if (va == 0)
			panic("NULL deref\n");

#if 0
	if (old_old_va)
		thunk_pwrite(debug_fh, (void *) old_old_va, PAGE_SIZE, old_old_va);
	if (old_va)
		thunk_pwrite(debug_fh, (void *) old_va, PAGE_SIZE, old_va);
	thunk_pwrite(debug_fh, (void *) va, PAGE_SIZE, va);
#endif
	old_old_va = old_va;
	old_va = va;

		if (rv) {
			aprint_debug("uvm_fault returned error %d\n", rv);

			/* something got wrong */
			if (kmem) {
				/* copyin / copyout */
				if (!onfault)
					panic("kernel fault");
				panic("%s: can't call onfault yet\n", __func__);
				/* jump to given onfault */
				tf = &kernel_tf;
				memset(tf, 0, sizeof(struct trapframe));
				// tf->tf_pc = onfault;
				// tf->tf_io[0] = (rv == EACCES) ? EFAULT : rv;
				recurse--;
				return;
			}
			panic("should deliver a trap to the process");
			/* XXX HOWTO see arm/arm/syscall.c illegal instruction signal */
		}

		thunk_seterrno(lwp_errno);
		pcb->pcb_errno = lwp_errno;
	}
	if (recurse > 1)
		printf("leaving trap recursion level %d\n", recurse);
	recurse--;
}

static void
illegal_instruction_handler(int sig, siginfo_t *info, void *ctx)
{
	static volatile int recurse = 0;
	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;
	vaddr_t va;

	recurse++;
	if (recurse > 1)
		printf("enter trap recursion level %d\n", recurse);
	if (info->si_signo == SIGILL) {
		l = curlwp;
		p = l->l_proc;
		pcb = lwp_getpcb(l);

#if 1
		va = (vaddr_t) info->si_addr;
		printf("illegal instruction trap lwp = %p pid = %d lid = %d, va = %p\n",
		    curlwp,
		    curlwp->l_proc->p_pid,
		    curlwp->l_lid,
		    (void *) va);
#endif
#if 1
		printf("SIGILL!\n");
		printf("\tsi_signo = %d\n", info->si_signo);
		printf("\tsi_errno = %d\n", info->si_errno);
		printf("\tsi_code  = %d\n", info->si_code);
		if (info->si_code == ILL_ILLOPC)
			printf("\t\tIllegal opcode");
		if (info->si_code == ILL_ILLOPN)
			printf("\t\tIllegal operand");
		if (info->si_code == ILL_ILLADR)
			printf("\t\tIllegal addressing mode");
		if (info->si_code == ILL_ILLTRP)
			printf("\t\tIllegal trap");
		if (info->si_code == ILL_PRVOPC)
			printf("\t\tPrivileged opcode");
		if (info->si_code == ILL_PRVREG)
			printf("\t\tPrivileged register");
		if (info->si_code == ILL_COPROC)
			printf("\t\tCoprocessor error");
		if (info->si_code == ILL_BADSTK)
			printf("\t\tInternal stack error");
		printf("\tsi_addr = %p\n", info->si_addr);
		printf("\tsi_trap = %d\n", info->si_trap);
#endif

		if (recurse > 1)
			printf("leaving trap recursion level %d\n", recurse);
		recurse--;

		panic("illegal instruction encountered\n");
	}
}
