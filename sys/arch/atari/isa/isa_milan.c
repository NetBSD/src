/*	$NetBSD: isa_milan.c,v 1.3 2001/05/28 08:41:37 leo Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <machine/iomap.h>

void	isa_bus_init(void);
#if 0
static void init_icu(void);
#endif
static void set_icus(void);
static void calc_imask(void);
static void isa_callback(int);

/*
 * Bitmask of currently enabled isa interrupts. Used by set_icus().
 */
static u_int16_t imask_enable = 0xffff;

#define	IRQ_SLAVE	2

/*
 * Interrupt routing table
 */
#define MILAN_MAX_ISA_INTS	16

static isa_intr_info_t milan_isa_iinfo[MILAN_MAX_ISA_INTS];

void
isa_bus_init()
{
#if 0
	init_icu();
#endif
	set_icus();
}

#if 0
/*
 * XXX: For some reason, this does not work at all... (Leo).
 */
static void
init_icu()
{
#define	ICU_OFFSET	0

	u_int8_t	*icu;

	icu = (u_int8_t*)(AD_8259_MASTER);

	icu[0] = 0x11;			/* reset; program device, four bytes */
	icu[1] = ICU_OFFSET;		/* starting at this vector index */
	icu[1] = (1 << IRQ_SLAVE);	/* slave on line 2 */
	icu[1] = 1;			/* 8086 mode */
	icu[1] = 0xff;			/* leave interrupts masked */

	icu = (u_int8_t*)(AD_8259_SLAVE);

	icu[0] = 0x11;			/* reset; program device, four bytes */
	icu[1] = ICU_OFFSET + 8;	/* starting at this vector index */
	icu[1] = IRQ_SLAVE;		/* slave on line 2 */
	icu[1] = 1;			/* 8086 mode */
	icu[1] = 0xff;			/* leave interrupts masked */
}
#endif

static void
set_icus()
{
	u_int8_t	*icu;

	icu = (u_int8_t*)AD_8259_MASTER;
	icu[1] = imask_enable & 0xff;
	icu = (u_int8_t*)AD_8259_SLAVE;
	icu[1] = (imask_enable >> 8) & 0xff;
}

static void
calc_imask()
{
	int		irq;
	u_int16_t	nmask = 0;

	for (irq = 0; irq < MILAN_MAX_ISA_INTS; irq++) {
		if (milan_isa_iinfo[irq].ifunc != NULL)
			nmask |= 1 << irq;
		if (nmask >= 0x100)
			nmask |= 1 << IRQ_SLAVE;
	}
	imask_enable = ~nmask;
	set_icus();
}

static void
isa_callback(vector)
	int	vector;
{
	isa_intr_info_t	*iinfo_p;
	int		s;

	iinfo_p = &milan_isa_iinfo[vector];

	s = splx(iinfo_p->ipl);
	(void) (iinfo_p->ifunc)(iinfo_p->iarg);
	splx(s);
}

void milan_isa_intr(int, int);
void
milan_isa_intr(vector, sr)
	int	vector, sr;
{
	isa_intr_info_t *iinfo_p;

	if (vector >= MILAN_MAX_ISA_INTS) {
		printf("milan_isa_intr: Bogus vector %d\n", vector);
		return;
	}

	/* Acknowledge interrupt XXX 0x20 == EOI	*/
	if (vector > 7)
		*((u_char *)AD_8259_SLAVE) = 0x20;
	*((u_char *)AD_8259_MASTER) = 0x20;

	iinfo_p = &milan_isa_iinfo[vector];
	if (iinfo_p->ifunc == NULL) {
		printf("milan_isa_intr: Stray interrupt: %d (mask:%04x)\n",
				vector, imask_enable);
		return;
	}
	if ((sr & PSL_IPL) >= (iinfo_p->ipl & PSL_IPL)) {
		/*
		 * We're running at a too high priority now.
		 */
		add_sicallback((si_farg)isa_callback, (void*)vector, 0);
	}
	else {
		s = splx(iinfo_p->ipl);
		(void) (iinfo_p->ifunc)(iinfo_p->iarg);
		splx(s);
	}
}


/*
 * Try to allocate a free interrupt... On the Milan, we have available:
 * 5, 9, 10, 11, 13. Or in a bitmask: 0x1720.
 */
#define	MILAN_AVAIL_ISA_INTS	0x1720

int
isa_intr_alloc(ic, mask, type, irq)
	isa_chipset_tag_t ic;
	int mask;
	int type;
	int *irq;
{
	int	i;

	/*
	 * The Hades only supports edge triggered interrupts!
	 * XXX: Find out about the Milan....
	 */
	if (type != IST_EDGE)
		return 1;

	/*
	 * Say no to impossible questions...
	 */
	if (!(mask &= MILAN_AVAIL_ISA_INTS))
		return 1;

	for (i = 0; i < MILAN_MAX_ISA_INTS; i++) {
		if (mask & (1<<i)) {
		    if (milan_isa_iinfo[i].ifunc == NULL) {
			*irq = i;
			return 0;
		    }
		}
	}
	return (1);
}

void *
isa_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	isa_chipset_tag_t ic;
	int		  irq, type, level;
	int		  (*ih_fun) __P((void *));
	void		  *ih_arg;
{
	isa_intr_info_t *iinfo_p;

	/*
	 * The Hades only supports edge triggered interrupts!
	 * XXX: Find out oubout the Milan...
	 */
	if (type != IST_EDGE)
		return NULL;

	iinfo_p = &milan_isa_iinfo[irq];

	if (iinfo_p->ifunc != NULL) {
		printf("isa_intr_establish: interrupt %d was already "
			"established\n", irq);
		return NULL;
	}

	iinfo_p->slot  = 0;	/* Unused on Milan */
	iinfo_p->ihand = NULL;	/* Unused on Milan */
	iinfo_p->ipl   = level;
	iinfo_p->ifunc = ih_fun;
	iinfo_p->iarg  = ih_arg;

	calc_imask();
	return(iinfo_p);
}

void
isa_intr_disestablish(ic, handler)
	isa_chipset_tag_t	ic;
	void			*handler;
{
	isa_intr_info_t *iinfo_p = (isa_intr_info_t *)handler;

	if (iinfo_p->ifunc == NULL)
	    panic("isa_intr_disestablish: interrupt was not established\n");

	iinfo_p->ifunc = NULL;
	calc_imask();
}
