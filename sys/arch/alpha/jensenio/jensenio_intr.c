/* $NetBSD: jensenio_intr.c,v 1.18 2021/07/15 01:43:54 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: jensenio_intr.c,v 1.18 2021/07/15 01:43:54 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>

#include <dev/ic/i8259reg.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

static bus_space_tag_t pic_iot;
static bus_space_handle_t pic_ioh[2];
static bus_space_handle_t pic_elcr_ioh;

static int		jensenio_eisa_intr_map(void *, u_int,
			    eisa_intr_handle_t *);
static const char *	jensenio_eisa_intr_string(void *, int, char *, size_t);
static const struct evcnt *jensenio_eisa_intr_evcnt(void *, int);
static void *		jensenio_eisa_intr_establish(void *, int, int, int,
			    int (*)(void *), void *);
static void		jensenio_eisa_intr_disestablish(void *, void *);
static int		jensenio_eisa_intr_alloc(void *, int, int, int *);

#define	JENSEN_MAX_IRQ		16

static struct alpha_shared_intr *jensenio_eisa_intr;

static void	jensenio_iointr(void *, u_long);

static void	jensenio_enable_intr(int, int);
static void	jensenio_setlevel(int, int);
static void	jensenio_pic_init(void);

static const int jensenio_intr_deftype[JENSEN_MAX_IRQ] = {
	IST_EDGE,		/*  0: interval timer 0 output */
	IST_EDGE,		/*  1: line printer */
	IST_UNUSABLE,		/*  2: (cascade) */
	IST_NONE,		/*  3: EISA pin B25 */
	IST_NONE,		/*  4: EISA pin B24 */
	IST_NONE,		/*  5: EISA pin B23 */
	IST_NONE,		/*  6: EISA pin B22 (floppy) */
	IST_NONE,		/*  7: EISA pin B21 */
	IST_EDGE,		/*  8: RTC */
	IST_NONE,		/*  9: EISA pin B04 */
	IST_NONE,		/* 10: EISA pin D03 */
	IST_NONE,		/* 11: EISA pin D04 */
	IST_NONE,		/* 12: EISA pin D05 */
	IST_UNUSABLE,		/* 13: not connected */
	IST_NONE,		/* 14: EISA pin D07 (SCSI) */
	IST_NONE,		/* 15: EISA pin D06 */
};

static inline void
jensenio_specific_eoi(int irq)
{

	if (irq > 7) {
		bus_space_write_1(pic_iot, pic_ioh[1], PIC_OCW2,
		    OCW2_EOI | OCW2_SL | (irq & 0x07));
	}
	bus_space_write_1(pic_iot, pic_ioh[0], PIC_OCW2,
	    OCW2_EOI | OCW2_SL | (irq > 7 ? 2 : irq));
}

void
jensenio_intr_init(struct jensenio_config *jcp)
{
	eisa_chipset_tag_t ec = &jcp->jc_ec;
	isa_chipset_tag_t ic = &jcp->jc_ic;
	struct evcnt *ev;
	const char *cp;
	int i;

	pic_iot = &jcp->jc_eisa_iot;

	jensenio_pic_init();

	jensenio_eisa_intr = alpha_shared_intr_alloc(JENSEN_MAX_IRQ);
	for (i = 0; i < JENSEN_MAX_IRQ; i++) {
		alpha_shared_intr_set_dfltsharetype(jensenio_eisa_intr,
		    i, jensenio_intr_deftype[i]);

		ev = alpha_shared_intr_evcnt(jensenio_eisa_intr, i);
		cp = alpha_shared_intr_string(jensenio_eisa_intr, i);

		evcnt_attach_dynamic(ev, EVCNT_TYPE_INTR, NULL, "eisa", cp);
	}

	/*
	 * The cascasde interrupt must be edge triggered and always enabled.
	 */
	jensenio_setlevel(2, 0);
	jensenio_enable_intr(2, 1);

	/*
	 * Initialize the EISA chipset.
	 */
	ec->ec_v = jcp;
	ec->ec_intr_map = jensenio_eisa_intr_map;
	ec->ec_intr_string = jensenio_eisa_intr_string;
	ec->ec_intr_evcnt = jensenio_eisa_intr_evcnt;
	ec->ec_intr_establish = jensenio_eisa_intr_establish;
	ec->ec_intr_disestablish = jensenio_eisa_intr_disestablish;

	/*
	 * Initialize the ISA chipset.
	 */
	ic->ic_v = jcp;
	ic->ic_intr_establish = jensenio_eisa_intr_establish;
	ic->ic_intr_disestablish = jensenio_eisa_intr_disestablish;
	ic->ic_intr_alloc = jensenio_eisa_intr_alloc;
	ic->ic_intr_evcnt = jensenio_eisa_intr_evcnt;
}

