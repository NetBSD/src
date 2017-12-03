/* $NetBSD: dbau1500.c,v 1.7.12.1 2017/12/03 11:36:08 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: dbau1500.c,v 1.7.12.1 2017/12/03 11:36:08 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <mips/locore.h>

#include <evbmips/alchemy/obiovar.h>
#include <evbmips/alchemy/board.h>
#include <evbmips/alchemy/dbau1500reg.h>

#define	GET16(x)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)))
#define	PUT16(x, v)	\
	(*((volatile uint16_t *)MIPS_PHYS_TO_KSEG1(x)) = (v))

static void dbau1500_init(void);
static int dbau1500_pci_intr_map(const struct pci_attach_args *,
				 pci_intr_handle_t *);
static void dbau1500_reboot(void);

static const struct obiodev dbau1500_devices[] = {
#if 0
	{ "aupcmcia", -1, -1 },
	{ "auaudio", -1, -1 },
#endif
	{ NULL },
};

static struct alchemy_board dbau1500_info = {
	"AMD Alchemy DBAu1500",
	dbau1500_devices,
	dbau1500_init,
	dbau1500_pci_intr_map,
	dbau1500_reboot,
	NULL,	/* poweroff */
};

const struct alchemy_board *
board_info(void)
{

	return &dbau1500_info;
}

void
dbau1500_init(void)
{
	uint32_t	whoami;

	if (MIPS_PRID_COPTS(mips_options.mips_cpu_id) != MIPS_AU1500)
		panic("dbau1500: CPU not an AU1500!");

	/* check the whoami register for a match */
	whoami = *((volatile uint32_t *)MIPS_PHYS_TO_KSEG1(DBAU1500_WHOAMI));

	if (DBAU1500_WHOAMI_BOARD(whoami) != DBAU1500_WHOAMI_DBAU1500)
		panic("dbau1500: WHOAMI (%x) not DBAu1500!", whoami);

	printf("DBAu1500 (zinfandel), CPLDv%d, ",
	    DBAU1500_WHOAMI_CPLD(whoami));

	if (DBAU1500_WHOAMI_DAUGHTER(whoami) != 0xf)
		printf("daughtercard 0x%x\n",
		    DBAU1500_WHOAMI_DAUGHTER(whoami));
	else
		printf("no daughtercard\n");

	/* leave console and clocks alone -- YAMON should have got it right! */
}

int
dbau1500_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/*
	 * This platform has PCI slot and IDE interrupts mapped
	 * identically.  So we just need to look at which of the four
	 * PCI interrupts it is.
	 */

	switch (pa->pa_intrpin) {
	case 0:
		/* not used */
		return 1;
	case 1:
		*ihp = 1;
		break;
	case 2:
		*ihp = 2;
		break;
	case 3:
		*ihp = 4;
		break;
	case 4:
		*ihp = 5;
		break;
	default:
		printf("pci: bad interrupt pin %d\n", pa->pa_intrpin);
		return 1;
	}
	return 0;
}

void
dbau1500_reboot(void)
{
	PUT16(DBAU1500_SOFTWARE_RESET, 0);
	delay(100000);	/* 100 msec */
}
