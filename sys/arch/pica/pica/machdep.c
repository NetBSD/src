/*	$NetBSD: machdep.c,v 1.14 1997/07/01 09:32:28 jonathan Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif

#include <vm/vm_kern.h>
#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pio.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <mips/locore.h>		/* wbflush() */
#include <mips/cpuregs.h>
#include <mips/psl.h>
#ifdef DDB
#include <mips/db_machdep.h>
#endif

#include <sys/exec_ecoff.h>

#include <dev/cons.h>

#include <pica/pica/pica.h>
#include <pica/pica/picatype.h>

#include <asc.h>

#if NASC > 0
#include <pica/dev/ascreg.h>
#endif

extern struct consdev *cn_tab;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */
char	cpu_model[30];

vm_map_t buffer_map;

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	msgbufmapped = 0;	/* set when safe to use msgbuf */
int	maxmem;			/* max memory per process */
int	physmem;		/* max supported memory, changes to actual */
int	memcfg;			/* memory config register */
int	brdcfg;			/* motherboard config register */
int	cpucfg;			/* Value of processor config register */
int	cputype;		/* Mother board type */
int	ncpu = 1;		/* At least one cpu in the system */
int	isa_io_base;		/* Base address of ISA io port space */
int	isa_mem_base;		/* Base address of ISA memory space */

/*
 * Interrupt-blocking functions defined in locore. These names aren't used
 * directly except here and in interrupt handlers.
 */

/* Block out one hardware interrupt-enable bit. */
extern int	Mach_spl0 __P((void)), Mach_spl1 __P((void));
extern int	Mach_spl2 __P((void)), Mach_spl3 __P((void));

/* Block out nested interrupt-enable bits. */
extern int	cpu_spl0 __P((void)), cpu_spl1 __P((void));
extern int	cpu_spl2 __P((void)), cpu_spl3 __P((void));
extern int	splhigh __P((void));

/*
 * Instead, we declare the standard splXXX names as function pointers,
 * and initialie them to point to the above functions to match
 * the way a specific motherboard is  wired up.
 */
int	(*Mach_splbio) __P((void)) = splhigh;
int	(*Mach_splnet)__P((void)) = splhigh;
int	(*Mach_spltty)__P((void)) = splhigh;
int	(*Mach_splimp)__P((void)) = splhigh;
int	(*Mach_splclock)__P((void)) = splhigh;
int	(*Mach_splstatclock)__P((void)) = splhigh;

void	dumpsys __P((void));			/* do a dump */
int	savectx __P((struct user *up));		/* XXX save state b4 crash*/

/* initialize bss, etc. from kernel start, before main() is called. */
extern	void
mach_init __P((int argc, char *argv[], u_int code));


/*
 * Pica video-console output (for output before console is autoconfigured)
 */
static void  vid_scroll __P((void));
void vid_print_string __P((const char *str));
void vid_putchar __P((dev_t dev, char c));
extern	int atoi __P((const char *cp));

#ifdef DEBUG
/* stacktrace code violates prototypes to get callee's registers */
extern void stacktrace __P((void)); /*XXX*/
#endif



/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

struct	user *proc0paddr;
struct	proc nullproc;		/* for use by swtch_exit() */

extern void mips_vector_init  __P((void));


/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the BIOS.
 * Reset mapping and set up mapping to hardware and init "wired" reg.
 * Return the first page address following the system.
 */
