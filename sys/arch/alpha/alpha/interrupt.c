/* $NetBSD: interrupt.c,v 1.17 1997/04/07 06:36:24 cgd Exp $ */

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

#include <machine/options.h>		/* Pull in config options headers */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>

#include <machine/autoconf.h>
#include <machine/reg.h>
#include <machine/frame.h>

#ifdef EVCNT_COUNTERS
#include <sys/device.h>
#else
#include <machine/intrcnt.h>
#endif

struct logout {
#define	LOGOUT_RETRY	0x1000000000000000	/* Retry bit. */
#define	LOGOUT_LENGTH	0xffff			/* Length mask. */
	u_int64_t q1;				/* Retry and length */
	/* Unspecified. */
};

static void	machine_check __P((struct trapframe *, unsigned long,
		    unsigned long));
static void	nullintr __P((void *, unsigned long));
static void	real_clockintr __P((void *, unsigned long));

static void	(*iointr) __P((void *, unsigned long)) = nullintr;
static void	(*clockintr) __P((void *, unsigned long)) = nullintr;
static volatile int mc_expected, mc_received;

#ifdef EVCNT_COUNTERS
struct evcnt	clock_intr_evcnt;	/* event counter for clock intrs. */
#endif

void
interrupt(a0, a1, a2, framep)
	unsigned long a0, a1, a2;
	struct trapframe *framep;
{

	if (a0 == 1) {			/* clock interrupt */
		cnt.v_intr++;
		(*clockintr)(framep, a1);
	} else if (a0 == 3) {		/* I/O device interrupt */
		cnt.v_intr++;
		(*iointr)(framep, a1);
	} else if (a0 == 2) {		/* Machine Check or Correctable Error */
		machine_check(framep, a1, a2);
	} else if (a0 == 5) {		/* Passive Release (?) interrupt XXX */
		cnt.v_intr++;
		printf("passive release(?) interrupt vec 0x%lx (ignoring)\n",
		    a1);
	} else {
		/*
		 * Not expected or handled:
		 *	0	Interprocessor interrupt
		 *	4	Performance counter
		 */
		panic("unexpected interrupt: type 0x%lx, vec 0x%lx\n", a0, a1);
	}
}

static void
nullintr(framep, vec)
	void *framep;
	unsigned long vec;
{
}

static void
real_clockintr(framep, vec)
	void *framep;
	unsigned long vec;
{

#ifdef EVCNT_COUNTERS
	clock_intr_evcnt.ev_count++;
#else
	intrcnt[INTRCNT_CLOCK]++;
#endif
	hardclock(framep);
}

void
set_clockintr()
{

	if (clockintr != nullintr)
		panic("set clockintr twice");

	clockintr = real_clockintr;
}

void
set_iointr(niointr)
	void (*niointr) __P((void *, unsigned long));
{

	if (iointr != nullintr)
		panic("set iointr twice");

	iointr = niointr;
}

static void
machine_check(framep, vector, param)
	struct trapframe *framep;
	unsigned long vector, param;
{
	unsigned long mces;
	const char *type;

	mces = alpha_pal_rdmces();

	/* If not a machine check, we have no clue ho we got here. */
	if ((mces & ALPHA_MCES_MIP) == 0) {
		type = "fatal machine check or error (unknown type)";
		goto fatal;
	}

	/* If we weren't expecting it, then we punt. */
	if (!mc_expected) {
		type = "unexpected machine check";
		goto fatal;
	}

	mc_expected = 0;
	mc_received = 1;

	/* Clear pending machine checks and correctable errors */
	alpha_pal_wrmces(mces);
	return;

fatal:
	printf("\n");
	printf("%s:\n", type);
	printf("\n");
	printf("    mces    = 0x%lx\n", mces);
	printf("    vector  = 0x%lx\n", vector);
	printf("    param   = 0x%lx\n", param);
	printf("    pc      = 0x%lx\n", framep->tf_regs[FRAME_PC]);
	printf("    ra      = 0x%lx\n", framep->tf_regs[FRAME_RA]);
	printf("    curproc = %p\n", curproc);
	if (curproc != NULL)
		printf("        pid = %d, comm = %s\n", curproc->p_pid,
		    curproc->p_comm);
	printf("\n");
	panic("machine check");
}

int
badaddr(addr, size)
	void *addr;
	size_t size;
{
	long rcpt;

	/* Get rid of any stale machine checks that have been waiting.  */
	alpha_pal_draina();

	/* Tell the trap code to expect a machine check. */
	mc_received = 0;
	mc_expected = 1;

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
		panic("badaddr: invalid size (%ld)\n", size);
	}
	alpha_mb();

	/* Make sure we took the machine check, if we caused one. */
	alpha_pal_draina();

	/* disallow further machine checks */
	mc_expected = 0;

	/* Return non-zero (i.e. true) if it's a bad address. */
	return (mc_received);
}
