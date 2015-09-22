/*	$NetBSD: machdep.c,v 1.10.6.1 2015/09/22 12:05:40 skrll Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
 * 	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.10.6.1 2015/09/22 12:05:40 skrll Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <mips/cache.h>
#include <machine/psl.h>
#include <machine/autoconf.h>
#include <emips/stand/common/prom_iface.h>
#include <machine/sysconf.h>
#include <machine/bootinfo.h>
#include <machine/locore.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

#define _EMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

vaddr_t iospace = 64 * 1024; /* BUGBUG make it an option? */
vsize_t iospace_size;

#include "ksyms.h"

/*
 * Extent map to manage I/O register space.  We allocate storage for
 * 32 regions in the map.  iomap_ex_malloc_safe will indicate that it's
 * safe to use malloc() to dynamically allocate region descriptors in
 * case we run out.
 */
static long iomap_ex_storage[EXTENT_FIXED_STORAGE_SIZE(32) / sizeof(long)];
static struct extent *iomap_ex;
static int iomap_ex_malloc_safe;

/* maps for VM objects */
struct vm_map *phys_map = NULL;

int		systype;		    /* mother board type */
char   *bootinfo = NULL;	/* pointer to bootinfo structure */
int		cpuspeed = 30;		/* approx # instr per usec. */
intptr_t	physmem_boardmax;	/* {model,SIMM}-specific bound on physmem */
int		mem_cluster_cnt;
phys_ram_seg_t	mem_clusters[VM_PHYSSEG_MAX];

void	mach_init (int, char *[], int, intptr_t, u_int, char *); /* XXX */

/* Motherboard or system-specific initialization vector */
static void	unimpl_bus_reset(void);
static void	unimpl_cons_init(void);
static void	unimpl_iointr(uint32_t, vaddr_t, uint32_t);
static void	unimpl_intr_establish(device_t, void *, int,
		    int (*)(void *, void *), void *);
static int	unimpl_memsize(void *);

struct platform platform = {
	"iobus not set",
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_iointr,
	unimpl_intr_establish,
	unimpl_memsize
};

extern char *esym;			/* XXX */
extern struct consdev promcd;		/* XXX */
extern const struct callback *callv;
extern const struct callback callvec;

/*
 * Do all the stuff that locore normally does before calling main().
 * The first 4 argments are passed by PROM monitor, and remaining two
 * are built on temporary stack by our boot loader.
 */
void
mach_init(int argc, char *argv[], int code, intptr_t cv, u_int bim, char *bip)
{
	char *cp;
	const char *bootinfo_msg;
	u_long first, last;
	int i, howtoboot;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	void *ssym = 0;
	struct btinfo_symtab *bi_syms;
#endif
	void *kernend;
	extern char edata[], end[];	/* XXX */

	/* Set up bootinfo structure looking at stack. */
	if (bim == BOOTINFO_MAGIC) {
		struct btinfo_magic *bi_magic;

		bootinfo = bip;
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL || bi_magic->magic != BOOTINFO_MAGIC)
			bootinfo_msg =
			    "invalid magic number in bootinfo structure.\n";
		else
			bootinfo_msg = NULL;
	} else
		bootinfo_msg = "invalid bootinfo pointer (old bootblocks?)\n";

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * Do it before we decide to keep symbols.
	 * NB: "boothowto" is in the BSS.
	 */
	howtoboot = 0;
#ifdef KADB
	howtoboot |= RB_KDB;