void
mach_init(argc, argv, code)
	int argc;
	char *argv[];
	u_int code;
{
	register char *cp;
	register int i;
	register unsigned firstaddr;
	register caddr_t v;
	caddr_t start;
	struct tlb tlb;
	extern char edata[], end[];

	/* clear the BSS segment in NetBSD code */
	v = (caddr_t)pica_round_page(end);
	bzero(edata, v - edata);


	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 *
	 * XXX this may clobber PTEs needed by the BIOS.
	 */
	mips_vector_init();

#ifdef DDB
	/*
	 * Initialize machine-dependent DDB commands, in case of early panic.
	 */
	db_machine_init();
#endif

	/* check what model platform we are running on */
	cputype = ACER_PICA_61; /* FIXME find systemtype */

	/*
	 * Get config register now as mapped from BIOS since we are
	 * going to demap these addresses later. We want as may TLB
	 * entries as possible to do something useful :-).
	 */

	switch (cputype) {
	case ACER_PICA_61:	/* ALI PICA 61 */
		memcfg = in32(PICA_MEMORY_SIZE_REG);
		brdcfg = in32(PICA_CONFIG_REG);
		isa_io_base = PICA_V_ISA_IO;
		isa_mem_base = PICA_V_ISA_MEM;
		break;
	default:
		memcfg = -1;
		break;
	}

	/* look at argv[0] and compute bootdev */
	makebootdev(argv[0]);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_SINGLE;
#ifdef KADB
	boothowto |= RB_KDB;
#endif
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if(strncmp("OSLOADOPTIONS=",argv[i],14) == 0) {
				for (cp = argv[i]+14; *cp; cp++) {
					switch (*cp) {
					case 'a': /* autoboot */
						boothowto &= ~RB_SINGLE;
						break;

					case 'd': /* use compiled in default root */
						boothowto |= RB_DFLTROOT;
						break;

					case 'm': /* mini root present in memory */
						boothowto |= RB_MINIROOT;
						break;

					case 'n': /* ask for names */
						boothowto |= RB_ASKNAME;
						break;

					case 'N': /* don't ask for names */
						boothowto &= ~RB_ASKNAME;
						break;
					}

				}
			}
		}
	}

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT) {
		boothowto |= RB_DFLTROOT;
		v += mfs_initminiroot(v);
	}
#endif

	/*
	 * Now its time to abandon the BIOS and be self supplying.
	 * Start with cleaning out the TLB. Bye bye Microsoft....
	 */
#ifdef	PREDATES_LOCORE_VECTOR_INIT
	cpu_arch = 3;
	mips3_SetWIRED(0);
	mips3_TLBFlush();
	mips3_SetWIRED(MIPS3_TLB_WIRED_ENTRIES);
	mips3_vector_init();
