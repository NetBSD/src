/*	$NetBSD: machdep.c,v 1.119.6.1 2015/09/22 12:05:43 skrll Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura, All rights reserved.
 * Copyright (c) 1999-2001 SATO Kazumi, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

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
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 *
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.119.6.1 2015/09/22 12:05:43 skrll Exp $");

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"
#include "opt_boot_standalone.h"
#include "opt_modular.h"
#include "opt_spec_platform.h"
#include "biconsdev.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_rtc_offset.h"
#include "opt_kloader.h"
#include "opt_kloader_kernel_path.h"
#include "debug_hpc.h"
#include "opt_md.h"
#include "opt_memsize.h"
#include "opt_no_symbolsz_entry.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <ufs/mfs/mfs_extern.h>	/* mfs_initminiroot() */
#include <dev/cons.h>		/* cntab access (cpu_reboot) */

#include <machine/locore.h>
#include <machine/psl.h>
#include <machine/sysconf.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/kloader.h>
#include <machine/debug.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE         DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#if NBICONSDEV > 0 
#include <sys/conf.h>
#include <dev/hpc/biconsvar.h>
#include <dev/hpc/bicons.h>
#define biconscnpollc	nullcnpollc
cons_decl(bicons);
static struct consdev bicons __attribute((__unused__)) = cons_init(bicons);
static int __bicons_enable;
#define DPRINTF(arg)	{ if (__bicons_enable) printf arg; }
#else /* NBICONSDEV > 0 */
#define DPRINTF(arg)
#endif /* NBICONSDEV > 0 */

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#ifdef MEMORY_DISK_DYNAMIC
#include <dev/md.h>
#endif

/* the following is used externally (sysctl_hw) */
static char	hpcmips_cpuname[40];		/* set CPU depend xx_init() */

int	cpuspeed = 1;			/* approx # instr per usec. */

/* CPU core switch table */
struct platform platform;
#ifdef VR41XX
extern void	vr_init(void);
#endif
#ifdef TX39XX
extern void	tx_init(void);
#endif

/* boot environment */
static struct bootinfo bi_copy;
struct bootinfo *bootinfo;
char booted_kernel[128];
extern void makebootdev(const char *);
#ifdef KLOADER
#if !defined(KLOADER_KERNEL_PATH)
#define KLOADER_KERNEL_PATH	"/netbsd"
#endif /* !KLOADER_KERNEL_PATH */
static char kernel_path[] = KLOADER_KERNEL_PATH;
#endif /* KLOADER */

/* maps for VM objects */
struct vm_map *phys_map;

/* physical memory */
int	mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

void mach_init(int, char *[], struct bootinfo *);

#ifdef DEBUG
/* stacktrace code violates prototypes to get callee's registers */
extern void stacktrace(void); /*XXX*/
#endif

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the boot loader. 
 * Return the first page address following the system.
 */
void
mach_init(int argc, char *argv[], struct bootinfo *bi)
{
	/* 
	 * this routines stack is never polluted since stack pointer
	 * is lower than kernel text segment, and at exiting, stack pointer
	 * is changed to proc0.
	 */
#ifdef KLOADER
	struct kloader_bootinfo kbi;
#endif
	extern char edata[], end[];
#if NKSYMS || defined(DDB) || defined(MODULAR)
	extern void *esym;
#endif
	void *kernend;
	char *cp;
	int i;

	/* clear the BSS segment */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	size_t symbolsz = 0;
	Elf_Ehdr *eh = (void *)end;
	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) == 0 &&
	    eh->e_ident[EI_CLASS] == ELFCLASS) {
		esym = end;
#ifndef NO_SYMBOLSZ_ENTRY
		if (eh->e_entry != 0) {
			/* pbsdboot */
			symbolsz = eh->e_entry;
		} else
#endif
		{
			/* hpcboot */
			Elf_Shdr *sh = (void *)(end + eh->e_shoff);
			for(i = 0; i < eh->e_shnum; i++, sh++)
				if (sh->sh_offset > 0 &&
				    (sh->sh_offset + sh->sh_size) > symbolsz)
					symbolsz = sh->sh_offset + sh->sh_size;
		}
		esym = (char*)esym + symbolsz;
		kernend = (void *)mips_round_page(esym);
		memset(edata, 0, end - edata);
	} else