#endif
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
			switch (*cp) {

#define RB_NOSYMBOLS 0x10000000
			case 'e': /* empty the symtable */
				howtoboot |= RB_NOSYMBOLS;
				break;

			case 'n': /* ask for names */
				howtoboot |= RB_ASKNAME;
				break;

			case 'N': /* don't ask for names */
				howtoboot &= ~RB_ASKNAME;
				break;

			default:
				BOOT_FLAG(*cp, howtoboot); /* see sys/boot_flag.h */
				break;
			}
		}
	}

	/* clear the BSS segment */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* Was it a valid bootinfo symtab info? */
	if ((bi_syms != NULL) && (!(howtoboot & RB_NOSYMBOLS))) {
		ssym = (void *)(intptr_t)bi_syms->ssym;
		esym = (void *)(intptr_t)bi_syms->esym;
		kernend = (void *)mips_round_page(esym);
		memset(edata, 0, end - edata);
	} else
#endif
	{
		kernend = (char *)mips_round_page(end);
		/* should be done by bootloader? */
		memset(edata, 0, (char *)kernend - (char *)edata);
	}

	/* Initialize callv so we can do PROM output... */
	callv = (code == PROM_MAGIC) ? (void *)cv : &callvec;

	/* Use PROM console output until we initialize a console driver. */
	cn_tab = &promcd;

#if 1
	if (bootinfo_msg != NULL)
		printf(bootinfo_msg);
#endif
	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init(NULL, false);

	/*
	 * We know the CPU type now.  Initialize our DMA tags (might
	 * need this early, for certain types of console devices!!).
	 */
	emips_bus_dma_init();

	/* Look at argv[0] and compute bootdev */
	makebootdev(argv[0]);

	boothowto = howtoboot & ~RB_NOSYMBOLS;

	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend = (char *)kernend
		    + round_page(mfs_initminiroot(kernend));

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* init symbols if present */
	if (esym) {
		ksyms_addsyms_elf((char *)esym - (char *)ssym, ssym, esym);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Initialize physmem_boardmax; assume no SIMM-bank limits.
	 * Adjust later in model-specific code if necessary.
	 */
	physmem_boardmax = MIPS_MAX_MEM_ADDR;

	/*
	 * Determine what model of computer we are running on.
	 */
	systype = ((prom_systype() >> 16) & 0xff);
	if (systype >= nsysinit) {
		platform_not_supported();
		/* NOTREACHED */
	}

	/* Machine specific initialization. */
	(*sysinit[systype].init)();

	/* Find out how much memory is available. */
	physmem = (*platform.memsize)(kernend);

	/*
	 * Load the rest of the available pages into the VM system.
	 * NB: The kernel can span multiple segments.
	 */
	for (i = 0, physmem = 0; i < mem_cluster_cnt; ++i) {
		first = mem_clusters[i].start;
		if (first < round_page(MIPS_KSEG0_TO_PHYS(kernend)))
			first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
		last = mem_clusters[i].start + mem_clusters[i].size;
		physmem += atop(mem_clusters[i].size);

		/* if the kernel spans multiple segments (does on ML40x) */
		if (last <= first)
			continue;

		uvm_page_physload(atop(first), atop(last), atop(first),
		    atop(last), VM_FREELIST_DEFAULT);
	}

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	iospace = pmap_limits.virtual_start;
	pmap_limits.virtual_start += iospace_size;
	pmap_bootstrap();

	mips_init_lwp0_uarea();
}

void
mips_machdep_cache_config(void)
{
}

