/*	$NetBSD: machdep.c,v 1.51.2.1 2006/02/01 14:51:27 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2004 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.51.2.1 2006/02/01 14:51:27 yamt Exp $");

#include "opt_md.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "fs_mfs.h"
#include "fs_nfs.h"
#include "biconsdev.h"
#include "opt_kloader_kernel_path.h"
#include "debug_hpc.h"
#include "hd64465if.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>

#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <sh3/cpu.h>
#include <sh3/exception.h>
#include <sh3/cache.h>
#include <sh3/clock.h>
#include <sh3/intcreg.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "ksyms.h"

#if NKSYMS || defined(LKM) || defined(DDB) || defined(KGDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define	ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif /* DDB || KGDB */

#include <dev/cons.h> /* consdev */
#include <dev/md.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/autoconf.h>		/* makebootdev() */
#include <machine/kloader.h>
#include <machine/intr.h>

#ifdef NFS
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#endif

#include <dev/hpc/apm/apmvar.h>

#include <hpcsh/dev/hd6446x/hd6446xintcvar.h>
#include <hpcsh/dev/hd6446x/hd6446xintcreg.h>
#include <hpcsh/dev/hd64465/hd64465var.h>

#ifdef DEBUG
#define	DPRINTF_ENABLE
#define	DPRINTF_DEBUG	machdep_debug
#endif /* DEBUG */
#include <machine/debug.h>

/*
 * D-RAM location (Windows CE machine specific)
 *
 * Jornada 690 (32MB model) SH7709A
 *  + SH7709A split CS3 to 2 banks.
 *
 * CS3 (0x0c000000-0x0fffffff
 * 0x0c000000 --- onboard   16MByte
 * 0x0d000000 --- onboard   16MByte (shadow)
 * 0x0e000000 --- extension 16MByte
 * 0x0f000000 --- extension 16MByte (shadow)
 *
 * PERSONA HPW-650PA (16MB model) SH7750
 * SH7750
 *
 * CS3 (0x0c000000-0x0fffffff
 * 0x0c000000 --- onboard   16MByte
 * 0x0d000000 --- onboard   16MByte (shadow)
 * 0x0e000000 --- onboard   16MByte (shadow)
 * 0x0f000000 --- onboard   16MByte (shadow)
 */

#define	SH_CS3_START			0x0c000000
#define	SH_CS3_END			(SH_CS3_START + 0x04000000)

#define	SH7709_CS3_BANK0_START		0x0c000000
#define	SH7709_CS3_BANK0_END		(SH7709_CS3_BANK0_START + 0x02000000)
#define	SH7709_CS3_BANK1_START		0x0e000000
#define	SH7709_CS3_BANK1_END		(SH7709_CS3_BANK1_START + 0x02000000)

/* Machine */
char machine[]		= MACHINE;
char machine_arch[]	= MACHINE_ARCH;
extern char cpu_model[];
struct bootinfo *bootinfo;

/* Physical memory */
static int	mem_cluster_init(paddr_t);
static void	mem_cluster_load(void);
static void	__find_dram_shadow(paddr_t, paddr_t);
#ifdef NARLY_MEMORY_PROBE
static int	__check_dram(paddr_t, paddr_t);
#endif
int		mem_cluster_cnt;
phys_ram_seg_t	mem_clusters[VM_PHYSSEG_MAX];

void main(void) __attribute__((__noreturn__));
void machine_startup(int, char *[], struct bootinfo *)
	__attribute__((__noreturn__));
void (*__sleep_func)(void *);	/* model dependent sleep function holder */
void *__sleep_ctx;

