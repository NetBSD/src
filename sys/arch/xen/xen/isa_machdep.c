/*	$NetBSD: isa_machdep.c,v 1.13.2.1 2009/05/13 17:18:50 jym Exp $	*/
/*	NetBSD isa_machdep.c,v 1.11 2004/06/20 18:04:08 thorpej Exp 	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.13.2.1 2009/05/13 17:18:50 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <machine/bus.h>
#include <machine/bus_private.h>

#include <machine/pio.h>
#include <machine/cpufunc.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <uvm/uvm_extern.h>

#ifdef XEN3
#include "ioapic.h"
#endif

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpconfig.h>
#endif

static int _isa_dma_may_bounce(bus_dma_tag_t, bus_dmamap_t, int, int *);

struct x86_bus_dma_tag isa_bus_dma_tag = {
	0,				/* _tag_needs_free */
	ISA_DMA_BOUNCE_THRESHOLD,	/* _bounce_thresh */
	0,				/* _bounce_alloc_lo */
	ISA_DMA_BOUNCE_THRESHOLD,	/* _bounce_alloc_hi */
	_isa_dma_may_bounce,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmatag_subregion,
	_bus_dmatag_destroy,
};

#define	IDTVEC(name)	__CONCAT(X,name)
typedef void (vector)(void);
extern vector *IDTVEC(intr)[];

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < NUM_LEGACY_IRQS && (x) != 2)

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	panic("isa_intr_alloc: notyet");
	return (0);
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
isa_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level,
	int (*ih_fun)(void *), void *ih_arg)
{
	int evtch;
	char evname[8];
	struct xen_intr_handle ih;
#if NIOAPIC > 0
	struct ioapic_softc *pic = NULL;
#endif

	ih.pirq = 0;

#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, irq, &ih) == 0 ||
		    intr_find_mpmapping(mp_eisa_bus, irq, &ih) == 0) {
			if (!APIC_IRQ_ISLEGACY(ih.pirq)) {
				pic = ioapic_find(APIC_IRQ_APIC(ih.pirq));
				if (pic == NULL) {
					printf("isa_intr_establish: "
					    "unknown apic %d\n",
					    APIC_IRQ_APIC(ih.pirq));
					return NULL;
				}
			}
		} else
			printf("isa_intr_establish: no MP mapping found\n");
	}
#endif
	ih.pirq |= (irq & 0xff);

	evtch = xen_intr_map(&ih.pirq, type);
	if (evtch == -1)
		return NULL;
#if NIOAPIC > 0
	if (pic)
		snprintf(evname, sizeof(evname), "%s pin %d",
		    device_xname(pic->sc_dev), APIC_IRQ_PIN(ih.pirq));
	else
#endif
		snprintf(evname, sizeof(evname), "irq%d", irq);

	return (void *)pirq_establish(irq, evtch, ih_fun, ih_arg, level,
	    evname);
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(isa_chipset_tag_t ic, void *arg)
{
	//XXX intr_disestablish(ih);
}

void
isa_attach_hook(device_t parent, device_t self, struct isabus_attach_args *iba)
{
	extern struct x86_isa_chipset x86_isa_chipset;
	extern int isa_has_been_seen;

	/*
	 * Notify others that might need to know that the ISA bus
	 * has now been attached.
	 */
	if (isa_has_been_seen)
		panic("isaattach: ISA bus already seen!");
	isa_has_been_seen = 1;

	/*
	 * Since we can only have one ISA bus, we just use a single
	 * statically allocated ISA chipset structure.  Pass it up
	 * now.
	 */
	iba->iba_ic = &x86_isa_chipset;
}

int
isa_mem_alloc(bus_space_tag_t t, bus_size_t size, bus_size_t align, bus_addr_t boundary, int flags, bus_addr_t *addrp, bus_space_handle_t *bshp)
{

	/*
	 * Allocate physical address space in the ISA hole.
	 */
	return (bus_space_alloc(t, IOM_BEGIN, IOM_END - 1, size, align,
	    boundary, flags, addrp, bshp));
}

void
isa_mem_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	bus_space_free(t, bsh, size);
}

/*
 * ISA only has 24-bits of address space.  This means
 * we can't DMA to pages over 16M.  In order to DMA to
 * arbitrary buffers, we use "bounce buffers" - pages
 * in memory below the 16M boundary.  On DMA reads,
 * DMA happens to the bounce buffers, and is copied into
 * the caller's buffer.  On writes, data is copied into
 * but bounce buffer, and the DMA happens from those
 * pages.  To software using the DMA mapping interface,
 * this looks simply like a data cache.
 *
 * If we have more than 16M of RAM in the system, we may
 * need bounce buffers.  We check and remember that here.
 *
 * There are exceptions, however.  VLB devices can do
 * 32-bit DMA, and indicate that here.
 *
 * ...or, there is an opposite case.  The most segments
 * a transfer will require is (maxxfer / PAGE_SIZE) + 1.  If
 * the caller can't handle that many segments (e.g. the
 * ISA DMA controller), we may have to bounce it as well.
 */
static int
_isa_dma_may_bounce(bus_dma_tag_t t, bus_dmamap_t map, int flags,
		    int *cookieflagsp)
{
	if ((flags & ISABUS_DMA_32BIT) != 0)
		map->_dm_bounce_thresh = 0;

	if (((map->_dm_size / PAGE_SIZE) + 1) > map->_dm_segcnt)
		*cookieflagsp |= X86_DMA_MIGHT_NEED_BOUNCE;
	return 0;
}
