/*	$NetBSD: machdep.c,v 1.19 2005/12/24 20:07:03 perry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.19 2005/12/24 20:07:03 perry Exp $");

#include "opt_marvell.h"
#include "opt_ev64260.h"
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

#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtethreg.h>

#include "gtmpsc.h"
#if (NGTMPSC > 0)
#include <dev/marvell/gtsdmareg.h>
#include <dev/marvell/gtmpscreg.h>
#include <dev/marvell/gtmpscvar.h>
#endif

#include "ksyms.h"

/*
 * Global variables used here and there
 */
extern struct user *proc0paddr;

#define	PMONMEMREGIONS	32
struct mem_region physmemr[PMONMEMREGIONS], availmemr[PMONMEMREGIONS];

char *bootpath;

void initppc(u_int, u_int, u_int, void *); /* Called from locore */
void strayintr(int);
int lcsplx(int);
void gt_bus_space_init(void);
void gt_find_memory(bus_space_tag_t, bus_space_handle_t, paddr_t);
void gt_halt(bus_space_tag_t, bus_space_handle_t);
void return_to_dink(int);
void calc_delayconst(void);

void kcomcnputs(dev_t, const char *);

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
struct powerpc_bus_space gt_obio0_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO0_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_obio1_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO1_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_obio2_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO2_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_obio3_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO3_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_bootcs_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space gt_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	GT_BASE, 0x00000000, 0x00010000,
};

bus_space_handle_t gt_memh;

struct powerpc_bus_space *obio_bs_tags[5] = {
	&gt_obio0_bs_tag, &gt_obio1_bs_tag, &gt_obio2_bs_tag,
	&gt_obio3_bs_tag, &gt_bootcs_bs_tag
};

