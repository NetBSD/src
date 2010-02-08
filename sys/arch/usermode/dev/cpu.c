/* $NetBSD: cpu.c,v 1.7 2010/02/08 19:02:32 joerg Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.7 2010/02/08 19:02:32 joerg Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/mbuf.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/mainbus.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

/* #define CPU_DEBUG */

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

struct cpu_info cpu_info_primary;
char cpu_model[48] = "virtual processor";

typedef struct cpu_softc {
	device_t	sc_dev;
	struct cpu_info	*sc_ci;
} cpu_softc_t;

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
	sc->sc_ci->ci_dev = 0;
	sc->sc_ci->ci_self = &cpu_info_primary;
#if notyet
	sc->sc_ci->ci_curlwp = &lwp0;
#endif
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
	extern void exit(int);
	extern void abort(void);

	splhigh();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN)
		exit(0);

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n");

	/*
	 * XXXJDM If we've panic'd, make sure we dump a core
	 */
	abort();

	/* NOTREACHED */
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	ci->ci_want_resched = 1;
}

void
cpu_need_proftick(struct lwp *l)
{
}

lwp_t *
cpu_switchto(lwp_t *oldlwp, lwp_t *newlwp, bool returning)
{
	extern int errno;
	struct pcb *oldpcb = oldlwp ? lwp_getpcb(oldlwp) : NULL;
	struct pcb *newpcb = lwp_getpcb(newlwp);
	struct cpu_info *ci = curcpu();

#ifdef CPU_DEBUG
	printf("cpu_switchto [%s] -> [%s]\n",
	    oldlwp ? oldlwp->l_name : "none",
	    newlwp ? newlwp->l_name : "none");
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
		if (swapcontext(&oldpcb->pcb_ucp, &newpcb->pcb_ucp))
			panic("swapcontext failed: %d", errno);
	} else {
		if (setcontext(&newpcb->pcb_ucp))
			panic("setcontext failed: %d", errno);
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
#ifdef CPU_DEBUG
	printf("cpu_signotify\n");
#endif
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
	extern int usleep(useconds_t);
	struct cpu_info *ci = curcpu();

	if (ci->ci_want_resched)
		return;

#if notyet
	usleep(10000);
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
	lwp_startup(curcpu()->ci_stash, curlwp);

	func(arg);
}

void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	extern int errno;
	struct pcb *pcb = lwp_getpcb(l2);

#ifdef CPU_DEBUG
	printf("cpu_lwp_fork [%s/%p] -> [%s/%p] stack=%p stacksize=%d\n",
	    l1 ? l1->l_name : "none", l1,
	    l2 ? l2->l_name : "none", l2,
	    stack, (int)stacksize);
#endif

	/* XXXJDM */
	if (stack == NULL) {
		stack = malloc(PAGE_SIZE, M_TEMP, M_NOWAIT);
		stacksize = PAGE_SIZE;
		pcb->pcb_needfree = true;
	} else
		pcb->pcb_needfree = false;

	if (getcontext(&pcb->pcb_ucp))
		panic("getcontext failed: %d", errno);
	pcb->pcb_ucp.uc_stack.ss_sp = stack;
	pcb->pcb_ucp.uc_stack.ss_size = stacksize;
	pcb->pcb_ucp.uc_link = NULL;
	pcb->pcb_ucp.uc_flags = _UC_STACK | _UC_CPU;
	makecontext(&pcb->pcb_ucp, (void (*)(void))cpu_lwp_trampoline,
	    2, func, arg);
}

void
cpu_initclocks(void)
{
}

void
cpu_startup(void)
{
	char pbuf[9];

	printf("%s%s", copyright, version);
	format_bytes(pbuf, sizeof(pbuf), ptoa(physmem));
	printf("total memory = %s\n", pbuf);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

void
cpu_rootconf(void)
{
	device_t rdev;

	rdev = device_find_by_xname("md0");

	setroot(rdev, 0);
}

bool
cpu_intr_p(void)
{
	printf("cpu_intr_p\n");
	return false;
}
