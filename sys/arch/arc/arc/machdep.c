/*	$NetBSD: machdep.c,v 1.106 2007/05/17 14:51:15 yamt Exp $	*/
/*	$OpenBSD: machdep.c,v 1.36 1999/05/22 21:22:19 weingart Exp $	*/

/*
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
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.106 2007/05/17 14:51:15 yamt Exp $");

#include "fs_mfs.h"
#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <uvm/uvm_extern.h>
#include <sys/mount.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#ifdef MFS
#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */
#endif

#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pio.h>
#include <machine/bus.h>
#include <machine/trap.h>
#include <machine/autoconf.h>
#include <machine/platform.h>
#include <machine/wired_map.h>
#include <mips/pte.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>
#include <mips/psl.h>
#include <mips/cache.h>
#ifdef DDB
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <dev/cons.h>

#include <dev/ic/i8042reg.h>
#include <dev/isa/isareg.h>

#include <arc/arc/arcbios.h>
#include <arc/arc/timervar.h>

#include "ksyms.h"

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#ifndef COMCONSOLE
#define COMCONSOLE	0
#endif

#ifndef CONADDR
#define CONADDR	0	/* use default address if CONADDR isn't configured */
#endif

#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#endif /* NCOM */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* maps for VM objects */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	maxmem;			/* max memory per process */
int	physmem;		/* max supported memory, changes to actual */
int	cpuspeed = 150;		/* approx CPU clock [MHz] */
vsize_t kseg2iobufsize = 0;	/* to reserve PTEs for KSEG2 I/O space */
struct arc_bus_space arc_bus_io;/* Bus tag for bus.h macros */
struct arc_bus_space arc_bus_mem;/* Bus tag for bus.h macros */

char *bootinfo;			/* pointer to bootinfo structure */
static char bi_buf[BOOTINFO_SIZE]; /* buffer to store bootinfo data */
static const char *bootinfo_msg = NULL;

#if NCOM > 0
int	com_freq = COM_FREQ;	/* unusual clock frequency of dev/ic/com.c */
int	com_console = COMCONSOLE;
int	com_console_address = CONADDR;
int	com_console_speed = CONSPEED;
int	com_console_mode = CONMODE;
#else
#ifndef COMCONSOLE
#error COMCONSOLE is defined without com driver configured.
#endif
int	com_console = 0;
#endif /* NCOM */

char **environment;		/* On some arches, pointer to environment */

int mem_reserved[VM_PHYSSEG_MAX]; /* the cluster is reserved, i.e. not free */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

/* initialize bss, etc. from kernel start, before main() is called. */
void mach_init(int, char *[], u_int, void *);

const char *firmware_getenv(const char *env);
void arc_sysreset(bus_addr_t, bus_size_t);

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 * Used as an argument to splx().
 * XXX disables interrupt 5 to disable mips3 on-chip clock.
 */
int	safepri = MIPS3_PSL_LOWIPL;

