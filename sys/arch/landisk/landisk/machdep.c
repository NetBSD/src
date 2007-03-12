/*	$NetBSD: machdep.c,v 1.3.2.1 2007/03/12 05:48:42 rmind Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.3.2.1 2007/03/12 05:48:42 rmind Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_kloader.h"
#include "opt_kloader_kernel_path.h"
#include "opt_memsize.h"
#include "fs_mfs.h"

#include "ksyms.h"
#include "scif.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>
#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <dev/cons.h>

#include <sh3/bscreg.h>
#include <sh3/cpgreg.h>
#include <sh3/exception.h>
#include <sh3/wdtreg.h>

#include <sh3/cache.h>
#include <sh3/cache_sh4.h>
#include <sh3/mmu_sh4.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>

#include <landisk/landisk/landiskreg.h>

#ifdef KLOADER
#include <machine/kloader.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#include <sh3/dev/scifvar.h>
#endif

#if NKSYMS || defined(LKM) || defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef	DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define	ELFSIZE	DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* landisk */
char machine_arch[] = MACHINE_ARCH;	/* sh3el */

#ifdef KLOADER
struct kloader_bootinfo kbootinfo;
#else
struct bootinfo _bootinfo;
#endif
struct bootinfo *bootinfo;

void main(void) __attribute__((__noreturn__));
void landisk_startup(int, void *) __attribute__((__noreturn__));

void
cpu_startup(void)
{

	/* XXX: show model (LANDISK/USL-5P) */
	strcpy(cpu_model, "Model: I-O DATA LANDISK\n");

        sh_startup();
}

void
landisk_startup(int howto, void *bi)
{
	extern char edata[], end[];
	vaddr_t kernend;
	size_t symbolsize;
	int i;

	/* Clear bss */
	memset(edata, 0, end - edata);

	/* Symbol table size */
	symbolsize = 0;
	if (memcmp(&end, ELFMAG, SELFMAG) == 0) {
		Elf_Ehdr *eh = (void *)end;
		Elf_Shdr *sh = (void *)(end + eh->e_shoff);
		for (i = 0; i < eh->e_shnum; i++, sh++) {
			if (sh->sh_offset > 0 &&
			    (sh->sh_offset + sh->sh_size) > symbolsize) {
				symbolsize = sh->sh_offset + sh->sh_size;
			}
		}
	}

	/* Start to determine heap area */
	kernend = (vaddr_t)sh3_round_page(end + symbolsize);

	/* Copy bootinfo */
#ifdef KLOADER
	bootinfo = &kbootinfo.bootinfo;
#else
	bootinfo = &_bootinfo;
#endif
	memcpy(bootinfo, bi, sizeof(struct bootinfo));
	boothowto = howto;

	/* Initilize CPU ops. */
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7751R);	

	/* Initialize console */
	consinit();

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT) {
	        size_t fssz;
		fssz = sh3_round_page(mfs_initminiroot((void *)kernend));
#ifdef MEMORY_DISK_DYNAMIC
		md_root_setconf((void *)kernend, fssz);
#endif
		kernend += fssz;
	}
#endif /* MFS */

#ifdef KLOADER
	/* copy boot parameter for kloader */
	kloader_bootinfo_set(&kbootinfo, 0, NULL, NULL, true);
#endif

	/* Load memory to UVM */
	physmem = atop(IOM_RAM_SIZE);
	kernend = atop(round_page(SH3_P1SEG_TO_PHYS(kernend)));
	uvm_page_physload(
		physmem, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		VM_FREELIST_DEFAULT);

	/* Initialize proc0 u-area */
	sh_proc0_init();

	/* Initialize pmap and start to address translation */
	pmap_bootstrap();

	/* Debugger. */
#if NKSYMS || defined(DDB) || defined(LKM)
	if (symbolsize != 0) {
		ksyms_init(symbolsize, &end, end + symbolsize);
	}
#endif
#if defined(DDB)
	if (boothowto & RB_KDB) {
		Debugger();
	}
#endif
#if defined(KGDB) && (NSCIF > 0)
	if (scif_kgdb_init() == 0) {
		if (boothowto & RB_KDB) {
			kgdb_debug_init = 1;
			kgdb_connect(1);
		}
	}
#endif /* KGDB && NSCIF > 0 */

	/* Jump to main */
	__asm volatile(
		"jmp	@%0;"
		"mov	%1, sp"
		:: "r" (main), "r" (lwp0.l_md.md_pcb->pcb_sf.sf_r7_bank));
	/* NOTREACHED */
	for (;;) {
		continue;
	}
}

void *
lookup_bootinfo(int type)
{
	struct btinfo_common *help;
	int n = bootinfo->nentries;

	help = (struct btinfo_common *)(bootinfo->info);
	while (n--) {
		if (help->type == type)
			return (help);
		help = (struct btinfo_common *)((char *)help + help->len);
	}
	return (NULL);
}

