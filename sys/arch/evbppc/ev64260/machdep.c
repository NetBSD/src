/*	$NetBSD: machdep.c,v 1.7 2003/03/18 19:33:50 matt Exp $	*/

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

#include "opt_marvell.h"
#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/extent.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/bus.h>
#include <machine/db_machdep.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <powerpc/oea/bat.h>
#include <powerpc/marvell/watchdog.h>

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
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>

#include "gtmpsc.h"
#if (NGTMPSC > 0)
#include <dev/marvell/gtsdmareg.h>
#include <dev/marvell/gtmpscreg.h>
#include <dev/marvell/gtmpscvar.h>
#endif

/*
 * Global variables used here and there
 */
extern struct user *proc0paddr;

#define	PMONMEMREGIONS	32
struct mem_region physmemr[PMONMEMREGIONS], availmemr[PMONMEMREGIONS];

char *bootpath;

paddr_t avail_end;			/* XXX temporary */

void initppc(u_int, u_int, u_int, void *); /* Called from locore */
void strayintr(int);
int lcsplx(int);
void gt_bus_space_init(void);
void return_to_dink(int);
void calc_delayconst(void);

struct powerpc_bus_space gt_pci0_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_pci0_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_pci1_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_pci1_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_obio2_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|2,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	GT_BASE, 0x00000000, 0x00010000,
};

bus_space_handle_t gt_memh;

bus_space_tag_t obio_bs_tags[5] = {
	NULL, NULL, &gt_obio2_bs_tag, NULL, NULL
};

static char ex_storage[6][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

#if 0
cons_decl(gtmpsc);

struct consdev constab[] = {
	cons_init_halt(gtmpsc),
	{ 0 }
};
#endif

void
initppc(startkernel, endkernel, args, btinfo)
	u_int startkernel, endkernel, args;
	void *btinfo;
{
#ifdef DDB
	extern void *startsym, *endsym;
#endif

	/*
	 * Hardcode 32MB for now--we should probe for this or get it
	 * from a boot loader, but for now, we are booting via an
	 * S-record loader.
	 */
	{	/* XXX AKB */
		u_int32_t	physmemsize;

		physmemsize = 92 * 1024 * 1024;
		physmemr[0].start = 0;
		physmemr[0].size = physmemsize;
		physmemr[1].size = 0;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = physmemsize - availmemr[0].start;
		availmemr[1].size = 0;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	oea_batinit(0xf0000000, BAT_BL_256M);
	oea_init((void (*)(void))ext_intr);

	gt_bus_space_init();

	calc_delayconst();			/* Set CPU clock */

	consinit();

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
	register_t msr;

	oea_startup(NULL);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

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
consinit()
{
#ifdef MPSC_CONSOLE
	/* PMON using MPSC0 @ 9600 */
	gtmpsccnattach(&gt_mem_bs_tag, gt_memh, 0, 9600,
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#else
	/* PPCBOOT using COM1 @ 57600 */
	comcnattach(&gt_obio2_bs_tag, 0, 57600, COM_FREQ*2, 
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#endif
}

#if (NPCKBC > 0) && (NPCKBD == 0)
/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbc_machdep_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
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
cpu_reboot(howto, what)
	int howto;
	char *what;
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
		cnhalt();
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
#if 0
	{
		void mvpppc_reboot(void);
		mvpppc_reboot();
	}
#endif
	gt_watchdog_reset();
	/* NOTREACHED */
	while (1);
}

int
lcsplx(ipl)
	int ipl;
{
	return spllower(ipl);
}

int
gtget_macaddr(struct gt_softc *gt, int macno, char *enaddr)
{
	enaddr[0] = 0x02;
	enaddr[1] = 0x00;
	enaddr[2] = 0x04;
	enaddr[3] = 0x00;
	enaddr[4] = 0x00;
	enaddr[5] = 0x04 + macno;

	return 0;
}

void
gt_bus_space_init(void)
{
	bus_space_tag_t gt_memt = &gt_mem_bs_tag;
	uint32_t datal, datah;
	int error;

	error = bus_space_init(&gt_mem_bs_tag, "gtmem",
	    ex_storage[0], sizeof(ex_storage[0]));


	error = bus_space_map(gt_memt, 0, 4096, 0, &gt_memh);

	datal = bus_space_read_4(gt_memt, gt_memh, GT_CS2_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_CS2_High_Decode);
	gt_obio2_bs_tag.pbs_offset = GT_LowAddr_GET(datal);
	gt_obio2_bs_tag.pbs_limit  = GT_HighAddr_GET(datah) + 1 -
	    gt_obio2_bs_tag.pbs_offset;

	error = bus_space_init(&gt_obio2_bs_tag, "obio2",
	    ex_storage[1], sizeof(ex_storage[1]));

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_Mem0_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_Mem0_High_Decode);
	gt_pci0_mem_bs_tag.pbs_base  = GT_LowAddr_GET(datal);
	gt_pci0_mem_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1;

	error = bus_space_init(&gt_pci0_mem_bs_tag, "pci0-mem",
	    ex_storage[2], sizeof(ex_storage[2]));

	/*
	 * Make sure PCI0 Memory is BAT mapped.
	 */
	oea_iobat_add(gt_pci0_mem_bs_tag.pbs_base & SEGMENT_MASK, BAT_BL_256M);

	/*
	 * Make sure that I/O space start at 0.
	 */
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_IO_Remap, 0);

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_IO_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_IO_High_Decode);
	gt_pci0_io_bs_tag.pbs_offset  = GT_LowAddr_GET(datal);
	gt_pci0_io_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1 -
	    gt_pci0_io_bs_tag.pbs_offset;

	error = bus_space_init(&gt_pci0_io_bs_tag, "pci0-ioport",
	    ex_storage[3], sizeof(ex_storage[3]));

#if 0
	error = extent_alloc_region(gt_pci0_io_bs_tag.pbs_extent,
	    0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("gt_bus_space_init: can't block out reserved "
		    "I/O space 0x10000-0x7fffff: error=%d\n", error);
#endif

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_Mem0_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_Mem0_High_Decode);
	gt_pci1_mem_bs_tag.pbs_base  = GT_LowAddr_GET(datal);
	gt_pci1_mem_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1;

	error = bus_space_init(&gt_pci1_mem_bs_tag, "pci1-mem",
	    ex_storage[4], sizeof(ex_storage[4]));

	/*
	 * Make sure PCI1 Memory is BAT mapped.
	 */
	oea_iobat_add(gt_pci1_mem_bs_tag.pbs_base & SEGMENT_MASK, BAT_BL_256M);

	/*
	 * Make sure that I/O space start at 0.
	 */
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_IO_Remap, 0);

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_IO_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_IO_High_Decode);
	gt_pci1_io_bs_tag.pbs_offset = GT_LowAddr_GET(datal);
	gt_pci1_io_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1 -
	    gt_pci1_io_bs_tag.pbs_offset;

	error = bus_space_init(&gt_pci1_io_bs_tag, "pci1-ioport",
	    ex_storage[5], sizeof(ex_storage[5]));

#if 0
	error = extent_alloc_region(gt_pci1_io_bs_tag.pbs_extent,
	     0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("gt_bus_space_init: can't block out reserved "
		    "I/O space 0x10000-0x7fffff: error=%d\n", error);
#endif
}