static void
jensenio_intr_dispatch(void *arg, unsigned long vec)
{
	struct jensenio_scb_intrhand *jih = arg;

	jih->jih_evcnt.ev_count++;
	(void) jih->jih_func(jih->jih_arg);
}

static void
jensenio_intr_dispatch_wrapped(void *arg, unsigned long vec)
{
	KERNEL_LOCK(1, NULL);
	jensenio_intr_dispatch(arg, vec);
	KERNEL_UNLOCK_ONE(NULL);
}

void
jensenio_intr_establish(struct jensenio_scb_intrhand *jih,
    unsigned long vec, int flags, int (*func)(void *), void *arg)
{
	void (*scb_func)(void *, unsigned long);

	/*
	 * Jensen systems are all uniprocessors, but we still do all
	 * of the KERNEL_LOCK handling as a formality to keep assertions
	 * valid in MI code.
	 */
	KASSERT(CPU_IS_PRIMARY(curcpu()));
	KASSERT(ncpu == 1);
	if (flags & ALPHA_INTR_MPSAFE)
		scb_func = jensenio_intr_dispatch;
	else
		scb_func = jensenio_intr_dispatch_wrapped;

	jih->jih_func = func;
	jih->jih_arg = arg;
	jih->jih_vec = vec;

	snprintf(jih->jih_vecstr, sizeof(jih->jih_vecstr), "0x%lx",
	    jih->jih_vec);
	evcnt_attach_dynamic(&jih->jih_evcnt, EVCNT_TYPE_INTR,
	    NULL, "vector", jih->jih_vecstr);

	mutex_enter(&cpu_lock);

	scb_set(vec, scb_func, jih);
	curcpu()->ci_nintrhand++;

	mutex_exit(&cpu_lock);
}

static int
jensenio_eisa_intr_map(void *v, u_int eirq, eisa_intr_handle_t *ihp)
{

	if (eirq >= JENSEN_MAX_IRQ) {
		printf("jensenio_eisa_intr_map: bogus IRQ %d", eirq);
		*ihp = -1;
		return (1);
	}

	if (jensenio_intr_deftype[eirq] == IST_UNUSABLE) {
		printf("jensenio_eisa_intr_map: unusable irq %d\n",
		    eirq);
		*ihp = -1;
		return (1);
	}

	*ihp = eirq;
	return (0);
}

static const char *
jensenio_eisa_intr_string(void *v, int eirq, char *buf, size_t len)
{
	if (eirq >= JENSEN_MAX_IRQ)
		panic("%s: bogus IRQ %d", __func__, eirq);

	snprintf(buf, len, "eisa irq %d", eirq);
	return buf;
}

static const struct evcnt *
jensenio_eisa_intr_evcnt(void *v, int eirq)
{

	if (eirq >= JENSEN_MAX_IRQ)
		panic("%s: bogus IRQ %d", __func__, eirq);

	return (alpha_shared_intr_evcnt(jensenio_eisa_intr, eirq));
}

