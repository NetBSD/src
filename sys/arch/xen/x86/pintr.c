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
__KERNEL_RCSID(0, "$NetBSD: pintr.c,v 1.18 2020/07/19 14:27:07 jdolecek Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "isa.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/i8259.h>
#include <machine/pio.h>
#include <xen/evtchn.h>
#include <xen/intr.h>

#ifdef __HAVE_PCI_MSI_MSIX
#include <x86/pci/msipic.h>
#endif

#include "acpica.h"
#include "ioapic.h"
#include "opt_mpbios.h"

#if NIOAPIC > 0
/* XXX: todo - compat with lapic.c and XEN for x2apic */
bool x2apic_mode __read_mostly = false;
/* for x86/i8259.c */
struct intrstub legacy_stubs[NUM_LEGACY_IRQS] = {{0,0,0}};
/* for x86/ioapic.c */
struct intrstub ioapic_edge_stubs[MAX_INTR_SOURCES] = {{0,0,0}};
struct intrstub ioapic_level_stubs[MAX_INTR_SOURCES] = {{0,0,0}};
struct intrstub x2apic_edge_stubs[MAX_INTR_SOURCES] = {{0,0,0}};
struct intrstub x2apic_level_stubs[MAX_INTR_SOURCES] = {{0,0,0}};
#include <machine/i82093var.h>
#endif /* NIOAPIC */

// XXX NR_EVENT_CHANNELS is 2048, use some sparse structure?
short irq2port[NR_EVENT_CHANNELS] = {0}; /* actually port + 1, so that 0 is invaid */

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

static int
xen_map_msi_pirq(struct pic *pic, int count, int *gsi)
{
	struct physdev_map_pirq map_irq;
	const struct msipic_pci_info *i = msipic_get_pci_info(pic);
	int ret;

	if (count == -1)
		count = i->mp_veccnt;
	KASSERT(count > 0);

	memset(&map_irq, 0, sizeof(map_irq));
	map_irq.domid = DOMID_SELF;
	map_irq.type = MAP_PIRQ_TYPE_MSI_SEG;
	map_irq.index = -1;
	map_irq.pirq = -1;
	map_irq.bus = i->mp_bus;
 	map_irq.devfn = (i->mp_dev << 3) | i->mp_fun;
	map_irq.entry_nr = count;
	if (pic->pic_type == PIC_MSI && i->mp_veccnt > 1) {
		map_irq.type = MAP_PIRQ_TYPE_MULTI_MSI;
	} else if (pic->pic_type == PIC_MSIX) {
		map_irq.table_base = i->mp_table_base;
	}

	ret = HYPERVISOR_physdev_op(PHYSDEVOP_map_pirq, &map_irq);

	if (ret == 0) {
		KASSERT(map_irq.entry_nr == count);
		*gsi = map_irq.pirq;
	}

	return ret;
}

/*
 * Check if we can map MSI interrupt. The Xen call fails if VT-d is not
 * available or disabled.
 */
int
xen_pci_msi_probe(struct pic *pic, int count)
{
	int pirq, ret;

	ret = xen_map_msi_pirq(pic, count, &pirq);

	if (ret == 0) {
		struct physdev_unmap_pirq unmap_irq;
		unmap_irq.domid = DOMID_SELF;
		unmap_irq.pirq = pirq;
		
		(void)HYPERVISOR_physdev_op(PHYSDEVOP_unmap_pirq, &unmap_irq);
	} else {
		aprint_debug("PHYSDEVOP_map_pirq() failed %d, MSI disabled\n",
		    ret);
	}

	return ret;
}

/*
 * This function doesn't "allocate" anything. It merely translates our
 * understanding of PIC to the XEN 'gsi' namespace. In the case of
 * MSIs, pirq == irq. In the case of everything else, the hypervisor
 * doesn't really care, so we just use the native conventions that
 * have been setup during boot by mpbios.c/mpacpi.c
 */
int
xen_pic_to_gsi(struct pic *pic, int pin)
{
	int ret;
	int gsi;

	KASSERT(pic != NULL);

	/*
	 * We assume that mpbios/mpacpi have done the right thing.
	 * If so, legacy_irq should identity map correctly to gsi.
	 */
	gsi = pic->pic_vecbase + pin;
	KASSERT(gsi < NR_EVENT_CHANNELS);

	switch (pic->pic_type) {
	case PIC_I8259:
		KASSERT(gsi < 16);
		/* FALLTHROUGH */
	case PIC_IOAPIC:
	    {
		KASSERT(gsi < 255);

		if (irq2port[gsi] != 0) {
			/* Already mapped the shared interrupt */
			break;
		}

		struct physdev_irq irq_op;
		memset(&irq_op, 0, sizeof(irq_op));
		irq_op.irq = gsi;
		ret = HYPERVISOR_physdev_op(PHYSDEVOP_alloc_irq_vector,
		    &irq_op);
		if (ret < 0) {
			panic("physdev_op(PHYSDEVOP_alloc_irq_vector) %d"
			    " fail %d", gsi, ret);
		}
		KASSERT(irq_op.vector == gsi);
		break;
	    }
	case PIC_MSI:
	case PIC_MSIX:
#ifdef __HAVE_PCI_MSI_MSIX
		ret = xen_map_msi_pirq(pic, -1, &gsi);
		if (ret != 0)
			panic("physdev_op(PHYSDEVOP_map_pirq) MSI fail %d",
			    ret);
		break;
#endif
	default:
		panic("unknown pic_type %d", pic->pic_type);
		break;
	}

	return gsi;
}


#endif /* defined(DOM0OPS) || NPCI > 0 */