void
consinit(void)
{
	/*
	 * Init I/O memory extent map. Must be done before cninit()
	 * is called; we may want to use iospace in the console routines.
	 */
	KASSERT(iospace != 0);
	iomap_ex = extent_create("iomap", iospace,
	    iospace + iospace_size - 1,
	    (void *) iomap_ex_storage, sizeof(iomap_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

	/*
	 * Up until now we have kept the TLB disabled,
	 * and that allowed the "PROM" to work.
	 * Specifically, romputc() and the debugger's getc() functions worked.
	 * Now is the last chance we get to turn it on.
	 * That means no more console I/O until autoconf() [sigh!], or..
	 * The platform-specific code will have to map [1:1 probably]
	 * the I/O registers.
	 */
	register_t s = mips_cp0_status_read();
	s &= ~MIPS_SR_TS;
	mips_cp0_status_write(s);

	(*platform.cons_init)();

	/* Do NOT call cninit(); It will clobber cn_tab using constab[] which we do not use
     */
}

/*
 * Allocates a virtual range suitable for mapping in physical memory.
 * Uses resource maps when allocating space, which is allocated from 
 * the IOMAP submap. SIZE is a linear range (NOT vax-pages like the VAX).
 * If the page requested is bigger than a logical page, space is
 * allocated from the kernel map instead.
 */
vaddr_t
mips_map_physmem(paddr_t phys, vsize_t size)
{
	vaddr_t addr;
	int error;
	static int warned = 0;

	size += phys & PAGE_MASK;
	if (size >= PAGE_SIZE) {
		addr = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY);
		if (addr == 0)
			panic("mips_map_physmem: kernel map full");
	} else {
		error = extent_alloc(iomap_ex, size, PAGE_SIZE, 0,
		    EX_FAST | EX_NOWAIT |
		    (iomap_ex_malloc_safe ? EX_MALLOCOK : 0), (u_long *)&addr);
		if (error) {
			if (warned++ == 0) /* Warn only once */
				printf("mips_map_physmem: iomap too small");
			return 0;
		}
	}
	ioaccess(addr, phys, size);
#ifdef PHYSMEMDEBUG
	printf("mips_map_physmem: alloc'ed %lx bytes for paddr %lx, at %lx\n",
	    size, phys, addr);
#endif
	return addr | (phys & PAGE_MASK);
}

/*
 * Unmaps the previous mapped (addr, size) pair.
 */
void
mips_unmap_physmem(vaddr_t addr, vsize_t size)
{
#ifdef PHYSMEMDEBUG
	printf("mips_unmap_physmem: unmapping %lx bytes at addr %lx\n", 
	    size, addr);
#endif
	size += addr & PAGE_MASK;
	addr &= ~PAGE_MASK;

	iounaccess(addr, size);
	if (size >= PAGE_SIZE)
		uvm_km_free(kernel_map, addr, size, UVM_KMF_VAONLY);
	else if (extent_free(iomap_ex, addr, size,
	    EX_NOWAIT | (iomap_ex_malloc_safe ? EX_MALLOCOK : 0)))
		printf("mips_unmap_physmem: addr 0x%llx size %llx: "
		    "can't free region\n", (long long)addr, (long long)size);
}

/*
 * Machine-dependent startup code: allocate memory for variable-sized
 * tables.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;		/* XXX */
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_getmodel());
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.
	 */

	iomap_ex_malloc_safe = 1;

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

/*
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return (NULL);

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return ((void *)help);
		help += bt->next;
	} while (bt->next != 0 &&
		(size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return (NULL);
}

void
cpu_reboot(volatile int howto,	/* XXX volatile to keep gcc happy */
           char *bootstr)
{

	/* take a snap shot before clobbering any registers */
	if (curlwp)
		savectx(curpcb);

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
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	/* run any shutdown hooks */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	/* Finally, halt/reboot the system. */
	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");

	prom_halt(howto);
	for (;;) ;
	/*NOTREACHED*/
}

