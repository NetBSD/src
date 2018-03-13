/*	$NetBSD: isa_milan.c,v 1.14.56.1 2018/03/13 13:41:14 martin Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_milan.c,v 1.14.56.1 2018/03/13 13:41:14 martin Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <machine/iomap.h>

void	isa_bus_init(void);

static void new_imask(void);
static void isa_callback(int);

/*
 * Bitmask of currently enabled isa interrupts. Used by new_imask().
 */
static u_int16_t imask_enable = 0xffff;

#define	IRQ_SLAVE		2	/* Slave at level 2		*/
#define MILAN_MAX_ISA_INTS	16	/* Max. number of vectors	*/
#define	ICU_OFFSET		0	/* Interrupt vector base	*/

#define	WICU(icu, val)		*(volatile u_int8_t*)(icu) = val

static isa_intr_info_t milan_isa_iinfo[MILAN_MAX_ISA_INTS];

void
isa_bus_init(void)
{
	volatile u_int8_t	*icu;

	/*
	 * Initialize both the icu's:
	 *   - enter Special Mask Mode
	 *   - Block all interrupts
	 */
	icu = (u_int8_t*)(AD_8259_MASTER);

	icu[0] = 0x11;			/* reset; program device, four bytes */
	icu[1] = ICU_OFFSET;		/* starting at this vector index */
	icu[1] = (1 << IRQ_SLAVE);	/* slave on line 2 */
	icu[1] = 1;			/* 8086 mode */
	icu[1] = 0xff;			/* leave interrupts masked */
	icu[0] = 0x68;			/* special mask mode  */
	icu[0] = 0x0a;			/* Read IRR by default. */

	icu = (u_int8_t*)(AD_8259_SLAVE);

	icu[0] = 0x11;			/* reset; program device, four bytes */
	icu[1] = ICU_OFFSET + 8;	/* starting at this vector index */
	icu[1] = IRQ_SLAVE;		/* slave on line 2 */
	icu[1] = 1;			/* 8086 mode */
	icu[1] = 0xff;			/* leave interrupts masked */
	icu[0] = 0x68;			/* special mask mode  */
	icu[0] = 0x0a;			/* Read IRR by default. */
}

/*
 * Determine and activate new interrupt mask by scanning the milan_isa_iinfo
 * array for enabled interrupts.
 */
static void
new_imask(void)
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
	WICU(AD_8259_MASTER+1, imask_enable & 0xff);
	WICU(AD_8259_SLAVE+1 , (imask_enable >> 8) & 0xff);
}

static void
isa_callback(int vector)
{
	isa_intr_info_t	*iinfo_p;
	int		s;

	iinfo_p = &milan_isa_iinfo[vector];

	s = splx(iinfo_p->ipl);
	(void) (iinfo_p->ifunc)(iinfo_p->iarg);
	if (vector > 7) {
		WICU(AD_8259_SLAVE, 0x60 | (vector & 7));
		vector = IRQ_SLAVE;
	}
	WICU(AD_8259_MASTER, 0x60 | (vector & 7));
	splx(s);
}

void milan_isa_intr(int, int);
void
milan_isa_intr(int vector, int sr)
{
	isa_intr_info_t *iinfo_p;
	int		s;

	if (vector >= MILAN_MAX_ISA_INTS) {
		printf("milan_isa_intr: Bogus vector %d\n", vector);
		return;
	}

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
		if (vector > 7) {
			WICU(AD_8259_SLAVE, 0x60 | (vector & 7));
			vector = IRQ_SLAVE;
		}
		WICU(AD_8259_MASTER, 0x60 | (vector & 7));
		splx(s);
	}
}

/*
 * Try to allocate a free interrupt... On the Milan, we have available:
 * 5, 9, 10, 11, 13. Or in a bitmask: 0x1720.
 */
#define	MILAN_AVAIL_ISA_INTS	0x1720

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	int	i;

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
isa_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level, int (*ih_fun)(void *), void *ih_arg)
{
	isa_intr_info_t *iinfo_p;

	iinfo_p = &milan_isa_iinfo[irq];

	if (iinfo_p->ifunc != NULL) {
		printf("isa_intr_establish: interrupt %d was already "
			"established\n", irq);
		return NULL;
	}

	iinfo_p->slot  = 0;	/* Unused on Milan */
	iinfo_p->ihand = NULL;	/* Unused on Milan */
	iinfo_p->ipl   = ipl2psl_table[level];
	iinfo_p->ifunc = ih_fun;
	iinfo_p->iarg  = ih_arg;

	new_imask();
	return(iinfo_p);
}

void
isa_intr_disestablish(isa_chipset_tag_t ic, void *handler)
{
	isa_intr_info_t *iinfo_p = (isa_intr_info_t *)handler;

	if (iinfo_p->ifunc == NULL)
	    panic("isa_intr_disestablish: interrupt was not established");

	iinfo_p->ifunc = NULL;
	new_imask();
}
