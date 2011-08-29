/* $NetBSD: trap.c,v 1.12 2011/08/29 13:15:54 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.12 2011/08/29 13:15:54 jmcneill Exp $");

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


void setup_signal_handlers(void);
static void mem_access_handler(int sig, siginfo_t *info, void *ctx);


static struct sigaction sa;
extern int errno;

void
startlwp(void *arg)
{
}

void
setup_signal_handlers(void)
{
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sa.sa_sigaction = mem_access_handler;
	if (thunk_sigaction(SIGSEGV, &sa, NULL) == -1)
		panic("couldn't register SIGSEGV handler : %d", errno);
	if (thunk_sigaction(SIGBUS, &sa, NULL) == -1)
		panic("couldn't register SIGBUS handler : %d", errno);
}

static struct trapframe kernel_tf;

static void
mem_access_handler(int sig, siginfo_t *info, void *ctx)
{
	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;
	struct vmspace *vm;
	struct vm_map *vm_map;
	struct trapframe *tf;
	vm_prot_t atype;
	vaddr_t va;
	vaddr_t onfault;
	int kmem, rv;
	static volatile int recurse = 0;

	recurse++;
	aprint_debug("trap lwp=%p pid=%d lid=%d\n",
	    curlwp,
	    curlwp->l_proc->p_pid,
	    curlwp->l_lid);
	if (recurse > 1)
		printf("enter trap recursion level %d\n", recurse);
	if ((info->si_signo == SIGSEGV) || (info->si_signo == SIGBUS)) {
		l = curlwp;
		p = l->l_proc;
		pcb = lwp_getpcb(l);
		onfault = (vaddr_t) pcb->pcb_onfault;
		vm = p->p_vmspace;

#if 0
		printf("SIGSEGV or SIGBUS!\n");
		printf("\tsi_signo = %d\n", info->si_signo);
		printf("\tsi_errno = %d\n", info->si_errno);
		printf("\tsi_code  = %d\n", info->si_code);
		if (info->si_code == SEGV_MAPERR) {
			printf("\t\tSEGV_MAPERR\n");
		}
		if (info->si_code == SEGV_ACCERR) {
			printf("\t\tSEGV_ACCERR\n");
		}
		if (info->si_code == BUS_ADRALN) {
			printf("\t\tBUS_ADRALN\n");
		}
		if (info->si_code == BUS_ADRERR) {
			printf("\t\tBUS_ADRERR\n");
		}
		if (info->si_code == BUS_OBJERR) {
			printf("\t\tBUS_OBJERR\n");
		}
		printf("\tsi_addr = %p\n", info->si_addr);
		printf("\tsi_trap = %d\n", info->si_trap);
#endif

		va = (vaddr_t) info->si_addr;
		va = trunc_page(va);

		kmem = 1;
		vm_map = kernel_map;
		if ((va >= VM_MIN_ADDRESS) && (va < VM_MAXUSER_ADDRESS)) {
			kmem = 0;
			vm_map = &vm->vm_map;
		}
		if ((va < VM_MIN_ADDRESS) || (va > VM_MAX_ADDRESS)) {
			panic("peeing outside the box!");
		}

		/* XXX TODO determine atype?? */
atype = PROT_READ;
again:
		pcb->pcb_onfault = NULL;
		rv = uvm_fault(vm_map, (vaddr_t) va, atype);
		pcb->pcb_onfault = (void *) onfault;
if (rv) printf("uvm_fault rv = %d\n", rv);
if (rv == EACCES) {
	atype |= PROT_WRITE | PROT_EXEC;
	goto again;
}
		if (rv) {
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
		}
		if (recurse > 1)
			printf("leaving trap recursion level %d\n", recurse);
		recurse--;
	}
}

