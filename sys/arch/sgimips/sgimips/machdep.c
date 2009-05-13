/*	$NetBSD: machdep.c,v 1.124.4.1 2009/05/13 17:18:21 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.124.4.1 2009/05/13 17:18:21 jym Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_execfmt.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"
#include "opt_modular.h"

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
#include <sgimips/dev/crimevar.h>
#include <sgimips/sgimips/arcemu.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR) || defined(KGDB)
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

#include "mcclock_mace.h"
#include "crime.h"

#if NMCCLOCK_MACE > 0
void mcclock_poweroff(void);
#endif

struct sgimips_intrhand intrtab[NINTR];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int mach_type = 0;	/* IPxx type */
int mach_subtype = 0;	/* subtype: eg., Guinness/Fullhouse for IP22 */
int mach_boardrev = 0;	/* machine board revision, in case it matters */

int physmem;		/* Total physical memory */
int arcsmem;		/* Memory used by the ARCS firmware */

int ncpus;

/* CPU interrupt masks */
const int *ipl2spl_table;

#define	IPL2SPL_TABLE_COMMON \
	[IPL_SOFTCLOCK]	= MIPS_SOFT_INT_MASK_1, \
	[IPL_HIGH]	= MIPS_INT_MASK,

#if defined(MIPS1)
static const int sgi_ip6_ipl2spl_table[] = {
	IPL2SPL_TABLE_COMMON

	[IPL_VM]	= MIPS_INT_MASK_1 |
			  MIPS_INT_MASK_0 |
			  MIPS_SOFT_INT_MASK_1,
			  MIPS_SOFT_INT_MASK_0,

	[IPL_SCHED]	= MIPS_INT_MASK_4 |
			  MIPS_INT_MASK_2 |
			  MIPS_INT_MASK_1 |
			  MIPS_INT_MASK_0 |
			  MIPS_SOFT_INT_MASK_1,
			  MIPS_SOFT_INT_MASK_0,
};
static const int sgi_ip12_ipl2spl_table[] = {
	IPL2SPL_TABLE_COMMON

	[IPL_VM]	= MIPS_INT_MASK_2 |
			  MIPS_INT_MASK_1 |
			  MIPS_INT_MASK_0 |
			  MIPS_SOFT_INT_MASK_1,
	    		  MIPS_SOFT_INT_MASK_0,

	[IPL_SCHED]	= MIPS_INT_MASK_4 |
			  MIPS_INT_MASK_3 |
			  MIPS_INT_MASK_2 |
	    		  MIPS_INT_MASK_1 |
			  MIPS_INT_MASK_0 |
			  MIPS_SOFT_INT_MASK_1,
			  MIPS_SOFT_INT_MASK_0,
};
#endif /* defined(MIPS1) */

#if defined(MIPS3)
static const int sgi_ip2x_ipl2spl_table[] = {
	IPL2SPL_TABLE_COMMON

	[IPL_VM]	= MIPS_INT_MASK_1 |
			  MIPS_INT_MASK_0 |
			  MIPS_SOFT_INT_MASK_1 |
			  MIPS_SOFT_INT_MASK_0,

	[IPL_SCHED]	= MIPS_INT_MASK_5 |
			  MIPS_INT_MASK_3 |
			  MIPS_INT_MASK_2 |
			  MIPS_INT_MASK_1 |
			  MIPS_INT_MASK_0 |
			  MIPS_SOFT_INT_MASK_1 |
			  MIPS_SOFT_INT_MASK_0,
};
static const int sgi_ip3x_ipl2spl_table[] = {
	IPL2SPL_TABLE_COMMON

	[IPL_VM]	= MIPS_INT_MASK_0 | 
			  MIPS_SOFT_INT_MASK_1 |
			  MIPS_SOFT_INT_MASK_0,

	[IPL_SCHED]	= MIPS_INT_MASK_5 |
			  MIPS_INT_MASK_0 |
	    		  MIPS_SOFT_INT_MASK_1 |
			  MIPS_SOFT_INT_MASK_0,
};
#endif /* defined(MIPS3) */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