#endif


	/*
	 * Set up mapping for hardware the way we want it!
	 */

	tlb.tlb_mask = MIPS3_PG_SIZE_256K;
	tlb.tlb_hi = mips3_vad_to_vpn(PICA_V_LOCAL_IO_BASE);
	tlb.tlb_lo0 = vad_to_pfn(PICA_P_LOCAL_IO_BASE) | MIPS3_PG_IOPAGE;
	tlb.tlb_lo1 = vad_to_pfn(PICA_P_INT_SOURCE) | MIPS3_PG_IOPAGE;
	mips3_TLBWriteIndexedVPS(1, &tlb);

	tlb.tlb_mask = MIPS3_PG_SIZE_1M;
	tlb.tlb_hi = mips3_vad_to_vpn(PICA_V_LOCAL_VIDEO_CTRL);
	tlb.tlb_lo0 = vad_to_pfn(PICA_P_LOCAL_VIDEO_CTRL) | MIPS3_PG_IOPAGE;
	tlb.tlb_lo1 = vad_to_pfn(PICA_P_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2) | MIPS3_PG_IOPAGE;
	mips3_TLBWriteIndexedVPS(2, &tlb);
	
	tlb.tlb_mask = MIPS3_PG_SIZE_1M;
	tlb.tlb_hi = mips3_vad_to_vpn(PICA_V_EXTND_VIDEO_CTRL);
	tlb.tlb_lo0 = vad_to_pfn(PICA_P_EXTND_VIDEO_CTRL) | MIPS3_PG_IOPAGE;
	tlb.tlb_lo1 = vad_to_pfn(PICA_P_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL/2) | MIPS3_PG_IOPAGE;
	mips3_TLBWriteIndexedVPS(3, &tlb);
	
	tlb.tlb_mask = MIPS3_PG_SIZE_4M;
	tlb.tlb_hi = mips3_vad_to_vpn(PICA_V_LOCAL_VIDEO);
	tlb.tlb_lo0 = vad_to_pfn(PICA_P_LOCAL_VIDEO) | MIPS3_PG_IOPAGE;
	tlb.tlb_lo1 = vad_to_pfn(PICA_P_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO/2) | MIPS3_PG_IOPAGE;
	mips3_TLBWriteIndexedVPS(4, &tlb);
	
	tlb.tlb_mask = MIPS3_PG_SIZE_16M;
	tlb.tlb_hi = mips3_vad_to_vpn(PICA_V_ISA_IO);
	tlb.tlb_lo0 = vad_to_pfn(PICA_P_ISA_IO) | MIPS3_PG_IOPAGE;
	tlb.tlb_lo1 = vad_to_pfn(PICA_P_ISA_MEM) | MIPS3_PG_IOPAGE;
	mips3_TLBWriteIndexedVPS(5, &tlb);
	
	/*
	 * Init mapping for u page(s) for proc[0], pm_tlbpid 1.
	 */
	v = (caddr_t) (((int)v+3) & -4);
	start = v;
	curproc->p_addr = proc0paddr = (struct user *)v;
	curproc->p_md.md_regs = proc0paddr->u_pcb.pcb_regs;
	firstaddr = MIPS_KSEG0_TO_PHYS(v);
	for (i = 0; i < UPAGES; i+=2) {
		tlb.tlb_mask = MIPS3_PG_SIZE_4K;
		tlb.tlb_hi = mips3_vad_to_vpn((UADDR + (i << PGSHIFT))) | 1;
		tlb.tlb_lo0 = vad_to_pfn(firstaddr) |
			(MIPS3_PG_V | MIPS3_PG_M | MIPS3_PG_CACHED);
		tlb.tlb_lo1 = vad_to_pfn(firstaddr + NBPG) |
			(MIPS3_PG_V | MIPS3_PG_M | MIPS3_PG_CACHED);
		curproc->p_md.md_upte[i] = tlb.tlb_lo0;
		curproc->p_md.md_upte[i+1] = tlb.tlb_lo1;
		mips3_TLBWriteIndexedVPS(i,&tlb);
		firstaddr += NBPG * 2;
	}
	v += UPAGES * NBPG;
	v = (caddr_t) (((int)v+3) & -4);
	MachSetPID(1);

	/*
	 * init nullproc for swtch_exit().
	 * init mapping for u page(s), pm_tlbpid 0
	 * This could be used for an idle process.
	 */
	nullproc.p_addr = (struct user *)v;
	nullproc.p_md.md_regs = nullproc.p_addr->u_pcb.pcb_regs;
	bcopy("nullproc", nullproc.p_comm, sizeof("nullproc"));
	firstaddr = MIPS_KSEG0_TO_PHYS(v);
	for (i = 0; i < UPAGES; i+=2) {
		nullproc.p_md.md_upte[i] = vad_to_pfn(firstaddr) |
			(MIPS3_PG_V | MIPS3_PG_M | MIPS3_PG_CACHED);
		nullproc.p_md.md_upte[i+1] = vad_to_pfn(firstaddr + NBPG) |
			(MIPS3_PG_V | MIPS3_PG_M | MIPS3_PG_CACHED);
		firstaddr += NBPG * 2;
	}
	v += UPAGES * NBPG;

	/* clear pages for u areas */
	bzero(start, v - start);

	/* check what model platform we are running on */
	switch (cputype) {
	case ACER_PICA_61:	/* ALI PICA 61 */
		/*
		 * Set up interrupt handling and I/O addresses.
		 */
#if 0 /* XXX FIXME */
		Mach_splnet = Mach_spl1;
		Mach_splbio = Mach_spl0;
		Mach_splimp = Mach_spl1;
		Mach_spltty = Mach_spl2;
		Mach_splstatclock = Mach_spl3;
#endif
		strcpy(cpu_model, "PICA_61");
		break;

	default:
		printf("kernel not configured for systype 0x%x\n", i);
		cpu_reboot(RB_HALT | RB_NOSYNC, NULL);
	}

	/*
	 * Find out how much memory is available.
	 */

	switch (cputype) {
	case ACER_PICA_61:	/* ALI PICA 61 */
		/*
		 * Size is determined from the memory config register.
		 *  d0-d2 = bank 0 size (sim id)
		 *  d3-d5 = bank 1 size
		 *  d6 = bus width. (doubels memory size)
		 */
		if((memcfg & 7) <= 5)
			physmem = 2097152 << (memcfg & 7);
		if(((memcfg >> 3) & 7) <= 5)
			physmem += 2097152 << ((memcfg >> 3) & 7);

		if((memcfg & 0x40) == 0)
			physmem += physmem;	/* 128 bit config */

		physmem = btoc(physmem);
		break;

	default:
		physmem = btoc((u_int)v - KERNBASE);
		cp = (char *)MIPS_PHYS_TO_KSEG0(physmem << PGSHIFT);
		while (cp < (char *)MIPS_MAX_MEM_ADDR) {
			if (badaddr(cp, 4))
				break;
			i = *(int *)cp;
			*(int *)cp = 0xa5a5a5a5;
			/*
			 * Data will persist on the bus if we read it right away
			 * Have to be tricky here.
			 */
			((int *)cp)[4] = 0x5a5a5a5a;
			wbflush();
			if (*(int *)cp != 0xa5a5a5a5)
				break;
			*(int *)cp = i;
			cp += NBPG;
			physmem++;
		}
		break;
	}

	maxmem = physmem;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	maxmem -= btoc(sizeof (struct msgbuf));
	msgbufp = (struct msgbuf *)(MIPS_PHYS_TO_KSEG0(maxmem << PGSHIFT));
	msgbufmapped = 1;

	/*
	 * Allocate space for system data structures.
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 *
	 * These data structures are allocated here instead of cpu_startup()
	 * because physical memory is directly addressable. We don't have
	 * to map these into virtual address space.
	 */
	start = v;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
