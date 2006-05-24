/*	$NetBSD: machdep.c,v 1.95.12.1 2006/05/24 15:48:21 tron Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001, 2002, 2003 Rafal K. Boni
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
 *          NetBSD Project.  See http://www.NetBSD.org/ for
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.95.12.1 2006/05/24 15:48:21 tron Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_execfmt.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
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
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>
#include <machine/intr.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>

#include <mips/locore.h>
#include <mips/cache.h>
#include <mips/cache_r5k.h>
#ifdef ENABLE_MIPS4_CACHE_R10K
#include <mips/cache_r10k.h>
#endif

#include <sgimips/dev/int2reg.h>
#include <sgimips/sgimips/arcemu.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM) || defined(KGDB)
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

struct sgimips_intrhand intrtab[NINTR];

const uint32_t mips_ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTSERIAL */
};

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
u_int32_t splmasks[IPL_CLOCK+1];

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

#if defined(INDY_R4600_CACHE)
extern void	ip22_sdcache_disable(void);
extern void	ip22_sdcache_enable(void);
#endif

#if defined(MIPS1)
extern void mips1_clock_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
extern unsigned long mips1_clkread(void);
#endif

#if defined(MIPS3)
extern void mips3_clock_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
extern unsigned long mips3_clkread(void);
#endif

void	mach_init(int, char **, int, struct btinfo_common *);

void	sgimips_count_cpus(struct arcbios_component *,
	    struct arcbios_treewalk_context *);

#ifdef KGDB
void kgdb_port_init(void);
void kgdb_connect(int);
#endif

void mips_machdep_find_l2cache(struct arcbios_component *comp, struct arcbios_treewalk_context *atc);

/* Motherboard or system-specific initialization vector */
static void	unimpl_bus_reset(void);
static void	unimpl_cons_init(void);
static void	*unimpl_intr_establish(int, int, int (*)(void *), void *);
static void	unimpl_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
static unsigned	long nulllong(void);
static void	nullvoid(void);

void ddb_trap_hook(int where);

