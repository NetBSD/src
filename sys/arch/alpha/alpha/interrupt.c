/* $NetBSD: interrupt.c,v 1.80.24.1 2016/03/19 11:29:54 skrll Exp $ */

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Keith Bostic, Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * Additional Copyright (c) 1997 by Matthew Jacob for NASA/Ames Research Center.
 * Redistribute and modify at will, leaving only this additional copyright
 * notice.
 */

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.80.24.1 2016/03/19 11:29:54 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>
#include <sys/sched.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <machine/cpuvar.h>
#include <machine/autoconf.h>
#include <machine/reg.h>
#include <machine/rpb.h>
#include <machine/frame.h>
#include <machine/cpuconf.h>
#include <machine/alpha.h>

struct scbvec scb_iovectab[SCB_VECTOIDX(SCB_SIZE - SCB_IOVECBASE)];
static bool scb_mpsafe[SCB_VECTOIDX(SCB_SIZE - SCB_IOVECBASE)];

void	netintr(void);

void	scb_stray(void *, u_long);

void
scb_init(void)
{
	u_long i;

	for (i = 0; i < SCB_NIOVECS; i++) {
		scb_iovectab[i].scb_func = scb_stray;
		scb_iovectab[i].scb_arg = NULL;
	}
}

void
scb_stray(void *arg, u_long vec)
{

	printf("WARNING: stray interrupt, vector 0x%lx\n", vec);
}

void
scb_set(u_long vec, void (*func)(void *, u_long), void *arg, int level)
{
	u_long idx;
	int s;

	s = splhigh();

	if (vec < SCB_IOVECBASE || vec >= SCB_SIZE ||
	    (vec & (SCB_VECSIZE - 1)) != 0)
		panic("scb_set: bad vector 0x%lx", vec);

	idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);

	if (scb_iovectab[idx].scb_func != scb_stray)
		panic("scb_set: vector 0x%lx already occupied", vec);

	scb_iovectab[idx].scb_func = func;
	scb_iovectab[idx].scb_arg = arg;
	scb_mpsafe[idx] = (level != IPL_VM);

	splx(s);
}

u_long
scb_alloc(void (*func)(void *, u_long), void *arg)
{
	u_long vec, idx;
	int s;

	s = splhigh();

	/*
	 * Allocate "downwards", to avoid bumping into
	 * interrupts which are likely to be at the lower
	 * vector numbers.
	 */
	for (vec = SCB_SIZE - SCB_VECSIZE;
	     vec >= SCB_IOVECBASE; vec -= SCB_VECSIZE) {
		idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);
		if (scb_iovectab[idx].scb_func == scb_stray) {
			scb_iovectab[idx].scb_func = func;
			scb_iovectab[idx].scb_arg = arg;
			splx(s);
			return (vec);
		}
	}

	splx(s);

	return (SCB_ALLOC_FAILED);
}

void
scb_free(u_long vec)
{
	u_long idx;
	int s;

	s = splhigh();

	if (vec < SCB_IOVECBASE || vec >= SCB_SIZE ||
	    (vec & (SCB_VECSIZE - 1)) != 0)
		panic("scb_free: bad vector 0x%lx", vec);

	idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);

	if (scb_iovectab[idx].scb_func == scb_stray)
		panic("scb_free: vector 0x%lx is empty", vec);

	scb_iovectab[idx].scb_func = scb_stray;
	scb_iovectab[idx].scb_arg = (void *) vec;

	splx(s);
}

void
interrupt(unsigned long a0, unsigned long a1, unsigned long a2,
    struct trapframe *framep)
{
	struct cpu_info *ci = curcpu();
	struct cpu_softc *sc = ci->ci_softc;