void
machine_startup(int argc, char *argv[], struct bootinfo *bi)
{
	extern char edata[], end[];
	vaddr_t kernend;
	size_t symbolsize;
	int i;
	char *p;
	/*
	 * this routines stack is never polluted since stack pointer
	 * is lower than kernel text segment, and at exiting, stack pointer
	 * is changed to proc0.
	 */
	struct kloader_bootinfo kbi;

	/* Symbol table size */
	symbolsize = 0;
	if (memcmp(&end, ELFMAG, SELFMAG) == 0) {
		Elf_Ehdr *eh = (void *)end;
		Elf_Shdr *sh = (void *)(end + eh->e_shoff);
		for(i = 0; i < eh->e_shnum; i++, sh++)
			if (sh->sh_offset > 0 &&
			    (sh->sh_offset + sh->sh_size) > symbolsize)
				symbolsize = sh->sh_offset + sh->sh_size;
	}

	/* Clear BSS */
	memset(edata, 0, end - edata);

	/* Setup bootinfo */
	bootinfo = &kbi.bootinfo;
	memcpy(bootinfo, bi, sizeof(struct bootinfo));
	if (bootinfo->magic == BOOTINFO_MAGIC) {
		platid.dw.dw0 = bootinfo->platid_cpu;
		platid.dw.dw1 = bootinfo->platid_machine;
	}

	/* CPU initialize */
	if (platid_match(&platid, &platid_mask_CPU_SH_3))
		sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7709A);
	else if (platid_match(&platid, &platid_mask_CPU_SH_4))
		sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750);

#ifndef RTC_OFFSET
	/*
	 * rtc_offset from bootinfo.timezone set by hpcboot.exe
	 */
	if (rtc_offset == 0
	    && bootinfo->timezone > (-12*60)
	    && bootinfo->timezone <= (12*60))
		rtc_offset = bootinfo->timezone;
#endif

	/* Start to determine heap area */
	kernend = (vaddr_t)sh3_round_page(end + symbolsize);

	/* Setup bootstrap options */
	makebootdev("wd0"); /* default boot device */
	boothowto = 0;
	for (i = 1; i < argc; i++) { /* skip 1st arg (kernel name). */
		char *cp = argv[i];
		switch (*cp) {
		case 'b':
			/* boot device: -b=sd0 etc. */
			p = cp + 2;
#ifdef NFS
			if (strcmp(p, "nfs") == 0)
				mountroot = nfs_mountroot;
			else
				makebootdev(p);
#else /* NFS */
			makebootdev(p);
#endif /* NFS */
			break;
		default:
			BOOT_FLAG(*cp, boothowto);
			break;
		}
	}

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT) {
		size_t fssz;
		fssz = sh3_round_page(mfs_initminiroot((void *)kernend));
#ifdef MEMORY_DISK_DYNAMIC
		md_root_setconf((caddr_t)kernend, fssz);
#endif
		kernend += fssz;
	}
#endif /* MFS */

	/* Console */
	consinit();
#ifdef HPC_DEBUG_LCD
	dbg_lcd_test();
#endif
	/* copy boot parameter for kloader */
	kloader_bootinfo_set(&kbi, argc, argv, bi, TRUE);

	/* Find memory cluster. and load to UVM */
	physmem = mem_cluster_init(SH3_P1SEG_TO_PHYS(kernend));
	_DPRINTF("total memory = %dMbyte\n", (int)(sh3_ptob(physmem) >> 20));
	mem_cluster_load();

	/* Initialize proc0 u-area */
	sh_proc0_init();

	/* Initialize pmap and start to address translation */
	pmap_bootstrap();

#if NKSYMS || defined(DDB) || defined(LKM)
	if (symbolsize) {
		ksyms_init(symbolsize, &end, end + symbolsize);
		_DPRINTF("symbol size = %d byte\n", symbolsize);
	}
#endif
#ifdef DDB
	/* Debugger. */
	if (boothowto & RB_KDB)
		Debugger();
#endif /* DDB */
#ifdef KGDB
	if (boothowto & RB_KDB) {
		if (kgdb_dev == NODEV) {
			printf("no kgdb console.\n");
		} else {
			kgdb_debug_init = 1;
			kgdb_connect(1);
		}
	}
#endif /* KGDB */

	/* Jump to main */
	__asm volatile(
		"jmp	@%0;"
		"mov	%1, sp"
		:: "r"(main),"r"(lwp0.l_md.md_pcb->pcb_sf.sf_r7_bank));

	/* NOTREACHED */
	for (;;)
		continue;
}

void
cpu_startup(void)
{

	sprintf(cpu_model, "%s\n", platid_name(&platid));

	sh_startup();
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
}