/*
 * machine dependent system variables.
 */
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
cpu_reboot(int howto, char *bootstr)
{

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

#ifdef KLOADER
	if ((howto & RB_HALT) == 0) {
		if ((howto & RB_STRING) && (bootstr != NULL)) {
			kloader_reboot_setup(bootstr);
		}
#ifdef KLOADER_KERNEL_PATH
		else {
			kloader_reboot_setup(KLOADER_KERNEL_PATH);
		}
#endif
	}
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0) {
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* wait 1s */
	delay(1 * 1000 * 1000);

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP) {
		dumpsys();
	}

haltsys:
	doshutdownhooks();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		_reg_write_1(LANDISK_PWRMNG, PWRMNG_POWEROFF);
		delay(1 * 1000 * 1000);
		printf("POWEROFF FAILED!\n");
	}

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}
#ifdef KLOADER
	else {
		delay(1 * 1000 * 1000);
		kloader_reboot();
		printf("\n");
		printf("Failed to load a new kernel.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}
#endif

	printf("rebooting...\n");
	machine_reset();

	/*NOTREACHED*/
	for (;;) {
		continue;
	}
}

void
machine_reset(void)
{

	_cpu_exception_suspend();
	_reg_write_4(SH_(EXPEVT), EXPEVT_RESET_MANUAL);
	(void)*(volatile uint32_t *)0x80000001;	/* CPU shutdown */

	/*NOTREACHED*/
	for (;;) {
		continue;
	}
}

#if !defined(DONT_INIT_BSC)
/*
 * InitializeBsc
 * : BSC(Bus State Controller)
 */
void InitializeBsc(void);

void
InitializeBsc(void)
{

	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = SDRAM, Area5 = Normal memory
	 * Area4 = Normal Memory
	 * Area6 = Normal memory
	 */
	_reg_write_4(SH4_BCR1, BSC_BCR1_VAL);

	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 16bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	_reg_write_2(SH4_BCR2, BSC_BCR2_VAL);

#if defined(SH4) && defined(SH7751R)
	if (cpu_product == CPU_PRODUCT_7751R) {
#ifdef BSC_BCR3_VAL
		_reg_write_2(SH4_BCR3, BSC_BCR3_VAL);
#endif
#ifdef BSC_BCR4_VAL
		_reg_write_4(SH4_BCR4, BSC_BCR4_VAL);
#endif
	}
#endif	/* SH4 && SH7751R */

	/*
	 * Idle cycle number in transition area and read to write
	 * Area6 = 3, Area5 = 3, Area4 = 3, Area3 = 3, Area2 = 3
	 * Area1 = 3, Area0 = 3
	 */
	_reg_write_4(SH4_WCR1, BSC_WCR1_VAL);

	/*
	 * Wait cycle
	 * Area 6 = 6
	 * Area 5 = 2
	 * Area 4 = 10
	 * Area 3 = 3
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
	_reg_write_4(SH4_WCR2, BSC_WCR2_VAL);

#ifdef BSC_WCR3_VAL
	_reg_write_4(SH4_WCR3, BSC_WCR3_VAL);
#endif

	/*
	 * RAS pre-charge = 2cycle, RAS-CAS delay = 3 cycle,
	 * write pre-charge=1cycle
	 * CAS before RAS refresh RAS assert time = 3 cycle
	 * Disable burst, Bus size=32bit, Column Address=10bit, Refresh ON
	 * CAS before RAS refresh ON, EDO DRAM
	 */
	_reg_write_4(SH4_MCR, BSC_MCR_VAL);

#ifdef BSC_SDMR2_VAL
	_reg_write_1(BSC_SDMR2_VAL, 0);
#endif

#ifdef BSC_SDMR3_VAL
	_reg_write_1(BSC_SDMR3_VAL, 0);
#endif /* BSC_SDMR3_VAL */

	/*
	 * PCMCIA Control Register
	 * OE/WE assert delay 3.5 cycle
	 * OE/WE negate-address delay 3.5 cycle
	 */
#ifdef BSC_PCR_VAL
	_reg_write_2(SH4_PCR, BSC_PCR_VAL);
#endif

	/*
	 * Refresh Timer Control/Status Register
	 * Disable interrupt by CMF, closk 1/16, Disable OVF interrupt
	 * Count Limit = 1024
	 * In following statement, the reason why high byte = 0xa5(a4 in RFCR)
	 * is the rule of SH3 in writing these register.
	 */
	_reg_write_2(SH4_RTCSR, BSC_RTCSR_VAL);

	/*
	 * Refresh Timer Counter
	 * Initialize to 0
	 */
#ifdef BSC_RTCNT_VAL
	_reg_write_2(SH4_RTCNT, BSC_RTCNT_VAL);
#endif

	/* set Refresh Time Constant Register */
	_reg_write_2(SH4_RTCOR, BSC_RTCOR_VAL);

	/* init Refresh Count Register */
#ifdef BSC_RFCR_VAL
	_reg_write_2(SH4_RFCR, BSC_RFCR_VAL);
#endif

	/*
	 * Clock Pulse Generator
	 */
	/* Set Clock mode (make internal clock double speed) */
	_reg_write_2(SH4_FRQCR, FRQCR_VAL);
}
#endif /* !DONT_INIT_BSC */
