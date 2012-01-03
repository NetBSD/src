/* $NetBSD: cpu.c,v 1.58 2012/01/03 12:05:01 reinoud Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_cpu.h"
#include "opt_hz.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.58 2012/01/03 12:05:01 reinoud Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/kmem.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/mainbus.h>
#include <machine/pcb.h>
#include <machine/machdep.h>
#include <machine/thunk.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#if __GNUC_PREREQ__(4,4)
#define cpu_unreachable()	__builtin_unreachable()
#else
#define cpu_unreachable()	do { thunk_abort(); } while (0)
#endif

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

struct cpu_info cpu_info_primary = {
	.ci_dev = 0,
	.ci_self = &cpu_info_primary,
	.ci_idepth = -1,
	.ci_curlwp = &lwp0,
};

char cpu_model[48] = "virtual processor";

typedef struct cpu_softc {
	device_t	sc_dev;
	struct cpu_info	*sc_ci;
} cpu_softc_t;

static struct pcb lwp0pcb;
static void *um_msgbuf;

CFATTACH_DECL_NEW(cpu, sizeof(cpu_softc_t), cpu_match, cpu_attach, NULL, NULL);

static int
cpu_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_CPU)
		return 0;

	return 1;
}

static void
cpu_attach(device_t parent, device_t self, void *opaque)
{
	cpu_softc_t *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_ci = &cpu_info_primary;
}

void
cpu_configure(void)
{
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	spl0();
}

void
cpu_reboot(int howto, char *bootstr)
{
	extern void usermode_reboot(void);

	splhigh();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN)
		thunk_exit(0);

	if (howto & RB_DUMP)
		thunk_abort();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n");

	usermode_reboot();

	/* NOTREACHED */
	cpu_unreachable();
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	ci->ci_want_resched |= flags;
}

void
cpu_need_proftick(struct lwp *l)
{
}

lwp_t *
cpu_switchto(lwp_t *oldlwp, lwp_t *newlwp, bool returning)
{
	struct pcb *oldpcb = oldlwp ? lwp_getpcb(oldlwp) : NULL;
	struct pcb *newpcb = lwp_getpcb(newlwp);
	struct cpu_info *ci = curcpu();

#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_switchto [%s,pid=%d,lid=%d] -> [%s,pid=%d,lid=%d]\n",
	    oldlwp ? oldlwp->l_name : "none",
	    oldlwp ? oldlwp->l_proc->p_pid : -1,
	    oldlwp ? oldlwp->l_lid : -1,
	    newlwp ? newlwp->l_name : "none",
	    newlwp ? newlwp->l_proc->p_pid : -1,
	    newlwp ? newlwp->l_lid : -1);
	if (oldpcb) {
		thunk_printf_debug("    oldpcb uc_link=%p, uc_stack.ss_sp=%p, "
		    "uc_stack.ss_size=%d\n",
		    oldpcb->pcb_ucp.uc_link,
		    oldpcb->pcb_ucp.uc_stack.ss_sp,
		    (int)oldpcb->pcb_ucp.uc_stack.ss_size);
	}
	if (newpcb) {
		thunk_printf_debug("    newpcb uc_link=%p, uc_stack.ss_sp=%p, "
		    "uc_stack.ss_size=%d\n",
		    newpcb->pcb_ucp.uc_link,
		    newpcb->pcb_ucp.uc_stack.ss_sp,
		    (int)newpcb->pcb_ucp.uc_stack.ss_size);
	}
#endif /* !CPU_DEBUG */

	ci->ci_stash = oldlwp;

	if (oldpcb) {
		oldpcb->pcb_errno = thunk_geterrno();
		thunk_seterrno(newpcb->pcb_errno);
		curlwp = newlwp;
		if (thunk_swapcontext(&oldpcb->pcb_ucp, &newpcb->pcb_ucp))
			panic("swapcontext failed");
	} else {
		thunk_seterrno(newpcb->pcb_errno);
		curlwp = newlwp;
		if (thunk_setcontext(&newpcb->pcb_ucp))
			panic("setcontext failed");
	}

#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_switchto: returning %p (was %p)\n", ci->ci_stash, oldlwp);
#endif
	return ci->ci_stash;
}