#endif /* NKSYMS || defined(DDB) || defined(MODULAR) */
	{
		kernend = (void *)mips_round_page(end);
		memset(edata, 0, (char *)kernend - edata);
	}

#if defined(BOOT_STANDALONE)
#if !defined (SPEC_PLATFORM) || SPEC_PLATFORM == 1
#error specify SPEC_PLATFORM=platid_mask_MACH_xxx_yyy in BOOT_STANDALONE case.
#error see platid_mask.c for platid_mask_MACH_xxx_yyy.
#else
	memcpy(&platid, &SPEC_PLATFORM, sizeof(platid));
#endif
#endif /* defined(BOOT_STANDALONE) && defined(SPEC_PLATFORM) */
	/*
	 *  Arguments are set up by boot loader.
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
	/* copy boot parameter for kloader */
#ifdef KLOADER
	kloader_bootinfo_set(&kbi, argc, argv, bi, false);
#endif

	/* 
	 * CPU core Specific Function Hooks 
	 */
#if defined(VR41XX) && defined(TX39XX)
	if (platid_match(&platid, &platid_mask_CPU_MIPS_VR_41XX))
		vr_init();
	else if (platid_match(&platid, &platid_mask_CPU_MIPS_TX_3900) ||
	    platid_match(&platid, &platid_mask_CPU_MIPS_TX_3920))
		tx_init();
#elif defined(VR41XX)
	vr_init();
#elif defined(TX39XX)
	tx_init();
#else
#error "define TX39XX and/or VR41XX"
#endif

#if NBICONSDEV > 0
	/* 
	 *  bicons don't need actual device initialize.  only bootinfo needed. 
	 */
	__bicons_enable = (bicons_init(&bicons) == 0);
	if (__bicons_enable)
		cn_tab = &bicons;
#endif

	/* Initialize frame buffer (to steal DMA buffer, stay here.) */
#ifdef HPC_DEBUG_LCD
	dbg_lcd_test();
#endif
	(*platform.fb_init)(&kernend);
	kernend = (void *)mips_round_page(kernend);

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
	mips_vector_init(NULL, false);
	intr_init();

#ifdef DEBUG
	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	if (bootinfo) {
		DPRINTF(("Bootinfo. available, "));
	}
	DPRINTF(("args: "));
	for (i = 0; i < argc; i++) {
		DPRINTF(("%s ", argv[i]));
	}
	DPRINTF(("\n"));
	DPRINTF(("platform ID: %08lx %08lx\n", platid.dw.dw0, platid.dw.dw1));
#endif /* DEBUG */

#ifndef RTC_OFFSET
	/*
	 * rtc_offset from bootinfo.timezone set by pbsdboot.exe
	 */
	if (rtc_offset == 0 && bootinfo
	    && bootinfo->timezone > (-12*60)
	    && bootinfo->timezone <= (12*60))
		rtc_offset = bootinfo->timezone;
#endif /* RTC_OFFSET */

	/* Compute bootdev */
	makebootdev("wd0"); /* default boot device */

	boothowto = 0;
#ifdef KADB
	boothowto |= RB_KDB;
#endif
	strncpy(booted_kernel, argv[0], sizeof(booted_kernel));
	booted_kernel[sizeof(booted_kernel)-1] = 0;
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
			switch (*cp) {
			case 'h': /* XXX, serial console */
				bootinfo->bi_cnuse |= BI_CNUSE_SERIAL;
				break;

			case 'b':
				/* boot device: -b=sd0 etc. */
				if (strcmp(cp+2, "nfs") == 0)
					rootfstype = MOUNT_NFS;
				else
					makebootdev(cp+2);
				cp += strlen(cp);
				break;
			default:
				BOOT_FLAG(*cp, boothowto);
				break;
			}
		}
	}
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT) {
		size_t fssz;
		fssz = round_page(mfs_initminiroot(kernend));
#ifdef MEMORY_DISK_DYNAMIC
		md_root_setconf(kernend, fssz);
#endif /* MEMORY_DISK_DYNAMIC */
		kernend = (char *)kernend + fssz;
	}

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* init symbols if present */
	if (esym)
		ksyms_addsyms_elf(symbolsz, &end, esym);
