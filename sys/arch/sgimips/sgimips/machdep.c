/*	$NetBSD: machdep.c,v 1.16 2001/06/02 18:09:20 chs Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_machtypes.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/device.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>
#include <machine/intr.h>
#include <machine/arcs.h>
#include <mips/locore.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <dev/cons.h>

/* For sysctl(3). */
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "SGI";

struct sgi_intrhand intrtab[NINTR];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

unsigned long cpuspeed;	/* Approximate number of instructions per usec */

/* Maps for VM objects. */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int mach_type;		/* IPxx type */
int mach_subtype;	/* subtype: eg., Guiness/Fullhouse for IP22 */
int mach_boardrev;	/* machine board revision, in case it matters */

int physmem;		/* Total physical memory */
int arcsmem;		/* Memory used by the ARCS firmware */

/* CPU interrupt masks */
u_int32_t biomask;
u_int32_t netmask;
u_int32_t ttymask;
u_int32_t clockmask;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

#ifdef IP20
void	ip20_init(void);
#endif

#ifdef IP22
void	ip22_init(void);
#endif

#ifdef IP32
void	ip32_init(void);
#endif

void * cpu_intr_establish(int, int, int (*)(void *), void *);

void	mach_init(int, char **, char **);
void	unconfigured_system_type(int);

extern int 	atoi(char *);	/* For parsing IP (archictecture) number */
extern void	arcsinit(void);

/* Motherboard or system-specific initialization vector */
static void	unimpl_bus_reset(void);
static void	unimpl_cons_init(void);
static void	unimpl_iointr(unsigned, unsigned, unsigned, unsigned);
static void	unimpl_intr_establish(int, int, int (*)(void *), void *);
static unsigned	nullwork(void);

struct platform platform = {
	1000000,
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_iointr,
	unimpl_intr_establish,
	(void *)nullwork,
};


/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern caddr_t esym;
extern u_int32_t ssir;
extern struct user *proc0paddr;

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the ARCS firmware.
 */
void
mach_init(argc, argv, envp)
	int argc;
	char **argv;
	char **envp;
{
	unsigned long first, last;
	caddr_t kernend, v;
	vsize_t size;
	extern char edata[], end[];
	struct arcs_mem *mem;
	struct arcs_component *root;
	char *cpufreq;
	int i;

	/*
	 * Clear the BSS segment.
	 */
#ifdef DDB 
	if (memcmp(((Elf_Ehdr *)end)->e_ident, ELFMAG, SELFMAG) == 0 &&
	    ((Elf_Ehdr *)end)->e_ident[EI_CLASS] == ELFCLASS) {
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

	arcsinit();
	consinit();

#if 1 /* skidt? */
	ARCS->FlushAllCaches();
#endif

	cpufreq = ARCS->GetEnvironmentVariable("cpufreq");

	if (cpufreq == 0)
		panic("no $cpufreq");

	cpuspeed = strtoul(cpufreq, NULL, 10) / 2;	/* XXX MIPS3 only */
#if 0	/* XXX create new mips/mips interface */
	... something ... = strtoul(cpufreq, NULL, 10) * 5000000;
#endif

	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	boothowto = RB_SINGLE;

	for (i = 0; i < argc; i++) {
#if 0
		if (strcmp(argv[i], "OSLoadOptions=auto") == 0) {
			boothowto &= ~RB_SINGLE;
		}
#endif
#if 0
		printf("argv[%d]: %s\n", i, argv[i]);
		/* delay(20000); */ /* give the user a little time.. */
#endif
	}

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	root = ARCS->GetChild(NULL);
	for (i = 0; root->Identifier[i] != '\0'; i++) {
		if (root->Identifier[i] >= '0' &&
		    root->Identifier[i] <= '9') {
			mach_type = atoi(&root->Identifier[i]);
			break;
		}
	}

	if (mach_type <= 0)
		panic("invalid architecture");

	switch (mach_type) {
	  case MACH_SGI_IP20:
#ifdef IP20
	    ip20_init();
#else
	    unconfigured_system_type(mach_type);
#endif
	    break;
	    
	  case MACH_SGI_IP22:
#ifdef IP22
	    ip22_init();
#else
	    unconfigured_system_type(mach_type);
#endif
	    break;

	  case MACH_SGI_IP32:
#ifdef IP32
	    ip32_init();
#else
	    unconfigured_system_type(mach_type);
#endif
	    break;

	  default:
	    panic("IP%d architecture not yet supported\n", mach_type);
	    break;
	}

	physmem = arcsmem = 0;
	mem_cluster_cnt = 0;
	mem = NULL;

#ifdef DEBUG
	i = 0;
	mem = NULL;

	do {
	    if ((mem = ARCS->GetMemoryDescriptor(mem)) != NULL) {
		i++;
		printf("Mem block %d: type %d, base %d, size %d\n", 
				i, mem->Type, mem->BasePage, mem->PageCount);
	    }
	} while (mem != NULL);
#endif

	mem = NULL;
	for (i = 0; i < VM_PHYSSEG_MAX; i++) { 
		mem = ARCS->GetMemoryDescriptor(mem);

		if (mem == NULL)
			break;

		first = round_page(mem->BasePage * ARCS_PAGESIZE);
		last = trunc_page(first + mem->PageCount * ARCS_PAGESIZE);
		size = last - first;

		switch (mem->Type) {
		case ARCS_MEM_CONT:
		case ARCS_MEM_FREE:
			if (last > MIPS_KSEG0_TO_PHYS(kernend))
				if (first < MIPS_KSEG0_TO_PHYS(kernend))
					first = MIPS_KSEG0_TO_PHYS(kernend);

			mem_clusters[mem_cluster_cnt].start = first;
			mem_clusters[mem_cluster_cnt].size = size;
			mem_cluster_cnt++;

			uvm_page_physload(atop(first), atop(last), atop(first),
					atop(last), VM_FREELIST_DEFAULT);

			break;
		case ARCS_MEM_TEMP:
		case ARCS_MEM_PERM:
			arcsmem += btoc(size);
			break;
		case ARCS_MEM_EXCEP:
		case ARCS_MEM_SPB:
		case ARCS_MEM_BAD:
		case ARCS_MEM_PROG:
			break;
		default:
			panic("unknown memory descriptor %d type %d",
				i, mem->Type); 
		}

		physmem += btoc(size);

	}

	if (mem_cluster_cnt == 0)
		panic("no free memory descriptors found");

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Compute the size of system data structures.  pmap_bootstrap()
	 * needs some of this information.
	 */
	size = (vsize_t)allocsys(NULL, NULL);

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (caddr_t)uvm_pageboot_alloc(USPACE); 
	proc0.p_addr = proc0paddr = (struct user *)v;
	proc0.p_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	v = (caddr_t)uvm_pageboot_alloc(size); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");
}

/*
 * Allocate memory for variable-sized tables.
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

	printf(version);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("%s memory", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
		    UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
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
		curbufsize = NBPG * ((i < residual) ? (base + 1) : base);

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
	pmap_update();

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf(", %s free", pbuf);
	format_bytes(pbuf, sizeof(pbuf), ctob(arcsmem));
	printf(", %s for ARCS", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf(", %s in %d buffers\n", pbuf, nbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

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
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}
}

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	/* Take a snapshot before clobbering any registers. */
	if (curproc)
		savectx((struct user *)curpcb);

#if 1	
	/* Clear and disable watchdog timer. */
	switch (mach_type) {
	  case MACH_SGI_IP22:
		*(volatile u_int32_t *)0xbfa00014 = 0;
		*(volatile u_int32_t *)0xbfa00004 &= ~0x100;
		break;

	  case MACH_SGI_IP32:
		*(volatile u_int32_t *)0xb4000034 = 0;
		*(volatile u_int32_t *)0xb400000c &= ~0x200;
		break;
	}
#endif

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) && (waittime < 0)) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

