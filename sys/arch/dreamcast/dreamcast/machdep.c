/*	$NetBSD: machdep.c,v 1.14 2002/03/08 13:22:12 uch Exp $	*/

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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_syscall_debug.h"
#include "opt_memsize.h"
#include "scif.h"

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

#include <uvm/uvm_extern.h>

#ifdef KGDB
#include <sys/kgdb.h>
#include <sh3/dev/scifvar.h>
#endif
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <sh3/cpu.h>
#include <sh3/mmu.h>
#include <dev/cons.h>

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* cpu "architecture" */
char machine_arch[] = MACHINE_ARCH;	/* machine_arch = "sh3" */

int physmem;
paddr_t msgbuf_paddr;
struct user *proc0paddr;

extern paddr_t avail_start, avail_end;
extern int nkpde;
extern char start[], _etext[], _edata[], _end[];

#define IOM_RAM_END	((paddr_t)IOM_RAM_BEGIN + IOM_RAM_SIZE - 1)

void main(void) __attribute__((__noreturn__));
void dreamcast_startup(void) __attribute__((__noreturn__));

/*
 * Machine-dependent startup code
 *
 * This is called from main() in kern/main.c.
 */
void
cpu_startup()
{

	sh3_startup();
	printf("%s\n", cpu_model);

#ifdef FORCE_RB_SINGLE
	boothowto |= RB_SINGLE;
#endif
}

/*
 * machine dependent system variables.
 */
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
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	static int waittime = -1;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/* resettodr(); */
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	for(;;)
		;
	/*NOTREACHED*/
}

void
dreamcast_startup()
{
	paddr_t avail;
	pd_entry_t *pagedir;
	pt_entry_t *pagetab, pte;
	u_int sp;
	int x;
	char *p;

	avail = sh3_round_page(_end);

	/* XXX nkpde = kernel page dir area (IOM_RAM_SIZE*2 Mbyte (why?)) */
	nkpde = IOM_RAM_SIZE >> (PDSHIFT - 1);

	/*
	 * clear .bss, .common area, page dir area,
	 *	process0 stack, page table area
	 */
	p = (char *)avail + (1 + UPAGES) * NBPG + NBPG * (1 + nkpde); /* XXX */
	memset(_edata, 0, p - _edata);

	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750);
/*
 *                          edata  end
 *	+-------------+------+-----+----------+-------------+------------+
 *	| kernel text | data | bss | Page Dir | Proc0 Stack | Page Table |
 *	+-------------+------+-----+----------+-------------+------------+
 *                                     NBPG       USPACE    (1+nkpde)*NBPG
 *                                                (= 4*NBPG)
 *	Build initial page tables
 */
	pagedir = (void *)avail;
	pagetab = (void *)(avail + SYSMAP);

	/*
	 * Construct a page table directory
	 * In SH3 H/W does not support PTD,
	 * these structures are used by S/W.
	 */
	pte = (pt_entry_t)pagetab;
	pte |= PG_KW | PG_V | PG_4K | PG_M | PG_N;
	pagedir[KERNTEXTOFF >> PDSHIFT] = pte;

	/* make pde for 0xd0000000, 0xd0400000, 0xd0800000,0xd0c00000,
		0xd1000000, 0xd1400000, 0xd1800000, 0xd1c00000 */
	pte += NBPG;
	for (x = 0; x < nkpde; x++) {
		pagedir[(VM_MIN_KERNEL_ADDRESS >> PDSHIFT) + x] = pte;
		pte += NBPG;
	}

	/* Install a PDE recursively mapping page directory as a page table! */
	pte = (u_int)pagedir;
	pte |= PG_V | PG_4K | PG_KW | PG_M | PG_N;
	pagedir[PDSLOT_PTE] = pte;

	/* set PageDirReg */
	SH_MMU_TTB_WRITE((u_int32_t)pagedir);

	/*
	 * Activate MMU
	 */
	sh_mmu_start();

	/*
	 * Now here is virtual address
	 */

	/* Set proc0paddr */
	proc0paddr = (void *)(avail + NBPG);

	/* Set pcb->PageDirReg of proc0 */
	proc0paddr->u_pcb.pageDirReg = (int)pagedir;

	/* avail_start is first available physical memory address */
	avail_start = avail + NBPG + USPACE + NBPG + NBPG * nkpde;

	proc0.p_addr = proc0paddr; /* page dir address */

	/* XXX: PMAP_NEW requires valid curpcb.   also init'd in cpu_startup */
	curpcb = &proc0.p_addr->u_pcb;

	consinit();

	splraise(-1);
	_cpu_exception_resume(0);	/* SR.BL = 0 */

#if defined(KGDB) && NSCIF > 0
	if (scif_kgdb_init() == 0) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif /* KGDB && NSCIF > 0 */

	avail_end = sh3_trunc_page(IOM_RAM_END + 1);

	/*
	 * Calculate check sum
	 */
	{
		u_short *p, sum;
		int size;
		
		size = _etext - start;
		p = (u_short *)start;
		sum = 0;
		size >>= 1;
		while (size--)
			sum += *p++;
		printf("Check Sum = 0x%x\r\n", sum);
	}

	/* number of pages of physmem addr space */
	physmem = btoc(IOM_RAM_END - IOM_RAM_BEGIN +1);

	/* Call pmap initialization to make new kernel address space */
	pmap_bootstrap(VM_MIN_KERNEL_ADDRESS);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	initmsgbuf((caddr_t)msgbuf_paddr, round_page(MSGBUFSIZE));

	/* setup proc0 stack */
	sp = avail + NBPG + USPACE - 16 - sizeof(struct trapframe);

	/* jump to main */
	__asm__ __volatile__(
		"jmp	@%0;"
		"mov	%1, sp" :: "r"(main), "r"(sp));
	/* NOTREACHED */
	while (1)
		;
}

void
consinit()
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	cninit();

#ifdef DDB
	ddb_init(0, NULL, NULL);
#endif
}