#endif /* DDB */

	/* Initialize console and KGDB serial port. */
	(*platform.cons_init)();

#if defined(DDB) || defined(KGDB)
	if (boothowto & RB_KDB) {
#ifdef DDB
		Debugger();
#endif /* DDB */
#ifdef KGDB
		kgdb_debug_init = 1;
		kgdb_connect(1);
#endif /* KGDB */
	}
#endif /* DDB || KGDB */

	/* Find physical memory regions. */
#ifdef MEMSIZE
	mem_clusters[0].start = 0;
	mem_clusters[0].size = (paddr_t) kernend - MIPS_KSEG0_START;
	mem_clusters[1].start = (paddr_t) kernend - MIPS_KSEG0_START;
	mem_clusters[1].size = MEMSIZE * 0x100000 - mem_clusters[1].start;
	mem_cluster_cnt = 2;
#else
	(*platform.mem_init)((paddr_t)kernend - MIPS_KSEG0_START);
#endif
	/* 
	 *  Clear currently unused D-RAM area 
	 *  (For reboot Windows CE clearly)
	 */
	{
		u_int32_t sp;
		__asm volatile("move %0, $29" : "=r"(sp));
		KDASSERT(sp > KERNBASE + 0x400);
		memset((void *)(KERNBASE + 0x400), 0, sp - (KERNBASE + 0x400));
	}

	printf("mem_cluster_cnt = %d\n", mem_cluster_cnt);
	physmem = 0;
	for (i = 0; i < mem_cluster_cnt; i++) {
		printf("mem_clusters[%d] = {%#"PRIxPADDR",%#"PRIxPSIZE"}\n", i,
		    (paddr_t)mem_clusters[i].start,
		    (psize_t)mem_clusters[i].size);
		physmem += atop(mem_clusters[i].size);
	}

	mips_page_physload(MIPS_KSEG0_START, (vaddr_t)kernend,
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Initialize lwp0's uarea.
	 */
	mips_init_lwp0_uarea();
}

int
cpuname_printf(const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(hpcmips_cpuname, sizeof(hpcmips_cpuname), fmt, ap);
	va_end(ap);

	return len;
}

/*
 * Machine-dependent startup code.
 * allocate memory for variable-sized tables, initialize CPU.
 */
void
cpu_startup(void)
{
	cpu_setmodel("%s (%s)", platid_name(&platid), hpcmips_cpuname);

	/* show again when verbose mode */
	aprint_verbose("total memory banks = %d\n", mem_cluster_cnt);
	for (size_t i = 0; i < mem_cluster_cnt; i++) {
		aprint_verbose("memory bank %zu = "
		    "%#08"PRIxPADDR" %"PRIdPSIZE"KB(%#"PRIxPSIZE")\n", i,
		    (paddr_t)mem_clusters[i].start,
		    (psize_t)mem_clusters[i].size/1024,
		    (psize_t)mem_clusters[i].size);
	}

	cpu_startup_common();
}

void
cpu_reboot(int howto, char *bootstr)
{

	/* take a snap shot before clobbering any registers */
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
	if ((boothowto & RB_HALT) != 0) {
		howto |= RB_HALT;
	}

#ifdef KLOADER
	if ((howto & RB_HALT) == 0) {
		if (howto & RB_STRING)
			kloader_reboot_setup(bootstr);
		else
			kloader_reboot_setup(kernel_path);
	}
#endif

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
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:

	/* run any shutdown hooks */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("halted.\n");
	} else {
#ifdef KLOADER
		kloader_reboot();
		/* NOTREACHED */
#endif
	}

	(*platform.reboot)(howto, bootstr);
	while(1)
		;
	/*NOTREACHED*/
}

void
consinit(void)
{
	/* platform.cons_init() do it */
}