	switch (a0) {
	case ALPHA_INTR_XPROC:	/* interprocessor interrupt */
#if defined(MULTIPROCESSOR)
		atomic_inc_ulong(&ci->ci_intrdepth);

		alpha_ipi_process(ci, framep);

		/*
		 * Handle inter-console messages if we're the primary
		 * CPU.
		 */
		if (ci->ci_cpuid == hwrpb->rpb_primary_cpu_id &&
		    hwrpb->rpb_txrdy != 0)
			cpu_iccb_receive();

		atomic_dec_ulong(&ci->ci_intrdepth);
#else
		printf("WARNING: received interprocessor interrupt!\n");
#endif /* MULTIPROCESSOR */
		break;
		
	case ALPHA_INTR_CLOCK:	/* clock interrupt */
		/*
		 * We don't increment the interrupt depth for the
		 * clock interrupt, since it is *sampled* from
		 * the clock interrupt, so if we did, all system
		 * time would be counted as interrupt time.
		 */
		sc->sc_evcnt_clock.ev_count++;
		ci->ci_data.cpu_nintr++;
		if (platform.clockintr) {
			/*
			 * Call hardclock().  This will also call
			 * statclock(). On the primary CPU, it
			 * will also deal with time-of-day stuff.
			 */
			(*platform.clockintr)((struct clockframe *)framep);

			/*
			 * If it's time to call the scheduler clock,
			 * do so.
			 */
			if ((++ci->ci_schedstate.spc_schedticks & 0x3f) == 0 &&
			    schedhz != 0)
				schedclock(ci->ci_curlwp);
		}
		break;

	case ALPHA_INTR_ERROR:	/* Machine Check or Correctable Error */
		atomic_inc_ulong(&ci->ci_intrdepth);
		a0 = alpha_pal_rdmces();
		if (platform.mcheck_handler != NULL &&
		    (void *)framep->tf_regs[FRAME_PC] != XentArith)
			(*platform.mcheck_handler)(a0, framep, a1, a2);
		else
			machine_check(a0, framep, a1, a2);
		atomic_dec_ulong(&ci->ci_intrdepth);
		break;

	case ALPHA_INTR_DEVICE:	/* I/O device interrupt */
	    {
		struct scbvec *scb;
		int idx = SCB_VECTOIDX(a1 - SCB_IOVECBASE);
		bool mpsafe = scb_mpsafe[idx];

		KDASSERT(a1 >= SCB_IOVECBASE && a1 < SCB_SIZE);

		atomic_inc_ulong(&sc->sc_evcnt_device.ev_count);
		atomic_inc_ulong(&ci->ci_intrdepth);

		if (!mpsafe) {
			KERNEL_LOCK(1, NULL);
		}
		ci->ci_data.cpu_nintr++;
		scb = &scb_iovectab[idx];
		(*scb->scb_func)(scb->scb_arg, a1);
		if (!mpsafe)
			KERNEL_UNLOCK_ONE(NULL);

		atomic_dec_ulong(&ci->ci_intrdepth);
		break;
	    }

	case ALPHA_INTR_PERF:	/* performance counter interrupt */
		printf("WARNING: received performance counter interrupt!\n");
		break;

	case ALPHA_INTR_PASSIVE:
#if 0
		printf("WARNING: received passive release interrupt vec "
		    "0x%lx\n", a1);
#endif
		break;

	default:
		printf("unexpected interrupt: type 0x%lx vec 0x%lx "
		    "a2 0x%lx"
#if defined(MULTIPROCESSOR)
		    " cpu %lu"
#endif
		    "\n", a0, a1, a2
#if defined(MULTIPROCESSOR)
		    , ci->ci_cpuid
#endif
		    );
		panic("interrupt");
		/* NOTREACHED */
	}
}

void
machine_check(unsigned long mces, struct trapframe *framep,
    unsigned long vector, unsigned long param)
{
	const char *type;
	struct mchkinfo *mcp;
	static struct timeval ratelimit[1];

	mcp = &curcpu()->ci_mcinfo;
	/* Make sure it's an error we know about. */
	if ((mces & (ALPHA_MCES_MIP|ALPHA_MCES_SCE|ALPHA_MCES_PCE)) == 0) {
		type = "fatal machine check or error (unknown type)";
		goto fatal;
	}

	/* Machine checks. */
	if (mces & ALPHA_MCES_MIP) {
		/* If we weren't expecting it, then we punt. */
		if (!mcp->mc_expected) {
			type = "unexpected machine check";
			goto fatal;
		}
		mcp->mc_expected = 0;
		mcp->mc_received = 1;
	}

	/* System correctable errors. */
	if (mces & ALPHA_MCES_SCE)
		printf("Warning: received system correctable error.\n");

	/* Processor correctable errors. */
	if (mces & ALPHA_MCES_PCE)
		printf("Warning: received processor correctable error.\n");

	/* Clear pending machine checks and correctable errors */
	alpha_pal_wrmces(mces);
	return;

fatal:
	alpha_pal_wrmces(mces);
	if ((void *)framep->tf_regs[FRAME_PC] == XentArith) {
		rlprintf(ratelimit, "Stray machine check\n");
		return;
	}

	printf("\n");
	printf("%s:\n", type);
	printf("\n");
	printf("    mces    = 0x%lx\n", mces);
	printf("    vector  = 0x%lx\n", vector);
	printf("    param   = 0x%lx\n", param);
	printf("    pc      = 0x%lx\n", framep->tf_regs[FRAME_PC]);
	printf("    ra      = 0x%lx\n", framep->tf_regs[FRAME_RA]);
	printf("    code    = 0x%lx\n", *(unsigned long *)(param + 0x10));
	printf("    curlwp = %p\n", curlwp);
	if (curlwp != NULL)
		printf("        pid = %d.%d, comm = %s\n",
		    curproc->p_pid, curlwp->l_lid,
		    curproc->p_comm);
	printf("\n");
	panic("machine check");
}

