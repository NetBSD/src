/* $NetBSD: interrupt.c,v 1.100 2021/11/10 16:53:28 msaitoh Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.100 2021/11/10 16:53:28 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>
#include <sys/sched.h>
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

/* Protected by cpu_lock */
struct scbvec scb_iovectab[SCB_VECTOIDX(SCB_SIZE - SCB_IOVECBASE)]
							__read_mostly;

static void	scb_stray(void *, u_long);

void
scb_init(void)
{
	u_long i;

	for (i = 0; i < SCB_NIOVECS; i++) {
		scb_iovectab[i].scb_func = scb_stray;
		scb_iovectab[i].scb_arg = NULL;
	}
}

static void
scb_stray(void *arg, u_long vec)
{

	printf("WARNING: stray interrupt, vector 0x%lx\n", vec);
}

void
scb_set(u_long vec, void (*func)(void *, u_long), void *arg)
{
	u_long idx;

	KASSERT(mutex_owned(&cpu_lock));

	if (vec < SCB_IOVECBASE || vec >= SCB_SIZE ||
	    (vec & (SCB_VECSIZE - 1)) != 0)
		panic("scb_set: bad vector 0x%lx", vec);

	idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);

	if (scb_iovectab[idx].scb_func != scb_stray)
		panic("scb_set: vector 0x%lx already occupied", vec);

	scb_iovectab[idx].scb_arg = arg;
	alpha_mb();
	scb_iovectab[idx].scb_func = func;
	alpha_mb();
}

u_long
scb_alloc(void (*func)(void *, u_long), void *arg)
{
	u_long vec, idx;

	KASSERT(mutex_owned(&cpu_lock));

	/*
	 * Allocate "downwards", to avoid bumping into
	 * interrupts which are likely to be at the lower
	 * vector numbers.
	 */
	for (vec = SCB_SIZE - SCB_VECSIZE;
	     vec >= SCB_IOVECBASE; vec -= SCB_VECSIZE) {
		idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);
		if (scb_iovectab[idx].scb_func == scb_stray) {
			scb_iovectab[idx].scb_arg = arg;
			alpha_mb();
			scb_iovectab[idx].scb_func = func;
			alpha_mb();
			return (vec);
		}
	}

	return (SCB_ALLOC_FAILED);
}

void
scb_free(u_long vec)
{
	u_long idx;

	KASSERT(mutex_owned(&cpu_lock));

	if (vec < SCB_IOVECBASE || vec >= SCB_SIZE ||
	    (vec & (SCB_VECSIZE - 1)) != 0)
		panic("scb_free: bad vector 0x%lx", vec);

	idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);

	if (scb_iovectab[idx].scb_func == scb_stray)
		panic("scb_free: vector 0x%lx is empty", vec);

	scb_iovectab[idx].scb_func = scb_stray;
	alpha_mb();
	scb_iovectab[idx].scb_arg = (void *) vec;
	alpha_mb();
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
		ci->ci_intrdepth++;

		alpha_ipi_process(ci, framep);

		/*
		 * Handle inter-console messages if we're the primary
		 * CPU.
		 */
		if (ci->ci_cpuid == hwrpb->rpb_primary_cpu_id &&
		    hwrpb->rpb_txrdy != 0)
			cpu_iccb_receive();

		ci->ci_intrdepth--;
#else
		printf("WARNING: received interprocessor interrupt!\n");
