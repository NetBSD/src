/*	$NetBSD: machdep.c,v 1.37 2002/05/03 01:49:22 rafal Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001, 2002 Rafal K. Boni
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
#include "opt_kgdb.h"
#include "opt_execfmt.h"
#include "opt_cputype.h"
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
#include <machine/bootinfo.h>
#include <mips/locore.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#if defined(DDB) || defined(KGDB)
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
char cpu_model[64 + 1];		/* sizeof(arcbios_system_identifier) */

struct sgimips_intrhand intrtab[NINTR];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int mach_type;		/* IPxx type */
int mach_subtype;	/* subtype: eg., Guiness/Fullhouse for IP22 */
int mach_boardrev;	/* machine board revision, in case it matters */

int physmem;		/* Total physical memory */
int arcsmem;		/* Memory used by the ARCS firmware */

int ncpus;

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

void	mach_init(int, char **, int, struct btinfo_common *);
void	unconfigured_system_type(int);

void	sgimips_count_cpus(struct arcbios_component *,
	    struct arcbios_treewalk_context *);

#ifdef KGDB
void zs_kgdb_init(void);
void kgdb_connect(int);
#endif

/* Motherboard or system-specific initialization vector */
static void	unimpl_bus_reset(void);
static void	unimpl_cons_init(void);
static void	unimpl_iointr(unsigned, unsigned, unsigned, unsigned);
static void	unimpl_intr_establish(int, int, int (*)(void *), void *);
static unsigned	long nullwork(void);

void ddb_trap_hook(int where);

struct platform platform = {
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

static struct btinfo_common *bootinfo;

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the ARCS firmware.
 */
void
mach_init(argc, argv, magic, btinfo)
	int argc;
	char **argv;
	int magic;
	struct btinfo_common *btinfo;
{
	extern char kernel_text[], _end[];
	paddr_t first, last;
	int firstpfn, lastpfn;
	caddr_t v;
	vsize_t size;
	struct arcbios_mem *mem;
	char *cpufreq;
	struct btinfo_symtab *bi_syms;
	caddr_t ssym;
	vaddr_t kernend;
	int kernstartpfn, kernendpfn;
	int i, nsym;

#if 0
	/* Clear the BSS segment.  XXX Is this really necessary? */
	memset(_edata, 0, _end - _edata);
#endif

	/*
	 * Initialize ARCS.  This will set up the bootstrap console.
	 */
	arcbios_init(MIPS_PHYS_TO_KSEG0(0x00001000));
	strcpy(cpu_model, arcbios_system_identifier);

	uvm_setpagesize();

	if (magic == BOOTINFO_MAGIC && btinfo != NULL) {
#ifdef DEBUG
		printf("Found bootinfo at %p\n", btinfo);
#endif
		bootinfo = btinfo;
	}

	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);
	if (bi_syms != NULL) {
		nsym = bi_syms->nsym;
		ssym = (caddr_t) bi_syms->ssym;
		esym = (caddr_t) bi_syms->esym;
		kernend = round_page((vaddr_t) esym);
	} else {
		nsym = 0;
		ssym = esym = NULL;
		kernend = round_page((vaddr_t) _end);
	}

	kernstartpfn = atop(MIPS_KSEG0_TO_PHYS((vaddr_t) kernel_text));
	kernendpfn = atop(MIPS_KSEG0_TO_PHYS(kernend));

	/*
	 * Now set up the real console.
	 * XXX Should be done later after we determine systype.
	 */
	consinit();

#if 1 /* skidt? */
	ARCBIOS->FlushAllCaches();
#endif

	cpufreq = ARCBIOS->GetEnvironmentVariable("cpufreq");

	if (cpufreq == 0)
		panic("no $cpufreq");

	/* 
	 * Note initial estimate of CPU speed... If we care enough, we'll
	 * use the RTC to get a better estimate later.
	 */
	curcpu()->ci_cpu_freq = strtoul(cpufreq, NULL, 10) * 1000000;

	/*
	 * argv[0] can be either the bootloader loaded by the PROM, or a
	 * kernel loaded directly by the PROM.
	 *
	 * If argv[0] is the bootloader, then argv[1] might be the kernel
	 * that was loaded.  How to tell which one to use?
	 *
	 * If argv[1] isn't an environment string, try to use it to set the
	 * boot device.
	 */
	if (strchr(argv[1], '=') != 0)
		makebootdev(argv[1]);