int
badaddr(void *addr, size_t size)
{

	return (badaddr_read(addr, size, NULL));
}

int
badaddr_read(void *addr, size_t size, void *rptr)
{
	struct mchkinfo *mcp = &curcpu()->ci_mcinfo;
	long rcpt;
	int rv;

	/* Get rid of any stale machine checks that have been waiting.  */
	alpha_pal_draina();

	/* Tell the trap code to expect a machine check. */
	mcp->mc_received = 0;
	mcp->mc_expected = 1;

	/* Read from the test address, and make sure the read happens. */
	alpha_mb();
	switch (size) {
	case sizeof (uint8_t):
		rcpt = *(volatile uint8_t *)addr;
		break;

	case sizeof (uint16_t):
		rcpt = *(volatile uint16_t *)addr;
		break;

	case sizeof (uint32_t):
		rcpt = *(volatile uint32_t *)addr;
		break;

	case sizeof (uint64_t):
		rcpt = *(volatile uint64_t *)addr;
		break;

	default:
		panic("badaddr: invalid size (%ld)", size);
	}
	alpha_mb();
	alpha_mb();	/* MAGIC ON SOME SYSTEMS */

	/* Make sure we took the machine check, if we caused one. */
	alpha_pal_draina();

	/* disallow further machine checks */
	mcp->mc_expected = 0;

	rv = mcp->mc_received;
	mcp->mc_received = 0;

	/*
	 * And copy back read results (if no fault occurred).
	 */
	if (rptr && rv == 0) {
		switch (size) {
		case sizeof (uint8_t):
			*(volatile uint8_t *)rptr = rcpt;
			break;

		case sizeof (uint16_t):
			*(volatile uint16_t *)rptr = rcpt;
			break;

		case sizeof (uint32_t):
			*(volatile uint32_t *)rptr = rcpt;
			break;

		case sizeof (uint64_t):
			*(volatile uint64_t *)rptr = rcpt;
			break;
		}
	}
	/* Return non-zero (i.e. true) if it's a bad address. */
	return (rv);
}

volatile unsigned long ssir;

/*
 * spl0:
 *
 *	Lower interrupt priority to IPL 0 -- must check for
 *	software interrupts.
 */
void
spl0(void)
{

	if (ssir) {
		(void) alpha_pal_swpipl(ALPHA_PSL_IPL_SOFT);
		softintr_dispatch();
	}

	(void) alpha_pal_swpipl(ALPHA_PSL_IPL_0);
}

/*
 * softintr_dispatch:
 *
 *	Process pending software interrupts.
 */
void
softintr_dispatch(void)
{

	/* XXX Nothing until alpha gets __HAVE_FAST_SOFTINTS */
}

#ifdef __HAVE_FAST_SOFTINTS
/*
 * softint_trigger:
 *
 *	Trigger a soft interrupt.
 */
void
softint_trigger(uintptr_t machdep)
{

	/* XXX Needs to be per-CPU */
	atomic_or_ulong(&ssir, 1 << (x))
}
#endif

/*
 * cpu_intr_p:
 *
 *	Return non-zero if executing in interrupt context.
 */
bool
cpu_intr_p(void)
{

	return curcpu()->ci_intrdepth != 0;
}

/*
 * Security sensitive rate limiting printf
 */
void
rlprintf(struct timeval *t, const char *fmt, ...)
{
	va_list ap;
	static const struct timeval msgperiod[1] = {{ 5, 0 }};

	if (!ratecheck(t, msgperiod))
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

const static uint8_t ipl2psl_table[] = {
	[IPL_NONE] = ALPHA_PSL_IPL_0,
	[IPL_SOFTCLOCK] = ALPHA_PSL_IPL_SOFT,
	[IPL_VM] = ALPHA_PSL_IPL_IO,
	[IPL_CLOCK] = ALPHA_PSL_IPL_CLOCK,
	[IPL_HIGH] = ALPHA_PSL_IPL_HIGH,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = ipl2psl_table[ipl]};
}
