/*	$NetBSD: machdep.c,v 1.1.2.2 2002/02/28 04:11:08 nathanw Exp $	*/

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
#include "opt_mvmetype.h"
#include "opt_ddb.h"

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
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <net/netisr.h>

#include <machine/autoconf.h>
#include <machine/bat.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/platform.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <dev/cons.h>

#if 0
#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
void comsoft(void);
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

void initppc(u_long, u_long, void *);
void dumpsys(void);
void strayintr(int);
int lcsplx(int);

/* Our exported CPU info; we have only one right now. */  
struct cpu_info cpu_info_store;

/*
 * Global variables used here and there
 */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

struct mvmeppc_bootinfo bootinfo;

char machine[] = MACHINE;		/* machine */
char machine_arch[] = MACHINE_ARCH;	/* machine architecture */

struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;

extern struct user *proc0paddr;

struct bat battable[16];

vaddr_t mvmeppc_intr_reg;	/* PReP-compatible  interrupt vector register */

struct mem_region physmemr, availmemr;

paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

paddr_t avail_end;			/* XXX temporary */

void install_extint(void (*)(void));


void
initppc(startkernel, endkernel, btinfo)
	u_long startkernel, endkernel;
	void *btinfo;
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
	int exc, scratch;

	/*
	 * Copy bootinfo.
	 */
	memcpy(&bootinfo, btinfo, sizeof(bootinfo));

	/*
	 * Figure out the board family/type.
	 */
	ident_platform();

	if (platform == NULL) {
		extern void _mvmeppc_unsup_board(const char *, const char *);
		char msg[80];

		sprintf(msg, "Unsupported model: MVME%04x",
		    bootinfo.bi_modelnumber);
		_mvmeppc_unsup_board(msg, &msg[strlen(msg)]);
		/* NOTREACHED */
	}

	/*
	 * Set memory region
	 */
	physmemr.start = 0;
	physmemr.size = bootinfo.bi_memsize & ~PGOFSET;
	availmemr.start = (endkernel + PGOFSET) & ~PGOFSET;
	availmemr.size = bootinfo.bi_memsize - availmemr.start;
	avail_end = physmemr.start + physmemr.size;    /* XXX temporary */

	/*
	 * Set CPU clock
	 */
	{
		extern u_long ticks_per_sec, ns_per_tick;

		ticks_per_sec = bootinfo.bi_clocktps;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	proc0.p_addr = proc0paddr;
	memset(proc0.p_addr, 0, sizeof *proc0.p_addr);

	curpcb = &proc0paddr->u_pcb;

	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

	/*
	 * boothowto
	 */
	boothowto = bootinfo.bi_boothowto;

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
	/* map the lowest 256 MB area */
	battable[0].batl = BATL(0x00000000, BAT_M, BAT_PP_RW);
	battable[0].batu = BATU(0x00000000, BAT_BL_256M, BAT_Vs);

	/* map the PCI/ISA I/O 256 MB area */
	battable[MVMEPPC_PHYS_BASE_IO >> 28].batl =
	    BATL(MVMEPPC_PHYS_BASE_IO, BAT_I, BAT_PP_RW);
	battable[MVMEPPC_PHYS_BASE_IO >> 28].batu =
	    BATU(MVMEPPC_KVA_BASE_IO, BAT_BL_256M, BAT_Vs);

	/* map the PCI/ISA MEMORY 256 MB area */
	battable[MVMEPPC_PHYS_BASE_MEM >> 28].batl =
	    BATL(MVMEPPC_PHYS_BASE_MEM, BAT_I, BAT_PP_RW);
	battable[MVMEPPC_PHYS_BASE_MEM >> 28].batu =
	    BATU(MVMEPPC_KVA_BASE_MEM, BAT_BL_256M, BAT_Vs);

	/*
	 * Now setup fixed bat registers
	 */
	asm volatile ("mtibatl 0,%0; mtibatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	asm volatile ("mtdbatl 0,%0; mtdbatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));

	asm volatile ("mtdbatl 2,%0; mtdbatu 1,%1"
	    :: "r"(battable[MVMEPPC_PHYS_BASE_IO >> 28].batl),
	       "r"(battable[MVMEPPC_PHYS_BASE_IO >> 28].batu));
	asm volatile ("mtdbatl 3,%0; mtdbatu 2,%1"
	    :: "r"(battable[MVMEPPC_PHYS_BASE_MEM >> 28].batl),
	       "r"(battable[MVMEPPC_PHYS_BASE_MEM >> 28].batu));

	asm volatile ("sync; isync");

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
			memcpy((void *)EXC_IMISS, &tlbimiss,
			    (size_t)&tlbimsize);
			break;
		case EXC_DLMISS:
			memcpy((void *)EXC_DLMISS, &tlbdlmiss,
			    (size_t)&tlbdlmsize);
			break;
		case EXC_DSMISS:
			memcpy((void *)EXC_DSMISS, &tlbdsmiss,
			    (size_t)&tlbdsmsize);
			break;
#ifdef DDB
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
			break;
#endif
		}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