#if defined(INDY_R4600_CACHE)
extern void	ip22_sdcache_disable(void);
extern void	ip22_sdcache_enable(void);
#endif

#if defined(MIPS1)
extern void mips1_fpu_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
#endif

#if defined(MIPS3)
extern void mips3_clock_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
#endif

void	mach_init(int, char **, u_int, void *);

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

static int badaddr_workaround(void *, size_t);

struct platform platform = {
	.badaddr		= badaddr_workaround,
	.bus_reset		= unimpl_bus_reset,
	.cons_init		= unimpl_cons_init,
	.intr_establish		= unimpl_intr_establish,
	.clkread		= nulllong,
	.watchdog_reset		= nullvoid,
	.watchdog_disable	= nullvoid,
	.watchdog_enable	= nullvoid,
	.intr0			= unimpl_intr,
	.intr1			= unimpl_intr,
	.intr2			= unimpl_intr,
	.intr3			= unimpl_intr,
	.intr4			= unimpl_intr,
	.intr5			= unimpl_intr
};

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern u_int32_t ssir;
extern struct user *proc0paddr;
extern char kernel_text[], edata[], end[];

uint8_t *bootinfo;			/* pointer to bootinfo structure */
static uint8_t bi_buf[BOOTINFO_SIZE];	/* buffer to store bootinfo data */
static const char *bootinfo_msg = NULL;

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
mach_init(int argc, char *argv[], u_int magic, void *bip)
{
	paddr_t first, last;
	int firstpfn, lastpfn;
	void *v;
	vsize_t size;
	struct arcbios_mem *mem;
	const char *cpufreq, *osload;
	char *bootpath = NULL;
	vaddr_t kernend;
	int kernstartpfn, kernendpfn;
	int i, rv;
#if NKSYMS > 0 || defined(DDB) || defined(MODULAR)
	int nsym = 0;
	char *ssym = NULL;
	char *esym = NULL;
	struct btinfo_symtab *bi_syms;
#endif

	/*
	 * Initialize firmware.  This will set up the bootstrap console.
	 * At this point we do not yet know the machine type, so we
	 * try to init real arcbios, and if that fails (return value 1),
	 * fall back to the emulator.  If the latter fails also we
	 * don't have much to panic with.
	 *
	 * The third argument (magic) is the environment variable array if
	 * there's no bootinfo.
	 */
	if (arcbios_init(ARCS_VECTOR) == 1) {
		if (magic == BOOTINFO_MAGIC)
			arcemu_init(NULL);	/* XXX - need some prom env */
		else
			arcemu_init((const char **)magic);
	}

	strcpy(cpu_model, arcbios_system_identifier);

	uvm_setpagesize();

	/* set up bootinfo structures */
	if (magic == BOOTINFO_MAGIC && bip != NULL) {
		struct btinfo_magic *bi_magic;
		struct btinfo_bootpath *bi_path;

		memcpy(bi_buf, bip, BOOTINFO_SIZE);
		bootinfo = bi_buf;
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic != NULL && bi_magic->magic == BOOTINFO_MAGIC) {
			bootinfo_msg = "bootinfo found.\n";
			bi_path = lookup_bootinfo(BTINFO_BOOTPATH);
			if (bi_path != NULL)
				bootpath = bi_path->bootpath;
		} else
			bootinfo_msg =
			    "invalid magic number in bootinfo structure.\n";
	} else
		bootinfo_msg = "no bootinfo found. (old bootblocks?)\n";

#if NKSYM > 0 || defined(DDB) || defined(MODULAR)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* check whether there is valid bootinfo symtab info */
	if (bi_syms != NULL) {
		nsym = bi_syms->nsym;
		ssym = (char *)bi_syms->ssym;
		esym = (char *)bi_syms->esym;
		kernend = mips_round_page(esym);
	} else
