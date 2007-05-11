/*	$NetBSD: pic_discovery.c,v 1.1.2.1 2007/05/11 05:51:48 matt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
 * All rights reserved.
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
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * extintr.c - external interrupt management for discovery
 *
 *	Interrupts are software-prioritized and preempting,
 *	they are only actually masked when pending.
 *	this allows avoiding slow, off-CPU mask reprogramming for spl/splx.
 *	When a lower priority interrupt preempts a high one,
 *	it is pended and masked.  Masks are re-enabled after service.
 *
 *	`ci->ci_cpl'   is a "priority level" not a bitmask.
 *	`irq'   is a bit number in the 128 bit imask_t which reflects
 *		the GT-64260 Main Cause register pair (64 bits), and
 *		GPP Cause register (32 bits) interrupts.
 *
 *	Intra IPL dispatch order is defined in cause_irq()
 *
 *	Summary bits in cause registers are not valid IRQs
 *
 * 	Within a cause register bit vector ISRs are called in
 *	order of IRQ (descending).
 *
 *	When IRQs are shared, ISRs are called in order on the queue
 *	(which is arbitrarily first-established-first-served).
 *
 *	GT64260 GPP setup is for edge-triggered interrupts.
 *	We maintain a mask of level-triggered type IRQs
 *	and gather unlatched level from the GPP Value register.
 *
 *	Software interrupts are just like regular IRQs,
 *	they are established, pended, and dispatched exactly the
 *	same as HW interrupts.
 *	
 *	128 bit imask_t operations could be implemented with Altivec
 *	("vand", "vor", etc however no "vcntlzw" or "vffs"....)
 *
 * creation	Tue Feb  6 17:27:18 PST 2001	cliff
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_discovery.c,v 1.1.2.1 2007/05/11 05:51:48 matt Exp $");

#include "opt_marvell.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <machine/psl.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <powerpc/pic/picvar.h>
#ifdef KGDB
#include <machine/db_machdep.h>
#endif
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>

#include <uvm/uvm_extern.h>

#if defined(DEBUG) && defined(DDB)
#endif

#ifdef DEBUG
# define STATIC
int intrdebug = 0;
# define DPRINTF(x)		do { if (intrdebug) printf x ; } while (0)
# define DPRINTFN(n, x)		do { if (intrdebug > (n)) printf x ; } while (0)
#else
# define STATIC static
# define DPRINTF(x)
# define DPRINTFN(n, x)
#endif

#ifdef DIAGNOSTIC
# define DIAGPRF(x)		printf x
#else
# define DIAGPRF(x)
#endif

#define ILLEGAL_IRQ(x) (((x) < 0) || ((x) >= NIRQ) || \
		 ((1<<((x)&IMASK_BITMASK)) & imres.bits[(x)>>IMASK_WORDSHIFT]))

extern struct powerpc_bus_space gt_mem_bs_tag; 
extern bus_space_handle_t gt_memh; 

static const char intr_source_strings[NIRQ][16] = {
	"unknown 0",	"dev",		"dma",		"cpu",
	"idma 01",	"idma 23",	"idma 45",	"idma 67",
	"timer 01",	"timer 23",	"timer 45",	"timer 67",
	"pci0-0",	"pci0-1",	"pci0-2",	"pci0-3",
/*16*/	"pci1-0",	"ecc",		"pci1-1",	"pci1-2",
	"pci1-3",	"pci0-outl",	"pci0-outh",	"pci1-outl",
	"pci1-outh",	"unknown 25",	"pci0-inl",	"pci0-inh",
	"pci1-inl",	"pci1-inh",	"unknown 30",	"unknown 31",
/*32*/	"ether 0",	"ether 1",	"ether 2",	"unknown 35",
	"sdma",		"iic",		"unknown 38",	"brg",
	"mpsc 0",	"unknown 41",	"mpsc 1",	"comm",
	"unknown 44",	"unknown 45",	"unknown 46",	"unknown 47",
/*48*/	"unknown 48",	"unknown 49",	"unknown 50",	"unknown 51",
	"unknown 52",	"unknown 53",	"unknown 54",	"unknown 55",
	"gppsum 0",	"gppsum 1",	"gppsum 2",	"gppsum 3",
	"unknown 60",	"unknown 61",	"unknown 62",	"unknown 63",
};

struct discovery_pic_ops {
	struct pic_ops dsc_pic;
	bus_space_tag_t dsc_memt;
	bus_space_tag_t dsc_memh;
	uint32_t dsc_interrupt_mask[2];
	uint8_t dsc_priority[64];
	uint8_t dsc_maxpriority[64];
};

struct gpp_pic_ops {
	struct pic_ops gpp_pic;
	bus_space_tag_t gpp_memt;
	bus_space_handle_t gpp_memh;
	uint32_t gpp_interrupt_mask;
	uint8_t gpp_priority[32];
	uint8_t gpp_maxpriority[32];
};

static void
gpp_source_name(struct pic_ops *pic, int irq, char *name, size_t len)
{
	snprintf(name, len, "gpp %d", irq);
}

#define GPP_RES ~GT_MPP_INTERRUPTS	/* from config */

