/*	$NetBSD: machdep.c,v 1.23.8.1 1999/12/27 18:33:20 wrstuden Exp $	*/

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

#include "fs_mfs.h"

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
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

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

int	maxmem;			/* max memory per process */
int	physmem;		/* max supported memory, changes to actual */
int	memcfg;			/* memory config register */
int	brdcfg;			/* motherboard config register */
int	cpucfg;			/* Value of processor config register */
int	cputype;		/* Mother board type */
int	ncpu = 1;		/* At least one cpu in the system */
int	isa_io_base;		/* Base address of ISA io port space */
int	isa_mem_base;		/* Base address of ISA memory space */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

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
	struct tlb tlb;
	u_long first, last;
	caddr_t kernend, v;
	vm_size_t size;
	extern char edata[], end[];

	/* clear the BSS segment in NetBSD code */
	kernend = (caddr_t)pica_round_page(end);
	bzero(edata, kernend - edata);

	/*
	 * Set the VM page size.
	 */
	vm_set_page_size();

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
		kernend += round_page(mfs_initminiroot(kernend));
	}
#endif

	/*
	 * Init the mapping for u page(s) for proc0, pm_tlbpid 1.
	 * This also initializes nullproc for switch_exit().
	 */
	mips_init_proc0(kernend);

	kernend += 2 * UPAGES * PAGE_SIZE;

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
		physmem = btoc((vm_offset_t)kernend - MIPS_KSEG0_START);
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
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = 0;		/* XXX is this correct? */
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;

	/*
	 * Load the rest of the pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	vm_page_physload(atop(first), atop(last), atop(first), atop(last));

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	size = (vm_size_t)allocsys(NULL, NULL);
	v = (caddr_t)pmap_steal_memory(size, NULL, NULL); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();
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
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

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
		curbufsize = NBPG * (i < residual ? base+1 : base);
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
	mb_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
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
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
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
