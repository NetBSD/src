/* $NetBSD: dbau1550.c,v 1.8.66.1 2010/01/20 09:04:33 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: dbau1550.c,v 1.8.66.1 2010/01/20 09:04:33 matt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <machine/bus.h>
#include <machine/locore.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/dev/aupcmciavar.h>
#include <mips/alchemy/dev/aupcmciareg.h>
#include <mips/alchemy/dev/augpioreg.h>
#include <mips/alchemy/dev/auspivar.h>
#include <evbmips/alchemy/obiovar.h>
#include <evbmips/alchemy/board.h>
#include <evbmips/alchemy/dbau1550reg.h>

#include "auspi.h"

/*
 * This should be converted to use bus_space routines.
 */
#define	GET16(x)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)))
#define	PUT16(x, v)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))
#define	GET32(x)	\
	(*((volatile uint32_t *)MIPS_PHYS_TO_KSEG1(x)))
#define	PUT32(x, v)	\
	(*((volatile uint32_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

static void dbau1550_init(void);
static int dbau1550_pci_intr_map(struct pci_attach_args *,
				 pci_intr_handle_t *);
static void dbau1550_poweroff(void);
static void dbau1550_reboot(void);
static bus_addr_t dbau1550_slot_offset(int);
static int dbau1550_slot_irq(int, int);
static void dbau1550_slot_enable(int);
static void dbau1550_slot_disable(int);
static int dbau1550_slot_status(int);
static const char *dbau1550_slot_name(int);
static const struct auspi_machdep *dbau1550_spi(bus_addr_t);

static const struct obiodev dbau1550_devices[] = {
	{ NULL },
};

static struct aupcmcia_machdep dbau1550_pcmcia = {
	2,	/* nslots */
	dbau1550_slot_offset,
	dbau1550_slot_irq,
	dbau1550_slot_enable,
	dbau1550_slot_disable,
	dbau1550_slot_status,
	dbau1550_slot_name,
};

static struct alchemy_board dbau1550_info = {
	.ab_name = "AMD Alchemy DBAu1550",
	.ab_devices = dbau1550_devices,
	.ab_init = dbau1550_init,
	.ab_pci_intr_map =dbau1550_pci_intr_map,
	.ab_reboot = dbau1550_reboot,
	.ab_poweroff = dbau1550_poweroff,
	.ab_pcmcia = &dbau1550_pcmcia,
	.ab_spi = dbau1550_spi,
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
	uint32_t		sysclk;
	uint32_t		pinfunc;

	if (MIPS_PRID_COPTS(mips_options.mips_cpu_id) != MIPS_AU1550)
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

	/*
	 * Initialize PSC clocks.
	 *
	 * PSC0 is SPI.   Use 48MHz FREQ1.
	 * PSC1 is AC97.
	 * PSC2 is SMBus, and must be 48MHz.  (Configured by YAMON)
	 * PSC3 is I2S.
	 *
	 * FREQ2 is 48MHz for USBH/USBD.
	 */
	sysclk = GET32(SYS_CLKSRC);
	sysclk &= ~(SCS_MP0(7) | SCS_DP0 | SCS_CP0);
	sysclk |= SCS_MP0(3);
	PUT32(SYS_CLKSRC, sysclk);

	/*
	 * Configure pin function for PSC devices.
	 */
	pinfunc = GET32(SYS_PINFUNC);
	/* configure PSC0 SYNC1 */
	pinfunc |= SPF_S0;
	/* configure PSC2 for SMBus (YAMON default) */
	pinfunc &= ~SPF_PSC2_MASK;
	pinfunc |= SPF_PSC2_SMBUS;
	/* configure PSC3 for I2S (YAMON default) */
	pinfunc &= ~SPF_PSC3_MASK;
	pinfunc |= SPF_PSC3_I2S;
	PUT32(SYS_PINFUNC, pinfunc);
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
	wbflush();
	delay(100000);	/* 100 msec */
}

void
dbau1550_poweroff(void)
{
	printf("\n- poweroff -\n");
	PUT16(DBAU1550_SOFTWARE_RESET,
	    DBAU1550_SOFTWARE_RESET_PWROFF | DBAU1550_SOFTWARE_RESET_RESET);
	wbflush();
	delay(100000);	/* 100 msec */
}

int
dbau1550_slot_irq(int slot, int which)
{
	static const int irqmap[2/*slot*/][2/*which*/] = {
		{ 35, 32 },		/* Slot 0: Bottom */
		{ 37, 33 },		/* Slot 1: Top */
	};

	if ((slot >= 2) || (which >= 2))
		return -1;

	return (irqmap[slot][which]);
}

bus_addr_t
dbau1550_slot_offset(int slot)
{
	switch (slot) {
	case 0:
		return (DBAU1550_PC0_ADDR);
	case 1:
		return (DBAU1550_PC1_ADDR);
	}

	return (bus_addr_t)-1;
}

void
dbau1550_slot_enable(int slot)
{
	uint16_t	status;
	uint16_t	vcc, vpp;
	int		shift;

	status = GET16(DBAU1550_STATUS);
	switch (slot) {
	case 0:
		status >>= DBAU1550_STATUS_PCMCIA0_VS_SHIFT;
		shift = DBAU1550_PCMCIA_PC0_SHIFT;
		break;
	case 1:
		status >>= DBAU1550_STATUS_PCMCIA1_VS_SHIFT;
		shift = DBAU1550_PCMCIA_PC1_SHIFT;
		break;
	default:
		return;
	}

	status &= DBAU1550_STATUS_PCMCIA_VS_MASK;
	switch (status) {
	case DBAU1550_STATUS_PCMCIA_VS_GND:
		vcc = DBAU1550_PCMCIA_VCC_GND;
		vpp = DBAU1550_PCMCIA_VPP_GND;
		break;
	case DBAU1550_STATUS_PCMCIA_VS_5V:
		vcc = DBAU1550_PCMCIA_VCC_5V;
		vpp = DBAU1550_PCMCIA_VPP_VCC;
		break;
	default:	/* covers both 3.3v cases */
		vcc = DBAU1550_PCMCIA_VCC_3V;
		vpp = DBAU1550_PCMCIA_VPP_VCC;
		break;
	}

	status = GET16(DBAU1550_PCMCIA);

	/* this clears all bits for this slot */
	status &= ~(DBAU1550_PCMCIA_MASK << shift);

	status |= vcc << shift;
	status |= vpp << shift;

	PUT16(DBAU1550_PCMCIA, status);
	wbflush();
	tsleep(&status, PWAIT, "pcmcia_reset_0", mstohz(100));

	status |= (DBAU1550_PCMCIA_DRV_EN << shift);
	PUT16(DBAU1550_PCMCIA, status);
	wbflush();
	tsleep(&status, PWAIT, "pcmcia_reset_start", mstohz(300));

	/* take it out of reset */
	status |= (DBAU1550_PCMCIA_RST << shift);
	PUT16(DBAU1550_PCMCIA, status);
	wbflush();

	/* spec says 20 msec, but experience shows even 200 is not enough */
	tsleep(&status, PWAIT, "pcmcia_reset_finish", mstohz(1000));

	/* NOTE: WE DO NOT SUPPORT DIFFERENT VCC/VPP LEVELS! */
	/* This means that 12V cards are not supported! */
}

void
dbau1550_slot_disable(int slot)
{
	int		shift;
	uint16_t	status;

	switch (slot) {
	case 0:
		shift = DBAU1550_PCMCIA_PC0_SHIFT;
		break;
	case 1:
		shift = DBAU1550_PCMCIA_PC1_SHIFT;
		break;
	}

	status = GET16(DBAU1550_PCMCIA);
	status &= ~(DBAU1550_PCMCIA_MASK);
	PUT16(DBAU1550_PCMCIA, status);
	wbflush();
}

int
dbau1550_slot_status(int slot)
{
	uint16_t	status, mask;
	status = GET16(DBAU1550_STATUS);
	switch (slot) {
	case 0:
		mask = DBAU1550_STATUS_PCMCIA0_INSERTED;
		break;
	case 1:
		mask = DBAU1550_STATUS_PCMCIA1_INSERTED;
		break;

	default:
		return 0;
	}

	return ((mask & status) ? 0 : 1);
}

const char *
dbau1550_slot_name(int slot)
{
	switch (slot) {
	case 0:	
		return "bottom slot";
	case 1:	
		return "top slot";
	default:
		return "???";
	}
}

#if NAUSPI > 0

static int
dbau1550_spi_select(void *arg, int slave)
{
	uint16_t	status;
	if ((slave < 0) || (slave > 1))
		return EINVAL;
	status = GET16(DBAU1550_BOARD_SPECIFIC);

	if (slave) {
		status |= DBAU1550_SPI_DEV_SEL;
	} else {
		status &= ~DBAU1550_SPI_DEV_SEL;
	}
	PUT16(DBAU1550_BOARD_SPECIFIC, status);
	return 0;
}

const struct auspi_machdep *
dbau1550_spi(bus_addr_t ba)
{
	static const struct auspi_machdep md = {
		.am_nslaves = 2,
		.am_cookie = NULL,
		.am_select = dbau1550_spi_select,
	};
	
	/* DBAU1550 only has SPI on PSC0 */
	if (ba != PSC0_BASE)
		return NULL;

	return &md;
}

#endif	/* NAUSPI > 0 */