#ifdef REAL_CLISTS
	valloc(cfree, struct cblock, nclist);
#endif
	valloc(callout, struct callout, ncallout);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

	/*
	 * Determine how many buffers to allocate.
	 * We allocate more buffer space than the BSD standard of
	 * using 10% of memory for the first 2 Meg, 5% of remaining.
	 * We just allocate a flat 10%. Ensure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		bufpages = physmem / 10 / CLSIZE;
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);

	/*
	 * Clear allocated memory.
	 */
	bzero(start, v - start);

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap((vm_offset_t)v);
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	static int initted;

	if (initted)
		return;
	initted = 1;
	cninit();
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	register unsigned i;
	int base, residual;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("real mem = %d\n", ctob(physmem));

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16 * NCARGS, TRUE);
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
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
	callout[i-1].c_next = NULL;

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %ld\n", ptoa(cnt.v_free_count));
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);
	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();
}


/*
 * machine dependent system variables.
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
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

int	waittime = -1;
struct user dumppcb;	/* Actually, struct pcb would do. */

void
cpu_reboot(howto, bootstr)
	register int howto;
	char *bootstr;
{

	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx((struct user *)curpcb);

#ifdef DEBUG
	if (panicstr)
		stacktrace();
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		/*
		 * Synchronize the disks....
		 */
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
	(void) splhigh();		/* extreme priority */
	if (howto & RB_HALT)
		printf("System halted.\n");
	else {
		if (howto & RB_DUMP)
			dumpsys();
		printf("System restart.\n");
	}
	while(1); /* Forever */
	/*NOTREACHED*/
}


/*
 * These variables are needed by /sbin/savecore
 */
