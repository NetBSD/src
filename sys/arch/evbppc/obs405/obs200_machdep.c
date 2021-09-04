/*	$NetBSD: obs200_machdep.c,v 1.25 2021/09/04 13:36:07 rin Exp $	*/
/*	Original: machdep.c,v 1.3 2005/01/17 17:24:09 shige Exp	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obs200_machdep.c,v 1.25 2021/09/04 13:36:07 rin Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/obs200.h>
#include <machine/century_bios.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/tlb.h>

#include <powerpc/ibm4xx/pci_machdep.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcivar.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>
#include <dev/ic/comreg.h>

#ifndef CONADDR
#define CONADDR		IBM405GP_UART0_BASE
#endif
#ifndef CONSPEED
#define CONSPEED	B9600
#endif
#ifndef CONMODE
			/* 8N1 */
#define CONMODE		((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)
#endif
#endif	/* NCOM */

#define	TLB_PG_SIZE 	(16*1024*1024)

void initppc(vaddr_t, vaddr_t, char *, void *);

void
initppc(vaddr_t startkernel, vaddr_t endkernel, char *args, void *info_block)
{
	u_int32_t pllmode __debugused;
	u_int32_t psr __debugused;
	vaddr_t va;
	u_int memsize;

	/* Disable all external interrupts */
	mtdcr(DCR_UIC0_BASE + DCR_UIC_ER, 0);
	pllmode = mfdcr(DCR_CPC0_PLLMR);
	psr = mfdcr(DCR_CPC0_PSR);

	/* Setup board from BIOS */
	bios_board_init(info_block, startkernel);
	memsize = bios_board_memsize_get();

	/* Linear map kernel memory. */
	for (va = 0; va < endkernel; va += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);

	/* Map console after physmem (see pmap_tlbmiss()). */
	ppc4xx_tlb_reserve(CONADDR, roundup(memsize, TLB_PG_SIZE), TLB_PG_SIZE,
	    TLB_I | TLB_G);

	/* Initialize IBM405GPr CPU */
	ibm40x_memsize_init(memsize, startkernel);
	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DEBUG
	bios_board_print();
	printf("  PLL Mode Register = 0x%08x\n", pllmode);
	printf("  Chip Pin Strapping Register = 0x%08x\n", psr);
#endif

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
consinit(void)
{

#if (NCOM > 0)
	com_opb_cnattach(OBS200_COM_FREQ, CONADDR, CONSPEED, CONMODE);
#endif
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{

	/*
	 * cpu common startup
	 */
	ibm4xx_cpu_startup("OpenBlockS S/R IBM PowerPC 405GP Board");

	/*
	 * Set up the board properties database.
	 */
	bios_board_info_set();

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * no fake mapiodev
	 */
	fake_mapiodev = 0;
}

int
ibm4xx_pci_bus_maxdevs(void *v, int busno)
{

	/*
	 * Bus number is irrelevant.  Configuration Mechanism 1 is in
	 * use, can have devices 0-32 (i.e. the `normal' range).
	 */
	return 31;
}

int
ibm4xx_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	/*
	 * We need to map the interrupt pin to the interrupt bit
	 * in the UIC associated with it.
	 *
	 * This platform has 4 PCI devices.
	 *
	 # External IRQ Mappings:
	 *  dev 7 (Ext IRQ3):	Realtek 8139 Ethernet
	 *  dev 8 (Ext IRQ0):	PCI Connector
	 */
	static const int irqmap[15/*device*/][4/*pin*/] = {
		{ -1, -1, -1, -1 },	/*  1: none */
		{ -1, -1, -1, -1 },	/*  2: none */
		{ -1, -1, -1, -1 },	/*  3: none */
		{ -1, -1, -1, -1 },	/*  4: none */
		{ -1, -1, -1, -1 },	/*  5: none */
		{ -1, -1, -1, -1 },	/*  6: none */
		{  3, -1, -1, -1 },	/*  7: none */
		{  0, -1, -1, -1 },	/*  8: none */
		{ -1, -1, -1, -1 },	/*  9: none */
		{ -1, -1, -1, -1 },	/* 10: none */
		{ -1, -1, -1, -1 },	/* 11: none */
		{ -1, -1, -1, -1 },	/* 12: none */
		{ -1, -1, -1, -1 },	/* 13: none */
		{ -1, -1, -1, -1 },	/* 14: none */
		{ -1, -1, -1, -1 },	/* 15: none */
	};

	int pin, dev, irq;

	pin = pa->pa_intrpin;
	dev = pa->pa_device;
        *ihp = -1;

	/* if interrupt pin not used... */
	if (pin == 0)
		return 1;

	if (pin > 4) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		return 1;
	}

	if ((dev < 1) || (dev > 15)) {
		printf("pci_intr_map: bad device %d\n", dev);
		return 1;
	}


	if ((irq = irqmap[dev - 1][pin - 1]) == -1) {
		printf("pci_intr_map: no IRQ routing for device %d pin %d\n",
			dev, pin);
		return 1;
	}

	*ihp = irq + 25;
	return 0;
}

void
ibm4xx_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{
	static const int ilinemap[15/*device*/] = {
		-1, -1, -1, -1,		/* device  1 -  4 */
		-1, -1, 28, 25,		/* device  5 -  8 */
		-1, -1, -1, -1,		/* device  9 - 12 */
		-1, -1, -1,		/* device 13 - 15 */
	};

	if (bus == 0) {
		if ((dev < 1) || (dev > 15)) {
			printf("pci_intr_map: bad device %d\n", dev);
			*iline = 0;
			return;
		}
		*iline = ilinemap[dev - 1];
	} else
		*iline = 19 + ((swiz + dev + 1) & 3);
}
