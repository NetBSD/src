/*	$NetBSD: machdep.c,v 1.55.38.1 2018/07/28 04:37:33 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.55.38.1 2018/07/28 04:37:33 pgoyette Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <machine/powerpc.h>
#include <machine/walnut.h>

#include <powerpc/trap.h>
#include <powerpc/pcb.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>

#include <powerpc/ibm4xx/pci_machdep.h>

#include <powerpc/pic/picvar.h>

#include <dev/cons.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include "ksyms.h"

#if defined(DDB)
#include <powerpc/db_machdep.h>
#include <ddb/db_extern.h>
#endif


#define TLB_PG_SIZE 	(16*1024*1024)

/*
 * Global variables used here and there
 */
struct vm_map *phys_map = NULL;

/*
 * This should probably be in autoconf!				XXX
 */
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

char bootpath[256];

void initppc(vaddr_t, vaddr_t, char *, void *);

static void dumpsys(void);

#define MEMREGIONS	8
struct mem_region physmemr[MEMREGIONS];		/* Hard code memory */
struct mem_region availmemr[MEMREGIONS];	/* Who's supposed to set these up? */

struct board_cfg_data board_data;

void
initppc(vaddr_t startkernel, vaddr_t endkernel, char *args, void *info_block)
{
	/* Disable all external interrupts */
	mtdcr(DCR_UIC0_BASE + DCR_UIC_ER, 0);

        /* Initialize cache info for memcpy, etc. */
        cpu_probe_cache();

	/* Save info block */
	memcpy(&board_data, info_block, sizeof(board_data));

	memset(physmemr, 0, sizeof physmemr);
	memset(availmemr, 0, sizeof availmemr);
	physmemr[0].start = 0;
	physmemr[0].size = board_data.mem_size & ~PGOFSET;
	/* Lower memory reserved by eval board BIOS */
	availmemr[0].start = startkernel;
	availmemr[0].size = board_data.mem_size - availmemr[0].start;

	/* Linear map kernel memory */
	for (vaddr_t va = 0; va < endkernel; va += TLB_PG_SIZE) {
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);
	}

	/* Map console after physmem (see pmap_tlbmiss()) */
	ppc4xx_tlb_reserve(0xef000000, roundup(physmemr[0].size, TLB_PG_SIZE),
	    TLB_PG_SIZE, TLB_I | TLB_G);

	mtspr(SPR_TCR, 0);	/* disable all timers */

	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DEBUG
	printf("Board config data:\n");
	printf("  usr_config_ver = %s\n", board_data.usr_config_ver);
	printf("  rom_sw_ver = %s\n", board_data.rom_sw_ver);
	printf("  mem_size = %u\n", board_data.mem_size);
	printf("  mac_address_local = %02x:%02x:%02x:%02x:%02x:%02x\n",
	    board_data.mac_address_local[0], board_data.mac_address_local[1],
	    board_data.mac_address_local[2], board_data.mac_address_local[3],
	    board_data.mac_address_local[4], board_data.mac_address_local[5]);
	printf("  mac_address_pci = %02x:%02x:%02x:%02x:%02x:%02x\n",
	    board_data.mac_address_pci[0], board_data.mac_address_pci[1],
	    board_data.mac_address_pci[2], board_data.mac_address_pci[3],
	    board_data.mac_address_pci[4], board_data.mac_address_pci[5]);
	printf("  processor_speed = %u\n", board_data.processor_speed);
	printf("  plb_speed = %u\n", board_data.plb_speed);
	printf("  pci_speed = %u\n", board_data.pci_speed);
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

/*
 * Machine dependent startup code.
 */

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	prop_number_t pn;
	prop_data_t pd;
	char pbuf[9];

	/*
	 * Initialize error message buffer (at end of core).
	 */
#if 0	/* For some reason this fails... --Artem
	 * Besides, do we really have to put it at the end of core?
	 * Let's use static buffer for now
	 */
	if (!(msgbuf_vaddr = uvm_km_alloc(kernel_map, round_page(MSGBUFSIZE), 0,
	    UVM_KMF_VAONLY)))
		panic("startup: no room for message buffer");
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa(msgbuf_vaddr + i * PAGE_SIZE,
		    msgbuf_paddr + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, 0);
	initmsgbuf((void *)msgbuf_vaddr, round_page(MSGBUFSIZE));
#else
	initmsgbuf((void *)msgbuf, round_page(MSGBUFSIZE));
#endif

	printf("%s%s", copyright, version);
	printf("Walnut PowerPC 405GP Evaluation Board\n");

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Set up the board properties dictionary.
	 */
	board_properties = prop_dictionary_create();
	KASSERT(board_properties != NULL);

	pn = prop_number_create_integer(board_data.mem_size);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

	pd = prop_data_create_data_nocopy(board_data.mac_address_local,
					  sizeof(board_data.mac_address_local));
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "emac0-mac-addr",
				pd) == false)
		panic("setting emac0-mac-addr");
	prop_object_release(pd);

	pd = prop_data_create_data_nocopy(board_data.mac_address_pci,
					  sizeof(board_data.mac_address_pci));
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "sip0-mac-addr",
				pd) == false)
		panic("setting sip0-mac-addr");
	prop_object_release(pd);

	pn = prop_number_create_integer(board_data.processor_speed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
				pn) == false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();
	fake_mapiodev = 0;
}


static void
dumpsys(void)
{

	printf("dumpsys: TBD\n");
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
		dumpsys();

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
	  /* Power off here if we know how...*/
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

		goto reboot;	/* XXX for now... */

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

 reboot:
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

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}


int
ibm4xx_pci_bus_maxdevs(void *v, int busno)
{

	/*
	 * Bus number is irrelevant.  Configuration Mechanism 1 is in
	 * use, can have devices 0-32 (i.e. the `normal' range).
	 */
	return 5;
}

int
ibm4xx_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin;
	int dev = pa->pa_device;

	if (pin == 0)
		/* No IRQ used. */
		goto bad;

	if (pin > 4) {
		printf("%s: bad interrupt pin %d\n", __func__, pin);
		goto bad;
	}

	/*
	 * We need to map the interrupt pin to the interrupt bit in the UIC
	 * associated with it.  This is highly machine-dependent.
	 */
	switch(dev) {
	case 1:
	case 2:
	case 3:
	case 4:
		*ihp = 27 + dev;
		break;
	default:
		printf("Hmm.. PCI device %d should not exist on this board\n",
			dev);
		goto bad;
	}
	return 0;

bad:
	*ihp = -1;
	return 1;
}

void
ibm4xx_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	if (bus == 0) {
		switch(dev) {
		case 1:
		case 2:
		case 3:
		case 4:
			*iline = 31 - dev;
		}
	} else
		*iline = 20 + ((swiz + dev + 1) & 3);
}