#if defined(MIPS_4GB_PHYSICAL_MEMORY)
#define trim_memory(n) n
#else
#if 0
#define TOO_MUCH (MIPS_PHYS_MASK+1)
#else
#define TOO_MUCH (2*64*1024*1024)
#endif
u_long trim_memory(uint32_t nbytes);/*cheat*/
u_long trim_memory(uint32_t nbytes)
{
	int i;
	u_long first, last;

	nbytes *= 4096;
	if (nbytes <= TOO_MUCH)
		return nbytes;

	/* We have more memory than we can handle */

	mem_clusters[mem_cluster_cnt].start = 0;/* sentinel record */
	mem_clusters[mem_cluster_cnt].size = 0;
	for (i = 0; i < mem_cluster_cnt;) {
		first = mem_clusters[i].start;
		last = mem_clusters[i].start + mem_clusters[i].size;

		if (first > TOO_MUCH) {
			printf("Too much memory, ignoring memory "
			    "range %08lx..%08lx\n", first, last);
			memcpy(mem_clusters+i,mem_clusters+i+1,
			    (sizeof(mem_clusters[0])*(mem_cluster_cnt-i)));
			mem_cluster_cnt--;
			continue;
		}

		if (last > TOO_MUCH) {
			last = TOO_MUCH;
			printf("Too much memory in cluster %d, trimming "
			   "memory to range %08lx..%08lx\n", 
			   i, first, last);
			mem_clusters[i].size = last - mem_clusters[i].start;
		}
		i++;
	}
	return TOO_MUCH;
}
#endif

/*
 * Find out how much memory is available by testing memory.
 */
int
memsize_scan(void *first)
{
	int i, mem;
	char *cp;

	mem = btoc((paddr_t)first - MIPS_KSEG0_START);
	cp = (char *)MIPS_PHYS_TO_KSEG1(mem << PGSHIFT);
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
		cp += PAGE_SIZE;
		mem++;
	}

	/*
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = 0;		/* XXX is this correct? */
	mem_clusters[0].size  = ctob(mem);
	mem_cluster_cnt = 1;

	/* clear any memory error conditions possibly caused by probe */
	(*platform.bus_reset)();
	return (mem);
}

/*
 * Find out how much memory is available by testing memory, starting at first.
 * Returns the total number of pages.
 */
int
memsize_pmt(void * first)
{
	int i, mem;
	struct _Pmt *Pmt = ThePmt;
	struct _Sram *ram;
	uint32_t addr, len;

	/*
	 * Build the RAM memory map from the PMT.
	 */
	mem = 0;
	for (i = 0; i < VM_PHYSSEG_MAX; Pmt--) {
		uint16_t tag = Pmt->Tag;

		if (tag == PMTTAG_END_OF_TABLE)
			break;

		if ((tag != PMTTAG_SRAM) && (tag != PMTTAG_DDRAM))
			continue;

		/*
		 * Got a memory controller segment,
		 * scan all the controllers in it
		 */
		ram = (struct _Sram *)(Pmt->TopOfPhysicalAddress << 16);

		for (;(ram->BaseAddressAndTag & SRAMBT_TAG) == tag;) {
			addr = ram->BaseAddressAndTag & SRAMBT_BASE;
			len  = ram->Control & SRAMST_SIZE;

			mem_clusters[i].start = addr;
			mem_clusters[i].size  = len;
			printf("memory segment %2d start %08lx size %08lx\n", i,
			    (long)mem_clusters[i].start,
			    (long)mem_clusters[i].size);
			i++;
			mem += len;

			/* SRAM and DDRAM have different sizes */
			ram = (tag == PMTTAG_SRAM) ? ram+1 : ram+2;
		}
	}
	mem_cluster_cnt = i;

	return trim_memory(btoc(mem));
}
/*
 *  Ensure all platform vectors are always initialized.
 */
static void
unimpl_bus_reset(void)
{

	panic("sysconf.init didn't set bus_reset");
}

static void
unimpl_cons_init(void)
{

	panic("sysconf.init didn't set cons_init");
}

static void
unimpl_iointr(uint32_t status, vaddr_t pc, uint32_t ipending)
{

	panic("sysconf.init didn't set intr");
}

static void
unimpl_intr_establish(device_t dev, void *cookie, int level,
                      int (*handler) (void *,void *), void *arg)
{

	panic("sysconf.init didn't set intr_establish");
}

static int
unimpl_memsize(void * first)
{

	panic("sysconf.init didn't set memsize");
}

/*
 * Wait "n" microseconds.
 */
void
delay(int n)
{

        DELAY(n);
}
