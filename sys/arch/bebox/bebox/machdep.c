/*	$NetBSD: machdep.c,v 1.1.2.1 1997/11/15 00:48:15 mellon Exp $	*/

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
#include "ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/callout.h>
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

#include <net/netisr.h>

#include <machine/bat.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

/*
 * Global variables used here and there
 */
char machine[] = MACHINE;		/* machine */
char machine_arch[] = MACHINE_ARCH;	/* machine architecture */

struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;

extern struct user *proc0paddr;

struct bat battable[16];

vm_offset_t bebox_mb_reg;		/* BeBox MotherBoard register */

int astpending;

char *bootpath;

extern vm_offset_t avail_end, virtual_avail;
caddr_t	msgbufaddr;

caddr_t allocsys __P((caddr_t));

static int fake_spl __P((void));
static int fake_splx __P((int));
static void fake_setsoft __P((void));
static void fake_clock_return __P((struct clockframe *, int));
static void fake_irq_establish __P((int, int, void (*)(void *), void *));
void install_extint __P((void (*)(void)));

struct machvec machine_interface = {
	fake_spl,
	fake_spl,
	fake_spl,
	fake_spl,
	fake_spl,
	fake_spl,
	fake_spl,
	fake_spl,
	fake_spl,
	fake_splx,
	fake_setsoft,
	fake_setsoft,
	fake_clock_return,
	fake_irq_establish,
};

int cold = 1;

void
initppc(startkernel, endkernel, size, args)
	u_int startkernel, endkernel, size, args;
{
	struct machvec *mp;
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
	int exc, scratch;

	/*
	 * BeBox MotherBoard's Register
	 *  Interrupt Mask Reset
	 */
#define CPU0_INT_MASK	0x7ffff0f0
#define CPU1_INT_MASK	0x7ffff1f0
	*(volatile unsigned int *)CPU0_INT_MASK = 0x0ffffffc;
	*(volatile unsigned int *)CPU0_INT_MASK = 0x80000023;
	*(volatile unsigned int *)CPU1_INT_MASK = 0x0ffffffc;  


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
#ifdef DDB
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
			bcopy(&ddblow, (void *)exc, (size_t)&ddbsize);
			break;
#endif
#if NIPKDB > 0
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
			bcopy(&ipkdblow, (void *)exc, (size_t)&ipkdbsize);
			break;
#endif
		}

	syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

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
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel, size);

#ifdef DDB
	ddb_init(startsym, endsym);
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
	syncicache((void *)&extint_call, sizeof extint_call);
	syncicache((void *)EXC_EXI, (int)&extsize);
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
	vm_offset_t minaddr, maxaddr;
	int base, residual;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(),
		    (vm_offset_t)((caddr_t)msgbufaddr + i * NBPG),
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));
	msgbufmapped = 1;
	
	proc0.p_addr = proc0paddr;
	v = (caddr_t)proc0paddr + USPACE;

	printf("%s", version);
	identifycpu();
	
	printf("real memory  = %d (%dK bytes)\n",
		ctob(physmem), ctob(physmem) / 1024);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys((caddr_t)0);
	if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v) - v != sz)
		panic("startup: table size inconsistency");
	
	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	sz = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr, sz, TRUE);
	buffers = (char *)minaddr;
	if (vm_map_find(buffer_map, vm_object_allocate(sz), (vm_offset_t)0,
			&minaddr, sz, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE) {
		/* Don't want to alloc more physical mem than ever needed */
		base = MAXBSIZE;
		residual = 0;
	}
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;
		
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base + 1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf + curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);
	
	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);
	
	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i - 1].c_next = &callout[i];
	
	printf("avail memory = %d (%dK bytes)\n",
		ptoa(cnt.v_free_count), ptoa(cnt.v_free_count) / 1024);
	printf("using %d buffers containing %d bytes of memory\n",
	       nbuf, bufpages * CLBYTES);
	
	/*
	 * Set up the buffers.
	 */
	bufinit();

	/*
	 * BeBox Mother Board's Register Mapping
	 */
	bebox_mb_reg  = physmemmap(0x7ffff000, NBPG);

	/*
	 * For now, use soft spl handling.
	 */
	{
		extern struct machvec soft_machvec;
		extern int cpl;

		cpl = IPL_NONE;
		machine_interface = soft_machvec;
	}

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;
		
		splhigh();
		asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"((u_short)(PSL_EE|PSL_RI)));
	}
	
	/*
	 * Configure devices.
	 */
	configure();
}

