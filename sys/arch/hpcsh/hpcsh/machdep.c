/*	$NetBSD: machdep.c,v 1.37 2002/03/28 15:27:05 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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
#undef LOAD_ALL_MEMORY

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
#include <sys/msgbuf.h>
#include <sys/boot_flag.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <sh3/cpu.h>
#include <sh3/exception.h>
#include <sh3/clock.h>
#include <sh3/intcreg.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif
#if defined(DDB) || defined(KGDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE		DB_ELFSIZE
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

#include <hpcsh/dev/hd6446x/hd6446xintcvar.h>
#include <hpcsh/dev/hd6446x/hd6446xintcreg.h>
#include <hpcsh/dev/hd64465/hd64465var.h>

#ifdef DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	machdep_debug
#endif /* DEBUG */
#include <machine/debug.h>

/* 
 * D-RAM location (Windows CE machine specific)
 *
 * sample) jornada 690 (32MByte) SH7709A
 * SH7709A has 2 banks in CS3
 *
 * CS3 (0x0c000000-0x0fffffff
 * 0x0c000000 --- main      16MByte
 * 0x0d000000 --- main      16MByte (shadow)
 * 0x0e000000 --- extension 16MByte
 * 0x10000000 --- extension 16MByte (shadow)
 */

#define DRAM_BANK_NUM		2
#define DRAM_BANK_SIZE		0x02000000	/* 32MByte */

#define DRAM_BANK0_START	0x0c000000
#define DRAM_BANK0_END		(DRAM_BANK0_START + DRAM_BANK_SIZE)
#define DRAM_BANK1_START	0x0e000000
#define DRAM_BANK1_END		(DRAM_BANK1_START + DRAM_BANK_SIZE)

#ifdef NFS
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#endif

/* Machine */
char machine[]		= MACHINE;
char machine_arch[]	= MACHINE_ARCH;
extern char cpu_model[];
struct bootinfo *bootinfo;

/* Physical memory */
extern paddr_t avail_start, avail_end;
static int	mem_cluster_init(paddr_t);
static void	mem_cluster_load(void);
static void	__find_dram_shadow(paddr_t, paddr_t);
#ifdef NARLY_MEMORY_PROBE
static int	__check_dram(paddr_t, paddr_t);
#endif
int		mem_cluster_cnt;
phys_ram_seg_t	mem_clusters[VM_PHYSSEG_MAX];
paddr_t msgbuf_paddr;

void main(void) __attribute__((__noreturn__));
void machine_startup(int, char *[], struct bootinfo *)
	__attribute__((__noreturn__));

void
machine_startup(int argc, char *argv[], struct bootinfo *bi)
{
	extern char edata[], end[];
	vaddr_t kernend;
	vsize_t sz;
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

	/* 
	 * Console
	 */
	consinit();
#ifdef HPC_DEBUG_LCD
	dbg_lcd_test();
#endif
	/* copy boot parameter for kloader */
	kloader_bootinfo_set(&kbi, argc, argv, bi, TRUE);

	/* 
	 * Find memory cluster.
	 */
	physmem = mem_cluster_init(SH3_P1SEG_TO_PHYS(kernend));
	_DPRINTF("total memory = %dMbyte\n", (int)(sh3_ptob(physmem) >> 20));

	/*
	 * Initialize proc0 and enable MMU.
	 */
	sz = sh_proc0_init(kernend, mem_clusters[0].start,
	    mem_clusters[1].start + mem_clusters[1].size - 1);
	/* adjust heap size */
	kernend += sz;
	mem_clusters[1].start += sz;
	mem_clusters[1].size -= sz;
	_DPRINTF("proc0: steal %ld KB, stack: 0x%08lx\n", sz >> 10,
	    proc0.p_addr->u_pcb.pcb_sp);

	/* 
	 * Debugger.
	 */
#ifdef DDB
	if (symbolsize) {
		ddb_init(symbolsize, &end, end + symbolsize);
		_DPRINTF("symbol size = %d byte\n", symbolsize);
	}
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
	/*
	 * Load physical memory to VM 
	 */
	mem_cluster_load();
	pmap_bootstrap(VM_MIN_KERNEL_ADDRESS);

	initmsgbuf((caddr_t)msgbuf_paddr, round_page(MSGBUFSIZE));

	/* Jump to main */
	__asm__ __volatile__(
		"jmp	@%0;"
		"mov	%1, sp" :: "r"(main), "r"(proc0.p_addr->u_pcb.pcb_sp));
	/* NOTREACHED */
	while (1)
		;
}