void
cpu_dumpconf(void)
{
#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_dumpconf\n");
#endif
}

void
cpu_signotify(struct lwp *l)
{
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;

#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_getmcontext\n");
#endif
	memcpy(mcp, &ucp->uc_mcontext, sizeof(mcontext_t));
	return;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;

#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_setmcontext\n");
#endif
	memcpy(&ucp->uc_mcontext, mcp, sizeof(mcontext_t));
	return 0;
}

void
cpu_idle(void)
{
	struct cpu_info *ci = curcpu();

	if (ci->ci_want_resched)
		return;

	thunk_idle();
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_lwp_free (dummy)\n");
#endif
}

void
cpu_lwp_free2(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_lwp_free2\n");
#endif

	if (pcb == NULL)
		return;

	if (pcb->pcb_needfree) {
#if 0
		free(pcb->pcb_ucp.uc_stack.ss_sp, M_TEMP);
		pcb->pcb_ucp.uc_stack.ss_sp = NULL;
		pcb->pcb_ucp.uc_stack.ss_size = 0;
#endif

		free(pcb->pcb_syscall_ucp.uc_stack.ss_sp, M_TEMP);
		pcb->pcb_syscall_ucp.uc_stack.ss_sp = NULL;
		pcb->pcb_syscall_ucp.uc_stack.ss_size = 0;

		free(pcb->pcb_pagefault_ucp.uc_stack.ss_sp, M_TEMP);
		pcb->pcb_pagefault_ucp.uc_stack.ss_sp = NULL;
		pcb->pcb_pagefault_ucp.uc_stack.ss_size = 0;

		pcb->pcb_needfree = false;
	}
}

static void
cpu_lwp_trampoline(ucontext_t *ucp, void (*func)(void *), void *arg)
{
#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_lwp_trampoline called with func %p, arg %p\n", (void *) func, arg);
#endif
	/* init lwp */
	lwp_startup(curcpu()->ci_stash, curlwp);

	/* actual jump */
	thunk_makecontext(ucp, (void (*)(void)) func, 1, arg, NULL, NULL);
	thunk_setcontext(ucp);
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb1 = lwp_getpcb(l1);
	struct pcb *pcb2 = lwp_getpcb(l2);
	void *stack_ucp, *stack_syscall_ucp, *stack_pagefault_ucp;

#ifdef CPU_DEBUG
	thunk_printf_debug("cpu_lwp_fork [%s/%p] -> [%s/%p] stack=%p stacksize=%d\n",
	    l1 ? l1->l_name : "none", l1,
	    l2 ? l2->l_name : "none", l2,
	    stack, (int)stacksize);
#endif

	if (stack)
		panic("%s: stack passed, can't handle\n", __func__);

	/* copy the PCB and its switchframes from parent */
	memcpy(pcb2, pcb1, sizeof(struct pcb));

	stacksize = 2*PAGE_SIZE;
	stack_ucp           = malloc(stacksize, M_TEMP, M_NOWAIT);
	stack_syscall_ucp   = malloc(stacksize, M_TEMP, M_NOWAIT);
	stack_pagefault_ucp = malloc(stacksize, M_TEMP, M_NOWAIT);
	pcb2->pcb_needfree = true;

	KASSERT(stack_ucp);
	KASSERT(stack_syscall_ucp);
	KASSERT(stack_pagefault_ucp);

	if (thunk_getcontext(&pcb2->pcb_ucp))
		panic("getcontext failed");

	/* set up the ucontext for the userland switch */
	/* XXX BUG TODO when is this stack space freed? */
	pcb2->pcb_ucp.uc_stack.ss_sp = stack_ucp;
	pcb2->pcb_ucp.uc_stack.ss_size = stacksize;
	pcb2->pcb_ucp.uc_flags = _UC_STACK | _UC_CPU;
	pcb2->pcb_ucp.uc_link = &pcb2->pcb_userret_ucp;
	thunk_makecontext(&pcb2->pcb_ucp,
	    (void (*)(void)) cpu_lwp_trampoline,
	    3, &pcb2->pcb_ucp, func, arg);

	/* set up the ucontext for the syscall */
	pcb2->pcb_syscall_ucp.uc_stack.ss_sp = stack_syscall_ucp;
	pcb2->pcb_syscall_ucp.uc_stack.ss_size = stacksize;
	pcb2->pcb_syscall_ucp.uc_flags = _UC_STACK | _UC_CPU;
	pcb2->pcb_syscall_ucp.uc_link = &pcb2->pcb_userret_ucp;
	thunk_makecontext(&pcb2->pcb_syscall_ucp, (void (*)(void)) syscall,
	    0, NULL, NULL, NULL);

	/* set up the ucontext for the pagefault */
	pcb2->pcb_pagefault_ucp.uc_stack.ss_sp = stack_pagefault_ucp;
	pcb2->pcb_pagefault_ucp.uc_stack.ss_size = stacksize;
	pcb2->pcb_pagefault_ucp.uc_flags = _UC_STACK | _UC_CPU;
	pcb2->pcb_pagefault_ucp.uc_link = &pcb2->pcb_trapret_ucp;
	thunk_makecontext(&pcb2->pcb_pagefault_ucp, (void (*)(void)) pagefault,
	    0, NULL, NULL, NULL);
}

