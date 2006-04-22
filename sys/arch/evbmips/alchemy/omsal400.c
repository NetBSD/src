/* $NetBSD: omsal400.c,v 1.4.2.2 2006/04/22 11:37:24 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * Copyright (c) 2006 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc
 * Written by Shigeyuki Fukushima.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omsal400.c,v 1.4.2.2 2006/04/22 11:37:24 simonb Exp $");

#include <sys/param.h>
#include <machine/bus.h>
#include <machine/locore.h>
#include <mips/alchemy/dev/augpiovar.h>
#include <mips/alchemy/dev/aupcmciavar.h>
#include <evbmips/alchemy/obiovar.h>
#include <evbmips/alchemy/board.h>
#include <evbmips/alchemy/omsal400reg.h>

#define	GET16(x)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)))
#define	PUT16(x, v)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

static void	omsal400_init(void);
static int	omsal400_pci_intr_map(struct pci_attach_args *,
					 pci_intr_handle_t *);
static void	omsal400_poweroff(void);
static void	omsal400_reboot(void);
static bus_addr_t omsal400_slot_offset(int);
static int omsal400_slot_irq(int, int);
static void omsal400_slot_enable(int);
static void omsal400_slot_disable(int);
static int omsal400_slot_status(int);
static const char *omsal400_slot_name(int);

static const struct obiodev omsal400_devices[] = {
	{ NULL },
};

static struct aupcmcia_machdep omsal400_pcmcia = {
	1,      /* nslots */
	omsal400_slot_offset,
	omsal400_slot_irq,
	omsal400_slot_enable,
	omsal400_slot_disable,
	omsal400_slot_status,
	omsal400_slot_name,
};

static struct alchemy_board omsal400_info = {
	"Plathome Open Micro Sever AL400/AMD Alchemy Au1550",
	omsal400_devices,
	omsal400_init,
	omsal400_pci_intr_map,
	omsal400_reboot,
	omsal400_poweroff,
	&omsal400_pcmcia,
};

const struct alchemy_board *
board_info(void)
{

	return &omsal400_info;
}

void
omsal400_init(void)
{
	/* uint16_t whoami; */

	if (MIPS_PRID_COPTS(cpu_id) != MIPS_AU1550)
		panic("omsal400: CPU not Au1550");

#if 0 /* XXX: TODO borad identification */
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
#endif

	/* leave console and clocks alone -- YAMON should have got it right! */
}

int
omsal400_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/*
	 * This platform has 4 PCI devices:
	 *  dev 1 (PCI_INTD):	PCI Connector
	 *  dev 2 (PCI_INTC):	NEC USB 2.0 uPD720101
	 *  dev 3 (PCI_INTB):	Intel GB Ether 82541PI
	 *  dev 4 (PCI_INTA):	Intel GB Ether 82541PI
	 */
	static const int irqmap[4/*device*/][4/*pin*/] = {
		{  6, -1, -1, -1 },	/* 1: PCI Connecter (not used) */
		{  5,  5,  5, -1 },	/* 2: NEC USB 2.0 */
		{  2, -1, -1, -1 },	/* 3: Intel GbE */
		{  1, -1, -1, -1 },	/* 4: Intel GbE */
	};

	int pin, dev, irq;

	/* if interrupt pin not used... */
	if ((pin = pa->pa_intrpin) == 0)
		return 1;

	if (pin > 4) {
		printf("pci: bad interrupt pin %d\n", pin);
		return 1;
	}

	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag, NULL, &dev, NULL);

	if ((dev < 1) || (dev > 4)) {
		printf("pci: bad device %d\n", dev);
		return 1;
	}

	if ((irq = irqmap[dev - 1][pin - 1]) == -1) {
		printf("pci: no IRQ routing for device %d pin %d\n", dev, pin);
		return 1;
	}

	*ihp = irq;
	return 0;
}

void
omsal400_reboot(void)
{

	/* XXX */
}

void
omsal400_poweroff(void)
{

	printf("\n- poweroff -\n");
	/* XXX */
}


int
omsal400_slot_irq(int slot, int which)
{
	static const int irqmap[1/*slot*/][2/*which*/] = {
		{ 35, 37 },		/* Slot 0: CF connector Type2 */
	};

	if ((slot >= 1) || (which >= 2))
		return -1;

	return irqmap[slot][which];
}

bus_addr_t
omsal400_slot_offset(int slot)
{

	switch (slot) {
	case 0:
		return (0);	/* offset 0 */
	}
	return (bus_addr_t)-1;
}

void
omsal400_slot_enable(int slot)
{

	/* nothing todo */
}

void
omsal400_slot_disable(int slot)
{

	/* nothing todo */
}

int
omsal400_slot_status(int slot)
{
	uint16_t	inserted = 0;

	switch (slot) {
	case 0:
		inserted = !AUGPIO_READ(5);	/* pin 5 */
		break;
	}

	return inserted;
}

const char *
omsal400_slot_name(int slot)
{
	switch (slot) {
	case 0:	
		return "CF connector Type2 on Static BUS#3";
	default:
		return "???";
	}
}