struct platform platform = {
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_intr_establish,
	nulllong,
	nullvoid,
	nullvoid,
	nullvoid,
	unimpl_intr,
	unimpl_intr,
	unimpl_intr,
	unimpl_intr,
	unimpl_intr,
	unimpl_intr,
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

#if defined(_LP64)
#define ARCS_VECTOR 0xa800000000001000
#else
#define ARCS_VECTOR (MIPS_PHYS_TO_KSEG0(0x00001000))
#endif

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the ARCS firmware.
 */
void
mach_init(int argc, char **argv, int magic, struct btinfo_common *btinfo)
{
	extern char kernel_text[], _end[];
	paddr_t first, last;
	int firstpfn, lastpfn;
	caddr_t v;
	vsize_t size;
	struct arcbios_mem *mem;
	const char *cpufreq;
	struct btinfo_symtab *bi_syms;
	caddr_t ssym;
	vaddr_t kernend;
	int kernstartpfn, kernendpfn;
	int i, rv, nsym;

	/*
	 * Initialize firmware.  This will set up the bootstrap console.
	 * At this point we do not yet know the machine type, so we
	 * try to init real arcbios, and if that fails (return value 1),
	 * fall back to the emulator.  If the latter fails also we
	 * don't have much to panic with.
	 */
	if (arcbios_init(ARCS_VECTOR) == 1)
		arcemu_init();

	strcpy(cpu_model, arcbios_system_identifier);

	uvm_setpagesize();

	nsym = 0;
	ssym = esym = NULL;
	kernend = round_page((vaddr_t) _end);
	bi_syms = NULL;
	bootinfo = NULL;

	if (magic == BOOTINFO_MAGIC && btinfo != NULL) {
		printf("Found bootinfo at %p\n", btinfo);
		bootinfo = btinfo;

		bi_syms = lookup_bootinfo(BTINFO_SYMTAB);
		if (bi_syms != NULL) {
			nsym = bi_syms->nsym;
			ssym = (caddr_t) bi_syms->ssym;
			esym = (caddr_t) bi_syms->esym;
			kernend = round_page((vaddr_t) esym);
		}
	}

	/* Leave 1 page before kernel untouched as that's where our initial
	 * kernel stack is */
	/* XXX We could free it in cpu_startup() though XXX */
	kernstartpfn = atop(MIPS_KSEG0_TO_PHYS((vaddr_t) kernel_text)) - 1;
	kernendpfn = atop(MIPS_KSEG0_TO_PHYS(kernend));

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
	if (argc > 1 && strchr(argv[1], '=') != 0)
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
	 * Pass the args again to check for flags -- This is done
	 * AFTER checking for OSLoadOptions to ensure that "auto"
	 * does not override the "-s" flag.
	 */

	for (i = 0; i < argc; i++) {
		/*
		 * Extract out any flags passed for the kernel in the
		 * argument string.  Warn for unknown/invalid flags,
		 * but silently skip non-flag arguments, as they are
		 * likely PROM environment values (if I knew those
		 * would always precede *any* flags, then I'd say we
		 * should warn about *all* unexpected values, but for
		 * now this should be fine).
		 *
		 * Use the MI boot-flag extractor since we don't use
		 * any special MD flags and to make sure we're up-to
		 * date with new MI flags whenever they're added.
		 */
		if (argv[i][0] == '-') {
			rv = 0;
			BOOT_FLAG(argv[i][1], rv);

			if (rv == 0) {
				printf("Unexpected option '%s' ignored",
				    argv[i]);
			} else {
				boothowto |= rv;
			}
		}
	}

#ifdef DEBUG
	boothowto |= AB_DEBUG;
#endif

	/*
	 * When the kernel is loaded directly by the firmware, and
	 * no explicit OSLoadPartition is set, we fall back on
	 * SystemPartition for the boot device.
	 */
	for (i = 0; i < argc; i++) {
		if (strncmp(argv[i], "SystemPartition", 15) == 0)
			makebootdev(argv[i] + 16);

		aprint_debug("argv[%d]: %s\n", i, argv[i]);
	}

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

#if defined(KGDB) || defined(DDB)
	/* Set up DDB hook to turn off watchdog on entry */
	db_trap_callback = ddb_trap_hook;

#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(nsym, ssym, esym);
#endif /* NKSYMS || defined(DDB) || defined(LKM) */

#  ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#  endif

#  ifdef KGDB
	kgdb_port_init();

	if (boothowto & RB_KDB)
		kgdb_connect(0);
#  endif
#endif

	switch (mach_type) {
#if defined(MIPS1)
	case MACH_SGI_IP12:
		i = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);
        	mach_boardrev = (i & 0x7000) >> 12; 

		if ((i & 0x8000) == 0) {
			if (mach_boardrev < 7)
				mach_subtype = MACH_SGI_IP12_4D_3X;
			else
				mach_subtype = MACH_SGI_IP12_VIP12;
		} else {
			if (mach_boardrev < 6)
				mach_subtype = MACH_SGI_IP12_HP1;
			else
				mach_subtype = MACH_SGI_IP12_HPLC;
                }

		splmasks[IPL_BIO] = 0x0b00;
		splmasks[IPL_NET] = 0x0b00;
		splmasks[IPL_TTY] = 0x1b00;
		splmasks[IPL_CLOCK] = 0x7f00;
		platform.intr3 = mips1_clock_intr;
		platform.clkread = mips1_clkread;
		break;
#endif /* MIPS1 */

#if defined(MIPS3)
	case MACH_SGI_IP20:
		i = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);
		mach_boardrev = (i & 0x7000) >> 12;

