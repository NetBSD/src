/*	$NetBSD: machdep.c,v 1.1.1.1 1999/09/16 12:23:20 takemura Exp $	*/

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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1.1.1 1999/09/16 12:23:20 takemura Exp $");

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include "fs_mfs.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
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
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>
#include <dev/cons.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/locore.h>

#ifdef DDB
#include <sys/exec_aout.h>		/* XXX backwards compatilbity for DDB */
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE         DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <hpcmips/dev/biconsvar.h>
#include <hpcmips/dev/bicons.h>

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[40];

/* maps for VM objects */
vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

int	systype;		/* mother board type */
int	maxmem;			/* max memory per process */
int	physmem;		/* max supported memory, changes to actual */
int	physmem_boardmax;	/* {model,SIMM}-specific bound on physmem */
int	mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 * Used as an argument to splx().
 * XXX disables interrupt 5 to disable mips3 on-chip clock, which also
 * disables mips1 FPU interrupts.
 */
int	safepri = MIPS3_PSL_LOWIPL;	/* XXX */

struct splvec	splvec;			/* XXX will go XXX */

void mach_init __P((int, char *[], struct bootinfo*));
static struct bootinfo bi_copy;
struct bootinfo *bootinfo = NULL;

unsigned (*clkread) __P((void)); /* high resolution timer if available */
unsigned nullclkread __P((void));

int	initcpu __P((void));
#ifdef notyet /*__hpcmips__*/
volatile struct chiptime *mcclock_addr;
#endif

#ifdef DEBUG
/* stacktrace code violates prototypes to get callee's registers */
extern void stacktrace __P((void)); /*XXX*/
#endif

/* Motherboard or system-specific initialization vector */
void	unimpl_os_init __P((void));
void	unimpl_bus_reset __P((void));
int	unimpl_intr __P((unsigned, unsigned, unsigned, unsigned));
void	unimpl_cons_init __P((void));
void	unimpl_device_register __P((struct device *, void *));
void 	unimpl_iointr __P ((void *, u_long));
void	unimpl_clockintr __P ((void *));

struct platform platform = {
	"iobus not set",
	unimpl_os_init,
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_device_register,
	unimpl_iointr,
	unimpl_clockintr
};
extern void vr_init __P((void));	/* XXX, We have no sysconf.c,
					   so call this directly. */

extern caddr_t esym;
extern struct user *proc0paddr;

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the boot loader. 
 * Return the first page address following the system.
 */
void
mach_init(argc, argv, bi)
	int argc;
	char *argv[];
	struct bootinfo *bi;
{
	u_long first, last;
	int i;
	caddr_t kernend, v;
	unsigned size;
	char *cp;
	extern char edata[], end[];