static char ex_storage[10][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

const struct gt_decode_info {
	bus_addr_t low_decode;
	bus_addr_t high_decode;
} decode_regs[] = {
    {	GT_SCS0_Low_Decode,	GT_SCS0_High_Decode },
    {	GT_SCS1_Low_Decode,	GT_SCS1_High_Decode },
    {	GT_SCS2_Low_Decode,	GT_SCS2_High_Decode },
    {	GT_SCS3_Low_Decode,	GT_SCS3_High_Decode },
    {	GT_CS0_Low_Decode,	GT_CS0_High_Decode },
    {	GT_CS1_Low_Decode,	GT_CS1_High_Decode },
    {	GT_CS2_Low_Decode,	GT_CS2_High_Decode },
    {	GT_CS3_Low_Decode,	GT_CS3_High_Decode },
    {	GT_BootCS_Low_Decode,	GT_BootCS_High_Decode },
};

void
initppc(startkernel, endkernel, args, btinfo)
	u_int startkernel, endkernel, args;
	void *btinfo;
{
	oea_batinit(0xf0000000, BAT_BL_256M);
	oea_init((void (*)(void))ext_intr);

	calc_delayconst();			/* Set CPU clock */

	DELAY(100000);

	gt_bus_space_init();
	gt_find_memory(&gt_mem_bs_tag, gt_memh, roundup(endkernel, PAGE_SIZE));
	gt_halt(&gt_mem_bs_tag, gt_memh);

	/*
	 * Now that we known how much memory, reinit the bats.
	 */
	oea_batinit(0xf0000000, BAT_BL_256M);

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

#if NKSYMS || defined(DDB) || defined(LKM)
	{
		extern void *startsym, *endsym;
		ksyms_init((int)((u_int)endsym - (u_int)startsym),
		    startsym, endsym);
	}
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

static inline void
gt_record_memory(int j, paddr_t start, paddr_t end, paddr_t endkernel)
{
	physmemr[j].start = start;
	physmemr[j].size = end - start;
	if (start < endkernel)
		start = endkernel;
	availmemr[j].start = start;
	availmemr[j].size = end - start;
}

void
gt_find_memory(bus_space_tag_t memt, bus_space_handle_t memh,
	paddr_t endkernel)
{
	paddr_t start = ~0, end = 0;
	int i, j = 0, first = 1;

	/*
	 * Round kernel end to a page boundary.
	 */
	for (i = 0; i < 4; i++) {
		paddr_t nstart, nend;
		nstart = GT_LowAddr_GET(bus_space_read_4(&gt_mem_bs_tag,
		    gt_memh, decode_regs[i].low_decode));
		nend = GT_HighAddr_GET(bus_space_read_4(&gt_mem_bs_tag,
		    gt_memh, decode_regs[i].high_decode)) + 1;
		if (nstart >= nend)
			continue;
		if (first) {
			/*
			 * First entry?  Just remember it.
			 */
			start = nstart;
			end  = nend;
			first = 0;
		} else if (nstart == end) {
			/*
			 * Contiguous?  Just update the end.
			 */
			end = nend;
		} else {
			/*
			 * Disjoint?  record it.
			 */
			gt_record_memory(j, start, end, endkernel);
			start = nstart;
			end = nend;
			j++;
		}
	}
	gt_record_memory(j, start, end, endkernel);
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
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
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
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
#ifdef MPSC_CONSOLE
	/* PMON using MPSC0 @ 9600 */
	gtmpsccnattach(&gt_mem_bs_tag, gt_memh, MPSC_CONSOLE, 9600,
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#else
	/* PPCBOOT using COM1 @ 57600 */
	comcnattach(&gt_obio2_bs_tag, 0, 57600,
	    COM_FREQ*2, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#endif
}

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
lcsplx(int ipl)
{
	return spllower(ipl);
}

void
gt_halt(bus_space_tag_t memt, bus_space_handle_t memh)
{
	int i;
	u_int32_t data;

	/*
	 * Shut down the MPSC ports
	 */
	for (i = 0; i < 2; i++) {
		bus_space_write_4(memt, memh,
		    SDMA_U_SDCM(i), SDMA_SDCM_AR|SDMA_SDCM_AT);
		for (;;) {
			data = bus_space_read_4(memt, memh,
			    SDMA_U_SDCM(i));
			if (((SDMA_SDCM_AR|SDMA_SDCM_AT) & data) == 0)
				break;
		}
	}

	/*
	 * Shut down the Ethernets
	 */
	for (i = 0; i < 3; i++) {
		bus_space_write_4(memt, memh,
		    ETH_ESDCMR(2), ETH_ESDCMR_AR|ETH_ESDCMR_AT);
		for (;;) {
			data = bus_space_read_4(memt, memh,
			    ETH_ESDCMR(i));
			if (((ETH_ESDCMR_AR|ETH_ESDCMR_AT) & data) == 0)
				break;
		}
		data = bus_space_read_4(memt, memh, ETH_EPCR(i));
		data &= ~ETH_EPCR_EN;
		bus_space_write_4(memt, memh, ETH_EPCR(i), data);
	}
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
	const struct gt_decode_info *di;
	uint32_t datal, datah;
	int error;
	int bs = 0;
	int j;

	error = bus_space_init(&gt_mem_bs_tag, "gtmem",
	    ex_storage[bs], sizeof(ex_storage[bs]));

	error = bus_space_map(gt_memt, 0, 0x10000, 0, &gt_memh);

	for (j = 0, di = &decode_regs[4]; j < 5; j++, di++) {
		struct powerpc_bus_space *memt = obio_bs_tags[j];
		datal = bus_space_read_4(gt_memt, gt_memh, di->low_decode);
		datah = bus_space_read_4(gt_memt, gt_memh, di->high_decode);

		if (GT_LowAddr_GET(datal) >= GT_HighAddr_GET(datal)) {
			obio_bs_tags[j] = NULL;
			continue;
		}
		memt->pbs_offset = GT_LowAddr_GET(datal);
		memt->pbs_limit  = GT_HighAddr_GET(datah) + 1 -
		    memt->pbs_offset;

		error = bus_space_init(memt, "obio2",
		    ex_storage[bs], sizeof(ex_storage[bs]));
		bs++;
	}

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_Mem0_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_Mem0_High_Decode);
#if defined(GT_PCI0_MEMBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI0_MEMBASE >> 20);
	bus_space_write_4(gt_memt, gt_memh, GT_PCI0_Mem0_Low_Decode, datal);
#endif
#if defined(GT_PCI0_MEMSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI0_MEMSIZE + GT_LowAddr_GET(datal) - 1)  >> 20;
	bus_space_write_4(gt_memt, gt_memh, GT_PCI0_Mem0_High_Decode, datal);
#endif
	gt_pci0_mem_bs_tag.pbs_base  = GT_LowAddr_GET(datal);
	gt_pci0_mem_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1;

	error = bus_space_init(&gt_pci0_mem_bs_tag, "pci0-mem",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

	/*
	 * Make sure PCI0 Memory is BAT mapped.
	 */
	if (GT_LowAddr_GET(datal) < GT_HighAddr_GET(datal))
		oea_iobat_add(gt_pci0_mem_bs_tag.pbs_base & SEGMENT_MASK, BAT_BL_256M);

	/*
	 * Make sure that I/O space start at 0.
	 */
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_IO_Remap, 0);

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_IO_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI0_IO_High_Decode);
#if defined(GT_PCI0_IOBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI0_IOBASE >> 20);
	bus_space_write_4(gt_memt, gt_memh, GT_PCI0_IO_Low_Decode, datal);
#endif
#if defined(GT_PCI0_IOSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI0_IOSIZE + GT_LowAddr_GET(datal) - 1)  >> 20;
	bus_space_write_4(gt_memt, gt_memh, GT_PCI0_IO_High_Decode, datal);
#endif
	gt_pci0_io_bs_tag.pbs_offset = GT_LowAddr_GET(datal);
	gt_pci0_io_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1 -
	    gt_pci0_io_bs_tag.pbs_offset;

	error = bus_space_init(&gt_pci0_io_bs_tag, "pci0-ioport",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

#if 0
	error = extent_alloc_region(gt_pci0_io_bs_tag.pbs_extent,
	    0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("gt_bus_space_init: can't block out reserved "
		    "I/O space 0x10000-0x7fffff: error=%d\n", error);
#endif

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_Mem0_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_Mem0_High_Decode);
#if defined(GT_PCI1_MEMBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI1_MEMBASE >> 20);
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_Mem0_Low_Decode, datal);
#endif
#if defined(GT_PCI1_MEMSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI1_MEMSIZE + GT_LowAddr_GET(datal) - 1)  >> 20;
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_Mem0_High_Decode, datal);
#endif
	gt_pci1_mem_bs_tag.pbs_base  = GT_LowAddr_GET(datal);
	gt_pci1_mem_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1;

	error = bus_space_init(&gt_pci1_mem_bs_tag, "pci1-mem",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

	/*
	 * Make sure PCI1 Memory is BAT mapped.
	 */
	if (GT_LowAddr_GET(datal) < GT_HighAddr_GET(datal))
		oea_iobat_add(gt_pci1_mem_bs_tag.pbs_base & SEGMENT_MASK, BAT_BL_256M);

	/*
	 * Make sure that I/O space start at 0.
	 */
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_IO_Remap, 0);

	datal = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_IO_Low_Decode);
	datah = bus_space_read_4(gt_memt, gt_memh, GT_PCI1_IO_High_Decode);
#if defined(GT_PCI1_IOBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI1_IOBASE >> 20);
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_IO_Low_Decode, datal);
#endif
#if defined(GT_PCI1_IOSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI1_IOSIZE + GT_LowAddr_GET(datal) - 1)  >> 20;
	bus_space_write_4(gt_memt, gt_memh, GT_PCI1_IO_High_Decode, datal);
#endif
	gt_pci1_io_bs_tag.pbs_offset = GT_LowAddr_GET(datal);
	gt_pci1_io_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1 -
	    gt_pci1_io_bs_tag.pbs_offset;

	error = bus_space_init(&gt_pci1_io_bs_tag, "pci1-ioport",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

#if 0
	error = extent_alloc_region(gt_pci1_io_bs_tag.pbs_extent,
	     0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("gt_bus_space_init: can't block out reserved "
		    "I/O space 0x10000-0x7fffff: error=%d\n", error);
#endif
}