/*
 * Allocate space for system data structures.
 */
caddr_t
allocsys(v)
	caddr_t v;
{
#define	valloc(name, type, num) \
	v = (caddr_t)(((name) = (type *)v) + (num))

	valloc(callout, struct callout, ncallout);
#ifdef	SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef	SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef	SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

	/*
	 * Decide on buffer space to use.
	 */
	if (bufpages == 0)
		bufpages = (physmem / 20) / CLSIZE;
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) & ~1;
		if (nswbuf > 256)
			nswbuf = 256;
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	
	return v;
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	static int initted;
	
	if (initted)
		return;
	initted = 1;
	cninit();		/* XXX temporary */
}

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
	int sig, mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oldonstack;
	
	frame.sf_signum = sig;
	
	tf = trapframe(p);
	oldonstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;
	
	/*
	 * Allocate stack space for signal handler.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK)
	    && !oldonstack
	    && (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp
					 + psp->ps_sigstk.ss_size);
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else
		fp = (struct sigframe *)tf->fixreg[1];
	fp = (struct sigframe *)((int)(fp - 1) & ~0xf);
	
	frame.sf_code = code;
	
	/*
	 * Generate signal context for SYS_sigreturn.
	 */
	frame.sf_sc.sc_onstack = oldonstack;
	frame.sf_sc.sc_mask = mask;
	bcopy(tf, &frame.sf_sc.sc_frame, sizeof *tf);
	if (copyout(&frame, fp, sizeof frame) != 0)
		sigexit(p, SIGILL);
	
	tf->fixreg[1] = (int)fp;
	tf->lr = (int)catcher;
	tf->fixreg[3] = (int)sig;
	tf->fixreg[4] = (int)code;
	tf->fixreg[5] = (int)&frame.sf_sc;
	tf->srr0 = (int)(((char *)PS_STRINGS)
			 - (p->p_emul->e_esigcode - p->p_emul->e_sigcode));
}

/*
 * System call to cleanup state after a signal handler returns.
 */
int
sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sigreturn_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext sc;
	struct trapframe *tf;
	int error;
	
	if (error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc))
		return error;
	tf = trapframe(p);
	if ((sc.sc_frame.srr1 & PSL_USERSTATIC) != (tf->srr1 & PSL_USERSTATIC))
		return EINVAL;
	bcopy(&sc.sc_frame, tf, sizeof *tf);
	if (sc.sc_onstack & 1)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = sc.sc_mask & ~sigcantmask;
	return EJUSTRETURN;
}

/*
 * Machine dependent system variables.
 * None for now.
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
		return ENOTDIR;
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
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
	printf("cpu_dumpconf: TBD\n");
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
	int isr = netisr;

	netisr = 0;
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
	while(1);
}

/*
 * Initial Machine Interface.
 */
static int
fake_spl()
{
	int scratch;

	asm volatile ("mfmsr %0; andi. %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch) : "K"((u_short)~(PSL_EE|PSL_ME)));
	return -1;
}

static void
fake_setsoft()
{
	/* Do nothing */
}

static int
fake_splx(new)
	int new;
{
	return fake_spl();
}

static void
fake_clock_return(frame, nticks)
	struct clockframe *frame;
	int nticks;
{
	/* Do nothing */
}

static void
fake_irq_establish(irq, level, handler, arg)
	int irq, level;
	void (*handler) __P((void *));
	void *arg;
{
	panic("fake_irq_establish");
}

/* not impliment */

void
dk_establish() {}
