/*	$NetBSD: machdep.c,v 1.1 2002/05/30 20:02:04 augustss Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
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
#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <powerpc/mpc6xx/bat.h>
#include <machine/bus.h>
#include <machine/db_machdep.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/pmppc.h>

#include <ddb/db_extern.h>

#include <dev/cons.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700uic.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Global variables used here and there
 */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

char machine[] = MACHINE;		/* machine */
char machine_arch[] = MACHINE_ARCH;	/* machine architecture */

/* Our exported CPU info; we have only one right now. */  
struct cpu_info cpu_info_store;

struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;

extern struct user *proc0paddr;

struct bat battable[16];

struct mem_region physmemr[2], availmemr[2];

paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

void lcsplx(int);		/* Called from locore */
void initppc(u_int, u_int, u_int, void *); /* Called from locore */

void install_extint(void (*)(void));

void dumpsys(void);
void strayintr(int);

void pmppc_setup(void);

#ifdef PMPPC_BAT_PRINT
static void
print_bats(void)
{
	int i;
	struct bat bats[16];

	asm volatile ("mfibatl %0,0; mfibatu %1,0"
		      : "=r"(bats[0].batl), "=r"(bats[0].batu));
	asm volatile ("mfibatl %0,1; mfibatu %1,1"
		      : "=r"(bats[1].batl), "=r"(bats[1].batu));
	asm volatile ("mfibatl %0,2; mfibatu %1,2"
		      : "=r"(bats[2].batl), "=r"(bats[2].batu));
	asm volatile ("mfibatl %0,3; mfibatu %1,3"
		      : "=r"(bats[3].batl), "=r"(bats[3].batu));
	for (i = 0; i < 4; i++)
		printf("BATI%d %08x %08x\n", i, bats[i].batu,
		       bats[i].batl);
	asm volatile ("mfdbatl %0,0; mfdbatu %1,0"
		      : "=r"(bats[0].batl), "=r"(bats[0].batu));
	asm volatile ("mfdbatl %0,1; mfdbatu %1,1"
		      : "=r"(bats[1].batl), "=r"(bats[1].batu));
	asm volatile ("mfdbatl %0,2; mfdbatu %1,2"
		      : "=r"(bats[2].batl), "=r"(bats[2].batu));
	asm volatile ("mfdbatl %0,3; mfdbatu %1,3"
		      : "=r"(bats[3].batl), "=r"(bats[3].batu));
	for (i = 0; i < 4; i++)
		printf("BATD%d %08x %08x\n", i, bats[i].batu,
		       bats[i].batl);
}
#endif