static int
gpp_get_irq(struct pic_ops *pic)
{
	struct gpppic_ops * const gpp = (void *)pic;
	uint32_t mask;
	int maybe_irq = -1;
	int maybe_priority = IPL_NONE;

#ifdef GPP_EDGE
	mask = bus_space_read_4(&gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Interrupt_Cause);
#else
	mask = bus_space_read_4(&gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Value);
#endif
	mask &= gpp->gpp_interrupt_mask;
	if (mask == 0)
		return NO_IRQ;

	while (mask != 0) {
		int irq = 32 - __builtin_clz(mask);
		if (gpp->gpp_priority[irq] > maybe_irq) {
			maybe_irq = irq;
			maybe_priority = gpp->gpp_priority[irq];
			if (maybe_priority > gpp->gpp_maxpriority[irq])
				break;
		}
		mask &= ~(1 << irq);
	}
	/*
	 * We now have the highest priority IRQ.
	 */
	KASSERT(maybe_irq >= 0);
#ifdef GPP_EDGE
	mask = 1 << maybe_irq;
	bus_space_write_4(&gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Interrupt_Cause, mask);
#endif

	return maybe_irq;
}

static void
gpp_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	struct gpppic_ops * const gpp = (void *)pic;
	const uint32_t mask = 1 << irq;

	KASSERT((unsigned) irq < 32);
#ifdef GPP_EDGE
	KASSERT(type == IST_EDGE);
#else
	KASSERT(type == IST_LEVEL);
#endif

	/*
	 * Set pin to input and active low.
	 */
	val = bus_space_read_4(gpp->gpp_memt, gpp->gpp_memh, GT_GPP_IO_Control);
	val &= ~mask;
	bus_space_write_4(gpp->gpp_memt, gpp->gpp_memh, GT_GPP_IO_Control, val);

	val = bus_space_read_4(gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Level_Control);
	val |= mask;
	bus_space_write_4(&gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Level_Control, val);

	gpp->gpp_priority[irq] = pri;

	/*
	 * recalculate the maxpriority of an interrupt.  This is highest
	 * priority interrupt from itself to gpp0.
	 */
	pri = IPL_NONE;
	for (i = 0; i < __arraycount(gpp->gpp_priority); i++) {
		if (gpp->gpp_priority[i] > pri)
			pri = gpp->gpp_priority[i];
		gpp->gpp_maxpriority[i] = pri;
	}
}

static void
gpp_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct gpppic_ops * const gpp = (void *)pic;
	const uint32_t mask = 1 << irq;

	KASSERT(type == IST_LEVEL);
	KASSERT(gpp->gpp_priority[irq] != IPL_NONE);
	gpp->gpp_interrupt_mask |= mask;
	bus_space_write_4(&gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Interrupt_Mask,
	    gpp->gpp_interrupt_mask);
}

static void
gpp_disable_irq(struct pic_ops *pic, int irq)
{
	struct gpppic_ops * const gpp = (void *)pic;
	const uint32_t mask = 1 << irq;

	gpp->gpp_interrupt_mask &= ~mask;
	bus_space_write_4(&gpp->gpp_memt, gpp->gpp_memh, GT_GPP_Interrupt_Mask,
	    gpp->gpp_interrupt_mask);
}

static void
gpp_ack_irq(struct pic_ops *pic, int irq)
{
}

static struct pic_ops *
gpp_pic_setup(bus_space_tag_t memt, bus_space_handle_t memh)
{
	struct gpppic_ops * gpp;
	uint32_t val;

	gpp = malloc(sizeof(*gpp), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (!gpp)
		panic("gpp_pic_setup: malloc(%zu) failed", sizeof(*gpp));

	gpp->gpp_memt = memt;
	gpp->gpp_memh = memh;
	gpp->gpp_pic.pic_get_irq = gpp_get_irq;
	gpp->gpp_pic.pic_enable_irq = gpp_enable_irq;
	gpp->gpp_pic.pic_reenable_irq = gpp_enable_irq;
	gpp->gpp_pic.pic_disable_irq = gpp_disable_irq;
	gpp->gpp_pic.pic_ack_irq = gpp_ack_irq;
	gpp->gpp_pic.pic_establish_irq = gpp_establish_irq;
	gpp->gpp_pic.pic_source_name = gpp_source_name;

	/*
	 * Force GPP interrupts to be level sensitive.
	 */
	val = bus_space_read_4(&gpp->gpp_memt, gpp->gpp_memh, 0xf300);
	bus_space_write_4(&gpp->gpp_memt, gpp->gpp_memh, 0xf300, val | 0x400);

	pic_add(&gpp->gpp_pic);

	return &gpp->gpp_pic;
}

static void
discovery_source_name(struct pic_ops *pic, int irq, char *name, size_t len)
{
	strlcpy(name, discovery_intr_source_strings[irq], len);
}

static int
discovery_get_irq(struct pic_ops *pic)
{
	struct discoverypic_ops * const dsc = (void *)pic;
	uint32_t mask;
	int irq_base = 0;
	int maybe_irq = -1;
	int maybe_priority = IPL_NONE;

	mask = bus_space_read_4(&dsc->dsc_memt, dsc->dsc_memh, ICR_CSC);
	if (!(mask & CSC_STAT))
		return NO_IRQ;
	irq_base = (mask & CSC_SEL) ? 32 : 0;
	mask &= dsc->dsc_interrupt_mask[(mask & CSC_SEL) ? 1 : 0];
	while (mask != 0) {
		int irq = 32 - __builtin_clz(mask);
		if (dsc->dsc_priority[irq_base + irq] > maybe_irq) {
			maybe_irq = irq_base + irq;
			maybe_priority = dsc->dsc_priority[irq_base + irq];
			if (maybe_priority > dsc->dsc_maxpriority[irq_base + irq])
				break;
		}
		mask &= ~(1 << irq);
	}
	/*
	 * We now have the highest priority IRQ (except it's cascaded).
	 */
	KASSERT(maybe_irq >= 0);
	return maybe_irq;
}

static void
discovery_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	struct discoverypic_ops * const dsc = (void *)pic;

	KASSERT((unsigned) irq < 32);
#ifdef GPP_EDGE
	KASSERT(type == IST_EDGE);
#else
	KASSERT(type == IST_LEVEL);
#endif

	dsc->dsc_priority[irq] = pri;

	/*
	 * recalculate the maxpriority of an interrupt.  This is highest
	 * priority interrupt from itself to irq 0.
	 */
	pri = IPL_NONE;
	for (i = 0; i < __arraycount(dsc->dsc_priority); i++) {
		if (dsc->dsc_priority[i] > pri)
			pri = dsc->dsc_priority[i];
		dsc->dsc_maxpriority[i] = pri;
	}
}