#endif /* MULTIPROCESSOR */
		break;
		
	case ALPHA_INTR_CLOCK:	/* clock interrupt */
		/*
		 * Rather than simply increment the interrupt depth
		 * for the clock interrupt, we add 0x10.  Why?  Because
		 * while we only call out a single device interrupt
		 * level, technically the architecture specification
		 * supports two, meaning we could have intrdepth > 1
		 * just for device interrupts.
		 *
		 * Adding 0x10 here means that cpu_intr_p() can check
		 * for "intrdepth != 0" for "in interrupt context" and
		 * CLKF_INTR() can check "(intrdepth & 0xf) != 0" for
		 * "was processing interrupts when the clock interrupt
		 * happened".
		 */
		ci->ci_intrdepth += 0x10;
		sc->sc_evcnt_clock.ev_count++;
		ci->ci_data.cpu_nintr++;
		if (platform.clockintr) {
			/*
			 * Call hardclock().  This will also call
			 * statclock(). On the primary CPU, it
			 * will also deal with time-of-day stuff.
			 */
			(*platform.clockintr)((struct clockframe *)framep);

#if defined(MULTIPROCESSOR)
			if (alpha_use_cctr) {
				cc_hardclock(ci);
			}
#endif /* MULTIPROCESSOR */

			/*
			 * If it's time to call the scheduler clock,
			 * do so.
			 */
			if ((++ci->ci_schedstate.spc_schedticks & 0x3f) == 0 &&
			    schedhz != 0)
				schedclock(ci->ci_curlwp);
		}
		ci->ci_intrdepth -= 0x10;
		break;

	case ALPHA_INTR_ERROR:	/* Machine Check or Correctable Error */
		ci->ci_intrdepth++;
		a0 = alpha_pal_rdmces();
		if (platform.mcheck_handler != NULL &&
		    (void *)framep->tf_regs[FRAME_PC] != XentArith)
			(*platform.mcheck_handler)(a0, framep, a1, a2);
		else
			machine_check(a0, framep, a1, a2);
		ci->ci_intrdepth--;
		break;

	case ALPHA_INTR_DEVICE:	/* I/O device interrupt */
	    {
		const int idx = SCB_VECTOIDX(a1 - SCB_IOVECBASE);

		KDASSERT(a1 >= SCB_IOVECBASE && a1 < SCB_SIZE);

		atomic_inc_ulong(&sc->sc_evcnt_device.ev_count);
		ci->ci_intrdepth++;

		ci->ci_data.cpu_nintr++;

		struct scbvec * const scb = &scb_iovectab[idx];
		(*scb->scb_func)(scb->scb_arg, a1);

		ci->ci_intrdepth--;
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
	lwp_t * const l = curlwp;
	KPREEMPT_DISABLE(l);

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

	KPREEMPT_ENABLE(l);

	/* Return non-zero (i.e. true) if it's a bad address. */
	return (rv);
}

/*
 * Fast soft interrupt support.
 */

#define	SOFTINT_TO_IPL(si)						\
	(ALPHA_PSL_IPL_SOFT_LO + ((ALPHA_IPL2_SOFTINTS >> (si)) & 1))

#define	SOFTINTS_ELIGIBLE(ipl)						\
	((ALPHA_ALL_SOFTINTS << ((ipl) << 1)) & ALPHA_ALL_SOFTINTS)

/* Validate some assumptions the code makes. */
__CTASSERT(SOFTINT_TO_IPL(SOFTINT_CLOCK) == ALPHA_PSL_IPL_SOFT_LO);
__CTASSERT(SOFTINT_TO_IPL(SOFTINT_BIO) == ALPHA_PSL_IPL_SOFT_LO);
__CTASSERT(SOFTINT_TO_IPL(SOFTINT_NET) == ALPHA_PSL_IPL_SOFT_HI);
__CTASSERT(SOFTINT_TO_IPL(SOFTINT_SERIAL) == ALPHA_PSL_IPL_SOFT_HI);

__CTASSERT(IPL_SOFTCLOCK == ALPHA_PSL_IPL_SOFT_LO);
__CTASSERT(IPL_SOFTBIO == ALPHA_PSL_IPL_SOFT_LO);
__CTASSERT(IPL_SOFTNET == ALPHA_PSL_IPL_SOFT_HI);
__CTASSERT(IPL_SOFTSERIAL == ALPHA_PSL_IPL_SOFT_HI);

__CTASSERT(SOFTINT_CLOCK_MASK & 0x3);
__CTASSERT(SOFTINT_BIO_MASK & 0x3);
__CTASSERT(SOFTINT_NET_MASK & 0xc);
__CTASSERT(SOFTINT_SERIAL_MASK & 0xc);
__CTASSERT(SOFTINT_COUNT == 4);