#ifdef DEBUG
	/*
	 * i386 port says, that this shouldn't be here,
	 * but I really think the console should be initialized
	 * as early as possible.
	 */
	consinit();
#endif

	/*
	 * external interrupt handler install
	 */
	install_extint(platform->ext_intr);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	asm volatile ("eieio; mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : "=r"(scratch) : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

        /*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#ifdef DDB
	ddb_init((int)((u_long)endsym - (u_long)startsym), startsym, endsym);

	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
mem_regions(mem, avail)
	struct mem_region **mem, **avail;
{

	*mem = &physmemr;
	*avail = &availmemr;
}

void
install_extint(handler)
	void (*handler)(void);
{
	extern u_char extint[];
	extern u_long extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int omsr, msr;

#ifdef DIAGNOSTIC
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
	char pbuf[9];

	proc0.p_addr = proc0paddr;
	v = (caddr_t)proc0paddr + USPACE;

	/*
	 * Mapping PReP-compatible interrput vector register.
	 */
	if (!(mvmeppc_intr_reg = uvm_km_valloc(kernel_map, round_page(NBPG))))
		panic("startup: no room for interrupt register");
	pmap_enter(pmap_kernel(), mvmeppc_intr_reg, MVMEPPC_INTR_REG,
	    VM_PROT_READ|VM_PROT_WRITE, VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());

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

	printf("Model: %s\n", platform->model);
	printf("Core Speed: %dMHz, Bus Speed: %dMHz\n",
		bootinfo.bi_mpuspeed/1000000, bootinfo.bi_busspeed/1000000);

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
	 * We need to allocate an mbuf cluster submap if the pool
	 * allocater isn't using direct-mapped pool pages.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up the buffers.
	 */
	bufinit();

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;

		splraise(-1);
		asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
	}

	mvmeppc_bus_space_init();
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	static int initted = 0;

	if (initted)
		return;
	initted = 1;

#if 0

#if (NPFB > 0)
	if (!strcmp(consinfo->devname, "fb")) {
		pfb_cnattach(consinfo->addr);
#if (NPCKBC > 0)
		pckbc_cnattach(&mvmeppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif

#if (NVGA > 0) || (NGTEN > 0)
	if (!strcmp(consinfo->devname, "vga")) {
#if (NGTEN > 0)
		if (!gten_cnattach(&mvmeppc_mem_space_tag))
			goto dokbd;
#endif
#if (NVGA > 0)
		if (!vga_cnattach(&mvmeppc_io_space_tag, &mvmeppc_mem_space_tag,
				-1, 1))
			goto dokbd;
#endif
dokbd:
#if (NPCKBC > 0)
		pckbc_cnattach(&mvmeppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* PC | VGA */

#endif

#if (NCOM > 0)
	if (!strcmp(bootinfo.bi_consoledev, "PC16550")) {
		bus_space_tag_t tag = &mvmeppc_isa_io_bs_tag;
		bus_addr_t caddr[2] = {0x3f8, 0x2f8};
		int rv;
		rv = comcnattach(tag, caddr[bootinfo.bi_consolechan],
		    bootinfo.bi_consolespeed, COM_FREQ,
		    bootinfo.bi_consolecflag);
		if (rv)
			panic("can't init serial console");

		return;
	}
#endif
	panic("invalid console device %s", bootinfo.bi_consoledev);
}

void
dumpsys()
{

	printf("dumpsys: TBD\n");
}

/*
 * Soft networking interrupts.
 */
void
softnet()
{
	extern volatile int netisr;
	int isr;

	isr = netisr;
	netisr = 0;

#define DONETISR(bit, fn) do {	\
	if (isr & (1 << bit))	\
		fn();		\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

/*
 * Soft tty interrupts.
 */
void
softserial()
{

#if (NCOM > 0)
	comsoft();
#endif
}

/*
 * Stray interrupts.
 */
void
strayintr(irq)
	int irq;
{

	log(LOG_ERR, "stray interrupt %d\n", irq);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(howto, what)
	int howto;
	char *what;
{
	static int syncing;

	if (cold) {
		howto |= RB_HALT;
		goto halt_sys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && syncing == 0) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

	/* Disable intr */
	splhigh();

	/* Do dump if requested */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

halt_sys:
	doshutdownhooks();

	if (howto & RB_HALT) {
                printf("\n");
                printf("The operating system has halted.\n");
                printf("Please press any key to reboot.\n\n");
                cnpollc(1);	/* for proper keyboard command handling */
                cngetc();
                cnpollc(0);
	}

	printf("rebooting...\n\n");

#if 0
	(*platform->reset)();
#endif

	for (;;)
		continue;
	/* NOTREACHED */
}

/*
 * lcsplx() is called from locore; it is an open-coded version of
 * splx() differing in that it returns the previous priority level.
 */
int
lcsplx(ipl)
	int ipl;
{
	int oldcpl;

	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	oldcpl = cpl;
	cpl = ipl;
	if (ipending & ~ipl)
		do_pending_int();
	__asm__ volatile("sync; eieio\n");	/* reorder protect */

	return (oldcpl);
}