void
machine_sleep(void)
{

	if (__sleep_func != NULL)
		__sleep_func(__sleep_ctx);
}

void
machine_standby(void)
{
	// notyet
}

void
cpu_reboot(int howto, char *bootstr)
{

	/* take a snap shot before clobbering any registers */
	if (curlwp)
		savectx(curpcb);

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if ((boothowto & RB_HALT) != 0) {
		howto |= RB_HALT;
	}

#ifdef KLOADER_KERNEL_PATH
	if ((howto & RB_HALT) == 0)
		kloader_reboot_setup(KLOADER_KERNEL_PATH);
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
#if notyet
	if (howto & RB_DUMP)
		dumpsys();
#endif

 haltsys:
	/* run any shutdown hooks */
	doshutdownhooks();

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("halted.\n");
	} else {
#ifdef KLOADER_KERNEL_PATH
		kloader_reboot();
		/* NOTREACHED */
#endif
	}

#if NHD64465IF > 0
	hd64465_shutdown();
#endif

	cpu_reset();

	/* NOTREACHED */
	for (;;)
		continue;
}

/* return # of physical pages. */
int
mem_cluster_init(paddr_t addr)
{
	phys_ram_seg_t *seg;
	int npages, i;

	/* cluster 0 is always the kernel itself. */
	mem_clusters[0].start = SH_CS3_START;
	mem_clusters[0].size = addr - SH_CS3_START;
	mem_cluster_cnt = 1;

	/* search CS3 */
#ifdef SH3
	/* SH7709A's CS3 is split to 2 banks. */
	if (CPU_IS_SH3) {
		__find_dram_shadow(addr, SH7709_CS3_BANK0_END);
		__find_dram_shadow(SH7709_CS3_BANK1_START,
		    SH7709_CS3_BANK1_END);
	}
#endif
#ifdef SH4
	/* contiguous CS3 */
	if (CPU_IS_SH4) {
		__find_dram_shadow(addr, SH_CS3_END);
	}
#endif
	_DPRINTF("mem_cluster_cnt = %d\n", mem_cluster_cnt);
	npages = 0;
	for (i = 0, seg = mem_clusters; i < mem_cluster_cnt; i++, seg++) {
		_DPRINTF("mem_clusters[%d] = {0x%lx+0x%lx <0x%lx}", i,
		    (paddr_t)seg->start, (paddr_t)seg->size,
		    (paddr_t)seg->start + (paddr_t)seg->size);
		npages += sh3_btop(seg->size);
#ifdef NARLY_MEMORY_PROBE
		if (i == 0) {
			_DPRINTF(" don't check.\n");
			continue;
		}
		if (__check_dram((paddr_t)seg->start, (paddr_t)seg->start +
		    (paddr_t)seg->size) != 0)
			panic("D-RAM check failed.");
#else
		_DPRINTF("\n");
#endif /* NARLY_MEMORY_PROBE */
	}

	return (npages);
}

void
mem_cluster_load(void)
{
	paddr_t start, end;
	psize_t size;
	int i;

	/* Cluster 0 is always the kernel, which doesn't get loaded. */
	sh_dcache_wbinv_all();
	for (i = 1; i < mem_cluster_cnt; i++) {
		start = (paddr_t)mem_clusters[i].start;
		size = (psize_t)mem_clusters[i].size;

		_DPRINTF("loading 0x%lx,0x%lx\n", start, size);
		memset((void *)SH3_PHYS_TO_P1SEG(start), 0, size);
		end = atop(start + size);
		start = atop(start);
		uvm_page_physload(start, end, start, end, VM_FREELIST_DEFAULT);
	}
	sh_dcache_wbinv_all();
}