void
cpu_initclocks(void)
{
	extern timer_t clock_timerid;

	thunk_timer_start(clock_timerid, HZ);
}

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	size_t stacksize, msgbufsize = 32 * 1024;
	void *stack_pagefault_ucp;

	/* get ourself a message buffer */
	um_msgbuf = kmem_zalloc(msgbufsize, KM_SLEEP);
	if (um_msgbuf == NULL)
		panic("couldn't allocate msgbuf");
	initmsgbuf(um_msgbuf, msgbufsize);

	/* allocate a submap for physio, 1Mb enough? */
	minaddr = 0;
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   1024 * 1024, 0, false, NULL);

	/* say hi! */
	banner();

	/* init lwp0 */
	memset(&lwp0pcb, 0, sizeof(lwp0pcb));
	if (thunk_getcontext(&lwp0pcb.pcb_ucp))
		panic("getcontext failed");
	uvm_lwp_setuarea(&lwp0, (vaddr_t)&lwp0pcb);

	/* init trapframes (going nowhere!), maybe a panic func? */
	memcpy(&lwp0pcb.pcb_syscall_ucp,   &lwp0pcb.pcb_ucp, sizeof(ucontext_t));
	memcpy(&lwp0pcb.pcb_userret_ucp,   &lwp0pcb.pcb_ucp, sizeof(ucontext_t));
	memcpy(&lwp0pcb.pcb_pagefault_ucp, &lwp0pcb.pcb_ucp, sizeof(ucontext_t));
	memcpy(&lwp0pcb.pcb_trapret_ucp,   &lwp0pcb.pcb_ucp, sizeof(ucontext_t));

	/* set up the ucontext for the pagefault */
	stacksize = 16*PAGE_SIZE;
	stack_pagefault_ucp = malloc(stacksize, M_TEMP, M_NOWAIT);

	lwp0pcb.pcb_pagefault_ucp.uc_stack.ss_sp = stack_pagefault_ucp;
	lwp0pcb.pcb_pagefault_ucp.uc_stack.ss_size = stacksize;
	lwp0pcb.pcb_pagefault_ucp.uc_flags = _UC_STACK | _UC_CPU;
	lwp0pcb.pcb_pagefault_ucp.uc_link = &lwp0pcb.pcb_userret_ucp;
	thunk_makecontext(&lwp0pcb.pcb_pagefault_ucp, (void (*)(void)) pagefault,
	    0, NULL, NULL, NULL);
}

void
cpu_rootconf(void)
{
	device_t rdev;

	rdev = device_find_by_xname("ld0");
	if (rdev == NULL)
		rdev = device_find_by_xname("md0");

	aprint_normal("boot device: %s\n",
	    rdev ? device_xname(rdev) : "<unknown>");
	setroot(rdev, 0);
}

bool
cpu_intr_p(void)
{
	int idepth;

	kpreempt_disable();
	idepth = curcpu()->ci_idepth;
	kpreempt_enable();

	return (idepth >= 0);
}
