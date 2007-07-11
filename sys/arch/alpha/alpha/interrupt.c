/* $NetBSD: interrupt.c,v 1.72.8.1 2007/07/11 19:57:20 mjf Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.72.8.1 2007/07/11 19:57:20 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>
#include <sys/sched.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/time.h>

#include <machine/cpuvar.h>

/* XXX Network interrupts should be converted to new softintrs */
#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <machine/atomic.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/rpb.h>
#include <machine/frame.h>
#include <machine/cpuconf.h>
#include <machine/alpha.h>

#if defined(MULTIPROCESSOR)
#include <sys/device.h>
#endif

struct scbvec scb_iovectab[SCB_VECTOIDX(SCB_SIZE - SCB_IOVECBASE)];

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
scb_set(u_long vec, void (*func)(void *, u_long), void *arg)
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
	static int microset_iter;	/* call cc_microset() once per sec. */
	struct cpu_info *ci = curcpu();
	struct cpu_softc *sc = ci->ci_softc;

	switch (a0) {
	case ALPHA_INTR_XPROC:	/* interprocessor interrupt */
#if defined(MULTIPROCESSOR)
		atomic_add_ulong(&ci->ci_intrdepth, 1);

		alpha_ipi_process(ci, framep);

		/*
		 * Handle inter-console messages if we're the primary
		 * CPU.
		 */
		if (ci->ci_cpuid == hwrpb->rpb_primary_cpu_id &&
		    hwrpb->rpb_txrdy != 0)
			cpu_iccb_receive();

		atomic_sub_ulong(&ci->ci_intrdepth, 1);
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
		uvmexp.intrs++;
		/*
		 * Update the PCC frequency for use by microtime().
		 */
		if (
#if defined(MULTIPROCESSOR)
		    CPU_IS_PRIMARY(ci) &&
#endif

		    microset_iter-- == 0) {
			microset_iter = hz - 1;
			cc_microset_time = time;
#if defined(MULTIPROCESSOR)
			alpha_multicast_ipi(cpus_running,
			    ALPHA_IPI_MICROSET);
#endif
			cc_microset(ci);
		}
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
		atomic_add_ulong(&ci->ci_intrdepth, 1);
		a0 = alpha_pal_rdmces();
		if (platform.mcheck_handler != NULL &&
		    (void *)framep->tf_regs[FRAME_PC] != XentArith)
			(*platform.mcheck_handler)(a0, framep, a1, a2);
		else
			machine_check(a0, framep, a1, a2);
		atomic_sub_ulong(&ci->ci_intrdepth, 1);
		break;

	case ALPHA_INTR_DEVICE:	/* I/O device interrupt */
	    {
		struct scbvec *scb;

		KDASSERT(a1 >= SCB_IOVECBASE && a1 < SCB_SIZE);

		atomic_add_ulong(&sc->sc_evcnt_device.ev_count, 1);
		atomic_add_ulong(&ci->ci_intrdepth, 1);

		KERNEL_LOCK(1, NULL);

		uvmexp.intrs++;

		scb = &scb_iovectab[SCB_VECTOIDX(a1 - SCB_IOVECBASE)];
		(*scb->scb_func)(scb->scb_arg, a1);

		KERNEL_UNLOCK_ONE(NULL);

		atomic_sub_ulong(&ci->ci_intrdepth, 1);
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
	case sizeof (u_int8_t):
		rcpt = *(volatile u_int8_t *)addr;
		break;

	case sizeof (u_int16_t):
		rcpt = *(volatile u_int16_t *)addr;
		break;

	case sizeof (u_int32_t):
		rcpt = *(volatile u_int32_t *)addr;
		break;

	case sizeof (u_int64_t):
		rcpt = *(volatile u_int64_t *)addr;
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
		case sizeof (u_int8_t):
			*(volatile u_int8_t *)rptr = rcpt;
			break;

		case sizeof (u_int16_t):
			*(volatile u_int16_t *)rptr = rcpt;
			break;

		case sizeof (u_int32_t):
			*(volatile u_int32_t *)rptr = rcpt;
			break;

		case sizeof (u_int64_t):
			*(volatile u_int64_t *)rptr = rcpt;
			break;
		}
	}
	/* Return non-zero (i.e. true) if it's a bad address. */
	return (rv);
}