#endif
	{
		kernend = mips_round_page(end);
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
	 * Check machine (IPn) type.
	 *
	 * Note even on IP12 (which doesn't have ARCBIOS),
	 * arcbios_system_identifiler[] has been initilialized
	 * in arcemu_ip12_init().
	 */
	for (i = 0; arcbios_system_identifier[i] != '\0'; i++) {
		if (mach_type == 0 &&
		    arcbios_system_identifier[i] >= '0' &&
		    arcbios_system_identifier[i] <= '9') {
			mach_type = strtoul(&arcbios_system_identifier[i],
			    NULL, 10);
			break;
		}
	}
	if (mach_type <= 0)
		panic("invalid architecture");

	/*
	 * Get boot device infomation.
	 */

	/* Try to get the boot device information from bootinfo first. */
	if (bootpath != NULL)
		makebootdev(bootpath);
	else {
		/*
		 * The old bootloader prior to 5.0 doesn't pass bootinfo.
		 * If argv[0] is the bootloader, then argv[1] might be
		 * the kernel that was loaded.
		 * If argv[1] isn't an environment string, try to use it
		 * to set the boot device.
		 */
		if (argc > 1 && strchr(argv[1], '=') != 0)
			makebootdev(argv[1]);

		/*
		 * If we are loaded directly by ARCBIOS,
		 * argv[0] is the path of the loaded kernel,
		 * but booted_partition could be SGIVOLHDR in such case,
		 * so assume root is partition a.
		 */
		if (argc > 0 && argv[0] != NULL) {
			makebootdev(argv[0]);
			booted_partition = 0;
		}
	}

	/*
	 * Also try to get the default bootpath from ARCBIOS envronment
	 * bacause bootpath is not set properly by old bootloaders and
	 * argv[0] might be invalid on some machine.
	 */
	osload = ARCBIOS->GetEnvironmentVariable("OSLoadPartition");
	if (osload != NULL)
		makebootdev(osload);

	/*
	 * The case where the kernel has been loaded by a
	 * boot loader will usually have been catched by
	 * the first makebootdev() case earlier on, but
	 * we still use OSLoadPartition to get the preferred
	 * root filesystem location, even if it's not
	 * actually the location of the loaded kernel.
	 */
	for (i = 0; i < argc; i++) {
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
	}

	/*
	 * Single- or multi-user ('auto' in SGI terms).
	 *
	 * Query ARCBIOS first, then default to environment variables.
	 */

	/* Set default to single user. */
	boothowto = RB_SINGLE;

	osload = ARCBIOS->GetEnvironmentVariable("OSLoadOptions");
	if (osload != NULL && strcmp(osload, "auto") == 0)
		boothowto &= ~RB_SINGLE;

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "OSLoadOptions=auto") == 0)
			boothowto &= ~RB_SINGLE;
	}

	/*
	 * Pass the args again to check for flags -- This is done
	 * AFTER checking for OSLoadOptions to ensure that "auto"
	 * does not override the "-s" flag.
	 */

	for (i = 0; i < argc; i++) {
		/*
		 * Unfortunately, it appears that IP12's prom passes a '-a'
		 * flag when booting a kernel directly from a disk volume
		 * header. This corresponds to RB_ASKNAME in NetBSD, but
		 * appears to mean 'autoboot' in prehistoric SGI-speak.
		 */
		if (mach_type < MACH_SGI_IP20 && bootinfo == NULL &&
		    strcmp(argv[i], "-a") == 0)
			continue;

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
	aprint_debug("argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		aprint_debug("argv[%d] = %s\n", i, argv[i]);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* init symbols if present */
	if (esym)
		ksyms_addsyms_elf(nsym, ssym, esym);
#endif /* NKSYMS || defined(DDB) || defined(MODULAR) */

#if defined(KGDB) || defined(DDB)
	/* Set up DDB hook to turn off watchdog on entry */
	db_trap_callback = ddb_trap_hook;

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

#ifdef KGDB
	kgdb_port_init();

	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif
#endif

	switch (mach_type) {
#if defined(MIPS1)
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		ipl2spl_table = sgi_ip6_ipl2spl_table;
		platform.intr3 = mips1_fpu_intr;
		break;

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

		ipl2spl_table = sgi_ip12_ipl2spl_table;
		platform.intr0 = mips1_fpu_intr;
		break;
#endif /* MIPS1 */

#if defined(MIPS3)
	case MACH_SGI_IP20:
		i = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);
		mach_boardrev = (i & 0x7000) >> 12;
		ipl2spl_table = sgi_ip2x_ipl2spl_table;
		platform.intr5 = mips3_clock_intr;
		break;
	case MACH_SGI_IP22:
		ipl2spl_table = sgi_ip2x_ipl2spl_table;
		platform.intr5 = mips3_clock_intr;
		break;
	case MACH_SGI_IP30:
		ipl2spl_table = sgi_ip3x_ipl2spl_table;
		platform.intr5 = mips3_clock_intr;
		break;
	case MACH_SGI_IP32:
		ipl2spl_table = sgi_ip3x_ipl2spl_table;
		platform.intr5 = mips3_clock_intr;
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
	v = (void *)uvm_pageboot_alloc(USPACE);
	lwp0.l_addr = proc0paddr = (struct user *)v;
	lwp0.l_md.md_regs = (struct frame *)((char *)v + USPACE) - 1;
	proc0paddr->u_pcb.pcb_context[11] =
	    MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */
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
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

#ifdef BOOTINFO_DEBUG
	if (bootinfo_msg)
		printf(bootinfo_msg);
#endif

	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), ctob(arcsmem));
	printf("(%s reserved for ARCS)\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    VM_PHYS_SIZE, 0, false, NULL);

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

	pmf_system_shutdown(boothowto);

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
#if NMCCLOCK_MACE > 0
		if (mach_type == MACH_SGI_IP32) {
			mcclock_poweroff();
		} else 
#endif
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
#if NCRIME > 0
	if (mach_type == MACH_SGI_IP32) {
		crime_reboot();
	} else
#endif	
		ARCBIOS->Reboot();

	for (;;);
}