int	dumpmag = (int)0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

void
cpu_dumpconf()
{
	int nblks;

	dumpsize = physmem;
	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(physmem));
	}
	/*
	 * Don't dump on the first CLBYTES (why CLBYTES?)
	 * in case the dump device includes a disk label.
	 */
	if (dumplo < btodb(CLBYTES))
		dumplo = btodb(CLBYTES);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
void
dumpsys()
{
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	msgbufmapped = 0;
	if (dumpdev == NODEV)
		return;
	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %ld\n", dumpdev, dumplo);
	printf("dump ");

	/*
	 * XXX
	 * All but first arguments to  dump() bogus.
	 * What should blkno, va, size be?
	 */
	error = (*bdevsw[major(dumpdev)].d_dump)(dumpdev, 0, 0, 0);
	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("error %d\n", error);
		break;

	case 0:
		printf("succeeded\n");
	}
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  Unfortunately, we can't read the hardware registers.
 * We guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
#ifdef notdef
	tvp->tv_usec += clkread();
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
#endif
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

int
initcpu()
{

	/*
	 * Disable all interrupts. New masks will be set up
	 * during system configuration
	 */
	out16(PICA_SYS_LB_IE,0x000);
	out32(PICA_SYS_EXT_IMASK, 0x00);

	spl0();		/* safe to turn interrupts on now */
	return 0;
}

/*
 * Convert an ASCII string into an integer.
 */
int
atoi(s)
	const char *s;
{
	int c;
	unsigned base = 10, d;
	int neg = 0, val = 0;

	if (s == 0 || (c = *s++) == 0)
		goto out;

	/* skip spaces if any */
	while (c == ' ' || c == '\t')
		c = *s++;

	/* parse sign, allow more than one (compat) */
	while (c == '-') {
		neg = !neg;
		c = *s++;
	}

	/* parse base specification, if any */
	if (c == '0') {
		c = *s++;
		switch (c) {
		case 'X':
		case 'x':
			base = 16;
			c = *s++;
			break;
		case 'B':
		case 'b':
			base = 2;
			c = *s++;
			break;
		default:
			base = 8;
		}
	}

	/* parse number proper */
	for (;;) {
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'a' && c <= 'z')
			d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			d = c - 'A' + 10;
		else
			break;
		val *= base;
		val += d;
		c = *s++;
	}
	if (neg)
		val = -val;
out:
	return val;	
}

/*
 * This code is temporary for debugging at startup
 */

static int vid_xpos=0, vid_ypos=0;

static void 
vid_wrchar(char c)
{
	volatile unsigned short *video;

	video = (unsigned short *)(0xe08b8000) + vid_ypos * 80 + vid_xpos;
	*video = (*video & 0xff00) | 0x0f00 | (unsigned short)c;
}

static void
vid_scroll()
{
	volatile unsigned short *video;
	int i;

	video = (unsigned short *)(0xe08b8000);
	for (i = 0; i < 80 * 24; i++) {
		*video = *(video + 80);
		video++;
	}
	for (i = 0; i < 80; i++) {
		*video = (*video & 0xff00) | ' ';
		video++;
	}
}

void
vid_print_string(const char *str)
{
	unsigned char c;

	while ((c = *str++) != 0) {
		vid_putchar((dev_t)0, c);
	}
}

void
vid_putchar(dev_t dev, char c)
{
	switch(c) {
	case '\n':
		vid_xpos = 0;
		if(vid_ypos == 24)
			vid_scroll();
		else
			vid_ypos++;
		DELAY(500000);
		break;

	case '\r':
		vid_xpos = 0;
		break;

	case '\t':
		do {
			vid_putchar(dev, ' ');
		} while(vid_xpos & 7);
		break;

	default:
		vid_wrchar(c);
		vid_xpos++;
		if(vid_xpos == 80) {
			vid_xpos = 0;
			vid_putchar(dev, '\n');
		}
	}
}
