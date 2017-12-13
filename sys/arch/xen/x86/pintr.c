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
__KERNEL_RCSID(0, "$NetBSD: pintr.c,v 1.2 2017/12/13 16:30:18 bouyer Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/i8259.h>
#include <machine/pio.h>
#include <xen/evtchn.h>
#include <xen/intr.h>

#include "acpica.h"
#include "ioapic.h"
#include "opt_mpbios.h"

#if NIOAPIC > 0
/* XXX: todo - compat with lapic.c and XEN for x2apic */
bool x2apic_mode __read_mostly = false;
/* for x86/i8259.c */
struct intrstub i8259_stubs[NUM_LEGACY_IRQS] = {{0,0}};
/* for x86/ioapic.c */
struct intrstub ioapic_edge_stubs[MAX_INTR_SOURCES] = {{0,0}};
struct intrstub ioapic_level_stubs[MAX_INTR_SOURCES] = {{0,0}};
struct intrstub x2apic_edge_stubs[MAX_INTR_SOURCES] = {{0,0}};
struct intrstub x2apic_level_stubs[MAX_INTR_SOURCES] = {{0,0}};
#include <machine/i82093var.h>
int irq2port[NR_EVENT_CHANNELS] = {0}; /* actually port + 1, so that 0 is invaid */
int irq2vect[256] = {0};
int vect2irq[256] = {0};
#endif /* NIOAPIC */
#if NACPICA > 0
#include <machine/mpconfig.h>
#include <machine/mpacpi.h>
#endif
#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

#if NPCI > 0
#include <dev/pci/ppbreg.h>
#endif

#if defined(DOM0OPS) || NPCI > 0
int
xen_pirq_alloc(intr_handle_t *pirq, int type)
{
	physdev_op_t op;
	int irq = *pirq;
#if NIOAPIC > 0
	extern struct cpu_info phycpu_info_primary; /* XXX */
	/*
	 * The hypervisor has already allocated vectors and IRQs for the
	 * devices. Reusing the same IRQ doesn't work because as we bind
	 * them for each devices, we can't then change the route entry
	 * of the next device if this one used this IRQ. The easiest is
	 * to allocate IRQs top-down, starting with a high number.
	 * 250 and 230 have been tried, but got rejected by Xen.
	 *
	 * Xen 3.5 also rejects 200. Try out all values until Xen accepts
	 * or none is available.
	 */
	static int xen_next_irq = 200;
	struct ioapic_softc *ioapic = ioapic_find(APIC_IRQ_APIC(*pirq));
	struct pic *pic = &ioapic->sc_pic;
	int pin = APIC_IRQ_PIN(*pirq);

	if (*pirq & APIC_INT_VIA_APIC) {
		irq = vect2irq[ioapic->sc_pins[pin].ip_vector];
		if (ioapic->sc_pins[pin].ip_vector == 0 || irq == 0) {
			/* allocate IRQ */
			irq = APIC_IRQ_LEGACY_IRQ(*pirq);
			if (irq <= 0 || irq > 15)
				irq = xen_next_irq--;
retry:
			/* allocate vector and route interrupt */
			op.cmd = PHYSDEVOP_ASSIGN_VECTOR;
			op.u.irq_op.irq = irq;
			if (HYPERVISOR_physdev_op(&op) < 0) {
				irq = xen_next_irq--;
				if (xen_next_irq == 15)
					panic("PHYSDEVOP_ASSIGN_VECTOR irq %d", irq);
				goto retry;
			}
			KASSERT(irq2vect[irq] == 0);
			irq2vect[irq] = op.u.irq_op.vector;
			KASSERT(vect2irq[op.u.irq_op.vector] == 0);
			vect2irq[op.u.irq_op.vector] = irq;
			pic->pic_addroute(pic, &phycpu_info_primary, pin,
			    op.u.irq_op.vector, type);
		}
		*pirq &= ~0xff;
		*pirq |= irq;
	} else
#endif /* NIOAPIC */
	{
		if (irq2port[irq] == 0) {
			op.cmd = PHYSDEVOP_ASSIGN_VECTOR;
			op.u.irq_op.irq = irq;
			if (HYPERVISOR_physdev_op(&op) < 0) {
				panic("PHYSDEVOP_ASSIGN_VECTOR irq %d", irq);
			}
			KASSERT(irq2vect[irq] == 0);
			irq2vect[irq] = op.u.irq_op.vector;
			KASSERT(vect2irq[op.u.irq_op.vector] == 0);
			vect2irq[op.u.irq_op.vector] = irq;
			KASSERT(irq2port[irq] == 0);
			irq2port[irq] = bind_pirq_to_evtch(irq) + 1;
		}
	}
	KASSERT(irq2port[irq] > 0);
	return (irq2port[irq] - 1);
}
#endif /* defined(DOM0OPS) || NPCI > 0 */
