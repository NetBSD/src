/*	$NetBSD: machdep.c,v 1.24 2003/02/03 17:10:14 matt Exp $	*/

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
#include "opt_ddb.h"
#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"
#include "opt_ipkdb.h"

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
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <powerpc/bat.h>
#include <machine/bus.h>
#include <machine/db_machdep.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <powerpc/openpic.h>

#include <ddb/db_extern.h>

#include <dev/cons.h>

#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "isa.h"
#if (NISA > 0)
void isa_intr_init(void);
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Global variables used here and there
 */

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

char *bootpath;
unsigned char *eumb_base;

paddr_t avail_end;			/* XXX temporary */

void initppc __P((u_int, u_int, u_int, void *)); /* Called from locore */
void strayintr __P((int));
void lcsplx __P((int));

#ifdef DDB
extern void *startsym, *endsym;
#endif
extern void consinit (void);
extern void ext_intr (void);

void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{

#if 1
	{ extern unsigned char *edata, *end;
	  memset(&edata, 0, (u_int) &end - (u_int) &edata);
	}
#endif

	/*
	 * Hardcode 32MB for now--we should probe for this or get it
	 * from a boot loader, but for now, we are booting via an
	 * S-record loader.
	 */
	{	/* XXX AKB */
		u_int32_t	physmemsize;

		physmemsize = 32 * 1024 * 1024;
		physmemr[0].start = 0;
		physmemr[0].size = physmemsize;
		physmemr[1].size = 0;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = physmemsize - availmemr[0].start;
		availmemr[1].size = 0;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Get CPU clock
	 */
	{	/* XXX AKB */
		extern u_long ticks_per_sec, ns_per_tick;

		ticks_per_sec = 100000000;	/* 100 MHz */
		/* ticks_per_sec = 66000000;	* 66 MHz */
		ticks_per_sec /= 4;	/* 4 cycles per DEC tick */
		cpu_timebase = ticks_per_sec;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * boothowto
	 */
	boothowto = RB_SINGLE;

	sandpoint_bus_space_init();

consinit();
printf("avail_end %x\n", (unsigned) avail_end);
printf("availmemr[0].start %x\n", (unsigned) availmemr[0].start);
printf("availmemr[0].size %x\n", (unsigned) availmemr[0].size);

	/*
	 * Initialize BAT registers.  Map the EUMB MEMORY 1M area to the
	 * end of the address space This includes the PCI/ISA 16M I/O space,
	 * PCI configuration registers, etc.
	 */
	oea_batinit(
	    SANDPOINT_BUS_SPACE_EUMB, BAT_BL_64M,
	    SANDPOINT_BUS_SPACE_MEM,  BAT_BL_256M,
	    0);

	eumb_base = (unsigned char *) SANDPOINT_BUS_SPACE_EUMB;


	/*
	 * Set up trap vectors and interrupt handler
	 */
	oea_init(ext_intr);

	/*
	 * Set EUMB base address
	 */
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0x80000078);
	out32rb(SANDPOINT_PCI_CONFIG_DATA, SANDPOINT_BUS_SPACE_EUMB);
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0);

	openpic_init(eumb_base + 0x40000);
#if (NISA > 0)
	isa_intr_init();
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
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif
#ifdef IPKDB
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{
	*mem = physmemr;
	*avail = availmemr;
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	int msr;

	oea_startup(NULL);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	sandpoint_bus_space_mallocok();

	/*
	 * Now allow hardware interrupts.
	 */
	splhigh();
	__asm __volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
	    :	"=r"(msr)
	    :	"K"(PSL_EE));
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{
	static int initted;
#if (NCOM > 0)
	bus_space_tag_t tag;
#endif

	if (initted)
		return;
	initted = 1;

#if (NCOM > 0)
	tag = &sandpoint_isa_io_bs_tag;

	if(comcnattach(tag, 0x3F8, 38400, COM_FREQ,
	    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
		panic("can't init serial console");
	else
		return;
#endif

	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
}

#if (NPCKBC > 0) && (NPCKBD == 0)
/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbc_machdep_cnattach(pckbc_tag_t kbctag, pckbc_slot_t kbcslot)
{
#if (NPC > 0)
	return (pcconskbd_cnattach(kbctag, kbcslot));
#else
	return (ENXIO);
#endif
}
#endif

/*
 * Stray interrupts.
 */
void
strayintr(int irq)
{
	log(LOG_ERR, "stray interrupt %d\n", irq);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		printf("halted\n\n");
		while(1);
	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
	doshutdownhooks();
	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;
#if 1
{ extern void sandpoint_reboot __P((void));
	sandpoint_reboot();
}
#endif
	while (1);
}

void
lcsplx(int ipl)
{
	splx(ipl);
}
