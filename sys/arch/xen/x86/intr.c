/*	NetBSD: intr.c,v 1.15 2004/04/10 14:49:55 kochi Exp 	*/

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *     @(#)isa.c       7.2 (Berkeley) 5/13/91
 */
/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
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
 *     This product includes software developed by the University of
 *     California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *     @(#)isa.c       7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.7.6.1 2006/04/22 11:38:11 simonb Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/cdefs.h>
#include <sys/param.h> 
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/errno.h>

#include <machine/atomic.h>
#include <machine/i8259.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/evtchn.h>

#ifdef XEN3
#include "acpi.h"
#if NACPI > 0
/* for x86/i8259.c */
struct intrstub i8259_stubs[NUM_LEGACY_IRQS] = {{0}};
#endif
#endif

/*
 * Recalculate the interrupt from scratch for an event source.
 */
void
intr_calculatemasks(struct evtsource *evts)
{
	struct intrhand *ih;

	evts->ev_maxlevel = IPL_NONE;
	evts->ev_imask = 0;
	for (ih = evts->ev_handlers; ih != NULL; ih = ih->ih_evt_next) {
		if (ih->ih_level > evts->ev_maxlevel)
			evts->ev_maxlevel = ih->ih_level;
		evts->ev_imask |= (1 << ih->ih_level);
	}

}

/*
 * Fake interrupt handler structures for the benefit of symmetry with
 * other interrupt sources.
 */
struct intrhand fake_softclock_intrhand;
struct intrhand fake_softnet_intrhand;
struct intrhand fake_softserial_intrhand;
struct intrhand fake_timer_intrhand;
struct intrhand fake_ipi_intrhand;
#if defined(DOM0OPS)
struct intrhand fake_softxenevt_intrhand;

extern void Xsoftxenevt(void);
#endif

/*
 * Event counters for the software interrupts.
 */
struct evcnt softclock_evtcnt;
struct evcnt softnet_evtcnt;
struct evcnt softserial_evtcnt;
#if defined(DOM0OPS)
struct evcnt softxenevt_evtcnt;
#endif

/*
 * Initialize all handlers that aren't dynamically allocated, and exist
 * for each CPU. Also init ci_iunmask[].
 */
void
cpu_intr_init(struct cpu_info *ci)
{
	struct iplsource *ipl;
	int i;

	ci->ci_iunmask[0] = 0xfffffffe;
	for (i = 1; i < NIPL; i++)
		ci->ci_iunmask[i] = ci->ci_iunmask[i - 1] & ~(1 << i);

	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftclock;
	ipl->ipl_resume = Xsoftclock;
	fake_softclock_intrhand.ih_level = IPL_SOFTCLOCK;
	ipl->ipl_handlers = &fake_softclock_intrhand;
	ci->ci_isources[SIR_CLOCK] = ipl;
	evcnt_attach_dynamic(&softclock_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "softclock");

	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftnet;
	ipl->ipl_resume = Xsoftnet;
	fake_softnet_intrhand.ih_level = IPL_SOFTNET;
	ipl->ipl_handlers = &fake_softnet_intrhand;
	ci->ci_isources[SIR_NET] = ipl;
	evcnt_attach_dynamic(&softnet_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "softnet");

	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftserial;
	ipl->ipl_resume = Xsoftserial;
	fake_softserial_intrhand.ih_level = IPL_SOFTSERIAL;
	ipl->ipl_handlers = &fake_softserial_intrhand;
	ci->ci_isources[SIR_SERIAL] = ipl;
	evcnt_attach_dynamic(&softserial_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "softserial");

#if defined(DOM0OPS)
	MALLOC(ipl, struct iplsource *, sizeof (struct iplsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ipl == NULL)
		panic("can't allocate fixed interrupt source");
	ipl->ipl_recurse = Xsoftxenevt;
	ipl->ipl_resume = Xsoftxenevt;
	fake_softxenevt_intrhand.ih_level = IPL_SOFTXENEVT;
	ipl->ipl_handlers = &fake_softxenevt_intrhand;
	ci->ci_isources[SIR_XENEVT] = ipl;
	evcnt_attach_dynamic(&softxenevt_evtcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "xenevt");
#endif /* defined(DOM0OPS) */
}

#if NPCI > 0 || NISA > 0
void *
intr_establish(int legacy_irq, struct pic *pic, int pin,
    int type, int level, int (*handler)(void *) , void *arg)
{
	struct pintrhand *ih;

	ih = pirq_establish(legacy_irq, bind_pirq_to_evtch(legacy_irq),
	    handler, arg, level);
	return ih;
}

void 
intr_disestablish(struct intrhand *ih)
{
	printf("intr_disestablish irq\n");
}
#endif


#ifdef INTRDEBUG
void
intr_printconfig(void)
{
	int i;
	struct intrhand *ih;
	struct intrsource *isp;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	for (CPU_INFO_FOREACH(cii, ci)) {
		printf("cpu%d: interrupt masks:\n", ci->ci_apicid);
		for (i = 0; i < NIPL; i++)
			printf("IPL %d mask %lx unmask %lx\n", i,
			    (u_long)ci->ci_imask[i], (u_long)ci->ci_iunmask[i]);
		simple_lock(&ci->ci_slock);
		for (i = 0; i < MAX_INTR_SOURCES; i++) {
			isp = ci->ci_isources[i];
			if (isp == NULL)
				continue;
			printf("cpu%u source %d is pin %d from pic %s maxlevel %d\n",
			    ci->ci_apicid, i, isp->is_pin,
			    isp->is_pic->pic_name, isp->is_maxlevel);
			for (ih = isp->is_handlers; ih != NULL;
			     ih = ih->ih_next)
				printf("\thandler %p level %d\n",
				    ih->ih_fun, ih->ih_level);

		}
		simple_unlock(&ci->ci_slock);
	}
}
#endif
