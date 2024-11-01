/*	$NetBSD: intr.c,v 1.4 2024/11/01 14:28:08 mlelstv Exp $	*/

/*-
 * Copyright (c) 1996, 2023 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.4 2024/11/01 14:28:08 mlelstv Exp $");

#define _VIRT68K_INTR_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/vmmeter.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <machine/vectors.h>

#include <uvm/uvm_extern.h>

#include <dev/goldfish/gfpicvar.h>

struct intrhand {
	LIST_ENTRY(intrhand)	ih_link;
	int			(*ih_func)(void *);
	void			*ih_arg;
	struct evcnt		*ih_evcnt;
	int			ih_irq;
};

static const char * const cpu_irq_group = "cpu irq";

static const char * const pic_irq_names[] = {
	"irq 1",  "irq 2",  "irq 3",  "irq 4",
	"irq 5",  "irq 6",  "irq 7",  "irq 8",
	"irq 9",  "irq 10", "irq 11", "irq 12",
	"irq 13", "irq 14", "irq 15", "irq 16",
	"irq 17", "irq 18", "irq 19", "irq 20",
	"irq 21", "irq 22", "irq 23", "irq 24",
	"irq 25", "irq 26", "irq 27", "irq 28",
	"irq 29", "irq 30", "irq 31", "irq 32",
};

#define	PIC_EVCNTS(g)							  \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[0]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[1]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[2]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[3]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[4]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[5]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[6]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[7]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[8]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[9]), \
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[10]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[11]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[12]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[13]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[14]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[15]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[16]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[17]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[18]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[19]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[20]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[21]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[22]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[23]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[24]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[25]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[26]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[27]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[28]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[29]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[30]),\
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, NULL, pic_irq_names[31])

static struct evcnt intr_evcnt[] = {
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "spur"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "lev1"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "lev2"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "lev3"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "lev4"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "lev5"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "lev6"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, cpu_irq_group, "nmi"),
	PIC_EVCNTS(pic_irq_groups[0]),
	PIC_EVCNTS(pic_irq_groups[1]),
	PIC_EVCNTS(pic_irq_groups[2]),
	PIC_EVCNTS(pic_irq_groups[3]),
	PIC_EVCNTS(pic_irq_groups[4]),
	PIC_EVCNTS(pic_irq_groups[5]),
};
__CTASSERT(__arraycount(intr_evcnt) == NIRQ);

#define	IHLIST_COUNT	 (NIRQ - IRQ_PIC_BASE)
#define	IRQ_TO_IHLIST(x) ((x) - IRQ_PIC_BASE)
#define	IRQ_TO_IPL(x)	 ((IRQ_TO_IHLIST(x) >> 5) + 1)
static LIST_HEAD(intrhand_list, intrhand) intrhands[IHLIST_COUNT];

volatile unsigned int	intr_depth;	/* used in locore.s */

static struct gfpic_softc *pics[NPIC];

static inline int
ipl_to_pic(const int ipl)
{
	if (__predict_true(ipl >= 1 && ipl <= 6)) {
		return (ipl - 1);
	}
	return -1;
}

/*
 * intr_init --
 *	Initialize the interrupt subsystem.
 */
void
intr_init(void)
{
	int i;

	for (i = 0; i < IHLIST_COUNT; i++) {
		LIST_INIT(&intrhands[i]);
	}

	/*
	 * Attach the CPU IRQ event counters first.  The
	 * PIC event counters will be attached as the PICs
	 * are registered with us.
	 */
	for (i = 0; i < IRQ_PIC_BASE; i++) {
		evcnt_attach_static(&intr_evcnt[i]);
	}
}

/*
 * intr_register_pic --
 *	Register a Goldfish PIC at the specified CPU IRQ.
 */
void
intr_register_pic(device_t dev, int ipl)
{
	const int idx = ipl_to_pic(ipl);

	KASSERT(idx >= 0);

	const int base = IRQ_PIC_BASE + (idx * NIRQ_PER_PIC);
	int pirq;

	KASSERT(pics[idx] == NULL);
	pics[idx] = device_private(dev);

	for (pirq = 0; pirq < NIRQ_PER_PIC; pirq++) {
		intr_evcnt[base + pirq].ev_group = device_xname(dev);
		evcnt_attach_static(&intr_evcnt[base + pirq]);
	}
}

/*
 * intr_establish --
 *	Establish an interrupt handler at the specified system IRQ.
 *	XXX We don't do anything with the flags yet.
 */
void *
intr_establish(int (*func)(void *), void *arg, int irq, int ipl,
    int flags __unused)
{
	struct intrhand *ih;
	int s;

	if (irq < IRQ_PIC_BASE || irq >= NIRQ) {
		return NULL;
	}
	if (ipl < IPL_BIO || ipl > IPL_SCHED) {
		return NULL;
	}

	const int pic = IRQ_TO_PIC(irq);
	if (pics[pic] == NULL) {
		return NULL;
	}

	ih = kmem_zalloc(sizeof(*ih), KM_SLEEP);
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_evcnt = &intr_evcnt[irq];

	s = splhigh();
	LIST_INSERT_HEAD(&intrhands[IRQ_TO_IHLIST(irq)], ih, ih_link);
	gfpic_enable(pics[pic], IRQ_TO_PIRQ(irq));
	splx(s);

	return ih;
}

/*
 * intr_disestablish --
 *	Remove an interrupt handler.
 */
void
intr_disestablish(void *v)
{
	struct intrhand *ih = v;
	const int pic = IRQ_TO_PIC(ih->ih_irq);
	int s;

	KASSERT(pics[pic] != NULL);

	s = splhigh();
	LIST_REMOVE(ih, ih_link);
	if (LIST_EMPTY(&intrhands[IRQ_TO_IHLIST(ih->ih_irq)])) {
		gfpic_disable(pics[pic], IRQ_TO_PIRQ(ih->ih_irq));
	}
	splx(s);
}

/*
 * intr_string --
 *	Return a string in the specified buffer describing the
 *	interrupt.
 */
const char *
intr_string(void *v, char *buf, size_t bufsize)
{
	struct intrhand *ih = v;
	const int ipl = IRQ_TO_IPL(ih->ih_irq);

	snprintf(buf, bufsize, "%s %s (IPL %d)", ih->ih_evcnt->ev_group,
	    ih->ih_evcnt->ev_name, ipl);
	return buf;
}

/* Auto-vectored interrupts start at vector 0x18. */
#define	VEC_AVINTR	0x18

void
intr_dispatch(struct clockframe frame)
{
	const int ipl = VECO_TO_VECI(frame.cf_vo) - VEC_AVINTR;
	const int pic = ipl_to_pic(ipl);
	int pirq;

	if (__predict_false(pic < 0)) {
		return;
	}

	if (pics[pic] == NULL) {
		printf("Interrupt without a cause on CPU ipl %d\n", ipl);
		return;
	}

	const int base = pic * NIRQ_PER_PIC;
	struct intrhand *ih;
	bool rv = false;

	while ((pirq = gfpic_pending(pics[pic])) >= 0) {
		LIST_FOREACH(ih, &intrhands[base + pirq], ih_link) {
			void *arg = ih->ih_arg ? ih->ih_arg : &frame;
			if (ih->ih_func(arg)) {
				ih->ih_evcnt->ev_count++;
				rv = true;
			}
		}
	}
	if (!rv) {
		printf("Spurious interrupt on CPU ipl %d\n", ipl);
	}
}

bool
cpu_intr_p(void)
{
	return intr_depth != 0;
}
