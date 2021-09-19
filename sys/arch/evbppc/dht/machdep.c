/*	$NetBSD: machdep.c,v 1.3 2021/09/19 11:37:00 andvar Exp $	*/

/*
 * Taken from src/sys/arch/evbppc/walnut/machdep.c:
 *	NetBSD: machdep.c,v 1.67 2021/03/30 05:08:16 rin Exp
 */

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.3 2021/09/19 11:37:00 andvar Exp $");

#include "opt_ddb.h"
#include "opt_pci.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/dht.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/openbios.h>
#include <powerpc/ibm4xx/tlb.h>

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>
#ifndef CONADDR
#define	CONADDR		IBM405GP_UART0_BASE
#endif
#ifndef CONSPEED
#define	CONSPEED	B115200
#endif
#ifndef CONMODE
#define	CONMODE		TTYDEF_CFLAG
#endif
#endif /* NCOM > 0 */

#include "emac.h"
#if NEMAC > 0
#include <net/if_ether.h>
#endif

#include "pci.h"
#if NPCI > 0
#ifndef PCI_NETBSD_CONFIGURE
#error options PCI_NETBSD_CONFIGURE is mandatory.
#endif
#include <powerpc/ibm4xx/pci_machdep.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcivar.h>
#endif

#define TLB_PG_SIZE 	(16 * 1024 * 1024)

static u_int memsize;

void initppc(vaddr_t, vaddr_t, char *, void *);

/*
 * Get memory size from SDRAM bank register.
 */
static u_int
dht_memsize(void)
{
	u_int total = 0;
	uint32_t val, addr;

	for (addr = DCR_SDRAM0_B0CR; addr <= DCR_SDRAM0_B3CR; addr += 4) {
		mtdcr(DCR_SDRAM0_CFGADDR, addr);
		val = mfdcr(DCR_SDRAM0_CFGDATA);
		if (val & SDRAM0_BnCR_EN)
			total += SDRAM0_BnCR_SZ(val);
	}
	return total;
}

void
consinit(void)
{

#if NCOM > 0
	com_opb_cnattach(DHT_COM_FREQ, CONADDR, CONSPEED, CONMODE);
#endif
}

void
initppc(vaddr_t startkernel, vaddr_t endkernel, char *args, void *info_block)
{

	/* Disable all external interrupts */
	mtdcr(DCR_UIC0_BASE + DCR_UIC_ER, 0);

	memsize = dht_memsize();

	/* Linear map kernel memory */
	for (vaddr_t va = 0; va < endkernel; va += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);

	/* Map console after physmem (see pmap_tlbmiss()) */
	ppc4xx_tlb_reserve(IBM405GP_UART0_BASE, roundup(memsize, TLB_PG_SIZE),
	    TLB_PG_SIZE, TLB_I | TLB_G);

	/* Disable all timers */
	mtspr(SPR_TCR, 0);

	ibm40x_memsize_init(memsize, startkernel);
	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
cpu_startup(void)
{
	prop_number_t pn;

	ibm4xx_cpu_startup("DHT Walnut 405GP Evaluation Board");

	/*
	 * Set up the board properties database.
	 */
	board_info_init();

	pn = prop_number_create_integer(DHT_CPU_FREQ);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency", pn) ==
	    false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	pn = prop_number_create_integer(memsize);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

#if NEMAC > 0
	/*
	 * XXX
	 * Unfortunately, no MAC address is assigned to this board.
	 * Set fake address de:ad:be:ef:00:00.
	 *
	 * XXX
	 * This should be same with what U-Boot/PPC-Boot set.
	 */
	static uint8_t enaddr[ETHER_ADDR_LEN];
	enaddr[0] = 0xde; enaddr[1] = 0xad; enaddr[2] = 0xbe;
	enaddr[3] = 0xef; enaddr[4] = 0x00; enaddr[5] = 0x00;
	prop_data_t pd = prop_data_create_data_nocopy(enaddr, ETHER_ADDR_LEN);
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "emac0-mac-addr", pd) ==
	    false)
		panic("setting emac0-mac-addr");
	prop_object_release(pd);
#endif

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * No fake mapiodev.
	 */
	fake_mapiodev = 0;
}

#if NPCI > 0
int
ibm4xx_pci_bus_maxdevs(void *v, int busno)
{

	return 32;
}

/*
 * This is only possible mapping known to work b/w PCI devices and IRQ pins.
 */
#define	DEV_TO_IRQ(dev)		(27 + (dev))
#define	PARENT_DEV(swiz, dev)	((swiz) - (dev))

int
ibm4xx_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin, bus = pa->pa_bus, dev = pa->pa_device;

	if (pin <= 0 || pin > 4)
		goto bad;

	if (bus != 0) {
		/*
		 * XXX
		 * We only support ppb(4) directly attached to pci0.
		 */
		dev = PARENT_DEV(pa->pa_intrswiz, dev);
		goto out;
	}

	switch (dev) {
	case 2:
	case 3:
	case 4:
out:
		*ihp = DEV_TO_IRQ(dev);
		return 0;
	default:
bad:
		printf("%s: invalid request: bus %d dev %d pin %d\n",
		    __func__, bus, dev, pin);
		*ihp = -1;
		return 1;
	}
}

void
ibm4xx_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	if (bus != 0) {
		/*
		 * XXX
		 * See comment above.
		 */
		dev = PARENT_DEV(swiz, dev);
		goto out;
	}

	switch (dev) {
	case 2:
	case 3:
	case 4:
out:
		*iline = DEV_TO_IRQ(dev);
		break;
	default:
		printf("%s: invalid request: bus %d dev %d pin %d swiz %d\n",
		    __func__, bus, dev, pin, swiz);
		*iline = 0;
		break;
	}
}
#endif /* NPCI > 0 */
