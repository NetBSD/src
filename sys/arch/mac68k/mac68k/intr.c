/*	$NetBSD: intr.c,v 1.5 1999/06/28 01:56:57 briggs Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Link and dispatch interrupts.
 */

#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/vmmeter.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#define	NISR	8
#define	ISRLOC	0x18

static int intr_noint __P((void *));

static int ((*intr_func[NISR]) __P((void *))) = {
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
u_short	mac68k_ttyipl;
u_short	mac68k_bioipl;
u_short	mac68k_netipl;
u_short	mac68k_impipl;
u_short	mac68k_audioipl;
u_short	mac68k_clockipl;
u_short	mac68k_statclockipl;
u_short	mac68k_schedipl;

extern	int intrcnt[];		/* from locore.s */

void	intr_computeipl __P((void));

#define MAX_INAME_LENGTH 53
#define STD_INAMES \
	"spur\0via1\0via2\0unused1\0scc\0unused2\0unused3\0nmi\0clock\0"
#define AUX_INAMES \
	"spur\0soft\0via2\0ethernet\0scc\0sound\0via1\0nmi\0clock\0    "
#define AV_INAMES \
	"spur\0via1\0via2\0ethernet\0scc\0dsp\0unused1\0nmi\0clock\0   "

void
intr_init()
{
	extern long	intrnames;
	char		*inames, *g_inames;

	g_inames = (char *) &intrnames;
	if (mac68k_machine.aux_interrupts) {

		inames = AUX_INAMES;

		/* Standard spl(9) interrupt priorities */
		mac68k_ttyipl = (PSL_S | PSL_IPL1);
		mac68k_bioipl = (PSL_S | PSL_IPL2);
		mac68k_netipl = (PSL_S | PSL_IPL3);
		mac68k_impipl = (PSL_S | PSL_IPL6);
		mac68k_statclockipl = (PSL_S | PSL_IPL6);
		mac68k_clockipl = (PSL_S | PSL_IPL6);
		mac68k_schedipl = (PSL_S | PSL_IPL4);

		/* Non-standard interrupt priority */
		mac68k_audioipl = (PSL_S | PSL_IPL5);

	} else {
		inames = STD_INAMES;

		/* Standard spl(9) interrupt priorities */
		mac68k_ttyipl = (PSL_S | PSL_IPL1);
		mac68k_bioipl = (PSL_S | PSL_IPL2);
		mac68k_netipl = (PSL_S | PSL_IPL2);
		mac68k_impipl = (PSL_S | PSL_IPL2);
		mac68k_statclockipl = (PSL_S | PSL_IPL2);
		mac68k_clockipl = (PSL_S | PSL_IPL2);
		mac68k_schedipl = (PSL_S | PSL_IPL3);

		/* Non-standard interrupt priority */
		mac68k_audioipl = (PSL_S | PSL_IPL2);

		if (current_mac_model->class == MACH_CLASSAV) {
			inames = AV_INAMES;
			mac68k_bioipl = mac68k_netipl = (PSL_S | PSL_IPL4);
		}
	}

	memcpy(g_inames, inames, MAX_INAME_LENGTH);

	intr_computeipl();
}


/*
 * Compute the interrupt levels for the spl*()
 * calls.  This doesn't have to be fast.
 */
void
intr_computeipl()
{
	/*
	 * Enforce `bio <= net <= tty <= imp <= statclock <= clock <= sched'
	 * as defined in spl(9)
	 */
	if (mac68k_bioipl > mac68k_netipl)
		mac68k_netipl = mac68k_bioipl;

	if (mac68k_netipl > mac68k_ttyipl)
		mac68k_ttyipl = mac68k_netipl;

	if (mac68k_ttyipl > mac68k_impipl)
		mac68k_impipl = mac68k_ttyipl;

	if (mac68k_impipl > mac68k_statclockipl)
		mac68k_statclockipl = mac68k_impipl;

	if (mac68k_statclockipl > mac68k_clockipl)
		mac68k_clockipl = mac68k_statclockipl;

	if (mac68k_clockipl > mac68k_schedipl)
		mac68k_schedipl = mac68k_clockipl;
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
intr_establish(func, arg, ipl)
	int (*func) __P((void *));
	void *arg;
	int ipl;
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
intr_disestablish(ipl)
	int ipl;
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
void
intr_dispatch(evec)
	int evec;		/* format | vector offset */
{
	int ipl, vec;

	vec = (evec & 0xfff) >> 2;
#ifdef DIAGNOSTIC
	if ((vec < ISRLOC) || (vec >= (ISRLOC + NISR)))
		panic("intr_dispatch: bad vec 0x%x\n", vec);
#endif
	ipl = vec - ISRLOC;

	intrcnt[ipl]++;
	uvmexp.intrs++;

	(void)(*intr_func[ipl])(intr_arg[ipl]);
}

/*
 * Default interrupt handler:  do nothing.
 */
static int
intr_noint(arg)
	void *arg;
{
#ifdef DEBUG
	if (intr_debug)
		printf("intr_noint: ipl %d\n", (int)arg);
#endif
	return 0;
}

/*
 * XXX Why on earth isn't this in a common file?!
 */
void	netintr __P((void));
void	arpintr __P((void));
void	atintr __P((void));
void	ipintr __P((void));
void	nsintr __P((void));
void	clnlintr __P((void));
void	ccittintr __P((void));
void	pppintr __P((void));

void
netintr()
{
	int s, isr;

	for (;;) {
		s = splimp();
		isr = netisr;
		netisr = 0;
		splx(s);

		if (isr == 0)
			return;
#ifdef INET
#include "arp.h"
#if NARP > 0
		if (isr & (1 << NETISR_ARP))
			arpintr();
#endif
		if (isr & (1 << NETISR_IP))
			ipintr();
#endif
#ifdef NETATALK
		if (isr & (1 << NETISR_ATALK))
			atintr();
#endif
#ifdef NS
		if (isr & (1 << NETISR_NS))
			nsintr();
#endif
#ifdef ISO
		if (isr & (1 << NETISR_ISO))
			clnlintr();
#endif
#ifdef CCITT
		if (isr & (1 << NETISR_CCITT))
			ccittintr();
#endif
#include "ppp.h"
#if NPPP > 0
		if (isr & (1 << NETISR_PPP))
			pppintr();
#endif
	}
}