void delay(unsigned long n)
{
	register int __N = curcpu()->ci_divisor_delay * n;

	do {
		__asm("addiu %0,%1,-1" : "=r" (__N) : "0" (__N));
	} while (__N > 0);
}

/*
 * IP12 appears to be buggy and unable to reliably support badaddr.
 * Approximately 1.8% of the time a false negative (bad address said to
 * be good) is generated and we stomp on invalid registers. Testing has
 * not shown false positives, nor consecutive false negatives to occur.
 */
static int
badaddr_workaround(void *addr, size_t size)
{
	int i, bad;

	for (i = bad = 0; i < 100; i++) {
		if (badaddr(addr, size))
			bad++;
	}

	/* false positives appear not to occur */
	return (bad != 0);
}

/*
 *  Ensure all platform vectors are always initialized.
 */
static void
unimpl_bus_reset(void)
{

	panic("target init didn't set bus_reset");
}

static void
unimpl_cons_init(void)
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
nulllong(void)
{
	printf("nulllong\n");
	return (0);
}

static void
nullvoid(void)
{
	printf("nullvoid\n");
	return;
}

void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	uint8_t *bip;

	/* check for a bootinfo record first */
	if (bootinfo == NULL)
		return NULL;

	bip = bootinfo;
	do {
		bt = (struct btinfo_common *)bip;
		if (bt->type == type)
			return (void *)bt;
		bip += bt->next;
	} while (bt->next != 0 &&
	    bt->next < BOOTINFO_SIZE /* sanity */ &&
	    (size_t)bip < (size_t)bootinfo + BOOTINFO_SIZE);

	return NULL;
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

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._spl = ipl2spl_table[ipl]};
}
