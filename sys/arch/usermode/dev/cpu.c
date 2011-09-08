/* $NetBSD: cpu.c,v 1.35 2011/09/08 12:01:22 reinoud Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.35 2011/09/08 12:01:22 reinoud Exp $");

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

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/mainbus.h>
#include <machine/pcb.h>
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
static void *msgbuf;

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
	printf("cpu_switchto [%s,pid=%d,lid=%d] -> [%s,pid=%d,lid=%d]\n",
	    oldlwp ? oldlwp->l_name : "none",
	    oldlwp ? oldlwp->l_proc->p_pid : -1,
	    oldlwp ? oldlwp->l_lid : -1,
	    newlwp ? newlwp->l_name : "none",
	    newlwp ? newlwp->l_proc->p_pid : -1,
	    newlwp ? newlwp->l_lid : -1);
	if (oldpcb) {
		printf("    oldpcb uc_link=%p, uc_stack.ss_sp=%p, "
		    "uc_stack.ss_size=%d\n",
		    oldpcb->pcb_ucp.uc_link,
		    oldpcb->pcb_ucp.uc_stack.ss_sp,
		    (int)oldpcb->pcb_ucp.uc_stack.ss_size);
	}
	if (newpcb) {
		printf("    newpcb uc_link=%p, uc_stack.ss_sp=%p, "
		    "uc_stack.ss_size=%d\n",
		    newpcb->pcb_ucp.uc_link,
		    newpcb->pcb_ucp.uc_stack.ss_sp,
		    (int)newpcb->pcb_ucp.uc_stack.ss_size);
	}
#endif /* !CPU_DEBUG */

	ci->ci_stash = oldlwp;
	curlwp = newlwp;

	if (oldpcb) {
		oldpcb->pcb_errno = thunk_geterrno();
		thunk_seterrno(newpcb->pcb_errno);
		if (thunk_swapcontext(&oldpcb->pcb_ucp, &newpcb->pcb_ucp))
			panic("swapcontext failed");
	} else {
		thunk_seterrno(newpcb->pcb_errno);
		if (thunk_setcontext(&newpcb->pcb_ucp))
			panic("setcontext failed");
	}

#ifdef CPU_DEBUG
	printf("cpu_switchto: returning %p (was %p)\n", ci->ci_stash, oldlwp);
#endif
	return ci->ci_stash;
}

void
cpu_dumpconf(void)
{
#ifdef CPU_DEBUG
	printf("cpu_dumpconf\n");
#endif
}

void
cpu_signotify(struct lwp *l)
{
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
#ifdef CPU_DEBUG
	printf("cpu_getmcontext\n");
#endif
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
#ifdef CPU_DEBUG
	printf("cpu_setmcontext\n");
#endif
	return 0;
}

void
cpu_idle(void)
{
	struct cpu_info *ci = curcpu();

	if (ci->ci_want_resched)
		return;

#if notyet
	thunk_usleep(10000);
#endif
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
#ifdef CPU_DEBUG
	printf("cpu_lwp_free\n");
#endif
}

void
cpu_lwp_free2(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

#ifdef CPU_DEBUG
	printf("cpu_lwp_free2\n");
#endif

	if (pcb == NULL)
		return;

	if (pcb->pcb_needfree) {
		free(pcb->pcb_ucp.uc_stack.ss_sp, M_TEMP);
		pcb->pcb_ucp.uc_stack.ss_sp = NULL;
		pcb->pcb_ucp.uc_stack.ss_size = 0;
		pcb->pcb_needfree = false;
	}
}

static void
cpu_lwp_trampoline(void (*func)(void *), void *arg)
{
	struct pcb *pcb;

#ifdef CPU_DEBUG
	printf("cpu_lwp_trampoline called with func %p, arg %p\n", (void *) func, arg);
#endif
	lwp_startup(curcpu()->ci_stash, curlwp);

	func(arg);

	pcb = lwp_getpcb(curlwp);

	/* switch to userland */
printf("switching to userland\n");
	thunk_setcontext(&pcb->pcb_userland_ucp);

	panic("%s: shouldn't return", __func__);
}

extern int syscall(lwp_t *l);
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb1 = lwp_getpcb(l1);
	struct pcb *pcb2 = lwp_getpcb(l2);

#ifdef CPU_DEBUG
	printf("cpu_lwp_fork [%s/%p] -> [%s/%p] stack=%p stacksize=%d\n",
	    l1 ? l1->l_name : "none", l1,
	    l2 ? l2->l_name : "none", l2,
	    stack, (int)stacksize);
#endif

	/* copy the PCB and its switchframes from parent */
	memcpy(pcb2, pcb1, sizeof(struct pcb));

	/* XXXJDM */
	if (stack == NULL) {
		stack = malloc(PAGE_SIZE, M_TEMP, M_NOWAIT);
		stacksize = PAGE_SIZE;
		pcb2->pcb_needfree = true;
	} else
		pcb2->pcb_needfree = false;

	if (thunk_getcontext(&pcb2->pcb_ucp))
		panic("getcontext failed");

	/* set up the ucontext for the userland */
	pcb2->pcb_ucp.uc_stack.ss_sp = stack;
	pcb2->pcb_ucp.uc_stack.ss_size = stacksize;
	pcb2->pcb_ucp.uc_link = NULL;
	pcb2->pcb_ucp.uc_flags = _UC_STACK | _UC_CPU;
	thunk_makecontext(&pcb2->pcb_ucp, (void (*)(void))cpu_lwp_trampoline,
	    2, func, arg);

	/* set up the ucontext for the syscall */
	pcb2->pcb_ucp.uc_flags = _UC_CPU;
	/* hack for it sets the stack anyway!! */
	pcb2->pcb_syscall_ucp.uc_stack.ss_sp =
		((uint8_t *) pcb2->pcb_syscall_ucp.uc_stack.ss_sp) - 4;
	pcb2->pcb_syscall_ucp.uc_stack.ss_size = 0;

	thunk_makecontext_1(&pcb2->pcb_syscall_ucp, (void (*)(void)) syscall,
	    l2);
}

void
cpu_initclocks(void)
{
}

void
cpu_startup(void)
{

	msgbuf = thunk_malloc(PAGE_SIZE);
	if (msgbuf == NULL)
		panic("couldn't allocate msgbuf");
	initmsgbuf(msgbuf, PAGE_SIZE);

	banner();

	memset(&lwp0pcb, 0, sizeof(lwp0pcb));
	if (thunk_getcontext(&lwp0pcb.pcb_ucp))
		panic("getcontext failed");
	uvm_lwp_setuarea(&lwp0, (vaddr_t)&lwp0pcb);

	/* init trapframe (going nowhere!), maybe a panic func? */
	memcpy(&lwp0pcb.pcb_userland_ucp, &lwp0pcb.pcb_ucp, sizeof(ucontext_t));
	memcpy(&lwp0pcb.pcb_syscall_ucp,  &lwp0pcb.pcb_ucp, sizeof(ucontext_t));
//	thunk_makecontext_trapframe2go(&lwp0pcb.pcb_userland_ucp, NULL, NULL);
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