void
__find_dram_shadow(paddr_t start, paddr_t end)
{
	vaddr_t page, startaddr, endaddr;
	int x;

	_DPRINTF("search D-RAM from 0x%08lx for 0x%08lx\n", start, end);
	startaddr = SH3_PHYS_TO_P2SEG(start);
	endaddr = SH3_PHYS_TO_P2SEG(end);

	page = startaddr;

	x = random();
	*(volatile int *)(page + 0) = x;
	*(volatile int *)(page + 4) = ~x;

	if (*(volatile int *)(page + 0) != x ||
	    *(volatile int *)(page + 4) != ~x)
		return;

	for (page += PAGE_SIZE; page < endaddr; page += PAGE_SIZE) {
		if (*(volatile int *)(page + 0) == x &&
		    *(volatile int *)(page + 4) == ~x) {
			goto memend_found;
		}
	}

	page -= PAGE_SIZE;
	*(volatile int *)(page + 0) = x;
	*(volatile int *)(page + 4) = ~x;

	if (*(volatile int *)(page + 0) != x ||
	    *(volatile int *)(page + 4) != ~x)
		return; /* no memory in this bank */

 memend_found:
	KASSERT(mem_cluster_cnt < VM_PHYSSEG_MAX);

	mem_clusters[mem_cluster_cnt].start = start;
	mem_clusters[mem_cluster_cnt].size = page - startaddr;

	/* skip kernel area */
	if (mem_cluster_cnt == 1)
		mem_clusters[1].size -= mem_clusters[0].size;

	mem_cluster_cnt++;
}

#ifdef NARLY_MEMORY_PROBE
int
__check_dram(paddr_t start, paddr_t end)
{
	uint8_t *page;
	int i, x;

	_DPRINTF(" checking...");
	for (; start < end; start += PAGE_SIZE) {
		page = (uint8_t *)SH3_PHYS_TO_P2SEG (start);
		x = random();
		for (i = 0; i < PAGE_SIZE; i += 4)
			*(volatile int *)(page + i) = (x ^ i);
		for (i = 0; i < PAGE_SIZE; i += 4)
			if (*(volatile int *)(page + i) != (x ^ i))
				goto bad;
		x = random();
		for (i = 0; i < PAGE_SIZE; i += 4)
			*(volatile int *)(page + i) = (x ^ i);
		for (i = 0; i < PAGE_SIZE; i += 4)
			if (*(volatile int *)(page + i) != (x ^ i))
				goto bad;
	}
	_DPRINTF("success.\n");
	return (0);
 bad:
	_DPRINTF("failed.\n");
	return (1);
}
#endif /* NARLY_MEMORY_PROBE */

void
intc_intr(int ssr, int spc, int ssp)
{
	struct intc_intrhand *ih;
	int evtcode;
	uint16_t r;

	evtcode = _reg_read_4(CPU_IS_SH3 ? SH7709_INTEVT2 : SH4_INTEVT);

	ih = EVTCODE_IH(evtcode);
	KDASSERT(ih->ih_func);
	/*
	 * On entry, all interrrupts are disabled,
	 * and exception is enabled for P3 access. (kernel stack is P3,
	 * SH3 may or may not cause TLB miss when access stack.)
	 * Enable higher level interrupt here.
	 */
	r = _reg_read_2(HD6446X_NIRR);

	splx(ih->ih_level);

	if (evtcode == SH_INTEVT_TMU0_TUNI0) {
		struct clockframe cf;
		cf.spc = spc;
		cf.ssr = ssr;
		cf.ssp = ssp;
		(*ih->ih_func)(&cf);
		__dbg_heart_beat(HEART_BEAT_RED);
	} else if (evtcode ==
	    (CPU_IS_SH3 ? SH7709_INTEVT2_IRQ4 : SH_INTEVT_IRL11)) {
		int cause = r & hd6446x_ienable;
		struct hd6446x_intrhand *hh = &hd6446x_intrhand[ffs(cause) - 1];
		if (cause == 0) {
			printf("masked HD6446x interrupt.0x%04x\n", r);
			_reg_write_2(HD6446X_NIRR, 0x0000);
			return;
		}
		/* Enable higher level interrupt*/
		hd6446x_intr_resume(hh->hh_ipl);
		KDASSERT(hh->hh_func != NULL);
		(*hh->hh_func)(hh->hh_arg);
		__dbg_heart_beat(HEART_BEAT_GREEN);
	} else {
		(*ih->ih_func)(ih->ih_arg);
		__dbg_heart_beat(HEART_BEAT_BLUE);
	}
}