void
cpu_startup()
{
	platid_t cpu;
	int cpuclock, pclock;

	cpuclock = sh_clock_get_cpuclock();
	pclock = sh_clock_get_pclock();

	sh_startup();

	memcpy(&cpu, &platid, sizeof(platid_t));
	cpu.dw.dw1 = 0;	/* clear platform */
	sprintf(cpu_model, "[%s] %s", platid_name(&platid), platid_name(&cpu));

#define MHZ(x) ((x) / 1000000), (((x) % 1000000) / 1000)
	printf("%s %d.%02d MHz PCLOCK %d.%02d MHz\n", cpu_model,
	    MHZ(cpuclock), MHZ(pclock));
}

int
cpu_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &cn_tab->cn_dev,
		    sizeof cn_tab->cn_dev));
	default:
	}

	return (EOPNOTSUPP);
}

void
cpu_reboot(int howto, char *bootstr)
{

	/* take a snap shot before clobbering any registers */
	if (curproc)
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
	/*NOTREACHED*/
	while(1)
		;
}

/* return # of physical pages. */
int
mem_cluster_init(paddr_t addr)
{
	phys_ram_seg_t *seg;
	int npages, i;

	/* cluster 0 is always kernel myself. */
	mem_clusters[0].start = DRAM_BANK0_START;
	mem_clusters[0].size = addr - DRAM_BANK0_START;
	mem_cluster_cnt = 1;
	
	/* search CS3 */
	__find_dram_shadow(addr, DRAM_BANK0_END);
#ifdef LOAD_ALL_MEMORY /* notyet */
	__find_dram_shadow(DRAM_BANK1_START, DRAM_BANK1_END);
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
mem_cluster_load()
{
	paddr_t start, end;
	psize_t size;
#ifdef LOAD_ALL_MEMORY /*  notyet */
	int i;

	/* Cluster 0 is always the kernel, which doesn't get loaded. */
	for (i = 1; i < mem_cluster_cnt; i++) {
		start = (paddr_t)mem_clusters[i].start;
		size = (psize_t)mem_clusters[i].size;

		_DPRINTF("loading 0x%lx,0x%lx\n", start, size);
		start = SH3_PHYS_TO_P1SEG(start);
		memset((void *)start, 0, size);
		sh_dcache_wbinv_all();
		end = atop(start + size);
		start = atop(start);
		uvm_page_physload(start, end, start, end, VM_FREELIST_DEFAULT);
	}
#else
	/* load cluster 1 only. */
	start = (paddr_t)mem_clusters[1].start;
	size = (psize_t)mem_clusters[1].size;
	_DPRINTF("loading 0x%lx,0x%lx\n", start, size);

	start = SH3_PHYS_TO_P1SEG(start);
	end = start + size;
	memset((void *)start, 0, size);

	avail_start = start;
	avail_end = end;
#endif
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

	for (page += NBPG; page < endaddr; page += NBPG) {
		if (*(volatile int *)(page + 0) == x &&
		    *(volatile int *)(page + 4) == ~x) {
			goto memend_found;
		}
	}

	page -= NBPG;
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
	u_int8_t *page;
	int i, x;

	_DPRINTF(" checking...");
	for (; start < end; start += NBPG) {
		page = (u_int8_t *)SH3_PHYS_TO_P2SEG (start);
		x = random();
		for (i = 0; i < NBPG; i += 4)
			*(volatile int *)(page + i) = (x ^ i);
		for (i = 0; i < NBPG; i += 4)
			if (*(volatile int *)(page + i) != (x ^ i))
				goto bad;
		x = random();
		for (i = 0; i < NBPG; i += 4)
			*(volatile int *)(page + i) = (x ^ i);
		for (i = 0; i < NBPG; i += 4)
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
	u_int16_t r;

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
			goto ret;
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
 ret:
	/* Return to old interrupt level. */
	splx(ssr & 0xf0);
}
