/*	$NetBSD: dic.c,v 1.1.6.2 2011/06/06 09:05:05 jruoho Exp $ */

/*
 * Copyright (c) 2010, 2011 Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dic.c,v 1.1.6.2 2011/06/06 09:05:05 jruoho Exp $");

#define	_INTR_PRIVATE	/* for arm/pic/picvar.h */

#include "locators.h"
#include "opt_dic.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>
#include <sys/atomic.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/pic/picvar.h>

#include <arm/mpcore/mpcorevar.h>
#include <arm/mpcore/mpcorereg.h>
#include <arm/mpcore/dicreg.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

/*
 * 0 is the highest priority.
 */
#define	HW_TO_SW_IPL(ipl)	(IPL_HIGH - (ipl))
#define	SW_TO_HW_IPL(ipl)	(IPL_HIGH - (ipl))

struct dic_softc {
	device_t sc_dev;
	struct pic_softc sc_pic;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_cii_ioh;
	volatile uint32_t *sc_cii_vaddr;	/* CPU interface */
	bus_space_handle_t sc_gid_ioh;
	volatile uint32_t *sc_gid_vaddr;	/* Global distributor */
	int sc_nsrcs;
//	uint32_t sc_enabled_mask[4];
};

#define	PIC_TO_SOFTC(pic) \
	((struct dic_softc *)((char *)(pic) - \
		offsetof(struct dic_softc, sc_pic)))


static int dic_match(device_t, cfdata_t, void *);
static void dic_attach(device_t, device_t, void *);

static void dic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void dic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void dic_establish_irq(struct pic_softc *, struct intrsource *);

#define	DIC_READ(sc, offset)	\
	(*((sc)->sc_gid_vaddr + (offset) / sizeof (uint32_t)))
#define	DIC_WRITE(sc, offset, val)					\
	(*((sc)->sc_gid_vaddr + (offset) / sizeof (uint32_t)) = (val))

#define	CII_READ(sc, offset)	\
	(*((sc)->sc_cii_vaddr + (offset) / sizeof (uint32_t)))
#define	CII_WRITE(sc, offset, val)					\
	(*((sc)->sc_cii_vaddr + (offset) / sizeof (uint32_t)) = (val))

const struct pic_ops dic_pic_ops = {
	.pic_unblock_irqs = dic_unblock_irqs,
	.pic_block_irqs = dic_block_irqs,
	.pic_establish_irq = dic_establish_irq,
	.pic_source_name = NULL
};


CFATTACH_DECL_NEW(dic, sizeof(struct dic_softc),
    dic_match, dic_attach, NULL, NULL);

struct dic_softc *dic_softc;

static int
dic_match(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(cf->cf_name, "dic") == 0)
		return 1;

	return 0;
}


static void
dic_attach(device_t parent, device_t self, void *aux)
{
	struct dic_softc *dic = device_private(self);
	struct pmr_attach_args * const pa = aux;
	uint32_t typ;

	aprint_normal(": Distributed Interrupt Controller\n");
	aprint_naive("\n");

	dic->sc_dev = self;
	dic->sc_iot = pa->pa_iot;

	dic_softc = dic;

	if (bus_space_subregion(dic->sc_iot, pa->pa_ioh, 
		MPCORE_PMR_CII, MPCORE_PMR_CII_SIZE,
		&dic->sc_cii_ioh) ||
	    bus_space_subregion(dic->sc_iot, pa->pa_ioh, 
		MPCORE_PMR_GID, MPCORE_PMR_GID_SIZE,
		&dic->sc_gid_ioh)) {

		aprint_error_dev(self, "can't subregion\n");
		return;
	}

	dic->sc_cii_vaddr = bus_space_vaddr(dic->sc_iot, dic->sc_cii_ioh);
	dic->sc_gid_vaddr = bus_space_vaddr(dic->sc_iot, dic->sc_gid_ioh);

	typ = DIC_READ(dic, DIC_TYPE);

	dic->sc_nsrcs =
	    32 * (1 +
		((typ & DIC_TYPE_NLINES_MASK) >> DIC_TYPE_NLINES_SHIFT));
	
	aprint_normal_dev(self, "%d CPUs, %d interrupt sources\n",
	    1 + (u_int)((typ & DIC_TYPE_NCPUS_MASK) >> DIC_TYPE_NCPUS_SHIFT),
	    dic->sc_nsrcs);


	DIC_WRITE(dic, DIC_CONTROL, DIC_CONTROL_ENABLE);

	CII_WRITE(dic, CII_CONTROL, CII_CONTROL_ENABLE);

	dic->sc_pic.pic_ops = &dic_pic_ops;
	dic->sc_pic.pic_maxsources = dic->sc_nsrcs;
	strlcpy(dic->sc_pic.pic_name, device_xname(self),
	    sizeof(dic->sc_pic.pic_name));

	pic_add(&dic->sc_pic, 0);

	enable_interrupts(I32_bit|F32_bit);
}