static void
discovery_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct discoverypic_ops * const dsc = (void *)pic;
	const uint32_t mask = 1 << (irq & 31);

	KASSERT(type == IST_LEVEL);
	KASSERT(dsc->dsc_priority[irq] != IPL_NONE);
	if (irq < 32) {
		dsc->dsc_interrupt_mask[0] |= mask;
		bus_space_write_4(&dsc->dsc_memt, dsc->dsc_memh,
		    ICR_MIC_LO, dsc->dsc_interrupt_mask[0]);
	} else {
		dsc->dsc_interrupt_mask[1] |= mask;
		bus_space_write_4(&dsc->dsc_memt, dsc->dsc_memh,
		    ICR_MIC_HI, dsc->dsc_interrupt_mask[1]);
	}
}

static void
discovery_disable_irq(struct pic_ops *pic, int irq)
{
	struct discoverypic_ops * const dsc = (void *)pic;
	const uint32_t mask = 1 << (irq & 31);

	if (irq < 32) {
		dsc->dsc_interrupt_mask[0] &= ~mask;
		bus_space_write_4(&dsc->dsc_memt, dsc->dsc_memh,
		    ICR_MIC_LO, dsc->dsc_interrupt_mask[0]);
	} else {
		dsc->dsc_interrupt_mask[1] &= ~mask;
		bus_space_write_4(&dsc->dsc_memt, dsc->dsc_memh,
		    ICR_MIC_HI, dsc->dsc_interrupt_mask[1]);
	}
}

static void
discovery_ack_irq(struct pic_ops *pic, int irq)
{
}

void
discoverypic_setup(bus_space_tag_t memt, bus_space_handle_t memh)
{
	struct discoverypic_ops *dsc;
	uint32_t val;

	dsc = malloc(sizeof(*dsc), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (!dsc)
		panic("dsc_pic_setup: malloc(%zu) failed", sizeof(*dsc));

	dsc->dsc_memt = memt;
	dsc->dsc_memh = memh;
	dsc->dsc_pic.pic_get_irq = dsc_get_irq;
	dsc->dsc_pic.pic_enable_irq = dsc_enable_irq;
	dsc->dsc_pic.pic_reenable_irq = dsc_enable_irq;
	dsc->dsc_pic.pic_disable_irq = dsc_disable_irq;
	dsc->dsc_pic.pic_ack_irq = dsc_ack_irq;
	dsc->dsc_pic.pic_establish_irq = dsc_establish_irq;
	dsc->dsc_pic.pic_source_name = dsc_source_name;

	pic_add(&dsc->dsc_pic);
	KASSERT(dsc->dsc_pic.pic_intrbase == 0);

	pic = dscpic_setup(memt, memh);
	intr_establish(dsc->dsc_pic.pic_intrbase + IRQ_GPP7_0,
	    IST_LEVEL, IPL_NONE, pic_handle_intr, pic);
	intr_establish(dsc->dsc_pic.pic_intrbase + IRQ_GPP15_8,
	    IST_LEVEL, IPL_NONE, pic_handle_intr, pic);
	intr_establish(dsc->dsc_pic.pic_intrbase + IRQ_GPP23_16,
	    IST_LEVEL, IPL_NONE, pic_handle_intr, pic);
	intr_establish(dsc->dsc_pic.pic_intrbase + IRQ_GPP31_24,
	    IST_LEVEL, IPL_NONE, pic_handle_intr, pic);
}