const uint32_t *ipl_sr_bits;
const uint32_t mips_ipl_si_to_sr[SI_NQUEUES] = {
	[SI_SOFT] = MIPS_SOFT_INT_MASK_0,
	[SI_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[SI_SOFTNET] = MIPS_SOFT_INT_MASK_1,
	[SI_SOFTSERIAL] = MIPS_SOFT_INT_MASK_1,
};

extern char kernel_text[], edata[], end[];
extern struct user *proc0paddr;

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the BIOS.
 * Reset mapping and set up mapping to hardware and init "wired" reg.
 * Return the first page address following the system.
 */
void
mach_init(int argc, char *argv[], u_int bim, void *bip)
{
	const char *cp;
	int i;
	paddr_t kernstartpfn, kernendpfn, first, last;
	char *kernend;
	vaddr_t v;
#if NKSYMS > 0 || defined(DDB) || defined(LKM)
	char *ssym = NULL;
	char *esym = NULL;
	struct btinfo_symtab *bi_syms;
#endif

	/* set up bootinfo structures */
	if (bim == BOOTINFO_MAGIC && bip != NULL) {
		struct btinfo_magic *bi_magic;

		memcpy(bi_buf, bip, BOOTINFO_SIZE);
		bootinfo = bi_buf;
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL || bi_magic->magic != BOOTINFO_MAGIC)
			bootinfo_msg =
			    "invalid magic number in bootinfo structure.\n";
		else
			bootinfo_msg = "bootinfo found.\n";
	} else
		bootinfo_msg = "no bootinfo found. (old bootblocks?)\n";

	/* clear the BSS segment in kernel code */
#if NKSYM > 0 || defined(DDB) || defined(LKM)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* check whether there is valid bootinfo symtab info */
	if (bi_syms != NULL) {
		ssym = (char *)bi_syms->ssym;
		esym = (char *)bi_syms->esym;
		kernend = (void *)mips_round_page(esym);
#if 0	
		/*
		 * Don't clear BSS here since bi_buf[] is allocated in BSS
		 * and it has been cleared by the bootloader in this case.
		 */
		memset(edata, 0, end - edata);
#endif
	} else
#endif
	{
		kernend = (void *)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

	environment = &argv[1];

	if (bios_ident()) { /* ARC BIOS present */
		bios_init_console();
		bios_save_info();
		physmem = bios_configure_memory(mem_reserved, mem_clusters,
		    &mem_cluster_cnt);
	}

	/* Initialize the CPU type */
	ident_platform();
	if (platform == NULL) {
		/* This is probably the best we can do... */
		printf("kernel not configured for this system:\n");
		printf("ID [%s] Vendor [%8.8s] Product [%02x",
		    arc_id, arc_vendor_id, arc_product_id[0]);
		for (i = 1; i < sizeof(arc_product_id); i++)
			printf(":%02x", arc_product_id[i]);
		printf("]\n");
		printf("DisplayController [%s]\n", arc_displayc_id);
#if 1
		for (;;)
			;
#else
		cpu_reboot(RB_HALT | RB_NOSYNC, NULL);
#endif
	}

	physmem = btoc(physmem);

	/* compute bootdev for autoconfig setup */
	cp = firmware_getenv("OSLOADPARTITION");
	makebootdev(cp != NULL ? cp : argv[0]);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * Default to SINGLE and ASKNAME if no args or
	 * SINGLE and DFLTROOT if this is a ramdisk kernel.
	 */
#ifdef MEMORY_DISK_IS_ROOT
	boothowto = RB_SINGLE;
#else
	boothowto = RB_SINGLE | RB_ASKNAME;
#endif /* MEMORY_DISK_IS_ROOT */
#ifdef KADB
	boothowto |= RB_KDB;
#endif

	cp = firmware_getenv("OSLOADOPTIONS");
	if (cp) {
		while (*cp) {
			switch (*cp++) {
			case 'a': /* autoboot */
				boothowto &= ~RB_SINGLE;
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

			case 's': /* use serial console */
				com_console = 1;
				break;

			case 'q': /* boot quietly */
				boothowto |= AB_QUIET;
				break;

			case 'v': /* boot verbosely */
				boothowto |= AB_VERBOSE;
				break;
			}

		}
	}

	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();

	/* make sure that we don't call BIOS console from now on */
	cn_tab = NULL;

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 *
	 * Now its time to abandon the BIOS and be self supplying.
	 * Start with cleaning out the TLB. Bye bye Microsoft....
	 *
	 * This may clobber PTEs needed by the BIOS.
	 */
	mips_vector_init();

	/*
	 * Map critical I/O spaces (e.g. for console printf(9)) on KSEG2.
	 * We cannot call VM functions here, since uvm is not initialized,
	 * yet.
	 * Since printf(9) is called before uvm_init() in main(),
	 * we have to handcraft console I/O space anyway.
	 *
	 * XXX - reserve these KVA space after UVM initialization.
	 */

	arc_init_wired_map();

	(*platform->init)();
	cpuspeed = platform->clock;
	curcpu()->ci_cpu_freq = platform->clock * 1000000;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + 500000) / 1000000);
	if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT) {
		curcpu()->ci_cycles_per_hz /= 2;
		curcpu()->ci_divisor_delay /= 2;
	}
	MIPS_SET_CI_RECIPRICAL(curcpu());
	sprintf(cpu_model, "%s %s%s",
	    platform->vendor, platform->model, platform->variant);

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend += round_page(mfs_initminiroot(kernend));
#endif

#if NKSYMS || defined(DDB) || defined(LKM)
	/* init symbols if present */
	if (esym)
		ksyms_init(esym - ssym, ssym, esym);
#ifdef SYMTAB_SPACE
	else
		ksyms_init(0, NULL, NULL);
