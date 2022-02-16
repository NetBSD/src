/*	$NetBSD: machdep.c,v 1.19 2022/02/16 23:49:26 riastradh Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.19 2022/02/16 23:49:26 riastradh Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/powerpc.h>

#include <powerpc/pmap.h>
#include <powerpc/trap.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pic/picvar.h>
#include <powerpc/include/pio.h>

#include <dev/pci/pcivar.h>
#include <dev/ic/ibm82660reg.h>

#include <dev/cons.h>

void initppc(u_long, u_long, u_int, void *);
void dumpsys(void);
vaddr_t prep_intr_reg;			/* PReP interrupt vector register */
uint32_t prep_intr_reg_off;

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

paddr_t avail_end;			/* XXX temporary */
struct pic_ops *isa_pic;
int isa_pcmciamask = 0x8b28;

void
initppc(u_long startkernel, u_long endkernel, u_int args, void *btinfo)
{

	uint32_t sa, ea, banks;
	u_long memsize = 0;
	pcitag_t tag;

	/*
	 * Set memory region by reading the memory size from the PCI
	 * host bridge.
	 */

	tag = genppc_pci_indirect_make_tag(NULL, 0, 0, 0);

	out32rb(PCI_MODE1_ADDRESS_REG, tag | IBM_82660_MEM_BANK0_START);
	sa = in32rb(PCI_MODE1_DATA_REG);

	out32rb(PCI_MODE1_ADDRESS_REG, tag | IBM_82660_MEM_BANK0_END);
	ea = in32rb(PCI_MODE1_DATA_REG);

	/* Which memory banks are enabled? */
	out32rb(PCI_MODE1_ADDRESS_REG, tag | IBM_82660_MEM_BANK_ENABLE);
	banks = in32rb(PCI_MODE1_DATA_REG) & 0xFF;

	/* Reset the register for the next call. */
	out32rb(PCI_MODE1_ADDRESS_REG, 0);

	if (banks & IBM_82660_MEM_BANK0_ENABLED)
		memsize += IBM_82660_BANK0_ADDR(ea) - IBM_82660_BANK0_ADDR(sa) + 1;

	if (banks & IBM_82660_MEM_BANK1_ENABLED)
		memsize += IBM_82660_BANK1_ADDR(ea) - IBM_82660_BANK1_ADDR(sa) + 1;

	if (banks & IBM_82660_MEM_BANK2_ENABLED)
		memsize += IBM_82660_BANK2_ADDR(ea) - IBM_82660_BANK2_ADDR(sa) + 1;

	if (banks & IBM_82660_MEM_BANK3_ENABLED)
		memsize += IBM_82660_BANK3_ADDR(ea) - IBM_82660_BANK3_ADDR(sa) + 1;

	memsize <<= 20;

	physmemr[0].start = 0;
	physmemr[0].size = memsize & ~PGOFSET;
	availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
	availmemr[0].size = memsize - availmemr[0].start;

	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Set CPU clock
	 */
	{
		extern u_long ticks_per_sec, ns_per_tick;

		ticks_per_sec = 16666666;		/* hardcoded */
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * boothowto
	 */
	boothowto = 0;		/* XXX - should make this an option */

	prep_initppc(startkernel, endkernel, args, 0);
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	/*
	 * Mapping PReP interrput vector register.
	 */
	prep_intr_reg = (vaddr_t) mapiodev(PREP_INTR_REG, PAGE_SIZE, false);
	if (!prep_intr_reg)
		panic("startup: no room for interrupt register");
	prep_intr_reg_off = INTR_VECTOR_REG;

	/*
	 * Do common startup.
	 */
	oea_startup("IBM NetworkStation 1000 (8362-XXX)");

	pic_init();
	isa_pic = setup_prepivr(PIC_IVR_IBM);

        oea_install_extint(pic_ext_intr);

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;

		splraise(-1);
		__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
	}

	/*
	 * Now safe for bus space allocation to use malloc.
	 */
	bus_space_mallocok();
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;

	if (cold) {
		howto |= RB_HALT;
		goto halt_sys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && syncing == 0) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

	/* Disable intr */
	splhigh();

	/* Do dump if requested */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		oea_dumpsys();

halt_sys:
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if (howto & RB_HALT) {
                aprint_normal("\n");
                aprint_normal("The operating system has halted.\n");
                aprint_normal("Please press any key to reboot.\n\n");
                cnpollc(1);	/* for proper keyboard command handling */
                cngetc();
                cnpollc(0);
	}

	aprint_normal("rebooting...\n\n");


        {
	    int msr;
	    u_char reg;

	    __asm volatile("mfmsr %0" : "=r"(msr));
	    msr |= PSL_IP;
	    __asm volatile("mtmsr %0" :: "r"(msr));

	    reg = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92);
	    reg &= ~1UL;
	    *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92) = reg;

	    __asm volatile("sync; eieio" ::: "memory");

	    reg = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92);
	    reg |= 1;
	    *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92) = reg;

	    __asm volatile("sync; eieio" ::: "memory");
	}

	for (;;)
		continue;
	/* NOTREACHED */
}