__CTASSERT((ALPHA_ALL_SOFTINTS & ~0xfUL) == 0);
__CTASSERT(SOFTINTS_ELIGIBLE(IPL_NONE) == ALPHA_ALL_SOFTINTS);
__CTASSERT(SOFTINTS_ELIGIBLE(IPL_SOFTCLOCK) == ALPHA_IPL2_SOFTINTS);
__CTASSERT(SOFTINTS_ELIGIBLE(IPL_SOFTBIO) == ALPHA_IPL2_SOFTINTS);
__CTASSERT(SOFTINTS_ELIGIBLE(IPL_SOFTNET) == 0);
__CTASSERT(SOFTINTS_ELIGIBLE(IPL_SOFTSERIAL) == 0);

/*
 * softint_trigger:
 *
 *	Trigger a soft interrupt.
 */
void
softint_trigger(uintptr_t const machdep)
{
	/* No need for an atomic; called at splhigh(). */
	KASSERT(alpha_pal_rdps() == ALPHA_PSL_IPL_HIGH);
	curcpu()->ci_ssir |= machdep;
}

/*
 * softint_init_md:
 *
 *	Machine-dependent initialization for a fast soft interrupt thread.
 */
void
softint_init_md(lwp_t * const l, u_int const level, uintptr_t * const machdep)
{
	lwp_t ** lp = &l->l_cpu->ci_silwps[level];
	KASSERT(*lp == NULL || *lp == l);
	*lp = l;

	const uintptr_t si_bit = __BIT(level);
	KASSERT(si_bit & ALPHA_ALL_SOFTINTS);
	*machdep = si_bit;
}

/*
 * Helper macro.
 *
 * Dispatch a softint and then restart the loop so that higher
 * priority softints are always done first.
 */
#define	DOSOFTINT(level)						\
	if (ssir & SOFTINT_##level##_MASK) {				\
		ci->ci_ssir &= ~SOFTINT_##level##_MASK;			\
		alpha_softint_switchto(l, IPL_SOFT##level,		\
		    ci->ci_silwps[SOFTINT_##level]);			\
		KASSERT(alpha_pal_rdps() == ALPHA_PSL_IPL_HIGH);	\
		continue;						\
	}								\

/*
 * alpha_softint_dispatch:
 *
 *	Process pending soft interrupts that are eligible to run
 *	at the specified new IPL.  Must be called at splhigh().
 */
void
alpha_softint_dispatch(int const ipl)
{
	struct lwp * const l = curlwp;
	struct cpu_info * const ci = l->l_cpu;
	unsigned long ssir;
	const unsigned long eligible = SOFTINTS_ELIGIBLE(ipl);

	KASSERT(alpha_pal_rdps() == ALPHA_PSL_IPL_HIGH);

	for (;;) {
		ssir = ci->ci_ssir & eligible;
		if (ssir == 0)
			break;

		DOSOFTINT(SERIAL);
		DOSOFTINT(NET);
		DOSOFTINT(BIO);
		DOSOFTINT(CLOCK);
	}
}

/*
 * spllower:
 *
 *	Lower interrupt priority.  May need to check for software
 *	interrupts.
 */
void
spllower(int const ipl)
{

	if (ipl < ALPHA_PSL_IPL_SOFT_HI && curcpu()->ci_ssir) {
		(void) alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);
		alpha_softint_dispatch(ipl);
	}
	(void) alpha_pal_swpipl(ipl);
}

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

void	(*alpha_intr_redistribute)(void);

/*
 * cpu_intr_redistribute:
 *
 *	Redistribute interrupts amongst CPUs eligible to handle them.
 */
void
cpu_intr_redistribute(void)
{
	if (alpha_intr_redistribute != NULL)
		(*alpha_intr_redistribute)();
}

/*
 * cpu_intr_count:
 *
 *	Return the number of device interrupts this CPU handles.
 */
unsigned int
cpu_intr_count(struct cpu_info * const ci)
{
	return ci->ci_nintrhand;
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