#endif
#endif

	maxmem = physmem;

	/* XXX: revisit here */

	/*
	 * Load the rest of the pages into the VM system.
	 */
	kernstartpfn = atop(trunc_page(
	    MIPS_KSEG0_TO_PHYS((kernel_text) - UPAGES * PAGE_SIZE)));
	kernendpfn = atop(round_page(MIPS_KSEG0_TO_PHYS(kernend)));
#if 0
	/* give all free memory to VM */
	/* XXX - currently doesn't work, due to "panic: pmap_enter: pmap" */
	for (i = 0; i < mem_cluster_cnt; i++) {
		if (mem_reserved[i])
			continue;
		first = atop(round_page(mem_clusters[i].start));
		last = atop(trunc_page(mem_clusters[i].start +
		    mem_clusters[i].size));
		if (last <= kernstartpfn || kernendpfn <= first) {
			uvm_page_physload(first, last, first, last,
			    VM_FREELIST_DEFAULT);
		} else {
			if (first < kernstartpfn)
				uvm_page_physload(first, kernstartpfn,
				    first, kernstartpfn, VM_FREELIST_DEFAULT);
			if (kernendpfn < last)
				uvm_page_physload(kernendpfn, last,
				    kernendpfn, last, , VM_FREELIST_DEFAULT);
		}
	}
#elif 0 /* XXX */
	/* give all free memory above the kernel to VM (non-contig version) */
	for (i = 0; i < mem_cluster_cnt; i++) {
		if (mem_reserved[i])
			continue;
		first = atop(round_page(mem_clusters[i].start));
		last = atop(trunc_page(mem_clusters[i].start +
		    mem_clusters[i].size));
		if (kernendpfn < last) {
			if (first < kernendpfn)
				first = kernendpfn;
			uvm_page_physload(first, last, first, last,
			    VM_FREELIST_DEFAULT);
		}
	}
#else
	/* give all memory above the kernel to VM (contig version) */
#if 1
	mem_clusters[0].start = 0;
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;
#endif

	first = kernendpfn;
	last = physmem;
	uvm_page_physload(first, last, first, last, VM_FREELIST_DEFAULT);
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = uvm_pageboot_alloc(USPACE);
	lwp0.l_addr = proc0paddr = (struct user *)v;
	lwp0.l_md.md_regs = (struct frame *)(v + USPACE) - 1;
	proc0paddr->u_pcb.pcb_context[11] =
	    MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */
}

void
mips_machdep_cache_config(void)
{

	mips_sdcache_size = arc_cpu_l2cache_size;
}

/*
 * Return a pointer to the given environment variable.
 */
const char *
firmware_getenv(const char *envname)
{
	char **env;
	int l;

	l = strlen(envname);

	for (env = environment; env[0]; env++) {
		if (strncasecmp(envname, env[0], l) == 0 && env[0][l] == '=') {
			return &env[0][l + 1];
		}
	}
	return NULL;
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	(*platform->cons_init)();
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */

#endif

#ifdef BOOTINFO_DEBUG
	if (bootinfo_msg)
		printf(bootinfo_msg);
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, false, NULL);

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

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	arc_bus_space_malloc_set_safe();
}

void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *bip;

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

static int waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

	/* take a snap shot before clobbering any registers */
	if (curlwp)
		savectx((struct user *)curpcb);

#ifdef DEBUG
	if (panicstr)
		stacktrace();
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		/* fill curlwp with live object */
		if (curlwp == NULL)
			curlwp = &lwp0;
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
	(void)splhigh();		/* extreme priority */

	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n");
	delay(1000000);

	(*platform->reset)();

	__asm(" li $2, 0xbfc00000; jr $2; nop\n");
	for (;;)
		; /* Forever */
	/* NOTREACHED */
}

/*
 * Pass system reset command to keyboard controller (8042).
 */
void
arc_sysreset(bus_addr_t addr, bus_size_t cmd_offset)
{
	volatile uint8_t *kbdata = (uint8_t *)addr + KBDATAP;
	volatile uint8_t *kbcmd = (uint8_t *)addr + cmd_offset;

#define KBC_ARC_SYSRESET 0xd1

	delay(1000);
	*kbcmd = KBC_ARC_SYSRESET;
	delay(1000);
	*kbdata = 0;
}
