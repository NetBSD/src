/*	$NetBSD: isa_machdep.c,v 1.14 2006/10/12 01:30:44 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.14 2006/10/12 01:30:44 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <machine/bus.h>
#include <machine/bus_private.h>

#include <machine/pio.h>
#include <machine/cpufunc.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <uvm/uvm_extern.h>

#include "ioapic.h"

#if NIOAPIC > 0
#include <machine/i82093var.h>
#include <machine/mpbiosvar.h>
#endif

static int _isa_dma_may_bounce(bus_dma_tag_t, bus_dmamap_t, int, int *);

struct x86_bus_dma_tag isa_bus_dma_tag = {
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
};

#define	IDTVEC(name)	__CONCAT(X,name)
typedef void (vector) __P((void));
extern vector *IDTVEC(intr)[];

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < NUM_LEGACY_IRQS && (x) != 2)

int
isa_intr_alloc(isa_chipset_tag_t ic __unused, int mask, int type, int *irq)
{
	int i, tmp, bestirq, count;
	struct intrhand **p, *q;
	struct intrsource *isp;
	struct cpu_info *ci;

	if (type == IST_NONE)
		panic("intr_alloc: bogus type");

	ci = &cpu_info_primary;

	bestirq = -1;
	count = -1;

	/* some interrupts should never be dynamically allocated */
	mask &= 0xdef8;

	/*
	 * XXX some interrupts will be used later (6 for fdc, 12 for pms).
	 * the right answer is to do "breadth-first" searching of devices.
	 */
	mask &= 0xefbf;

	simple_lock(&ci->ci_slock);

	for (i = 0; i < NUM_LEGACY_IRQS; i++) {
		if (LEGAL_IRQ(i) == 0 || (mask & (1<<i)) == 0)
			continue;
		isp = ci->ci_isources[i];
		if (isp == NULL) {
			/*
			 * if nothing's using the irq, just return it
			 */
			*irq = i;
			simple_unlock(&ci->ci_slock);
			return (0);
		}

		switch(isp->is_type) {
		case IST_EDGE:
		case IST_LEVEL:
			if (type != isp->is_type)
				continue;
			/*
			 * if the irq is shareable, count the number of other
			 * handlers, and if it's smaller than the last irq like
			 * this, remember it
			 *
			 * XXX We should probably also consider the
			 * interrupt level and stick IPL_TTY with other
			 * IPL_TTY, etc.
			 */
			for (p = &isp->is_handlers, tmp = 0; (q = *p) != NULL;
			     p = &q->ih_next, tmp++)
				;
			if ((bestirq == -1) || (count > tmp)) {
				bestirq = i;
				count = tmp;
			}
			break;

		case IST_PULSE:
			/* this just isn't shareable */
			continue;
		}
	}

	simple_unlock(&ci->ci_slock);

	if (bestirq == -1)
		return (1);

	*irq = bestirq;

	return (0);
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic __unused, int irq __unused)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
isa_intr_establish(
    isa_chipset_tag_t ic __unused,
    int irq,
    int type,
    int level,
    int (*ih_fun)(void *),
    void *ih_arg
)
{
	struct pic *pic;
	int pin;
#if NIOAPIC > 0
	int mpih;
#endif

	pin = irq;
	pic = &i8259_pic;

#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, irq, &mpih) == 0 ||
		    intr_find_mpmapping(mp_eisa_bus, irq, &mpih) == 0) {
			if (!APIC_IRQ_ISLEGACY(mpih)) {
				pin = APIC_IRQ_PIN(mpih);
				pic = (struct pic *)
				    ioapic_find(APIC_IRQ_APIC(mpih));
				if (pic == NULL) {
					printf("isa_intr_establish: "
					       "unknown apic %d\n",
					    APIC_IRQ_APIC(mpih));
					return NULL;
				}
			}
		} else
			printf("isa_intr_establish: no MP mapping found\n");
	}
#endif
	return intr_establish(irq, pic, pin, type, level, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(isa_chipset_tag_t ic __unused, void *arg)
{
	struct intrhand *ih = arg;

	if (!LEGAL_IRQ(ih->ih_pin))
		panic("intr_disestablish: bogus irq");

	intr_disestablish(ih);
}

void
isa_attach_hook(struct device *parent __unused, struct device *self __unused,
    struct isabus_attach_args *iba)
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
isa_mem_alloc(t, size, align, boundary, flags, addrp, bshp)
	bus_space_tag_t t;
	bus_size_t size, align;
	bus_addr_t boundary;
	int flags;
	bus_addr_t *addrp;
	bus_space_handle_t *bshp;
{

	/*
	 * Allocate physical address space in the ISA hole.
	 */
	return (bus_space_alloc(t, IOM_BEGIN, IOM_END - 1, size, align,
	    boundary, flags, addrp, bshp));
}

void
isa_mem_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
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
_isa_dma_may_bounce(bus_dma_tag_t t __unused, bus_dmamap_t map, int flags,
    int *cookieflagsp)
{
	if ((flags & ISABUS_DMA_32BIT) != 0)
		map->_dm_bounce_thresh = 0;

	if (((map->_dm_size / PAGE_SIZE) + 1) > map->_dm_segcnt)
		*cookieflagsp |= X86_DMA_MIGHT_NEED_BOUNCE;
	return 0;
}
