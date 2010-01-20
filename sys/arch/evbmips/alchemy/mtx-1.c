/* $NetBSD: mtx-1.c,v 1.3.98.1 2010/01/20 09:04:33 matt Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mtx-1.c,v 1.3.98.1 2010/01/20 09:04:33 matt Exp $");

#include <sys/param.h>
#include <machine/bus.h>
#include <machine/locore.h>
#include <evbmips/alchemy/obiovar.h>
#include <evbmips/alchemy/board.h>

#define	MTX1_RESET	0xE00001C

#define	GET16(x)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)))
#define	PUT16(x, v)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

static void mtx1_init(void);
static int mtx1_pci_intr_map(struct pci_attach_args *,
				 pci_intr_handle_t *);
static void mtx1_reboot(void);

static const struct obiodev mtx1_devices[] = {
#if 0
	{ "aupcmcia", -1, -1 },
	{ "auaudio", -1, -1 },
#endif
	{ NULL },
};

static struct alchemy_board mtx1_info = {
	"4G Systems MTX-1",
	mtx1_devices,
	mtx1_init,
	mtx1_pci_intr_map,
	mtx1_reboot,
	NULL,	/* poweroff */
};

const struct alchemy_board *
board_info(void)
{

	return &mtx1_info;
}

void
mtx1_init(void)
{

	if (MIPS_PRID_COPTS(mips_options.mips_cpu_id) != MIPS_AU1500)
		panic("mtx-1: CPU not an AU1500!");

	/*
	 * If we had any kind of identification registers, we could
	 * print them here.  Apparently the MTX-1 doesn't have that
	 * kind of info.
	 */

	/* leave console and clocks alone -- YAMON should have got it right! */
}

int
mtx1_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/*
	 * The board has up to 4 adapters, each with two minipci slots,
	 * giving up to 8 devices.  Each slot 0 is the top, and slot 1
	 * is the bottom.
	 *
	 * As these are mini PCI slots, only 2 interrupt pins can be
	 * used on each slot.
	 */
	static const int irqmap[8/*device*/][4/*pin*/] = {
		{  1,  2, -1, -1 },	/* IDSEL 0 - Adapter A - Slot 0 */
		{  1,  2, -1, -1 },	/* IDSEL 1 - Adapter A - Slot 1 */
		{  4,  5, -1, -1 },	/* IDSEL 2 - Adapter B - Slot 0 */
		{  5,  4, -1, -1 },	/* IDSEL 3 - Adapter B - Slot 1 */

		{  1,  2, -1, -1 },	/* IDSEL 4 - Adapter C - Slot 0 */
		{  1,  2, -1, -1 },	/* IDSEL 5 - Adapter C - Slot 1 */
		{  4,  5, -1, -1 },	/* IDSEL 6 - Adapter D - Slot 0 */
		{  5,  4, -1, -1 },	/* IDSEL 7 - Adapter D - Slot 1 */
	};
	int	pin, dev, irq;

	/* if interrupt pin not used... */
	if ((pin = pa->pa_intrpin) == 0)
		return 1;

	if (pin > 4) {
		printf("pci: bad interrupt pin %d\n", pin);
		return 1;
	}

	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag, NULL, &dev, NULL);
	if ((dev < 0) || (dev > 7)) {
		printf("pci: bad device %d\n", dev);
		return 1;
	}

	if ((irq = irqmap[dev][pin - 1]) == -1) {
		printf("pci: no IRQ routing for device %d pin %d\n", dev, pin);
		return 1;
	}

	*ihp = irq;
	return 0;
}

void
mtx1_reboot(void)
{
	/* fyi, this looks like the same as the DBAu1500 reset */
	PUT16(MTX1_RESET , 0);
	delay(100000);	/* 100 msec */
}