		splmasks[IPL_BIO] = 0x0700;
		splmasks[IPL_NET] = 0x0700;
		splmasks[IPL_TTY] = 0x0f00;
		splmasks[IPL_CLOCK] = 0xbf00;
		platform.intr5 = mips3_clock_intr;
		platform.clkread = mips3_clkread;
		break;
	case MACH_SGI_IP22:
		splmasks[IPL_BIO] = 0x0700;
		splmasks[IPL_NET] = 0x0700;
		splmasks[IPL_TTY] = 0x0f00;
		splmasks[IPL_CLOCK] = 0xbf00;
		platform.intr5 = mips3_clock_intr;
		platform.clkread = mips3_clkread;
		break;
	case MACH_SGI_IP30:
		splmasks[IPL_BIO] = 0x0700;
		splmasks[IPL_NET] = 0x0700;
		splmasks[IPL_TTY] = 0x0700;
		splmasks[IPL_CLOCK] = 0x8700;
		platform.intr5 = mips3_clock_intr;
		platform.clkread = mips3_clkread;
		break;
	case MACH_SGI_IP32:
		splmasks[IPL_BIO] = 0x0700;
		splmasks[IPL_NET] = 0x0700;
		splmasks[IPL_TTY] = 0x0700;
		splmasks[IPL_CLOCK] = 0x8700;
		platform.intr5 = mips3_clock_intr;
		platform.clkread = mips3_clkread;
		break;
#endif /* MIPS3 */
	default:
		panic("IP%d architecture not supported", mach_type);
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
			printf("Mem block %d: type %d, "
			    "base 0x%04lx, size 0x%04lx\n",
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
			if (kernstartpfn >= lastpfn ||
			    kernendpfn <= firstpfn) {
				/* Kernel is not in this cluster at all */
				
				aprint_debug("Loading cluster %d: "
				    "0x%x / 0x%x\n",
				    i, firstpfn, lastpfn);
				uvm_page_physload(firstpfn, lastpfn,
				    firstpfn, lastpfn, VM_FREELIST_DEFAULT);
			} else {
				if (firstpfn < kernstartpfn) {
					/*
					 * There is space before kernel in
					 * this cluster
					 */

					aprint_debug("Loading cluster %d "
					    "(before kernel): 0x%x / 0x%x\n",
					    i, firstpfn, kernstartpfn);
					uvm_page_physload(firstpfn,
					    kernstartpfn, firstpfn,
					    kernstartpfn, VM_FREELIST_DEFAULT);
				}

				if (lastpfn > kernendpfn) {
					/*
					 * There is space after kernel in
					 * this cluster
					 */

					aprint_debug("Loading cluster %d "
					    "(after kernel): 0x%x / 0x%x\n",
					    i, kernendpfn, lastpfn);
					uvm_page_physload(kernendpfn,
					    lastpfn, kernendpfn,
					    lastpfn, VM_FREELIST_DEFAULT);
				}
			}
			mem_clusters[mem_cluster_cnt].start = first;
			mem_clusters[mem_cluster_cnt].size = size;
			mem_cluster_cnt++;
			break;

		case ARCBIOS_MEM_FirmwareTemporary:
		case ARCBIOS_MEM_FirmwarePermanent:
			arcsmem += btoc(size);
			break;

		case ARCBIOS_MEM_ExceptionBlock:
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
	 * Initialize mips version-dependent DMA handlers.
	 */
	sgimips_bus_dma_init();

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

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (caddr_t)uvm_pageboot_alloc(USPACE);
	lwp0.l_addr = proc0paddr = (struct user *)v;
	lwp0.l_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &lwp0.l_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */
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
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), ctob(arcsmem));
	printf("(%s reserved for ARCS)\n", pbuf);

	minaddr = 0;
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
	printf("avail memory = %s\n", pbuf);
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	/* Take a snapshot before clobbering any registers. */
	if (curlwp)
		savectx((struct user *)curpcb);

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

	/* Clear and disable watchdog timer. */
	(void)(*platform.watchdog_disable)();

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
microtime(struct timeval *tvp)
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

inline void
delay(unsigned long n)
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

static void *
unimpl_intr_establish(int level, int ipl, int (*handler) (void *), void *arg)
{
	panic("target init didn't set intr_establish");
	return (void *)NULL;
}

static void
unimpl_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	printf("spurious interrupt, ipending %x\n", ipending);
}

static unsigned long
nulllong()
{
	printf("nulllong\n");
	return (0);
}

static void
nullvoid()
{
	printf("nullvoid\n");
	return;
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
		(void)(*platform.watchdog_disable)();
		break;

	case 0:		/* Exit from DDB, turn watchdog back on */
		(void)(*platform.watchdog_enable)();
		break;
	}
}

#endif

void mips_machdep_cache_config(void)
{
	arcbios_tree_walk(mips_machdep_find_l2cache, NULL);

	switch (MIPS_PRID_IMPL(cpu_id)) {
#if defined(INDY_R4600_CACHE)
	case MIPS_R4600:
		/*
		 * R4600 is on Indy-class machines only.  Disable and
		 * flush pcache.
		 */
		mips_sdcache_size = 0;
		mips_sdcache_line_size = 0;
		ip22_sdcache_disable();
		break;
#endif
#if defined(MIPS3)
	case MIPS_R5000:
	case MIPS_RM5200:
		r5k_enable_sdcache();
		break;
#endif
	}
}

void
mips_machdep_find_l2cache(struct arcbios_component *comp, struct arcbios_treewalk_context *atc)
{
	struct device *self = atc->atc_cookie;

	if (comp->Class != COMPONENT_CLASS_CacheClass)
		return;

	switch (comp->Type) {
	case COMPONENT_TYPE_SecondaryICache:
		panic("%s: split L2 cache", self->dv_xname);
	case COMPONENT_TYPE_SecondaryDCache:
	case COMPONENT_TYPE_SecondaryCache:
		mips_sdcache_size = COMPONENT_KEY_Cache_CacheSize(comp->Key);
		mips_sdcache_line_size =
		    COMPONENT_KEY_Cache_LineSize(comp->Key);
		/* XXX */
		mips_sdcache_ways = 1;
		break;
	}
}
