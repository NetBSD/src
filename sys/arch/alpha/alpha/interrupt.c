/* $NetBSD: interrupt.c,v 1.14.2.3 1997/07/22 05:54:35 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.14.2.3 1997/07/22 05:54:35 cgd Exp $");
__KERNEL_COPYRIGHT(0, \
    "Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>

#include <machine/autoconf.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/conf.h>

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

int		correctable_errors_fatal = 0;

extern int	cputype;

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
	int (*mcheck) __P((struct trapframe *, unsigned long, unsigned long,
	    unsigned long *, int)) = cpusw[cputype].machine_check;
	unsigned long mces, orig_mces;
	int fatalm;

	/*
	 * Find out what's pending
	 */
	orig_mces = mces = alpha_pal_rdmces();

	/*
	 * Call the machine-dependent machine check handler.
	 * If it handles anything, it'll clear the bits in 'mces'.
	 * If a machine check is expected, it should leave mces's
	 * ALPHA_MCES_MIP _SET_.
	 *
	 * If mc_expected is true and the machine check really is a fatal
	 * one (i.e. it can't possibly be expected), mcheck should return
	 * non-zero.
	 */
	if (mcheck != NULL)
		fatalm = (*mcheck)(framep, vector, param, &mces, mc_expected);
	else
		fatalm = 0;

	/*
	 * Clear the error bits in the MCES, now that we've examined
	 * the logout area (in the MD mcheck handler, if there was one).
	 * That lets the PALcode write over the logout area.  If we don't
	 * do this before we drop IPL (e.g. because of panic), we'll lose
	 * badly if we take another machine check or correctable error.
	 */
	alpha_pal_wrmces(orig_mces);

	/*
	 * If a machine check was expected, clear that bit.
	 */
	if ((mces & ALPHA_MCES_MIP) != 0 && mc_expected && !fatalm) {
		mc_expected = 0;
		mc_received = 1;

		mces &= ~ALPHA_MCES_MIP;
	}

	/*
	 * Try handling correctable errors.
	 */
	if ((mces & ALPHA_MCES_SCE) != 0 && !correctable_errors_fatal) {
		printf("Warning: system correctable error encountered.\n");
		mces &= ~ALPHA_MCES_SCE;
	}
	if ((mces & ALPHA_MCES_PCE) != 0 && !correctable_errors_fatal) {
		printf("Warning: processor correctable error encountered.\n");
		mces &= ~ALPHA_MCES_PCE;
	}

	/*
	 * If everything was handled, go on as normal.
	 */
	if ((mces & (ALPHA_MCES_MIP | ALPHA_MCES_SCE | ALPHA_MCES_PCE)) == 0)
		return;

	/*
	 * Otherwise, die!
	 */
	printf("\n");

	{
		int output = 0;

#define PRINTBIT(x, l)							\
		if ((mces & (x)) != 0) {				\
			if (output)					\
				printf(", ");				\
			printf(l);					\
			output = 1;					\
		}							\

		PRINTBIT(ALPHA_MCES_MIP, "machine check");
		PRINTBIT(ALPHA_MCES_SCE, "system correctable error");
		PRINTBIT(ALPHA_MCES_PCE, "processor correctable error");
		printf(":\n");
#undef PRINTBIT
	}
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
