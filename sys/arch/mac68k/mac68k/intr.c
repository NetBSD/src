/*	$NetBSD: intr.c,v 1.31 2020/07/21 06:10:26 rin Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Jason R. Thorpe.
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
 * Link and dispatch interrupts.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.31 2020/07/21 06:10:26 rin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/vmmeter.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/psc.h>
#include <machine/viareg.h>

#define	NISR	8
#define	ISRLOC	0x18

static int intr_noint(void *);

static int ((*intr_func[NISR])(void *)) = {
	intr_noint,
	intr_noint,
	intr_noint,
	intr_noint,
	intr_noint,
	intr_noint,
	intr_noint,
	intr_noint
};
static void *intr_arg[NISR] = {
	(void *)0,
	(void *)1,
	(void *)2,
	(void *)3,
	(void *)4,
	(void *)5,
	(void *)6,
	(void *)7
};

#ifdef DEBUG
int	intr_debug = 0;
#endif

/*
 * Some of the below are not used yet, but might be used someday on the
 * IIfx/Q700/900/950/etc. where the interrupt controller may be reprogrammed
 * to interrupt on different levels as listed in locore.s
 */
uint16_t ipl2psl_table[NIPL];
int idepth;
volatile int ssir;

extern	int intrcnt[];		/* from locore.s */

void	intr_computeipl(void);

#define MAX_INAME_LENGTH 53
#define STD_INAMES \
	"spur\0via1\0via2\0unused1\0scc\0unused2\0unused3\0nmi\0clock\0"
#define AUX_INAMES \
	"spur\0soft\0via2\0ethernet\0scc\0sound\0via1\0nmi\0clock\0    "
#define AV_INAMES \
	"spur\0via1\0via2\0ethernet\0scc\0dsp\0unused1\0nmi\0clock\0   "

void
intr_init(void)
{
	extern char	intrnames[MAX_INAME_LENGTH];
	extern char	eintrnames[] __diagused;
	const char	*inames;

	ipl2psl_table[IPL_NONE]       = 0;
	ipl2psl_table[IPL_SOFTCLOCK]  = PSL_S|PSL_IPL1;
	ipl2psl_table[IPL_SOFTNET]    = PSL_S|PSL_IPL1;
	ipl2psl_table[IPL_SOFTSERIAL] = PSL_S|PSL_IPL1;
	ipl2psl_table[IPL_SOFTBIO]    = PSL_S|PSL_IPL1;
	ipl2psl_table[IPL_HIGH]       = PSL_S|PSL_IPL7;

	if (mac68k_machine.aux_interrupts) {
		inames = AUX_INAMES;

		/* Standard spl(9) interrupt priorities */
		ipl2psl_table[IPL_VM]        = (PSL_S | PSL_IPL6);
		ipl2psl_table[IPL_SCHED]     = (PSL_S | PSL_IPL6);
	} else {
		inames = STD_INAMES;

		/* Standard spl(9) interrupt priorities */
		ipl2psl_table[IPL_VM]        = (PSL_S | PSL_IPL2);
		ipl2psl_table[IPL_SCHED]     = (PSL_S | PSL_IPL3);

		if (current_mac_model->class == MACH_CLASSAV) {
			inames = AV_INAMES;
			ipl2psl_table[IPL_VM]    = (PSL_S | PSL_IPL4);
			ipl2psl_table[IPL_SCHED] = (PSL_S | PSL_IPL4);
		}
	}

	KASSERT(MAX_INAME_LENGTH <=
		((uintptr_t)eintrnames - (uintptr_t)intrnames));
	memcpy(intrnames, inames, MAX_INAME_LENGTH);

	intr_computeipl();

	/* Initialize the VIAs */
	via_init();

	/* Initialize the PSC (if present) */
	psc_init();
}


/*
 * Compute the interrupt levels for the spl*()
 * calls.  This doesn't have to be fast.
 */
void
intr_computeipl(void)
{
	/*
	 * Enforce the following relationship, as defined in spl(9):
	 * `bio <= net <= tty <= vm <= statclock <= clock <= sched <= serial'
	 */
	if (ipl2psl_table[IPL_VM] > ipl2psl_table[IPL_SCHED])
		ipl2psl_table[IPL_SCHED] = ipl2psl_table[IPL_VM];

	if (ipl2psl_table[IPL_SCHED] > ipl2psl_table[IPL_HIGH])
		ipl2psl_table[IPL_HIGH] = ipl2psl_table[IPL_SCHED];
}

/*
 * Establish an autovectored interrupt handler.
 * Called by driver attach functions.
 *
 * XXX Warning!  DO NOT use Macintosh ROM traps from an interrupt handler
 * established by this routine, either directly or indirectly, without
 * properly saving and restoring all registers.  If not, chaos _will_
 * ensue!  (sar 19980806)
 */
void
intr_establish(int (*func)(void *), void *arg, int ipl)
{
	if ((ipl < 0) || (ipl >= NISR))
		panic("intr_establish: bad ipl %d", ipl);

#ifdef DIAGNOSTIC
	if (intr_func[ipl] != intr_noint)
		printf("intr_establish: attempt to share ipl %d\n", ipl);
#endif

	intr_func[ipl] = func;
	intr_arg[ipl] = arg;
}

/*
 * Disestablish an interrupt handler.
 */
void
intr_disestablish(int ipl)
{
	if ((ipl < 0) || (ipl >= NISR))
		panic("intr_disestablish: bad ipl %d", ipl);

	intr_func[ipl] = intr_noint;
	intr_arg[ipl] = (void *)ipl;
}

/*
 * This is the dispatcher called by the low-level
 * assembly language interrupt routine.
 *
 * XXX Note: see the warning in intr_establish()
 */
#if __GNUC_PREREQ__(8, 0)
/*
 * XXX rtclock_intr() requires this for unwinding stack frame.
 */
#pragma GCC push_options
#pragma GCC optimize "-fno-omit-frame-pointer"
#endif
void
intr_dispatch(int evec)		/* format | vector offset */
{
	int ipl, vec;

	idepth++;
	vec = (evec & 0xfff) >> 2;
#ifdef DIAGNOSTIC
	if ((vec < ISRLOC) || (vec >= (ISRLOC + NISR)))
		panic("intr_dispatch: bad vec 0x%x", vec);
#endif
	ipl = vec - ISRLOC;

	intrcnt[ipl]++;
	curcpu()->ci_data.cpu_nintr++;

	(void)(*intr_func[ipl])(intr_arg[ipl]);
	idepth--;
}
#if __GNUC_PREREQ__(8, 0)
#pragma GCC pop_options
#endif

/*
 * Default interrupt handler:  do nothing.
 */
static int
intr_noint(void *arg)
{
#ifdef DEBUG
	idepth++;
	if (intr_debug)
		printf("intr_noint: ipl %d\n", (int)arg);
	idepth--;
#endif
	return 0;
}

bool
cpu_intr_p(void)
{

	return idepth != 0;
}