void
dic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct dic_softc * const dic = PIC_TO_SOFTC(pic);
	size_t group = irq_base / 32;

	DIC_WRITE(dic, DIC_ENSET(group), irq_mask);
}

void
dic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct dic_softc * const dic = PIC_TO_SOFTC(pic);
	size_t group = irq_base / 32;

	DIC_WRITE(dic, DIC_ENCLEAR(group), irq_mask);
}

static __inline u_int
my_core_id(void)
{
	uint32_t id;

	__asm ("mrc p15, 0, %0, c0, c0, 5" : "=r" (id));

	return id & 0x0f;
}

static void
dic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct dic_softc * const dic = PIC_TO_SOFTC(pic);
	int irq = is->is_irq;
	int shift;
	int group;
	uint32_t reg;

	KASSERT(irq < dic->sc_nsrcs);
	KASSERT(is->is_ipl < 16);

#ifdef	NO_DIC_INITIALIZE
	/*
	 * DIC is configured by the firmware.
	 * don't change the settings.
	 */
#else

	group = irq / 4;
	shift = (irq % 4) * 8 + 4;
	reg = DIC_READ(dic, DIC_PRIORITY(group));
	reg &= ~(0xf << shift);
	reg |= SW_TO_HW_IPL(is->is_ipl) << shift;
	DIC_WRITE(dic, DIC_PRIORITY(group), reg);

	/* edge or level triggered.
	 * always use 1-N interrupt software model.
	 * XXX: limited to high-level or rising-edege trigger.
	 */
	shift = (irq % 16) * 2;
	group = (irq / 16);
	reg = DIC_READ(dic, DIC_CONFIG(group));
	reg &= ~(0x03 << shift);
	if (is->is_type == IST_EDGE)
		reg |= 0x01 << shift;
	else
		reg |= 0x03 << shift;
	DIC_WRITE(dic, DIC_CONFIG(group), reg);

	group = irq / 4;
	shift = (irq % 4) * 8;

	reg = DIC_READ(dic, DIC_TARGET(group));
	reg &= ~(0x0f << shift);
#ifdef	MULTIPROCESSOR
#error	not yet.
#else
	reg |= 1 << (my_core_id() + shift);
#endif
	DIC_WRITE(dic, DIC_TARGET(group), reg);

#endif	/* NO_DIC_INITIALIZE */

	/* enable the interrupt */
	group = irq / 32;
	DIC_WRITE(dic, DIC_ENSET(group), 1 << (irq % 32));
}

void
mpcore_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	int irq;
	uint32_t reg, intack;

	ci->ci_data.cpu_nintr++;

	for (;;) {
		struct intrsource *is;
		
		intack = CII_READ(dic_softc, CII_INTACK);
		irq = intack & CII_INTACK_INTID_MASK;

		if (irq == 1023)	/* spurious */
			break;

		reg = CII_READ(dic_softc, CII_RUNPRI);
		CII_WRITE(dic_softc, CII_PRIMASK, reg);
		
		is = dic_softc->sc_pic.pic_sources[irq];
		if (__predict_true(is != NULL)) {
			int oldipl = ci->ci_cpl;
			ci->ci_cpl = HW_TO_SW_IPL((reg & CII_PRIMASK_MASK)
			    >> CII_PRIMASK_SHIFT);

			cpsie(I32_bit);
			pic_dispatch(is, frame);
			cpsid(I32_bit);

			ci->ci_cpl = oldipl;
			CII_WRITE(dic_softc, CII_PRIMASK, 
			    SW_TO_HW_IPL(oldipl) << CII_PRIMASK_SHIFT);
		}
		CII_WRITE(dic_softc, CII_EOI, intack);
	}


#ifdef	DIC_CASCADED_IRQ
	/* handle cascaded interrupts through PIC framework */
	pic_do_pending_ints(I32_bit, ci->ci_cpl, frame);
#endif
}


int
_splraise(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(newipl < NIPL);
	if (newipl > ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		ci->ci_cpl = newipl;
		CII_WRITE(dic_softc, CII_PRIMASK, 
		    SW_TO_HW_IPL(newipl) << CII_PRIMASK_SHIFT);
		restore_interrupts(psw);
	}

	return oldipl;
}
int
_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(panicstr || newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		CII_WRITE(dic_softc, CII_PRIMASK, 
		    SW_TO_HW_IPL(newipl) << CII_PRIMASK_SHIFT);
		pic_do_pending_ints(psw, newipl, NULL);
		restore_interrupts(psw);
	}
	return oldipl;
}

void
splx(int savedipl)
{
	struct cpu_info * const ci = curcpu();
	KASSERT(savedipl < NIPL);
	if (savedipl < ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		CII_WRITE(dic_softc, CII_PRIMASK, 
		    SW_TO_HW_IPL(savedipl) << CII_PRIMASK_SHIFT);
#ifdef	DIC_CASCADED_IRQ
		pic_do_pending_ints(psw, savedipl, NULL);
#endif
		restore_interrupts(psw);
	}
	ci->ci_cpl = savedipl;
}