static void *
jensenio_eisa_intr_establish(void *v, int irq, int type, int level,
    int (*fn)(void *), void *arg)
{
	void *cookie;

	if (irq >= JENSEN_MAX_IRQ || type == IST_NONE)
		panic("jensenio_eisa_intr_establish: bogus irq or type");

	if (jensenio_intr_deftype[irq] == IST_UNUSABLE) {
		printf("jensenio_eisa_intr_establish: IRQ %d not usable\n",
		    irq);
		return NULL;
	}

	cookie = alpha_shared_intr_alloc_intrhand(jensenio_eisa_intr, irq,
	    type, level, 0, fn, arg, "eisa");

	if (cookie == NULL)
		return NULL;

	mutex_enter(&cpu_lock);

	if (! alpha_shared_intr_link(jensenio_eisa_intr, cookie, "eisa")) {
		mutex_exit(&cpu_lock);
		alpha_shared_intr_free_intrhand(cookie);
		return NULL;
	}

	if (alpha_shared_intr_firstactive(jensenio_eisa_intr, irq)) {
		scb_set(0x800 + SCB_IDXTOVEC(irq), jensenio_iointr, NULL);
		jensenio_setlevel(irq,
		    alpha_shared_intr_get_sharetype(jensenio_eisa_intr,
						    irq) == IST_LEVEL);
		jensenio_enable_intr(irq, 1);
	}

	mutex_exit(&cpu_lock);

	return cookie;
}

static void
jensenio_eisa_intr_disestablish(void *v, void *cookie)
{
	struct alpha_shared_intrhand *ih = cookie;
	int irq = ih->ih_num;

	mutex_enter(&cpu_lock);

	if (alpha_shared_intr_firstactive(jensenio_eisa_intr, irq)) {
		jensenio_enable_intr(irq, 0);
		alpha_shared_intr_set_dfltsharetype(jensenio_eisa_intr,
		    irq, jensenio_intr_deftype[irq]);
		scb_free(0x800 + SCB_IDXTOVEC(irq));
	}

	alpha_shared_intr_unlink(jensenio_eisa_intr, cookie, "eisa");

	mutex_exit(&cpu_lock);

	alpha_shared_intr_free_intrhand(cookie);
}

static int
jensenio_eisa_intr_alloc(void *v, int mask, int type, int *rqp)
{

	/* XXX Not supported right now. */
	return (1);
}

static void
jensenio_iointr(void *framep, u_long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x800);

	if (!alpha_shared_intr_dispatch(jensenio_eisa_intr, irq))
		alpha_shared_intr_stray(jensenio_eisa_intr, irq, "eisa");

	jensenio_specific_eoi(irq);
}

static void
jensenio_enable_intr(int irq, int onoff)
{
	int pic;
	uint8_t bit, mask;

	pic = irq >> 3;
	bit = 1 << (irq & 0x7);

	mask = bus_space_read_1(pic_iot, pic_ioh[pic], PIC_OCW1);
	if (onoff)
		mask &= ~bit;
	else
		mask |= bit;
	bus_space_write_1(pic_iot, pic_ioh[pic], PIC_OCW1, mask);
}

void
jensenio_setlevel(int irq, int level)
{
	int elcr;
	uint8_t bit, mask;

	elcr = irq >> 3;
	bit = 1 << (irq & 0x7);

	mask = bus_space_read_1(pic_iot, pic_elcr_ioh, elcr);
	if (level)
		mask |= bit;
	else
		mask &= ~bit;
	bus_space_write_1(pic_iot, pic_elcr_ioh, elcr, mask);
}

static void
jensenio_pic_init(void)
{
	static const int picaddr[2] = { IO_ICU1, IO_ICU2 };
	int pic;

	/*
	 * Map the PICs and mask off the interrupts on them.
	 */
	for (pic = 0; pic < 2; pic++) {
		if (bus_space_map(pic_iot, picaddr[pic], 2, 0, &pic_ioh[pic]))
			panic("jensenio_init_intr: unable to map PIC %d", pic);
		bus_space_write_1(pic_iot, pic_ioh[pic], PIC_OCW1, 0xff);
	}

	/*
	 * Map the ELCR registers and initialize all interrupts to EDGE
	 * trigger.
	 */
	if (bus_space_map(pic_iot, 0x4d0, 2, 0, &pic_elcr_ioh))
		panic("jensenio_init_intr: unable to map ELCR registers");
	bus_space_write_1(pic_iot, pic_elcr_ioh, 0, 0);
	bus_space_write_1(pic_iot, pic_elcr_ioh, 1, 0);
}
