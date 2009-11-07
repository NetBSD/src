/*	$NetBSD: machdep.c,v 1.41 2009/11/07 07:27:43 cegger Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.41 2009/11/07 07:27:43 cegger Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <prop/proplib.h>

#include <machine/bus.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/walnut.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/dcr405gp.h>

#include <dev/cons.h>

#include "ksyms.h"

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif


#define TLB_PG_SIZE 	(16*1024*1024)

/*
 * Global variables used here and there
 */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

/*
 * This should probably be in autoconf!				XXX
 */
char cpu_model[80];
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

extern struct user *proc0paddr;

char bootpath[256];
paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

#if NKSYMS || defined(DDB) || defined(MODULAR)
void *startsym, *endsym;
#endif

int lcsplx(int);
void initppc(u_int, u_int, char *, void *);

static void dumpsys(void);
static void install_extint(void (*)(void));

#define MEMREGIONS	8
struct mem_region physmemr[MEMREGIONS];		/* Hard code memory */
struct mem_region availmemr[MEMREGIONS];	/* Who's supposed to set these up? */

struct board_cfg_data board_data;

void
initppc(u_int startkernel, u_int endkernel, char *args, void *info_block)
{
	extern int defaulttrap, defaultsize;
	extern int sctrap, scsize;
	extern int alitrap, alisize;
	extern int dsitrap, dsisize;
	extern int isitrap, isisize;
	extern int mchktrap, mchksize;
	extern int tlbimiss4xx, tlbim4size;
	extern int tlbdmiss4xx, tlbdm4size;
	extern int pitfitwdog, pitfitwdogsize;
	extern int debugtrap, debugsize;
	extern int errata51handler, errata51size;
#ifdef DDB
	extern int ddblow, ddbsize;
#endif
#ifdef IPKDB
	extern int ipkdblow, ipkdbsize;
#endif
	vaddr_t va;
	int exc, dbcr0;
	struct cpu_info * const ci = curcpu();

	/* Disable all external interrupts */
	mtdcr(DCR_UIC0_ER, 0);

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
	for (va = 0; va < endkernel; va += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);

	/* Map console after physmem (see pmap_tlbmiss()) */
	ppc4xx_tlb_reserve(0xef000000, roundup(physmemr[0].size, TLB_PG_SIZE),
	    TLB_PG_SIZE, TLB_I | TLB_G);

	/*
	 * Initialize lwp0 and current pcb and pmap pointers.
	 */
	lwp0.l_cpu = ci;
	lwp0.l_addr = proc0paddr;
	memset(lwp0.l_addr, 0, sizeof *lwp0.l_addr);

	curpcb = &proc0paddr->u_pcb;
	curpcb->pcb_pm = pmap_kernel();

	/*
	 * Set up trap vectors
	 */
	for (exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100)
		switch (exc) {
		default:
			memcpy((void *)exc, &defaulttrap, (size_t)&defaultsize);
			break;
		case EXC_EXI:
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
		case EXC_SC:
			memcpy((void *)EXC_SC, &sctrap, (size_t)&scsize);
			break;
		case EXC_ALI:
			memcpy((void *)EXC_ALI, &alitrap, (size_t)&alisize);
			break;
		case EXC_DSI:
			memcpy((void *)EXC_DSI, &dsitrap, (size_t)&dsisize);
			break;
		case EXC_ISI:
			memcpy((void *)EXC_ISI, &isitrap, (size_t)&isisize);
			break;
		case EXC_MCHK:
			memcpy((void *)EXC_MCHK, &mchktrap, (size_t)&mchksize);
			break;
		case EXC_ITMISS:
			memcpy((void *)EXC_ITMISS, &tlbimiss4xx,
				(size_t)&tlbim4size);
			break;
		case EXC_DTMISS:
			memcpy((void *)EXC_DTMISS, &tlbdmiss4xx,
				(size_t)&tlbdm4size);
			break;
		/* 
		 * EXC_PIT, EXC_FIT, EXC_WDOG handlers 
		 * are spaced by 0x10 bytes only.. 
		 */
		case EXC_PIT:	
			memcpy((void *)EXC_PIT, &pitfitwdog,
				(size_t)&pitfitwdogsize);
			break;
		case EXC_DEBUG:
			memcpy((void *)EXC_DEBUG, &debugtrap,
				(size_t)&debugsize);
			break;
		case EXC_DTMISS|EXC_ALI:
                        /* PPC405GP Rev D errata item 51 */	
			memcpy((void *)(EXC_DTMISS|EXC_ALI), &errata51handler,
				(size_t)&errata51size);
			break;
#if defined(DDB) || defined(IPKDB)
		case EXC_PGM:
#if defined(DDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#elif defined(IPKDB)
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#endif
#endif /* DDB | IPKDB */
			break;
		}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);
	mtspr(SPR_EVPR, 0);		/* Set Exception vector base */

	consinit();

	/* Handle trap instruction as PGM exception */
	__asm volatile("mfspr %0,%1":"=r"(dbcr0):"K"(SPR_DBCR0));
	__asm volatile("mtspr %0,%1"::"K"(SPR_DBCR0),"r"(dbcr0 & ~DBCR0_TDE));

	/*
	 * external interrupt handler install
	 */
	install_extint(ext_intr);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : : "r"(0), "K"(PSL_IR|PSL_DR)); 
	/* XXXX PSL_ME - With ME set kernel gets stuck... */

	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

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

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef IPKDB
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
}

static void
install_extint(void (*handler)(void))
{
	extern int extint, extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif
	__asm volatile ("mfmsr %0; wrteei 0" : "=r"(msr));
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	__asm volatile ("mtmsr %0" :: "r"(msr));
}

/*
 * Machine dependent startup code.
 */

char msgbuf[MSGBUFSIZE];

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

int
lcsplx(int ipl)
{

	return spllower(ipl); 	/* XXX */
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}
