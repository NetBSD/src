/* $NetBSD: jensenio_intr.c,v 1.2.6.1 2001/08/03 04:10:45 lukem Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: jensenio_intr.c,v 1.2.6.1 2001/08/03 04:10:45 lukem Exp $");

#include <sys/types.h> 
#include <sys/param.h> 
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

static bus_space_tag_t pic_iot;
static bus_space_handle_t pic_ioh[2];
static bus_space_handle_t pic_elcr_ioh;

int	jensenio_eisa_intr_map(void *, u_int, eisa_intr_handle_t *);
const char *jensenio_eisa_intr_string(void *, int);
const struct evcnt *jensenio_eisa_intr_evcnt(void *, int);
void	*jensenio_eisa_intr_establish(void *, int, int, int,
	    int (*)(void *), void *);
void	jensenio_eisa_intr_disestablish(void *, void *);
int	jensenio_eisa_intr_alloc(void *, int, int, int *);

#define	JENSEN_MAX_IRQ		16

struct alpha_shared_intr *jensenio_eisa_intr;

void	jensenio_iointr(void *, u_long);

void	jensenio_enable_intr(int, int);
void	jensenio_setlevel(int, int);
void	jensenio_pic_init(void);

const int jensenio_intr_deftype[JENSEN_MAX_IRQ] = {
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

static __inline void
jensenio_specific_eoi(int irq)
{

	if (irq > 7)
		bus_space_write_1(pic_iot, pic_ioh[1],
		    0, 0x20 | (irq & 0x07));
	bus_space_write_1(pic_iot, pic_ioh[0],
	    0, 0x20 | (irq > 7 ? 2 : irq));
}

void
jensenio_intr_init(struct jensenio_config *jcp)
{
	eisa_chipset_tag_t ec = &jcp->jc_ec;
	isa_chipset_tag_t ic = &jcp->jc_ic;
	char *cp;
	int i;

	pic_iot = &jcp->jc_eisa_iot;

	jensenio_pic_init();

	jensenio_eisa_intr = alpha_shared_intr_alloc(JENSEN_MAX_IRQ, 16);
	for (i = 0; i < JENSEN_MAX_IRQ; i++) {
		alpha_shared_intr_set_dfltsharetype(jensenio_eisa_intr,
		    i, jensenio_intr_deftype[i]);
		/* Don't bother with stray interrupts. */
		alpha_shared_intr_set_maxstrays(jensenio_eisa_intr,
		    i, 0);

		cp = alpha_shared_intr_string(jensenio_eisa_intr, i);
		sprintf(cp, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    jensenio_eisa_intr, i), EVCNT_TYPE_INTR,
		    NULL, "eisa", cp);
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

int
jensenio_eisa_intr_map(void *v, u_int eirq, eisa_intr_handle_t *ihp)
{

	if (*ihp >= JENSEN_MAX_IRQ)
		panic("jensenio_eisa_intr_map: bogus IRQ %d\n", *ihp);

	if (jensenio_intr_deftype[eirq] == IST_UNUSABLE) {
		printf("jensenio_eisa_intr_map: unusable irq %d\n",
		    eirq);
		*ihp = -1;
		return (1);
	}

	*ihp = eirq;
	return (0);
}

const char *
jensenio_eisa_intr_string(void *v, int eirq)
{
	static char irqstr[64];

	if (eirq >= JENSEN_MAX_IRQ)
		panic("jensenio_eisa_intr_string: bogus IRQ %d\n", eirq);

	sprintf(irqstr, "eisa irq %d", eirq);

	return (irqstr);
}

const struct evcnt *
jensenio_eisa_intr_evcnt(void *v, int eirq)
{

	if (eirq >= JENSEN_MAX_IRQ)
		panic("jensenio_eisa_intr_evcnt: bogus IRQ %d\n", eirq);

	return (alpha_shared_intr_evcnt(jensenio_eisa_intr, eirq));
}

void *
jensenio_eisa_intr_establish(void *v, int irq, int type, int level,
    int (*fn)(void *), void *arg)
{
	void *cookie;

	if (irq >= JENSEN_MAX_IRQ || type == IST_NONE)
		panic("jensenio_eisa_intr_establish: bogus irq or type");

	if (jensenio_intr_deftype[irq] == IST_UNUSABLE) {
		printf("jensenio_eisa_intr_establish: IRQ %d not usable\n",
		    irq);
		return (NULL);
	}

	cookie = alpha_shared_intr_establish(jensenio_eisa_intr, irq,
	    type, level, fn, arg, "eisa irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(jensenio_eisa_intr, irq)) {
		scb_set(0x800 + SCB_IDXTOVEC(irq), jensenio_iointr, NULL);
		jensenio_setlevel(irq,
		    alpha_shared_intr_get_sharetype(jensenio_eisa_intr,
						    irq) == IST_LEVEL);
		jensenio_enable_intr(irq, 1);
	}

	return (cookie);
}

void
jensenio_eisa_intr_disestablish(void *v, void *cookie)
{
	struct alpha_shared_intrhand *ih = cookie;
	int s, irq = ih->ih_num;

	s = splhigh();

	/* Remove it from the link. */
	alpha_shared_intr_disestablish(jensenio_eisa_intr, cookie,
	    "eisa irq");

	if (alpha_shared_intr_isactive(jensenio_eisa_intr, irq) == 0) {
		jensenio_enable_intr(irq, 0);
		alpha_shared_intr_set_dfltsharetype(jensenio_eisa_intr,
		    irq, jensenio_intr_deftype[irq]);
		scb_free(0x800 + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

int
jensenio_eisa_intr_alloc(void *v, int mask, int type, int *rqp)
{

	/* XXX Not supported right now. */
	return (1);
}

void
jensenio_iointr(void *framep, u_long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x800);

	if (!alpha_shared_intr_dispatch(jensenio_eisa_intr, irq))
		alpha_shared_intr_stray(jensenio_eisa_intr, irq, "eisa irq");

	jensenio_specific_eoi(irq);
}

void
jensenio_enable_intr(int irq, int onoff)
{
	int pic;
	u_int8_t bit, mask;

	pic = irq >> 3;
	bit = 1 << (irq & 0x7);

	mask = bus_space_read_1(pic_iot, pic_ioh[pic], 1);
	if (onoff)
		mask &= ~bit;
	else
		mask |= bit;
	bus_space_write_1(pic_iot, pic_ioh[pic], 1, mask);
}

void
jensenio_setlevel(int irq, int level)
{
	int elcr;
	u_int8_t bit, mask;

	elcr = irq >> 3;
	bit = 1 << (irq & 0x7);

	mask = bus_space_read_1(pic_iot, pic_elcr_ioh, elcr);
	if (level)
		mask |= bit;
	else
		mask &= ~bit;
	bus_space_write_1(pic_iot, pic_elcr_ioh, elcr, mask);
}

void
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
		bus_space_write_1(pic_iot, pic_ioh[pic], 1, 0xff);
	}

	/*
	 * Map the ELCR registers.
	 */
	if (bus_space_map(pic_iot, 0x4d0, 2, 0, &pic_elcr_ioh))
		panic("jensenio_init_intr: unable to map ELCR registers");
}
