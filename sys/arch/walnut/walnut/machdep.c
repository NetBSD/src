/*	$NetBSD: machdep.c,v 1.2.2.4 2002/06/23 17:43:14 jdolecek Exp $	*/

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

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/map.h>
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
#include <sys/properties.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/bus.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/walnut.h>
#include <machine/dcr.h>

#include <powerpc/spr.h>

#include <dev/cons.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

/*
 * Global variables used here and there
 */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

/*
 * This should probably be in autoconf!				XXX
 */
char cpu_model[80];
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;		/* XXX - shouldn't need this on fpu-less CPUs */

extern struct user *proc0paddr;

char bootpath[256];
paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

#ifdef DDB
void *startsym, *endsym;
#endif

int lcsplx(int);
void initppc(u_int, u_int, char *, void *);

static void dumpsys(void);
static void install_extint(void (*)(void));

#define MEMREGIONS	8
struct mem_region physmemr[MEMREGIONS];		/* Hard code memory */
struct mem_region availmemr[MEMREGIONS];	/* Who's supposed to set these up? */

int fake_mapiodev = 1;
struct board_cfg_data board_data;
struct propdb *board_info = NULL;

void
initppc(u_int startkernel, u_int endkernel, char *args, void *info_block)
{
	extern int defaulttrap, defaultsize;
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
	int exc;
	extern char _edata, _end;

	/* Disable all external interrupts */
	mtdcr(DCR_UIC0_ER, 0);

        /* Initialize cache info for memcpy, etc. */
        cpu_probe_cache();

	memset(&_edata, 0, &_end-&_edata); /* Clear BSS area */

	/* Save info block */
	if (info_block == NULL)
		/* XXX why isn't r3 set correctly?!?!? */
		info_block = (void *)0x8e10;		
	memcpy(&board_data, info_block, sizeof(board_data));

	memset(physmemr, 0, sizeof physmemr);
	memset(availmemr, 0, sizeof availmemr);
	physmemr[0].start = 0;
	physmemr[0].size = board_data.mem_size & ~PGOFSET;
	/* Lower memory reserved by eval board BIOS */
	availmemr[0].start = startkernel; 
	availmemr[0].size = board_data.mem_size - availmemr[0].start;

	proc0.p_addr = proc0paddr;
	memset(proc0.p_addr, 0, sizeof *proc0.p_addr);

	curpcb = &proc0paddr->u_pcb;

	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

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
		case EXC_PGM:
#if defined(DDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#elif defined(IPKDB)
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#else
			memcpy((void *)exc, &pgmtrap, (size_t)&pgmsize);
#endif
			break;
		}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);
	mtspr(SPR_EVPR, 0);		/* Set Exception vector base */
	consinit();

	/* Handle trap instruction as PGM exception */
	{
	  int dbcr0;
	  asm volatile("mfspr %0,%1":"=r"(dbcr0):"K"(SPR_DBCR0));
	  asm volatile("mtspr %0,%1"::"K"(SPR_DBCR0),"r"(dbcr0 & ~DBCR0_TDE));
	}

	/*
	 * external interrupt handler install
	 */
	install_extint(ext_intr);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : : "r"(0), "K"(PSL_IR|PSL_DR)); 
	/* XXXX PSL_ME - With ME set kernel gets stuck... */

	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

	consinit();

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
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
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
	fake_mapiodev = 0;
	printf("Done with initppc()\n");
}

static void
install_extint(void (*handler)(void))
{
	extern int extint, extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int omsr, msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif
	asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(omsr), "=r"(msr) : "K"((u_short)~PSL_EE));
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	asm volatile ("mtmsr %0" :: "r"(omsr));
}

/*
 * Machine dependent startup code.
 */

char msgbuf[MSGBUFSIZE];

void
cpu_startup(void)
{
	int sz, i;
	caddr_t v;
	vaddr_t minaddr, maxaddr;
	int base, residual;
	char pbuf[9];

	proc0.p_addr = proc0paddr;
	v = (caddr_t)proc0paddr + USPACE;

	/*
	 * Initialize error message buffer (at end of core).
	 */
#if 0	/* For some reason this fails... --Artem
	 * Besides, do we really have to put it at the end of core?
	 * Let's use static buffer for now
	 */
	if (!(msgbuf_vaddr = uvm_km_alloc(kernel_map, round_page(MSGBUFSIZE))))
		panic("startup: no room for message buffer");
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa(msgbuf_vaddr + i * NBPG,
			msgbuf_paddr + i * NBPG, VM_PROT_READ|VM_PROT_WRITE);
	initmsgbuf((caddr_t)msgbuf_vaddr, round_page(MSGBUFSIZE));
#else
	initmsgbuf((caddr_t)msgbuf, round_page(MSGBUFSIZE));
#endif


	printf("%s", version);
	printf("Walnut PowerPC 405GP Evaluation Board\n");

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	sz = MAXBSIZE * nbuf;
	minaddr = 0;
	if (uvm_map(kernel_map, (vaddr_t *)&minaddr, round_page(sz),
		NULL, UVM_UNKNOWN_OFFSET, 0,
		UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
			    UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	buffers = (char *)minaddr;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE) {
		/* Don't want to alloc more physical mem than ever needed */
		base = MAXBSIZE;
		residual = 0;
	}
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = NBPG * (i < residual ? base + 1 : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up the buffers.
	 */
	bufinit();

	/*
	 * Set up the board properties database.
	 */
	if (!(board_info = propdb_create("board info")))
		panic("Cannot create board info database");

	if (board_info_set("mem-size", &board_data.mem_size, 
		sizeof(&board_data.mem_size), PROP_CONST, 0))
		panic("setting mem-size");
	if (board_info_set("emac-mac-addr", &board_data.mac_address_local, 
		sizeof(&board_data.mac_address_local), PROP_CONST, 0))
		panic("setting emac-mac-addr");
	if (board_info_set("sip0-mac-addr", &board_data.mac_address_pci, 
		sizeof(&board_data.mac_address_pci), PROP_CONST, 0))
		panic("setting sip0-mac-addr");
	if (board_info_set("processor-frequency", &board_data.processor_speed, 
		sizeof(&board_data.processor_speed), PROP_CONST, 0))
		panic("setting processor-frequency");

}


static void
dumpsys(void)
{

	printf("dumpsys: TBD\n");
}

/*
 * Soft networking interrupts.
 */
void
softnet(void)
{
	extern volatile int netisr;
	int isr;

	isr = netisr;
	netisr = 0;

#define DONETISR(bit, fn) do {		\
	if (isr & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR

}

#include "com.h"
/*
 * Soft tty interrupts.
 */
void
softserial(void)
{
#if NCOM > 0
	void comsoft(void);	/* XXX from dev/ic/com.c */

	comsoft();
#endif
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

/*
 * Allocate vm space and mapin the I/O address
 */
void *
mapiodev(paddr_t pa, psize_t len)
{
	paddr_t faddr;
	vaddr_t taddr, va;
	int off;

	/* 
	 * Initially we cannot use uvm_ functions, but we still have to map
	 * console.. 
	 */
	if (fake_mapiodev)	
		return (void *)pa;

	faddr = trunc_page(pa);
	off = pa - faddr;
	len = round_page(off + len);
	va = taddr = uvm_km_valloc(kernel_map, len);

	if (va == 0)
		return NULL;

	for (; len > 0; len -= NBPG) {
		pmap_kenter_pa(taddr, faddr, 
			VM_PROT_READ|VM_PROT_WRITE|PME_NOCACHE);
		faddr += NBPG;
		taddr += NBPG;
	}

	return (void *)(va + off);
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}
