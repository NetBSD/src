/*	$NetBSD: machdep.c,v 1.35.14.7 2007/05/10 16:23:37 garbled Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.35.14.7 2007/05/10 16:23:37 garbled Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ksyms.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/bus.h>
#include <machine/db_machdep.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>

#include <powerpc/oea/bat.h>
#include <powerpc/openpic.h>
#include <arch/powerpc/pic/picvar.h>

#include <ddb/db_extern.h>

#include <dev/cons.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "ksyms.h"

char bootinfo[BOOTINFO_MAXSIZE];

void initppc(u_int, u_int, u_int, void *);
void sandpoint_bus_space_init(void);
void consinit(void);

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

paddr_t avail_end;
struct pic_ops *isa_pic = NULL;

#if NKSYMS || defined(DDB) || defined(LKM)
extern void *startsym, *endsym;
#endif

void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{
	struct btinfo_magic *bi_magic = btinfo;

	if ((unsigned)btinfo != 0 && (unsigned)btinfo < startkernel
	    && bi_magic->magic == BOOTINFO_MAGIC)
		memcpy(bootinfo, btinfo, sizeof(bootinfo));
	else
		args = RB_SINGLE;	/* boot via S-record loader */
		
	/*
	 * XXX could determine real size by analysing MEMC SDRAM registers XXX
	 */
	{
		struct btinfo_memory *meminfo;
		size_t memsize = 32 * 1024 * 1024; /* assume 32MB */

		meminfo =
			(struct btinfo_memory *)lookup_bootinfo(BTINFO_MEMORY);
		if (meminfo)
			memsize = meminfo->memsize & ~PGOFSET;
		physmemr[0].start = 0;
		physmemr[0].size = memsize;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = memsize - availmemr[0].start;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Get CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		u_long ticks = 100 * 1000 * 1000; /* assume 100MHz */
		extern u_long ticks_per_sec, ns_per_tick;

		clockinfo =
			(struct btinfo_clock *)lookup_bootinfo(BTINFO_CLOCK);
		if (clockinfo)
			ticks = clockinfo->ticks_per_sec;
		ticks_per_sec = ticks;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * boothowto
	 */
	boothowto = args;

	/*
	 * Now setup fixed bat registers
	 * We setup the memory BAT, the IO space BAT, and a special
	 * BAT for MPC107/MPC824x EUMB space.
	 */
	oea_batinit(
	    SANDPOINT_BUS_SPACE_MEM,  BAT_BL_256M,
	    SANDPOINT_BUS_SPACE_IO,   BAT_BL_32M,
	    SANDPOINT_BUS_SPACE_EUMB, BAT_BL_256K,
	    0);

	/*
	 * Set EUMB base address.
	 */
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0x80000078);
	out32rb(SANDPOINT_PCI_CONFIG_DATA, SANDPOINT_BUS_SPACE_EUMB);
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0);

	/*
	 * Install vectors.
	 */
	oea_init(NULL);

	/*	
	 * Initialize bus_space.
	 */	
	sandpoint_bus_space_init();

	consinit();

        /*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
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
	void *baseaddr;

	/*
	 * Do common startup.
	 */
	oea_startup(NULL);

	/*
	 * Prepare EPIC and install external interrupt handler.
	 *  0..15	used by South bridge i8259 PIC if exists.
	 *  16..39/41	EPIC interrupts, 24 source or 26 source.
	 */
	baseaddr = (void *)(SANDPOINT_BUS_SPACE_EUMB + 0x40000);
	pic_init();
#if 1 /* PIC_I8259 */
	/* set up i8259 as a cascade on EPIC irq 0 */
	isa_pic = setup_i8259();
	(void)setup_openpic(baseaddr, 0);
	intr_establish(16, IST_LEVEL, IPL_NONE, pic_handle_intr, isa_pic);
#else
	(void)setup_openpic(baseaddr, 0);