void
netintr()
{
	int n, s;

	s = splhigh();
	n = netisr;
	netisr = 0;
	splx(s);

#define	DONETISR(bit, fn)						\
	do {								\
		if (n & (1 << (bit)))					\
			fn();						\
	} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

struct alpha_soft_intr alpha_soft_intrs[SI_NQUEUES];
volatile unsigned long ssir;

/* XXX For legacy software interrupts. */
struct alpha_soft_intrhand *softnet_intrhand;

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
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
void
softintr_init()
{
	static const char *softintr_names[] = SI_QUEUENAMES;
	struct alpha_soft_intr *asi;
	int i;

	for (i = 0; i < SI_NQUEUES; i++) {
		asi = &alpha_soft_intrs[i];
		TAILQ_INIT(&asi->softintr_q);
		simple_lock_init(&asi->softintr_slock);
		asi->softintr_siq = i;
		evcnt_attach_dynamic(&asi->softintr_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* XXX Establish legacy software interrupt handlers. */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);

	assert(softnet_intrhand != NULL);
}

/*
 * softintr_dispatch:
 *
 *	Process pending software interrupts.
 */
void
softintr_dispatch()
{
	struct alpha_soft_intr *asi;
	struct alpha_soft_intrhand *sih;
	u_int64_t n, i;

#ifdef DEBUG
	n = alpha_pal_rdps() & ALPHA_PSL_IPL_MASK;
	if (n != ALPHA_PSL_IPL_SOFT)
		panic("softintr_dispatch: entry at ipl %ld", n);
#endif

	KERNEL_LOCK(1, NULL);

#ifdef DEBUG
	n = alpha_pal_rdps() & ALPHA_PSL_IPL_MASK;
	if (n != ALPHA_PSL_IPL_SOFT)
		panic("softintr_dispatch: after kernel lock at ipl %ld", n);
#endif

	while ((n = atomic_loadlatch_ulong(&ssir, 0)) != 0) {
		for (i = 0; i < SI_NQUEUES; i++) {
			if ((n & (1 << i)) == 0)
				continue;

			asi = &alpha_soft_intrs[i];

			asi->softintr_evcnt.ev_count++;

			for (;;) {
				(void) alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);
				simple_lock(&asi->softintr_slock);

				sih = TAILQ_FIRST(&asi->softintr_q);
				if (sih != NULL) {
					TAILQ_REMOVE(&asi->softintr_q, sih,
					    sih_q);
					sih->sih_pending = 0;
				}

				simple_unlock(&asi->softintr_slock);
				(void) alpha_pal_swpipl(ALPHA_PSL_IPL_SOFT);

				if (sih == NULL)
					break;

				uvmexp.softs++;
				(*sih->sih_fn)(sih->sih_arg);
			}
		}
	}

	KERNEL_UNLOCK_ONE(NULL);
}

static int
ipl2si(int ipl)
{
	int si;

	switch (ipl) {
	case IPL_SOFTSERIAL:
		si = SI_SOFTSERIAL;
		break;
	case IPL_SOFTNET:
		si = SI_SOFTNET;
		break;
	case IPL_SOFTCLOCK:
		si = SI_SOFTCLOCK;
		break;
	case IPL_SOFT:
		si = SI_SOFT;
		break;
	default:
		panic("ipl2si: %d", ipl);
	}
	return si;
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct alpha_soft_intr *asi;
	struct alpha_soft_intrhand *sih;
	int si;

	si = ipl2si(ipl);
	asi = &alpha_soft_intrs[si];

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = asi;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	return (sih);
}

/*
 * softintr_disestablish:	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct alpha_soft_intrhand *sih = arg;
	struct alpha_soft_intr *asi = sih->sih_intrhead;
	int s;

	s = splhigh();
	simple_lock(&asi->softintr_slock);
	if (sih->sih_pending) {
		TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	simple_unlock(&asi->softintr_slock);
	splx(s);

	free(sih, M_DEVBUF);
}

/*
 * Security sensitive rate limiting printf
 */
void
rlprintf(struct timeval *t, const char *fmt, ...)
{
	va_list ap;
	static const struct timeval msgperiod[1] = {{ 5, 0 }};

	if (ratecheck(t, msgperiod))
		vprintf(fmt, ap);
}

const static uint8_t ipl2psl_table[] = {
	[IPL_NONE] = ALPHA_PSL_IPL_0,
	[IPL_SOFT] = ALPHA_PSL_IPL_SOFT,
	[IPL_SOFTCLOCK] = IPL_SOFT,
	[IPL_SOFTNET] = IPL_SOFT,
	[IPL_SOFTSERIAL] = IPL_SOFT,
	[IPL_BIO] = ALPHA_PSL_IPL_IO,
	[IPL_NET] = ALPHA_PSL_IPL_IO,
	[IPL_TTY] = ALPHA_PSL_IPL_IO,
	/* IPL_LPT == IPL_TTY */
	[IPL_VM] = ALPHA_PSL_IPL_IO,
	[IPL_CLOCK] = ALPHA_PSL_IPL_CLOCK,
	/* IPL_STATCLOCK == IPL_CLOCK */
	/* IPL_IPI == IPL_CLOCK */
	[IPL_HIGH] = ALPHA_PSL_IPL_HIGH,
	/* IPL_SCHED == IPL_HIGH */
	/* IPL_LOCK == IPL_HIGH */
	[IPL_SERIAL] = ALPHA_PSL_IPL_IO,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = ipl2psl_table[ipl]};
}
