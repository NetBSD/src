/*	$NetBSD: intr.c,v 1.9 1998/07/05 04:37:36 jonathan Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_uvm.h"
#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_iso.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <vm/vm.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include <machine/irqhandler.h>
#include <machine/cpu.h>

#include <net/netisr.h>
#include <net/if.h>

#ifdef INET
#include <netinet/in.h>
#include "arp.h"
#if NARP > 0
#include <netinet/if_inarp.h>
#endif	/* NARP > 0 */
#include <netinet/ip_var.h>
#endif 	/* INET */
#ifdef NS
#include <netns/ns_var.h>
#endif	/* NS */
#ifdef ISO
#include <netiso/iso.h>
#include <netiso/clnp.h>
#endif	/* ISO */
#ifdef CCITT
#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_extern.h>
#endif	/* CCITT */
#ifdef NATM
#include <netnatm/natm.h>
#endif	/* NATM */
#ifdef NETATALK
#include <netatalk/at_extern.h>
#endif	/* NETATALK */
#include "ppp.h"
#if NPPP > 0
#include <net/ppp_defs.h>
#include <net/if_ppp.h>
#endif	/* NPPP > 0 */

extern int current_intr_depth;
extern u_int spl_mask;
extern u_int soft_interrupts;
#ifdef IRQSTATS
extern u_int intrcnt[];
#endif	/* IRQSTATS */

/* Eventually this will become macros */

void
setsoftintr(intrmask)
	u_int intrmask;
{
	atomic_set_bit(&soft_interrupts, intrmask);
}

void
setsoftclock()
{
	atomic_set_bit(&soft_interrupts, IRQMASK_SOFTCLOCK);
}

void
setsoftnet()
{
	atomic_set_bit(&soft_interrupts, IRQMASK_SOFTNET);
}

int astpending;

void
setsoftast()
{
	astpending = 1;
}

extern int want_resched;

void
need_resched(void)
{
	want_resched = 1;
	setsoftast();
}

/* Handle software interrupts */

void
dosoftints()
{
	register u_int softints;
	int s;

	softints = soft_interrupts & spl_mask;
	if (softints == 0) return;

	if (current_intr_depth > 1)
		return;

	s = splsoft();

	/*
	 * Software clock interrupts
	 */

	if (softints & IRQMASK_SOFTCLOCK) {
#if defined(UVM)
		++uvmexp.softs;
#else
  		++cnt.v_soft;
#endif
#ifdef IRQSTATS
		++intrcnt[IRQ_SOFTCLOCK];
#endif	/* IRQSTATS */
		atomic_clear_bit(&soft_interrupts, IRQMASK_SOFTCLOCK);
		softclock();
	}

	/*
	 * Network software interrupts
	 */

	if (softints & IRQMASK_SOFTNET) {
#if defined(UVM)
		++uvmexp.softs;
#else
  		++cnt.v_soft;
#endif
#ifdef IRQSTATS
		++intrcnt[IRQ_SOFTNET];
#endif	/* IRQSTATS */
		atomic_clear_bit(&soft_interrupts, IRQMASK_SOFTNET);

#ifdef INET
#if NARP > 0
		if (netisr & (1 << NETISR_ARP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_ARP));
			arpintr();
		}
#endif
		if (netisr & (1 << NETISR_IP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_IP));
			ipintr();
		}
#endif
#ifdef NETATALK
		if (netisr & (1 << NETISR_ATALK)) {
			atomic_clear_bit(&netisr, (1 << NETISR_ATALK));
			atintr();
		}
#endif
#ifdef NS
		if (netisr & (1 << NETISR_NS)) {
			atomic_clear_bit(&netisr, (1 << NETISR_NS));
			nsintr();
		}
#endif
#ifdef IMP
		if (netisr & (1 << NETISR_IMP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_IMP));
			impintr();
		}
#endif
#ifdef ISO
		if (netisr & (1 << NETISR_ISO)) {
			atomic_clear_bit(&netisr, (1 << NETISR_ISO));
			clnlintr();
		}
#endif
#ifdef CCITT
		if (netisr & (1 << NETISR_CCITT)) {
			atomic_clear_bit(&netisr, (1 << NETISR_CCITT));
			ccittintr();
		}
#endif
#ifdef NATM
		if (netisr & (1 << NETISR_NATM)) {
			atomic_clear_bit(&netisr, (1 << NETISR_NATM));
			natmintr();
		}
#endif
#if NPPP > 0
		if (netisr & (1 << NETISR_PPP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_PPP));
			pppintr();
		}
#endif
	}
	(void)lowerspl(s);
}

/* End of intr.c */