	boothowto = RB_SINGLE;

	/*
	 * Single- or multi-user ('auto' in SGI terms).
	 */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "OSLoadOptions=auto") == 0)
			boothowto &= ~RB_SINGLE;

		/*
		 * The case where the kernel has been loaded by a
		 * boot loader will usually have been catched by
		 * the first makebootdev() case earlier on, but
		 * we still use OSLoadPartition to get the preferred
		 * root filesystem location, even if it's not
		 * actually the location of the loaded kernel.
		 */
		if (strncmp(argv[i], "OSLoadPartition=", 15) == 0)
			makebootdev(argv[i] + 16);
	}

	/*
	 * When the kernel is loaded directly by the firmware, and
	 * no explicit OSLoadPartition is set, we fall back on
	 * SystemPartition for the boot device.
	 */
	for (i = 0; i < argc; i++) {
		if (strncmp(argv[i], "SystemPartition", 15) == 0)
			makebootdev(argv[i] + 16);

#ifdef DEBUG
		printf("argv[%d]: %s\n", i, argv[i]);
#endif
	}

#if defined(KGDB) || defined(DDB)
	/* Set up DDB hook to turn off watchdog on entry */
	db_trap_callback = ddb_trap_hook;

#ifdef DDB
	ddb_init(nsym, ssym, esym);
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	zs_kgdb_init();			/* XXX */
	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif
