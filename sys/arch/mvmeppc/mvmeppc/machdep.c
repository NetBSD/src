/*	$NetBSD: machdep.c,v 1.8 2003/01/22 21:55:16 kleink Exp $	*/

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

#include "opt_compat_netbsd.h"
#include "opt_mvmetype.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <net/netisr.h>

#include <machine/autoconf.h>
#include <machine/bat.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/platform.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <dev/cons.h>

#if 0
#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
void comsoft(void);
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

void initppc(u_long, u_long, void *);
void strayintr(int);
int lcsplx(int);


/*
 * Global variables used here and there
 */
struct mvmeppc_bootinfo bootinfo;

vaddr_t mvmeppc_intr_reg;	/* PReP-compatible  interrupt vector register */

struct mem_region physmemr[2], availmemr[2];

paddr_t avail_end;			/* XXX temporary */

void
initppc(startkernel, endkernel, btinfo)
	u_long startkernel, endkernel;
	void *btinfo;
{
#ifdef DDB
	extern void *startsym, *endsym;
#endif

	/*
	 * Copy bootinfo.
	 */
	memcpy(&bootinfo, btinfo, sizeof(bootinfo));

	/*
	 * Figure out the board family/type.
	 */
	ident_platform();

	if (platform == NULL) {
		extern void _mvmeppc_unsup_board(const char *, const char *);
		char msg[80];

		sprintf(msg, "Unsupported model: MVME%04x",
		    bootinfo.bi_modelnumber);
		_mvmeppc_unsup_board(msg, &msg[strlen(msg)]);
		/* NOTREACHED */
	}

	/*
	 * Set memory region
	 */
	physmemr[0].start = 0;
	physmemr[0].size = bootinfo.bi_memsize & ~PGOFSET;
	availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
	availmemr[0].size = bootinfo.bi_memsize - availmemr[0].start;
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Set CPU clock
	 */
	{
		extern u_long ticks_per_sec, ns_per_tick;

		ticks_per_sec = bootinfo.bi_clocktps;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * boothowto
	 */
	mpc6xx_batinit(
	    MVMEPPC_PHYS_BASE_IO,  BAT_BL_256M,
	    MVMEPPC_PHYS_BASE_MEM, BAT_BL_256M,
	    0);

	/*
	 * Install vectors and interrupt handler.
	 */
	mpc6xx_init(platform->ext_intr);

#ifdef DEBUG
	/*
	 * i386 port says, that this shouldn't be here,
	 * but I really think the console should be initialized
	 * as early as possible.
	 */
	consinit();
#endif
        /*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#ifdef DDB
	ddb_init((int)((u_long)endsym - (u_long)startsym), startsym, endsym);

	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
mem_regions(mem, avail)
	struct mem_region **mem, **avail;
{

	*mem = physmemr;
	*avail = availmemr;
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup()
{
	char modelbuf[256];

	/*
	 * Mapping PReP-compatible interrput vector register.
	 */
	mvmeppc_intr_reg = (vaddr_t) mapiodev(MVMEPPC_INTR_REG, NBPG);
	if (!mvmeppc_intr_reg)
		panic("startup: no room for interrupt register");

	sprintf(modelbuf, "%s\nCore Speed: %dMHz, Bus Speed: %dMHz\n",
	    platform->model,
	    bootinfo.bi_mpuspeed/1000000,
	    bootinfo.bi_busspeed/1000000);
	mpc6xx_startup(modelbuf);

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;

		splraise(-1);
		__asm __volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
	}

	mvmeppc_bus_space_init();
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	static int initted = 0;

	if (initted)
		return;
	initted = 1;

#if 0

#if (NPFB > 0)
	if (!strcmp(consinfo->devname, "fb")) {
		pfb_cnattach(consinfo->addr);
#if (NPCKBC > 0)
		pckbc_cnattach(&mvmeppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif

#if (NVGA > 0) || (NGTEN > 0)
	if (!strcmp(consinfo->devname, "vga")) {
#if (NGTEN > 0)
		if (!gten_cnattach(&mvmeppc_mem_space_tag))
			goto dokbd;
#endif
#if (NVGA > 0)
		if (!vga_cnattach(&mvmeppc_io_space_tag, &mvmeppc_mem_space_tag,
				-1, 1))
			goto dokbd;
#endif
dokbd:
#if (NPCKBC > 0)
		pckbc_cnattach(&mvmeppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* PC | VGA */

#endif

#if (NCOM > 0)
	if (!strcmp(bootinfo.bi_consoledev, "PC16550")) {
		bus_space_tag_t tag = &mvmeppc_isa_io_bs_tag;
		bus_addr_t caddr[2] = {0x3f8, 0x2f8};
		int rv;
		rv = comcnattach(tag, caddr[bootinfo.bi_consolechan],
		    bootinfo.bi_consolespeed, COM_FREQ,
		    bootinfo.bi_consolecflag);
		if (rv)
			panic("can't init serial console");

		return;
	}
#endif
	panic("invalid console device %s", bootinfo.bi_consoledev);
}

/*
 * Soft tty interrupts.
 */
void
softserial()
{

#if (NCOM > 0)
	comsoft();
#endif
}

/*
 * Stray interrupts.
 */
void
strayintr(irq)
	int irq;
{

	log(LOG_ERR, "stray interrupt %d\n", irq);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(howto, what)
	int howto;
	char *what;
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
		mpc6xx_dumpsys();

halt_sys:
	doshutdownhooks();

	if (howto & RB_HALT) {
                printf("\n");
                printf("The operating system has halted.\n");
                printf("Please press any key to reboot.\n\n");
                cnpollc(1);	/* for proper keyboard command handling */
                cngetc();
                cnpollc(0);
	}

	printf("rebooting...\n\n");

#if 0
	(*platform->reset)();
#endif

	for (;;)
		continue;
	/* NOTREACHED */
}

/*
 * lcsplx() is called from locore; it is an open-coded version of
 * splx() differing in that it returns the previous priority level.
 */
int
lcsplx(ipl)
	int ipl;
{
	int oldcpl;

	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	oldcpl = cpl;
	cpl = ipl;
	if (ipending & ~ipl)
		do_pending_int();
	__asm__ volatile("sync; eieio\n");	/* reorder protect */

	return (oldcpl);
}