	/* clear the BSS segment */
#ifdef DDB
	if (!strncmp(end,Elf_e_ident,Elf_e_siz)) {
		esym = end;
		esym += ((Elf_Ehdr *)end)->e_entry;
		kernend = (caddr_t)mips_round_page(esym);
		bzero(edata, end - edata);
	} else
#endif
	{
		kernend = (caddr_t)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

	/*
	 *  Arguments are set up by boot lader.
	 */
	if (bi && bi->magic == BOOTINFO_MAGIC) {
		memset(&bi_copy, 0, sizeof(struct bootinfo));
		memcpy(&bi_copy, bi, min(bi->length, sizeof(struct bootinfo)));
		bootinfo = &bi_copy;
		if (bootinfo->platid_cpu != 0) {
			platid.dw.dw0 = bootinfo->platid_cpu;
		}
		if (bootinfo->platid_machine != 0) {
			platid.dw.dw1 = bootinfo->platid_machine;
		}
	}

	/* Use builtin console output until we initialize a console driver. */
	cn_tab = &builtincd;

	/* Initialize builtin console. */
	bicons_init();

	/*
	 * Set the VM page size.
	 */
	uvmexp.pagesize = NBPG; /* Notify the VM system of our page size. */
	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	/* XXX, for debugging. */
	if (bootinfo) {
		printf("Bootinfo. available, ");
	}
	printf("args: ");
	for (i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	printf("platform ID: %08lx %08lx\n", platid.dw.dw0, platid.dw.dw1);

	/* Compute bootdev */
	makebootdev("wd0"); /* XXX Should be passed up from boot lorder */

	boothowto = RB_SINGLE;
#ifdef KADB
	boothowto |= RB_KDB;
#endif
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
			switch (*cp) {
			case 'a': /* autoboot */
				boothowto &= ~RB_SINGLE;
				break;

			case 'd': /* break into the kernel debugger ASAP */
				boothowto |= RB_KDB;
				break;

			case 'm': /* mini root present in memory */
				boothowto |= RB_MINIROOT;
				break;

			case 'n': /* ask for names */
				boothowto |= RB_ASKNAME;
				break;

			case 'h': /* XXX, serial console */
				bootinfo->bi_cnuse |= BI_CNUSE_SERIAL;
				break;

			case 'N': /* don't ask for names */
				boothowto &= ~RB_ASKNAME;
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

#ifdef DDB
	/*
	 * Initialize machine-dependent DDB commands, in case of early panic.
	 */
	db_machine_init();
	/* init symbols if present */
	if (esym)
		ddb_init(1000, &end, (int*)esym);
#if 0
	/*
	 * XXX, We want to use console !
	 */
	if (boothowto & RB_KDB)
		Debugger();
#endif
#endif
	/*
	 * Alloc u pages for proc0 stealing KSEG0 memory.
	 */
	proc0.p_addr = proc0paddr = (struct user *)kernend;
	proc0.p_md.md_regs =
	    (struct frame *)((caddr_t)kernend + UPAGES * PAGE_SIZE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	memset(kernend, 0, UPAGES * PAGE_SIZE);

	kernend += UPAGES * PAGE_SIZE;
	/*
	 * Initialize physmem_boardmax; assume no SIMM-bank limits.
	 * Adjust later in model-specific code if necessary.
	 */
	physmem_boardmax = MIPS_MAX_MEM_ADDR;

	/*
	 *  XXX, We have no sysconf.c, so call vr_init() directly.
	 */
	vr_init();
	(*platform.os_init)();
	/* Initialize console. */
	(*platform.cons_init)();

	/*
	 * XXX, We want to use console !
	 */
	if (boothowto & RB_KDB)
		Debugger();

	/*
	 * Find out how much memory is available.
	 * Be careful to save and restore the original contents for msgbuf.
	 */
	physmem = btoc((paddr_t)kernend - MIPS_KSEG0_START);
	cp = (char *)MIPS_PHYS_TO_KSEG1(physmem << PGSHIFT);
	while (cp < (char *)physmem_boardmax) {
	  	int j;
		if (badaddr(cp, 4))
			break;
		i = *(int *)cp;
		j = ((int *)cp)[4];
		*(int *)cp = 0xa5a5a5a5;
		/*
		 * Data will persist on the bus if we read it right away.
		 * Have to be tricky here.
		 */
		((int *)cp)[4] = 0x5a5a5a5a;
		wbflush();
		if (*(int *)cp != 0xa5a5a5a5)
			break;
		*(int *)cp = i;
		((int *)cp)[4] = j;
		bzero(cp, NBPG);/* For WindowsCE's sake */
		cp += NBPG;
		physmem++;
	}
	bzero((char *)KERNBASE + 0x400, KERNTEXTOFF - KERNBASE - 0x800); /* For WindowsCE's sake */

	maxmem = physmem;

	/*
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = 0;		/* XXX is this correct? */
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first),
			  atop(last), VM_FREELIST_DEFAULT);

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
	size = (unsigned)allocsys(NULL, NULL);
	v = (caddr_t)pmap_steal_memory(size, NULL, NULL);
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();
}


/*
 * Machine-dependent startup code.
 * allocate memory for variable-sized tables, initialize cpu.
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("cpu_startup: cannot allocate VM for buffers");

	minaddr = (vaddr_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		bufpages = btoc(MAXBSIZE) * nbuf; /* do not overallocate RAM */
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;

	/* now allocate RAM for buffers */
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
		curbuf = (vaddr_t)buffers + (i * MAXBSIZE);
		curbufsize = CLBYTES * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.
	 */

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
	format_bytes(pbuf, sizeof(pbuf), bufpages * CLBYTES);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();
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
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &cn_tab->cn_dev,
		    sizeof cn_tab->cn_dev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

void
cpu_reboot(howto, bootstr)
	volatile int howto;	/* XXX volatile to keep gcc happy */
	char *bootstr;
{
	extern int cold;

	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx((struct user *)curpcb);

#ifdef DEBUG
	if (panicstr)
		stacktrace();
#endif

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if ((boothowto & RB_HALT) != 0)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0) {
		/*
		 * Synchronize the disks....
		 */
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested do it. */
#if 0
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
#else
	if (howto & RB_DUMP)
#endif
		dumpsys();

haltsys:

	/* run any shutdown hooks */
	doshutdownhooks();

	/* Finally, halt/reboot the system. */
	printf("%s\n\n", howto & RB_HALT ? "halted." : "rebooting...");
	while (1)
	    ;
	/*NOTREACHED*/
}

/*
 * Return the best possible estimate of the time in the timeval to
 * which tvp points.  We guarantee that the time will be greater than
 * the value obtained by a previous call.  Some models of DECstations
 * provide a high resolution timer circuit.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;

	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

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
#ifdef notyet /*__hpcmips__*/
	volatile struct chiptime *c;
#endif
	int i = 0;

	/*
	 * reset after autoconfig probe:
	 * clear  any memory errors, reset any pending interrupts.
	 */

	(*platform.bus_reset)();	/* XXX_cf_alpha */

	/*
	 * With newconf, this should be  done elswhere, but without it
	 * we hang (?)
	 */
#ifdef notyet /*__hpcmips__*/
#if 1 /*XXX*/
	/* disable clock interrupts (until startrtclock()) */
	if (mcclock_addr) {
		c = mcclock_addr;
		c->regb = REGB_DATA_MODE | REGB_HOURS_FORMAT;
		i = c->regc;
	}
	return (i);
#endif
#else
	return i;
#endif
}


/*
 * Wait "n" microseconds. (scsi code needs this).
 */
void
delay(n)
        int n;
{
        DELAY(n);
}

/*
 *  Ensure all platform vectors are always initialized.
 */
void
unimpl_os_init()
{
	panic("sysconf.init didnt set os_init");
}

void
unimpl_bus_reset()
{
	panic("sysconf.init didnt set bus_reset");
}

void
unimpl_cons_init()
{
	panic("sysconf.init didnt set cons_init");
}

void
unimpl_device_register(sc, arg)
	struct device *sc;
	void *arg;
{
	panic("sysconf.init didnt set device_register");

}

void
unimpl_iointr(arg, arg2)
	void *arg;
	u_long arg2;
{
	panic("sysconf.init didnt set iointr");
}

void
unimpl_clockintr(arg)
	void *arg;
{
	panic("sysconf.init didnt set clockintr");
}

int
unimpl_intr(mask, pc, statusreg, causereg)
	u_int mask;
	u_int pc;
	u_int statusreg;
	u_int causereg;
{
	panic("sysconf.init didnt set intr");
}

unsigned
nullclkread()
{
	return 0;
}	