#endif

	for (i = 0; arcbios_system_identifier[i] != '\0'; i++) {
		if (arcbios_system_identifier[i] >= '0' &&
		    arcbios_system_identifier[i] <= '9') {
			mach_type = strtoul(&arcbios_system_identifier[i],
			    NULL, 10);
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
		if ((mem = ARCBIOS->GetMemoryDescriptor(mem)) != NULL) {
			i++;
			printf("Mem block %d: type %d, base 0x%x, size 0x%x\n",
				i, mem->Type, mem->BasePage, mem->PageCount);
		}
	} while (mem != NULL);
#endif

	/*
	 * XXX This code assumes that ARCS provides the memory
	 * XXX sorted in ascending order.
	 */
	mem = NULL;
	for (i = 0; mem_cluster_cnt < VM_PHYSSEG_MAX; i++) {
		mem = ARCBIOS->GetMemoryDescriptor(mem);

		if (mem == NULL)
			break;

		first = round_page(mem->BasePage * ARCBIOS_PAGESIZE);
		last = trunc_page(first + mem->PageCount * ARCBIOS_PAGESIZE);
		size = last - first;

		firstpfn = atop(first);
		lastpfn = atop(last);

		switch (mem->Type) {
		case ARCBIOS_MEM_FreeContiguous:
		case ARCBIOS_MEM_FreeMemory:
		case ARCBIOS_MEM_LoadedProgram:
			if (firstpfn <= kernstartpfn &&
			    kernendpfn <= lastpfn) {
				/*
				 * Must compute the location of the kernel
				 * within the segment.
				 */
#ifdef DEBUG
				printf("Cluster %d contains kernel\n", i);
#endif
				if (firstpfn < kernstartpfn) {
					/*
					 * There is a chunk before the kernel.
					 */
#ifdef DEBUG
					printf("Loading chunk before kernel: "
					    "0x%x / 0x%x\n", firstpfn,
					    kernstartpfn);
#endif
					uvm_page_physload(firstpfn,
					    kernstartpfn,
					    firstpfn, kernstartpfn,
					    VM_FREELIST_DEFAULT);
				}
				if (kernendpfn < lastpfn) {
					/*
					 * There is a chunk after the kernel.
					 */
#ifdef DEBUG
					printf("Loading chunk after kernel: "
					    "0x%x / 0x%x\n", kernendpfn,
					    lastpfn);
#endif
					uvm_page_physload(kernendpfn,
					    lastpfn, kernendpfn,
					    lastpfn, VM_FREELIST_DEFAULT);
				}
			} else {
				/*
				 * Just load this cluster as one chunk.
				 */
#ifdef DEBUG
				printf("Loading cluster %d: 0x%x / 0x%x\n", i,
				    firstpfn, lastpfn);
#endif
				uvm_page_physload(firstpfn, lastpfn,
				    firstpfn, lastpfn, VM_FREELIST_DEFAULT);
			}
			mem_clusters[mem_cluster_cnt].start = first;
			mem_clusters[mem_cluster_cnt].size = size;
			mem_cluster_cnt++;
			break;

		case ARCBIOS_MEM_FirmwareTemporary:
		case ARCBIOS_MEM_FirmwarePermanent:
			arcsmem += btoc(size);
			break;

		case ARCBIOS_MEM_ExecptionBlock:
		case ARCBIOS_MEM_SystemParameterBlock:
		case ARCBIOS_MEM_BadMemory:
			break;

		default:
			panic("unknown memory descriptor %d type %d",
				i, mem->Type);
		}

		physmem += btoc(size);

	}

	if (mem_cluster_cnt == 0)
		panic("no free memory descriptors found");

	/* We can now no longer use bootinfo. */
	bootinfo = NULL;

	/*
	 * Walk the component tree and count the number of CPUs
	 * present in the system.
	 */
	arcbios_tree_walk(sgimips_count_cpus, NULL);

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

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

void
sgimips_count_cpus(struct arcbios_component *node,
    struct arcbios_treewalk_context *atc)
{

	switch (node->Class) {
	case COMPONENT_CLASS_ProcessorClass:
		if (node->Type == COMPONENT_TYPE_CPU)
			ncpus++;
		break;

	default:
		break;
	}
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
	pmap_update(pmap_kernel());

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
	if ((howto & RB_NOSYNC) == 0 && (waittime < 0)) {
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

	/*
	 * Calling ARCBIOS->PowerDown() results in a "CP1 unusable trap"
	 * which lands me back in DDB, at least on my Indy.  So, enable
	 * the FPU before asking the PROM to power down to avoid this..
	 * It seems to want the FPU to play the `poweroff tune' 8-/
	 */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		/* Set CP1 usable bit in SR */
	 	mips_cp0_status_write(mips_cp0_status_read() |
					MIPS_SR_COP_1_BIT);

		printf("powering off...\n\n");
		delay(500000);
		ARCBIOS->PowerDown();
		printf("WARNING: powerdown failed\n");
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
	}

	if (howto & RB_HALT) {
		printf("halting...\n\n");
		ARCBIOS->EnterInteractiveMode();
	}

	printf("rebooting...\n\n");
	ARCBIOS->Reboot();

	for (;;);
}

void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += (*platform.clkread)();

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
	u_long i;
	long divisor = curcpu()->ci_divisor_delay;

	while (n-- > 0)
		for (i = divisor; i > 0; i--)
			;
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

static unsigned long
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

	/*
	 * Service pending soft interrupts -- make sure to re-enable
	 * only those hardware interrupts that are not masked and 
	 * that weren't pending on the current invocation of the
	 * interrupt handler, else we risk infinite stack growth
	 * due to nested interrupts.
	 */
	/* software simulated interrupt */
	if ((ipending & MIPS_SOFT_INT_MASK_1) || 
	    (ssir && (status & MIPS_SOFT_INT_MASK_1))) {
		_splset(MIPS_SR_INT_IE |
			    (status & ~ipending & MIPS_HARD_INT_MASK));
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

void *
lookup_bootinfo(int type)
{
	struct btinfo_common *b = bootinfo;

	while (bootinfo != NULL) {
		if (b->type == type)
			return (b);
		b = b->next;
	}

	return (NULL);
}

#if defined(DDB) || defined(KGDB)

void ddb_trap_hook(int where)
{
	switch (where) {
	case 1:		/* Entry to DDB, turn watchdog off */
		switch (mach_type) {
		case MACH_SGI_IP32:
			*(volatile u_int32_t *)0xb4000034 = 0;
			*(volatile u_int32_t *)0xb400000c &= ~0x200;
			break;

		case MACH_SGI_IP22:
			*(volatile u_int32_t *)0xbfa00014 = 0;
			*(volatile u_int32_t *)0xbfa00004 &= ~0x100;
			break;
		}
		break;

	case 0:		/* Exit from DDB, turn watchdog back on */
		switch (mach_type) {
		case MACH_SGI_IP32:
			*(volatile u_int32_t *)0xb400000c |= 0x200;
			*(volatile u_int32_t *)0xb4000034 = 0;
			break;

		case MACH_SGI_IP22:
			*(volatile u_int32_t *)0xbfa00004 |= 0x100;
			*(volatile u_int32_t *)0xbfa00014 = 0;

			break;
		}
		break;
	}
}

#endif
