/*	$NetBSD: machdep.c,v 1.39 1999/06/22 02:04:06 sakamoto Exp $	*/

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
#include "ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/callout.h>
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

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/bat.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <dev/cons.h>

#include "pfb.h"

#include "pc.h"
#if (NPC > 0)
#include <machine/pccons.h>
#endif

#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/pckbcvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Global variables used here and there
 */
vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

char bootinfo[BOOTINFO_MAXSIZE];

char machine[] = MACHINE;		/* machine */
char machine_arch[] = MACHINE_ARCH;	/* machine architecture */

struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;

extern struct user *proc0paddr;

struct bat battable[16];

paddr_t bebox_mb_reg;		/* BeBox MotherBoard register */

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

int astpending;

char *bootpath;

paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

paddr_t avail_end;			/* XXX temporary */

void install_extint __P((void (*)(void)));
int cold = 1;

void
initppc(startkernel, endkernel, args, btinfo)
	u_int startkernel, endkernel, args;
	void *btinfo;
{
	extern trapcode, trapsize;
	extern dsitrap, dsisize;
	extern isitrap, isisize;
	extern decrint, decrsize;
	extern tlbimiss, tlbimsize;
	extern tlbdlmiss, tlbdlmsize;
	extern tlbdsmiss, tlbdsmsize;
#ifdef DDB
	extern ddblow, ddbsize;
	extern void *startsym, *endsym;
#endif
#if NIPKDB > 0
	extern ipkdblow, ipkdbsize;
#endif
	extern void consinit __P((void));
	extern void ext_intr __P((void));
	int exc, scratch, i;

	/*
	 * copy bootinfo
	 */
	bcopy(btinfo, bootinfo, sizeof (bootinfo));

	/*
	 * BeBox memory region set
	 */
	{
		struct btinfo_memory *meminfo;

		meminfo =
			(struct btinfo_memory *)lookup_bootinfo(BTINFO_MEMORY);
		if (!meminfo)
			panic("not found memory information in bootinfo");
		physmemr[0].start = 0;
		physmemr[0].size = meminfo->memsize & ~PGOFSET;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = meminfo->memsize - availmemr[0].start;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Get CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		extern u_long ticks_per_sec, ns_per_tick;

		clockinfo =
			(struct btinfo_clock *)lookup_bootinfo(BTINFO_CLOCK);
		if (!clockinfo)
			panic("not found clock information in bootinfo");
		ticks_per_sec = clockinfo->ticks_per_sec;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * BeBox MotherBoard's Register
	 *  Interrupt Mask Reset
	 */
	*(volatile u_int *)(MOTHER_BOARD_REG + CPU0_INT_MASK) = 0x0ffffffc;
	*(volatile u_int *)(MOTHER_BOARD_REG + CPU0_INT_MASK) = 0x80000023;
	*(volatile u_int *)(MOTHER_BOARD_REG + CPU1_INT_MASK) = 0x0ffffffc;

	proc0.p_addr = proc0paddr;
	bzero(proc0.p_addr, sizeof *proc0.p_addr);

	curpcb = &proc0paddr->u_pcb;

	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

	/*
	 * boothowto
	 */
	boothowto = args;

	/*
	 * i386 port says, that this shouldn't be here,
	 * but I really think the console should be initialized
	 * as early as possible.
	 */
	consinit();

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
	battable[0].batl = BATL(0x00000000, BAT_M);
	battable[0].batu = BATU(0x00000000);
	/* map the PCI/ISA I/O 256 MB area */
	battable[1].batl = BATL(BEBOX_BUS_SPACE_IO, BAT_I);
	battable[1].batu = BATU(BEBOX_BUS_SPACE_IO);
	/* map the PCI/ISA MEMORY 256 MB area */
	battable[2].batl = BATL(BEBOX_BUS_SPACE_MEM, BAT_I);
	battable[2].batu = BATU(BEBOX_BUS_SPACE_MEM);

	/*
	 * Now setup fixed bat registers
	 */
	asm volatile ("mtibatl 0,%0; mtibatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	asm volatile ("mtibatl 1,%0; mtibatu 1,%1"
		      :: "r"(battable[1].batl), "r"(battable[1].batu));
	asm volatile ("mtibatl 2,%0; mtibatu 2,%1"
		      :: "r"(battable[2].batl), "r"(battable[2].batu));

	asm volatile ("mtdbatl 0,%0; mtdbatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	asm volatile ("mtdbatl 1,%0; mtdbatu 1,%1"
		      :: "r"(battable[1].batl), "r"(battable[1].batu));
	asm volatile ("mtdbatl 2,%0; mtdbatu 2,%1"
		      :: "r"(battable[2].batl), "r"(battable[2].batu));

	/*
	 * Set up trap vectors
	 */
	for (exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100)
		switch (exc) {
		default:
			bcopy(&trapcode, (void *)exc, (size_t)&trapsize);
			break;
		case EXC_EXI:
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
		case EXC_DSI:
			bcopy(&dsitrap, (void *)EXC_DSI, (size_t)&dsisize);
			break;
		case EXC_ISI:
			bcopy(&isitrap, (void *)EXC_ISI, (size_t)&isisize);
			break;
		case EXC_DECR:
			bcopy(&decrint, (void *)EXC_DECR, (size_t)&decrsize);
			break;
		case EXC_IMISS:
			bcopy(&tlbimiss, (void *)EXC_IMISS, (size_t)&tlbimsize);
			break;
		case EXC_DLMISS:
			bcopy(&tlbdlmiss, (void *)EXC_DLMISS, (size_t)&tlbdlmsize);
			break;
		case EXC_DSMISS:
			bcopy(&tlbdsmiss, (void *)EXC_DSMISS, (size_t)&tlbdsmsize);
			break;
#if defined(DDB) || NIPKDB > 0
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB)
			bcopy(&ddblow, (void *)exc, (size_t)&ddbsize);
#else
			bcopy(&ipkdblow, (void *)exc, (size_t)&ipkdbsize);
#endif
			break;
#endif /* DDB || NIPKDB > 0 */
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
	pmap_bootstrap(startkernel, endkernel);

#ifdef DDB
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif
#if NIPKDB > 0
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
}

void
mem_regions(mem, avail)
	struct mem_region **mem, **avail;
{
	*mem = physmemr;
	*avail = availmemr;
}

/*
 * This should probably be in autoconf!				XXX
 */
int cpu;
char cpu_model[80];
char cpu_name[] = "PowerPC";	/* cpu architecture */

void
identifycpu()
{
	int phandle, pvr;
	char name[32];

	/*
	 * Find cpu type
	 */
	asm ("mfpvr %0" : "=r"(pvr));
	cpu = pvr >> 16;
	switch (cpu) {
	case 1:
		sprintf(cpu_model, "601");
		break;
	case 3:
		sprintf(cpu_model, "603");
		break;
	case 4:
		sprintf(cpu_model, "604");
		break;
	case 5:
		sprintf(cpu_model, "602");
		break;
	case 6:
		sprintf(cpu_model, "603e");
		break;
	case 7:
		sprintf(cpu_model, "603ev");
		break;
	case 9:
		sprintf(cpu_model, "604ev");
		break;
	case 20:
		sprintf(cpu_model, "620");
		break;
	default:
		sprintf(cpu_model, "Version %x", cpu);
		break;
	}
	sprintf(cpu_model + strlen(cpu_model), " (Revision %x)", pvr & 0xffff);
	printf("CPU: %s %s\n", cpu_name, cpu_model);
}

void
install_extint(handler)
	void (*handler) __P((void));
{
	extern extint, extsize;
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
	bcopy(&extint, (void *)EXC_EXI, (size_t)&extsize);
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
	 * BeBox Mother Board's Register Mapping
	 */
	if (!(bebox_mb_reg = uvm_km_valloc(kernel_map, round_page(NBPG))))
		panic("initppc: no room for interrupt register");
	pmap_enter(pmap_kernel(), bebox_mb_reg, MOTHER_BOARD_REG,
	    VM_PROT_READ|VM_PROT_WRITE, TRUE, VM_PROT_READ|VM_PROT_WRITE);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	if (!(msgbuf_vaddr = uvm_km_alloc(kernel_map, round_page(MSGBUFSIZE))))
		panic("startup: no room for message buffer");
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), msgbuf_vaddr + i * NBPG,
		    msgbuf_paddr + i * NBPG, VM_PROT_READ|VM_PROT_WRITE, TRUE,
		    VM_PROT_READ|VM_PROT_WRITE);
	initmsgbuf((caddr_t)msgbuf_vaddr, round_page(MSGBUFSIZE));

	printf("%s", version);
	identifycpu();

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
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
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
		curbufsize = CLBYTES * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("startup: not enough memory for "
					"buffer cache");
			pmap_enter(kernel_map->pmap, curbuf,
			    VM_PAGE_TO_PHYS(pg), VM_PROT_READ|VM_PROT_WRITE,
			    TRUE, VM_PROT_READ|VM_PROT_WRITE);
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

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i - 1].c_next = &callout[i];

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * CLBYTES);
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

		splhigh();
		asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
	}
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
		(size_t)help < (size_t)bootinfo + sizeof (bootinfo));

	return (NULL);
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	struct btinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

	consinfo = (struct btinfo_console *)lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
		panic("not found console information in bootinfo");