void print_intr_regs(void);
void
print_intr_regs(void)
{
	printf("CSR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_SR));
	printf("CER=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_ER));
	printf("CCR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_CR));
	printf("CPR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_PR));
	printf("CTR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_TR));
	printf("CMSR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_MSR));
	printf("CVR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_VR));
	printf("CVCR=%08x\n", in32(CPC_UIC_BASE + CPC_UIC_VCR));
}

void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{
	extern int trapcode, trapsize;
	extern int alitrap, alisize;
	extern int dsitrap, dsisize;
	extern int isitrap, isisize;
	extern int decrint, decrsize;
	extern int tlbimiss, tlbimsize;
	extern int tlbdlmiss, tlbdlmsize;
	extern int tlbdsmiss, tlbdsmsize;
#ifdef DDB
	extern int ddblow, ddbsize;
	extern void *startsym, *endsym;
#endif
#ifdef IPKDB
	extern int ipkdblow, ipkdbsize;
#endif
	extern void consinit(void);
	extern void ext_intr(void);
	extern u_long ticks_per_sec;
	extern unsigned char edata[], end[];
	int exc, scratch;

	memset(&edata, 0, end - edata); /* clear BSS */

	pmppc_setup();

	physmemr[0].start = 0;
	physmemr[0].size = a_config.a_mem_size;
	physmemr[1].size = 0;
	availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
	availmemr[0].size = a_config.a_mem_size - availmemr[0].start;
	availmemr[1].size = 0;

	proc0.p_addr = proc0paddr;
	memset(proc0.p_addr, 0, sizeof *proc0.p_addr);

	curpcb = &proc0paddr->u_pcb;

	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

#ifdef BOOTHOWTO
	/*
	 * boothowto
	 */
	boothowto = BOOTHOWTO;
#endif

	pmppc_bus_space_init();

	consinit();		/* XXX should not be here */
	printf("console set up\n");

	/*
	 * Get CPU clock
	 */
	ticks_per_sec = a_config.a_bus_freq;
	ticks_per_sec /= 4;	/* 4 cycles per DEC tick */
	cpu_timebase = ticks_per_sec;
	cpu_initclocks();

#ifdef PMPPC_BAT_PRINT
	print_bats();
#endif

	/*
	 * Initialize BAT registers to unmapped to not generate
	 * overlapping mappings below.
	 */
	asm volatile ("mtibatu 0,%0" :: "r"(0));
	asm volatile ("mtibatu 1,%0" :: "r"(0));
	asm volatile ("mtibatu 2,%0" :: "r"(0));
	asm volatile ("mtibatu 3,%0" :: "r"(0));
	asm volatile ("mtdbatu 0,%0" :: "r"(0));
	asm volatile ("mtdbatu 1,%0" :: "r"(0));
	asm volatile ("mtdbatu 2,%0" :: "r"(0));
	asm volatile ("mtdbatu 3,%0" :: "r"(0));

	/*
	 * Set up initial BAT table
	 */
	/* map the lowest 256M area */
	battable[0].batl = BATL(0x00000000, BAT_M, BAT_PP_RW);
	battable[0].batu = BATU(0x00000000, BAT_BL_256M, BAT_Vs);

	/* map the flash (etc) memory 256M area */
	battable[1].batl = BATL(PMPPC_FLASH_BASE, BAT_I, BAT_G | BAT_PP_RW);
	battable[1].batu = BATU(PMPPC_FLASH_BASE, BAT_BL_256M, BAT_Vs);

	/* map the PCI memory 256M area */
	battable[2].batl = BATL(CPC_PCI_MEM_BASE, BAT_I, BAT_G | BAT_PP_RW);
	battable[2].batu = BATU(CPC_PCI_MEM_BASE, BAT_BL_256M, BAT_Vs);

	/* map the PCI I/O 128M area */
	battable[3].batl = BATL(CPC_PCI_IO_BASE, BAT_I, BAT_G | BAT_PP_RW);
	battable[3].batu = BATU(CPC_PCI_IO_BASE, BAT_BL_128M, BAT_Vs);

	/*
	 * Now setup fixed bat registers
	 */
	asm volatile ("mtibatl 0,%0; mtibatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	asm volatile ("mtibatl 1,%0; mtibatu 1,%1"
		      :: "r"(battable[1].batl), "r"(battable[1].batu));
	asm volatile ("mtibatl 2,%0; mtibatu 2,%1"
		      :: "r"(battable[2].batl), "r"(battable[2].batu));
	asm volatile ("mtibatl 3,%0; mtibatu 3,%1"
		      :: "r"(battable[3].batl), "r"(battable[3].batu));

	asm volatile ("mtdbatl 0,%0; mtdbatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	asm volatile ("mtdbatl 1,%0; mtdbatu 1,%1"
		      :: "r"(battable[1].batl), "r"(battable[1].batu));
	asm volatile ("mtdbatl 2,%0; mtdbatu 2,%1"
		      :: "r"(battable[2].batl), "r"(battable[2].batu));
	asm volatile ("mtdbatl 3,%0; mtdbatu 3,%1"
		      :: "r"(battable[3].batl), "r"(battable[3].batu));

#ifdef ART_BAT_PRINT
	print_bats();
#endif

	/*
	 * Set up trap vectors
	 */
	for (exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100)
		switch (exc) {
		default:
			memcpy((void *)exc, &trapcode, (size_t)&trapsize);
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
		case EXC_DECR:
			memcpy((void *)EXC_DECR, &decrint, (size_t)&decrsize);
			break;
		case EXC_IMISS:
			memcpy((void *)EXC_IMISS, &tlbimiss, (size_t)&tlbimsize);
			break;
		case EXC_DLMISS:
			memcpy((void *)EXC_DLMISS, &tlbdlmiss, (size_t)&tlbdlmsize);
			break;
		case EXC_DSMISS:
			memcpy((void *)EXC_DSMISS, &tlbdsmiss, (size_t)&tlbdsmsize);
			break;
#if defined(DDB) || defined(IPKDB)
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#else
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#endif
			break;
#endif /* DDB || IPKDB */
		}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

	/*
	 * external interrupt handler install
	 */
	install_extint(ext_intr);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : "=r"(scratch) : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

        /*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel, NULL);

#ifdef DDB
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
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

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{
	*mem = physmemr;
	*avail = availmemr;
}

void
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
void
cpu_startup()
{
	int sz, i;
	caddr_t v;
	vaddr_t minaddr, maxaddr;
	int base, residual;
	int msr;
	char pbuf[9];

	proc0.p_addr = proc0paddr;
	v = (caddr_t)proc0paddr + USPACE;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	if (!(msgbuf_vaddr = uvm_km_alloc(kernel_map, round_page(MSGBUFSIZE))))
		panic("startup: no room for message buffer");
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), msgbuf_vaddr + i * NBPG,
		    msgbuf_paddr + i * NBPG, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	initmsgbuf((caddr_t)msgbuf_vaddr, round_page(MSGBUFSIZE));

	printf("%s", version);
	cpu_identify(NULL, 0);

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
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(sz),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
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

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("startup: not enough memory for "
					"buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(kernel_map->pmap);

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

#ifndef PMAP_MAP_POOLPAGE
	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    mclbytes*nmbclusters, VM_MAP_INTRSAFE, FALSE, NULL);
#endif

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	pmppc_bus_space_mallocok();

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);


	/*
	 * Set up the buffers.
	 */
	bufinit();

	/* Set up interrupt controller */
	cpc700_init_intr(&pmppc_mem_tag, CPC_UIC_BASE,
			 CPC_INTR_MASK(PMPPC_I_ETH_INT), 0);

	/*
	 * Now allow hardware interrupts.
	 */
	asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
		      : "=r"(msr) : "K"(PSL_EE));
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	static int initted;
#if (NCOM > 0)
	bus_space_tag_t tag;
#endif

	if (initted)
		return;
	initted = 1;

#if (NCOM > 0)
	tag = &pmppc_mem_tag;

	if(comcnattach(tag, CPC_COM0, 9600, CPC_COM_SPEED(a_config.a_bus_freq),
	    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8))) 
		panic("can't init serial console");
	else
		return;
#endif

	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
}

void
dumpsys()
{
	printf("dumpsys: not implemented\n");
}

/*
 * Soft networking interrupts.
 */
void
softnet(int isr)
{
#define DONETISR(bit, fn) do {		\
	if (isr & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

/*
 * Stray interrupts.
 */
void
strayintr(int irq)
{
	log(LOG_ERR, "stray interrupt %d\n", irq);
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
	extern void disable_intr(void);

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		printf("halted\n\n");
		while(1);
	}
	if (!cold && (howto & RB_DUMP))
		dumpsys();
	doshutdownhooks();
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

	disable_intr();

        /* Write the two byte reset sequence to the reset register. */
	out8(PMPPC_RESET, PMPPC_RESET_SEQ_STEP1);
	out8(PMPPC_RESET, PMPPC_RESET_SEQ_STEP2);

	while (1);
}

void
lcsplx(int ipl)
{
	splx(ipl);
}

void
setleds(int leds)
{
	out8(PMPPC_LEDS, leds);
}

void
pmppc_setup(void)
{
	uint config0, config1;

	config0 = in8(PMPPC_CONFIG0);
	config1 = in8(PMPPC_CONFIG1);

	/* from page 2-8 in the Artesyn User's manual */
	a_config.a_boot_device = config1 & 0x80 ? A_BOOT_FLASH : A_BOOT_ROM;
	a_config.a_has_ecc = (config1 & 0x40) != 0;
	switch (config1 & 0x30) {
	case 0x00: a_config.a_mem_size = 32 * 1024 * 1024; break;
	case 0x10: a_config.a_mem_size = 64 * 1024 * 1024; break;
	case 0x20: a_config.a_mem_size = 128 * 1024 * 1024; break;
	case 0x30: a_config.a_mem_size = 256 * 1024 * 1024; break;
	}
	a_config.a_l2_cache = (config1 >> 2) & 3;
	switch (config1 & 0x03) {
	case 0x00: a_config.a_bus_freq = 66666666; break;
	case 0x01: a_config.a_bus_freq = 83333333; break;
	case 0x02: a_config.a_bus_freq = 100000000; break;
	case 0x03: a_config.a_bus_freq = 0; break; /* XXX */
	}
	a_config.a_is_monarch = (config0 & 0x80) == 0;
	a_config.a_has_eth = (config0 & 0x20) != 0;
	a_config.a_has_rtc = (config0 & 0x10) == 0;
	switch (config0 & 0x0c) {
	case 0x00: a_config.a_flash_size = 256 * 1024 * 1024; break;
	case 0x04: a_config.a_flash_size = 128 * 1024 * 1024; break;
	case 0x08: a_config.a_flash_size = 64 * 1024 * 1024; break;
	case 0x0c: a_config.a_flash_size = 32 * 1024 * 1024; break;
	}
	switch (config0 & 0x03) {
	case 0x00: a_config.a_flash_width = 64; break;
	case 0x01: a_config.a_flash_width = 32; break;
	case 0x02: a_config.a_flash_width = 16; break;
	case 0x03: a_config.a_flash_width = 0; break;
	}
}
