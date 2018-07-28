/*	$NetBSD: obs266_machdep.c,v 1.20.52.1 2018/07/28 04:37:33 pgoyette Exp $	*/
/*	Original: md_machdep.c,v 1.3 2005/01/24 18:47:37 shige Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: obs266_machdep.c,v 1.20.52.1 2018/07/28 04:37:33 pgoyette Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/obs266.h>

#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/pci_machdep.h>
#include <powerpc/ibm4xx/openbios.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <dev/ic/comreg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include "ksyms.h"

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>

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

/*
 * Global variables used here and there
 */
char bootpath[256];

extern paddr_t msgbuf_paddr;

void initppc(vaddr_t, vaddr_t, char *, void *);

void
initppc(vaddr_t startkernel, vaddr_t endkernel, char *args, void *info_block)
{
	vaddr_t va;
	u_int memsize;

	/* Setup board from OpenBIOS */
	openbios_board_init(info_block, startkernel);
	memsize = openbios_board_memsize_get();

	/* Linear map kernel memory */
	for (va = 0; va < endkernel; va += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);

	/* Map console after RAM (see pmap_tlbmiss()) */
	ppc4xx_tlb_reserve(CONADDR, roundup(memsize, TLB_PG_SIZE), TLB_PG_SIZE,
	    TLB_I | TLB_G);

	/* Initialize IBM405GPr CPU */
	ibm40x_memsize_init(memsize, startkernel);
	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DEBUG
	openbios_board_print();
#endif

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Look for the ibm4xx modules in the right place.
	 */
	module_machine = module_machine_ibm4xx;
}

void
consinit(void)
{

#if (NCOM > 0)
	com_opb_cnattach(OBS266_COM_FREQ, CONADDR, CONSPEED, CONMODE);
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
	ibm4xx_cpu_startup("OpenBlockS266 IBM PowerPC 405GPr Board");

	/*
	 * Set up the board properties database.
	 */
	openbios_board_info_set();

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * no fake mapiodev
	 */
	fake_mapiodev = 0;
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

	splhigh();

	if (!cold && (howto & RB_DUMP))
		ibm4xx_dumpsys();

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
	  /* Power off here if we know how...*/
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

#if 0
		goto reboot;	/* XXX for now... */
#endif

#ifdef DDB
		printf("dropping to debugger\n");
		while(1)
			Debugger();
#endif
	}

	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;

	/* flush cache for msgbuf */
	__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

#if 0
 reboot:
#endif
	ppc4xx_reset();

	printf("ppc4xx_reset() failed!\n");
#ifdef DDB
	while(1)
		Debugger();
#else
	while (1)
		/* nothing */;
#endif
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
	 *  dev 1 (Ext IRQ3):	PCI Connector
	 *  dev 2 (Ext IRQ4):	PCI Connector
	 *  dev 3 (Ext IRQ5):	HPT IDE Controller
	 *  dev 4 (Ext IRQ6):	Davicom Ethernet
	 */
	static const int irqmap[4/*device*/][4/*pin*/] = {
		{  3,  3,  3,  3 },	/* 1: PCI Connector 1 */
		{  4,  4,  4,  4 },	/* 2: PCI Connector 2 */
		{  5,  5, -1, -1 },	/* 3: HPT IDE Controller */
		{  6,  6, -1, -1 },	/* 4: Damicom Ethernet */
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

	if ((dev < 1) || (dev > 4)) {
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

	static const int ilinemap[4/*device*/] = {
		28, 29, 30, 31
	};

	if ((dev < 1) || (dev > 4)) {
		printf("%s: bad device %d\n", __func__, dev);
		*iline = 0;
		return;
	}
	*iline = ilinemap[dev - 1];
}