#if (NPFB > 0)
	if (!strcmp(consinfo->devname, "be")) {
		pfb_cnattach(consinfo->addr);
#if (NPCKBC > 0)
		pckbc_cnattach(BEBOX_BUS_SPACE_IO, PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif

#if (NPC > 0) || (NVGA > 0)
	if (!strcmp(consinfo->devname, "vga")) {
#if (NVGA > 0)
		if (!vga_cnattach(BEBOX_BUS_SPACE_IO, BEBOX_BUS_SPACE_MEM,
				  -1, 1))
			goto dokbd;
#endif
#if (NPC > 0)
		pccnattach();
#endif
dokbd:
#if (NPCKBC > 0)
		pckbc_cnattach(BEBOX_BUS_SPACE_IO, PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* PC | VGA */

#if (NCOM > 0)
	if (!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = BEBOX_BUS_SPACE_IO;

		if(comcnattach(tag, consinfo->addr, consinfo->speed, COM_FREQ,
		    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
			panic("can't init serial console");

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#if (NPCKBC > 0) && (NPCKBD == 0)
/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbc_machdep_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
{
#if (NPC > 0)
	return (pcconskbd_cnattach(kbctag, kbcslot));
#else
	return (ENXIO);
#endif
}
#endif

/*
 * Set set up registers on exec.
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = trapframe(p);
	struct ps_strings arginfo;

	bzero(tf, sizeof *tf);
	tf->fixreg[1] = -roundup(-stack + 8, 16);

	/*
	 * XXX Machine-independent code has already copied arguments and
	 * XXX environment to userland.  Get them back here.
	 */
	(void)copyin((char *)PS_STRINGS, &arginfo, sizeof(arginfo));

	/*
	 * Set up arguments for _start():
	 *	_start(argc, argv, envp, obj, cleanup, ps_strings);
	 *
	 * Notes:
	 *	- obj and cleanup are the auxilliary and termination
	 *	  vectors.  They are fixed up by ld.elf_so.
	 *	- ps_strings is a NetBSD extention, and will be
	 * 	  ignored by executables which are strictly
	 *	  compliant with the SVR4 ABI.
	 *
	 * XXX We have to set both regs and retval here due to different
	 * XXX calling convention in trap.c and init_main.c.
	 */
	tf->fixreg[3] = arginfo.ps_nargvstr;
	tf->fixreg[4] = (register_t)arginfo.ps_argvstr;
	tf->fixreg[5] = (register_t)arginfo.ps_envstr;
	tf->fixreg[6] = 0;			/* auxillary vector */
	tf->fixreg[7] = 0;			/* termination vector */
	tf->fixreg[8] = (register_t)PS_STRINGS;	/* NetBSD extension */

	tf->srr0 = pack->ep_entry;
	tf->srr1 = PSL_MBO | PSL_USERSET | PSL_FE_DFLT;
	p->p_addr->u_pcb.pcb_flags = 0;
}

/*
 * Send a signal to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int onstack;

	tf = trapframe(p);

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (psp->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (psp->ps_sigact[sig].sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)psp->ps_sigstk.ss_sp +
						  psp->ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->fixreg[1];
	fp = (struct sigframe *)((int)(fp - 1) & ~0xf);

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;

	/* Save register context. */
	bcopy(tf, &frame.sf_sc.sc_frame, sizeof *tf);

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.sf_sc.__sc_mask13);
#endif

	if (copyout(&frame, fp, sizeof frame) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instructoin to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->fixreg[1] = (int)fp;
	tf->lr = (int)catcher;
	tf->fixreg[3] = (int)sig;
	tf->fixreg[4] = (int)code;
	tf->fixreg[5] = (int)&frame.sf_sc;
	tf->srr0 = (int)psp->ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal handler returns.
 */
int
sys___sigreturn14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext sc;
	struct trapframe *tf;
	int error;

	/*
	 * The trampoline hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal hander.
	 */
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);

	/* Restore the register context. */
	tf = trapframe(p);
	if ((sc.sc_frame.srr1 & PSL_USERSTATIC) != (tf->srr1 & PSL_USERSTATIC))
		return (EINVAL);
	bcopy(&sc.sc_frame, tf, sizeof *tf);

	/* Restore signal stack. */
	if (sc.sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &sc.sc_mask, 0);

	return (EJUSTRETURN);
}

/*
 * Machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case CPU_CACHELINE:
		return sysctl_rdint(oldp, oldlenp, newp, CACHELINESIZE);
	default:
		return (EOPNOTSUPP);
	}
}

/*
 * Crash dump handling.
 */
u_long dumpmag = 0x8fca0101;		/* magic number */
int dumpsize = 0;			/* size of dump in pages */
long dumplo = -1;			/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	int nblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpsize = physmem;

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
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
softnet(isr)
	int isr;
{
#ifdef	INET
#include "arp.h"
#if NARP > 0
	if (isr & (1 << NETISR_ARP))
		arpintr();
#endif
	if (isr & (1 << NETISR_IP))
		ipintr();
#endif
#ifdef	IMP
	if (isr & (1 << NETISR_IMP))
		impintr();
#endif
#ifdef	NS
	if (isr & (1 << NETISR_NS))
		nsintr();
#endif
#ifdef	ISO
	if (isr & (1 << NETISR_ISO))
		clnlintr();
#endif
#ifdef	CCITT
	if (isr & (1 << NETISR_CCITT))
		ccittintr();
#endif
#include "ppp.h"
#if NPPP > 0
	if (isr & (1 << NETISR_PPP))
		pppintr();
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
	static char str[256];
	char *ap = str, *ap1 = ap;

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
#if 0
		ppc_exit();
#endif 0
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
#if 0
	ppc_boot(str);
#endif 0
	while (1);
}

void
lcsplx(ipl)
	int ipl;
{
	splx(ipl);
}

/* not impliment */

void
dk_establish() {}

/*
 * Allocate vm space and mapin the I/O address
 */
void *
mapiodev(pa, len)
	paddr_t pa;
	psize_t len;
{
	paddr_t faddr;
	vaddr_t taddr, va;
	int off;

	faddr = trunc_page(pa);
	off = pa - faddr;
	len = round_page(off + len);
	va = taddr = uvm_km_valloc(kernel_map, len);

	if (va == 0)
		return NULL;

	for (; len > 0; len -= NBPG) {
		pmap_enter(pmap_kernel(), taddr, faddr,
			   VM_PROT_READ | VM_PROT_WRITE, 1, 0);
		faddr += NBPG;
		taddr += NBPG;
	}
	return (void *)(va + off);
}