#endif
	oea_install_extint(pic_ext_intr);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * Now allow hardware interrupts.
	 */
	splhigh();
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
	    :	"=r"(msr)
	    :	"K"(PSL_EE));
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	if (help->next == 0)
		return (NULL);	/* bootinfo[] was not made */ 
	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
		(size_t)help < (size_t)bootinfo + BOOTINFO_MAXSIZE);

	return (NULL);
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{
	struct btinfo_console *consinfo;
	struct btinfo_console bi_cons = { { 0, 0 },  "com", 0x3f8, 38400 };
	static int initted;

	if (initted)
		return;
	initted = 1;

	consinfo = (struct btinfo_console *)lookup_bootinfo(BTINFO_CONSOLE);
	if (consinfo == NULL)
		consinfo = &bi_cons;

#if (NCOM > 0)
	if (strcmp(consinfo->devname, "com") == 0
	    || strcmp(consinfo->devname, "eumb") == 0) {
		bus_space_tag_t tag = &genppc_isa_io_space_tag;
		int frq = COM_FREQ;	
#if 0
		if (strcmp(consinfo->devname, "eumb") == 0) {
			tag = &genppc_eumb_space_tag;
			frq = ticks_per_sec;
		}
#endif
		if (comcnattach(tag, consinfo->addr, consinfo->speed,
		    frq, COM_TYPE_NORMAL,
		    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
			panic("can't init serial console");

		return;
	}
#endif
	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
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

struct powerpc_bus_space sandpoint_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_offset = 0xfe000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0x00c00000,
};
struct powerpc_bus_space genppc_isa_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_offset = 0xfe000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0x00010000,
};
struct powerpc_bus_space sandpoint_mem_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = 0x80000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0xfe000000,	/* ??? */
};
struct powerpc_bus_space genppc_isa_mem_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = 0xfd000000,	/* ??? */
	.pbs_base = 0x00000000,
	.pbs_limit = 0xfe000000,	/* ??? */
};
#if 0
struct powerpc_bus_space sandpoint_eumb_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = 0xfc000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0x00040000,
};
#endif

static char ex_storage[2][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

void
sandpoint_bus_space_init(void)
{
	int error;

	error = bus_space_init(&sandpoint_io_space_tag, "ioport",
	    ex_storage[0], sizeof(ex_storage[0]));
	if (error)
		panic("sandpoint_bus_space_init: can't init ioport tag");

	error = extent_alloc_region(sandpoint_io_space_tag.pbs_extent,
	    0x00010000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("sandpoint_bus_space_init: can't block out reserved"
		    " I/O space 0x10000-0x7fffff: error=%d", error);

	genppc_isa_io_space_tag.pbs_extent = sandpoint_io_space_tag.pbs_extent;
	error = bus_space_init(&genppc_isa_io_space_tag, "isa-iomem",
	    ex_storage[1], sizeof(ex_storage[1]));
	if (error)
		panic("sandpoint_bus_space_init: can't init isa iomem tag");

	error = bus_space_init(&sandpoint_mem_space_tag, "iomem",
	    ex_storage[2], sizeof(ex_storage[2]));
	if (error)
		panic("sandpoint_bus_space_init: can't init iomem tag");

	genppc_isa_mem_space_tag.pbs_extent = sandpoint_mem_space_tag.pbs_extent;
	error = bus_space_init(&genppc_isa_mem_space_tag, "isa-iomem",
	    ex_storage[3], sizeof(ex_storage[3]));
	if (error)
		panic("sandpoint_bus_space_init: can't init isa iomem tag");
}

/* XXX XXX XXX */

unsigned epicsteer[] = {
	0x10200,	/* external irq 0 direct/serial */
	0x10220,	/* external irq 1 direct/serial */
	0x10240,	/* external irq 2 direct/serial */
	0x10260,	/* external irq 3 direct/serial */
	0x10280,	/* external irq 4 direct/serial */
	0x102a0,	/* external irq 5 serial mode */
	0x102c0,	/* external irq 6 serial mode */
	0x102e0,	/* external irq 7 serial mode */
	0x10300,	/* external irq 8 serial mode */
	0x10320,	/* external irq 9 serial mode */
	0x10340,	/* external irq 10 serial mode */
	0x10360,	/* external irq 11 serial mode */
	0x10380,	/* external irq 12 serial mode */
	0x103a0,	/* external irq 13 serial mode */
	0x103c0,	/* external irq 14 serial mode */
	0x103e0,	/* external irq 15 serial mode */
	0x11020,	/* I2C */
	0x11040,	/* DMA 0 */
	0x11060,	/* DMA 1 */
	0x110c0,	/* MU/I2O */
	0x01120,	/* Timer 0 */
	0x01160,	/* Timer 1 */
	0x011a0,	/* Timer 2 */
	0x011e0,	/* Timer 3 */
	0x11120,	/* DUART 0, MPC8245 */
	0x11140,	/* DUART 1, MPC8245 */
};
