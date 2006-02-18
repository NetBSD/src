/* $NetBSD: dbau1550.c,v 1.4.2.2 2006/02/18 15:38:32 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: dbau1550.c,v 1.4.2.2 2006/02/18 15:38:32 yamt Exp $");

#include <sys/param.h>
#include <machine/bus.h>
#include <machine/locore.h>
#include <evbmips/alchemy/obiovar.h>
#include <evbmips/alchemy/board.h>
#include <evbmips/alchemy/dbau1550reg.h>

#define	GET16(x)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)))
#define	PUT16(x, v)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

static void dbau1550_init(void);
static int dbau1550_pci_intr_map(struct pci_attach_args *,
				 pci_intr_handle_t *);
static void dbau1550_poweroff(void);
static void dbau1550_reboot(void);

static const struct obiodev dbau1550_devices[] = {
#if 0
	{ "aupcmcia", -1, -1 },
	{ "aupsc", -1, -1 },
	{ "aupsc", -1, -1 },
	{ "aupsc", -1, -1 },
#endif
	{ NULL },
};

static struct alchemy_board dbau1550_info = {
	"AMD Alchemy DBAu1550",
	dbau1550_devices,
	dbau1550_init,
	dbau1550_pci_intr_map,
	dbau1550_reboot,
	dbau1550_poweroff,
};

const struct alchemy_board *
board_info(void)
{

	return &dbau1550_info;
}

void
dbau1550_init(void)
{
	uint16_t		whoami;

	if (MIPS_PRID_COPTS(cpu_id) != MIPS_AU1550)
		panic("dbau1550: CPU not Au1550");

	/* check the whoami register for a match */
	whoami = GET16(DBAU1550_WHOAMI);

	if (DBAU1550_WHOAMI_BOARD(whoami) != DBAU1550_WHOAMI_DBAU1550_REV1)
		panic("dbau1550: WHOAMI (%x) not DBAu1550!", whoami);

	printf("DBAu1550 (cabernet), CPLDv%d, ",
	    DBAU1550_WHOAMI_CPLD(whoami));

	if (DBAU1550_WHOAMI_DAUGHTER(whoami) != 0xf)
		printf("daughtercard 0x%x\n",
		    DBAU1550_WHOAMI_DAUGHTER(whoami));
	else
		printf("no daughtercard\n");

	/* leave console and clocks alone -- YAMON should have got it right! */
}

int
dbau1550_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/*
	 * This platform has one onboard PCI IDE controller, and two
	 * PCI expansion slots.
	 */
	static const int irqmap[3/*device*/][4/*pin*/] = {
		{  5, -1, -1, -1 },	/* 11: IDE */
		{  2,  5,  6,  1 },	/* 12: PCI Slot 2 */
		{  1,  2,  5,  6 },	/* 13: PCI Slot 3 */
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
	if ((dev < 11) || (dev > 13)) {
		printf("pci: bad device %d\n", dev);
		return 1;
	}

	if ((irq = irqmap[dev - 11][pin - 1]) == -1) {
		printf("pci: no IRQ routing for device %d pin %d\n", dev, pin);
		return 1;
	}

	*ihp = irq;
	return 0;
}

void
dbau1550_reboot(void)
{
	PUT16(DBAU1550_SOFTWARE_RESET, 0);
	delay(100000);	/* 100 msec */
}

void
dbau1550_poweroff(void)
{
	printf("\n- poweroff -\n");
	PUT16(DBAU1550_SOFTWARE_RESET,
	    DBAU1550_SOFTWARE_RESET_PWROFF | DBAU1550_SOFTWARE_RESET_RESET);
	delay(100000);	/* 100 msec */
}