haltsys:

	doshutdownhooks();

#if 0
	if (howto & RB_POWERDOWN) {
		printf("powering off...\n\n");
		ARCS->PowerDown();
		printf("WARNING: powerdown failed\n");
	}
#endif

	if (howto & RB_HALT) {
		printf("halting...\n\n");
		ARCS->EnterInteractiveMode();
	}

	printf("rebooting...\n\n");
	ARCS->Reboot();

	for (;;);
}

void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;

	/*
	 * Make sure that the time returned is always greater
	 * than that returned by the previous call.
	 */
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

__inline void
delay(n)
	unsigned long n;
{
	register long N = cpuspeed * n;

	while (--N > 0);
}

/*
 *  Ensure all platform vectors are always initialized.
 */
static void
unimpl_bus_reset()
{

	panic("target init didn't set bus_reset");
}

static void
unimpl_cons_init()
{

	panic("target init didn't set cons_init");
}

static void
unimpl_iointr(mask, pc, statusreg, causereg)
	u_int mask;
	u_int pc;
	u_int statusreg;
	u_int causereg;
{

	panic("target init didn't set intr");
}

static void
unimpl_intr_establish(level, ipl, handler, arg)
	int level;
	int ipl;
	int (*handler) __P((void *));
	void *arg;
{
	panic("target init didn't set intr_establish");
}

static unsigned
nullwork()
{

	return (0);
}

void *
cpu_intr_establish(level, ipl, func, arg)
	int level;
	int ipl;
	int (*func)(void *);
	void *arg;
{
	(*platform.intr_establish)(level, ipl, func, arg);
	return (void *) -1;
}

void
cpu_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{
	uvmexp.intrs++;

	if (ipending & MIPS_HARD_INT_MASK)
		(*platform.iointr)(status, cause, pc, ipending);

	/* software simulated interrupt */
	if ((ipending & MIPS_SOFT_INT_MASK_1)
		    || (ssir && (status & MIPS_SOFT_INT_MASK_1))) {
	    _clrsoftintr(MIPS_SOFT_INT_MASK_1);
	    softintr_dispatch();
	}
}

void unconfigured_system_type(int ipnum)
{
	printf("Kernel not configured for IP%d support.  Add options `IP%d'\n",
								ipnum, ipnum);
	printf("to kernel configuration file to enable IP%d support!\n", 
								ipnum);
	printf("\n");

	panic("Kernel not configured for current hardware!");
}

