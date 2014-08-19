/*	$NetBSD: machdep.c,v 1.29.12.2 2014/08/20 00:02:59 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.29.12.2 2014/08/20 00:02:59 tls Exp $");

#include "opt_marvell.h"
#include "opt_modular.h"
#include "opt_ev64260.h"
#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_ns.h"
#include "opt_ipkdb.h"

#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

#include <machine/powerpc.h>

#include <powerpc/db_machdep.h>
#include <powerpc/pmap.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pic/picvar.h>
#include <powerpc/pio.h>

#include <ddb/db_extern.h>

#include <dev/cons.h>

#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>

#include "gtmpsc.h"
#if (NGTMPSC > 0)
#include <dev/marvell/gtbrgreg.h>
#include <dev/marvell/gtsdmareg.h>
#include <dev/marvell/gtmpscreg.h>
#include <dev/marvell/gtmpscvar.h>
#endif

#include "ksyms.h"
#include "locators.h"


/*
 * Global variables used here and there
 */
#define	PMONMEMREGIONS	32
struct mem_region physmemr[PMONMEMREGIONS], availmemr[PMONMEMREGIONS];

void initppc(u_int, u_int, u_int, void *); /* Called from locore */
static void gt_bus_space_init(void);
static inline void gt_record_memory(int, paddr_t, paddr_t, paddr_t);
static void gt_find_memory(paddr_t);

bus_addr_t gt_base = 0;

extern int primary_pic;
struct pic_ops *discovery_pic;
struct pic_ops *discovery_gpp_pic[4];


struct powerpc_bus_space ev64260_pci0_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_pci0_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_pci1_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_pci1_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_obio0_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO0_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_obio1_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO1_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_obio2_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO2_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_obio3_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE|OBIO3_STRIDE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_bootcs_bs_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00000000,
};
struct powerpc_bus_space ev64260_gt_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, GT_SIZE,
};

struct powerpc_bus_space *ev64260_obio_bs_tags[5] = {
	&ev64260_obio0_bs_tag, &ev64260_obio1_bs_tag, &ev64260_obio2_bs_tag,
	&ev64260_obio3_bs_tag, &ev64260_bootcs_bs_tag
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

struct powerpc_bus_dma_tag ev64260_bus_dma_tag = {
        0,				/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};


void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{
	extern struct cfdata cfdata[];
	cfdata_t cf = &cfdata[0];

	/* Get mapped address of gt(System Controller) */
	while (cf->cf_name != NULL) {
		if (strcmp(cf->cf_name, "gt") == 0 &&
		    *cf->cf_loc != MAINBUSCF_ADDR_DEFAULT)
			break;
		cf++;
	}
	if (cf->cf_name == NULL)
		panic("where is gt?");
	gt_base = *cf->cf_loc;

	ev64260_gt_bs_tag.pbs_offset = gt_base;
	ev64260_gt_bs_tag.pbs_base = gt_base;
	ev64260_gt_bs_tag.pbs_limit += gt_base;
	oea_batinit(gt_base, BAT_BL_256M);

	oea_init(NULL);

	gt_bus_space_init();
	gt_find_memory(roundup(endkernel, PAGE_SIZE));

	consinit();

	/*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	{
		extern void *startsym, *endsym;
		ksyms_addsyms_elf((int)((u_int)endsym - (u_int)startsym),
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

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	register_t msr;

	oea_startup(NULL);

	pic_init();
	discovery_pic = setup_discovery_pic();
	primary_pic = 0;
	discovery_gpp_pic[0] = setup_discovery_gpp_pic(discovery_pic, 0);
	discovery_gpp_pic[1] = setup_discovery_gpp_pic(discovery_pic, 8);
	discovery_gpp_pic[2] = setup_discovery_gpp_pic(discovery_pic, 16);
	discovery_gpp_pic[3] = setup_discovery_gpp_pic(discovery_pic, 24);
	/*
	 * GPP interrupts establishes later.
	 */

	oea_install_extint(pic_ext_intr);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * Now allow hardware interrupts.
	 */
	splraise(-1);
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
	const int brg = GTMPSC_CRR_BRG0;
	const int baud = 9600;
	uint32_t cr;

#if 1
	/*
	 * XXX HACK FIXME
	 * PMON output has not been flushed.  give him a chance
	 */
	DELAY(100000);  /* XXX */
#endif
	/* Setup MPSC Routing Registers */
	out32rb(gt_base + GTMPSC_MRR, GTMPSC_MRR_RES);
	cr = in32rb(gt_base + GTMPSC_RCRR);
	cr &= ~GTMPSC_CRR(MPSC_CONSOLE, GTMPSC_CRR_MASK);
	cr |= GTMPSC_CRR(MPSC_CONSOLE, brg);
	out32rb(gt_base + GTMPSC_RCRR, cr);
	out32rb(gt_base + GTMPSC_TCRR, cr);

	/* Setup Baud Rate Configuration Register of Baud Rate Generator */
	out32rb(gt_base + BRG_BCR(brg),
	    BRG_BCR_EN | GT_MPSC_CLOCK_SOURCE | compute_cdv(baud));

	gtmpsccnattach(&ev64260_gt_bs_tag, &ev64260_bus_dma_tag, gt_base,
	    MPSC_CONSOLE, brg, baud,
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#else
	/* PPCBOOT using COM1 @ 57600 */
	comcnattach(&gt_obio2_bs_tag, 0, 57600,
	    COM_FREQ*2, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#endif
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
		pmf_system_shutdown(boothowto);
		printf("halted\n\n");
		cnhalt();
		while(1);
	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
	doshutdownhooks();

	pmf_system_shutdown(boothowto);
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
	gt_watchdog_reset();
	/* NOTREACHED */
	while (1);
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}

static void
gt_bus_space_init(void)
{
	const struct gt_decode_info *di;
	uint32_t datal, datah;
	int bs, i;

	bs = 0;
	bus_space_init(&ev64260_gt_bs_tag, "gt",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

	for (i = 0, di = &decode_regs[4]; i < 5; i++, di++) {
		struct powerpc_bus_space *memt = ev64260_obio_bs_tags[i];

		datal = in32rb(gt_base + di->low_decode);
		datah = in32rb(gt_base + di->high_decode);

		if (GT_LowAddr_GET(datal) >= GT_HighAddr_GET(datal)) {
			ev64260_obio_bs_tags[i] = NULL;
			continue;
		}
		memt->pbs_offset = GT_LowAddr_GET(datal);
		memt->pbs_limit  = GT_HighAddr_GET(datah) + 1 -
		    memt->pbs_offset;

		bus_space_init(memt, "obio2",
		    ex_storage[bs], sizeof(ex_storage[bs]));
		bs++;
	}

	datal = in32rb(gt_base + GT_PCI0_Mem0_Low_Decode);
	datah = in32rb(gt_base + GT_PCI0_Mem0_High_Decode);
#if defined(GT_PCI0_MEMBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI0_MEMBASE >> 20);
	out32rb(gt_base + GT_PCI0_Mem0_Low_Decode, datal);
#endif
#if defined(GT_PCI0_MEMSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI0_MEMSIZE + GT_LowAddr_GET(datal) - 1) >> 20;
	out32rb(gt_base + GT_PCI0_Mem0_High_Decode, datal);
#endif
	ev64260_pci0_mem_bs_tag.pbs_base  = GT_LowAddr_GET(datal);
	ev64260_pci0_mem_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1;

	bus_space_init(&ev64260_pci0_mem_bs_tag, "pci0-mem",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

#if 1	/* XXXXXX */
	/*
	 * Make sure PCI0 Memory is BAT mapped.
	 */
	if (GT_LowAddr_GET(datal) < GT_HighAddr_GET(datal))
		oea_iobat_add(ev64260_pci0_mem_bs_tag.pbs_base & SEGMENT_MASK,
		    BAT_BL_256M);
#endif

	/*
	 * Make sure that I/O space start at 0.
	 */
	out32rb(gt_base + GT_PCI1_IO_Remap, 0);

	datal = in32rb(gt_base + GT_PCI0_IO_Low_Decode);
	datah = in32rb(gt_base + GT_PCI0_IO_High_Decode);
#if defined(GT_PCI0_IOBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI0_IOBASE >> 20);
	out32rb(gt_base + GT_PCI0_IO_Low_Decode, datal);
#endif
#if defined(GT_PCI0_IOSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI0_IOSIZE + GT_LowAddr_GET(datal) - 1) >> 20;
	out32rb(gt_base + GT_PCI0_IO_High_Decode, datal);
#endif
	ev64260_pci0_io_bs_tag.pbs_offset = GT_LowAddr_GET(datal);
	ev64260_pci0_io_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1 -
	    ev64260_pci0_io_bs_tag.pbs_offset;

	bus_space_init(&ev64260_pci0_io_bs_tag, "pci0-ioport",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

	datal = in32rb(gt_base + GT_PCI1_Mem0_Low_Decode);
	datah = in32rb(gt_base + GT_PCI1_Mem0_High_Decode);
#if defined(GT_PCI1_MEMBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI1_MEMBASE >> 20);
	out32rb(gt_base + GT_PCI1_Mem0_Low_Decode, datal);
#endif
#if defined(GT_PCI1_MEMSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI1_MEMSIZE + GT_LowAddr_GET(datal) - 1) >> 20;
	out32rb(gt_base + GT_PCI1_Mem0_High_Decode, datal);
#endif
	ev64260_pci1_mem_bs_tag.pbs_base  = GT_LowAddr_GET(datal);
	ev64260_pci1_mem_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1;

	bus_space_init(&ev64260_pci1_mem_bs_tag, "pci1-mem",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;

#if 1	/* XXXXXX */
	/*
	 * Make sure PCI1 Memory is BAT mapped.
	 */
	if (GT_LowAddr_GET(datal) < GT_HighAddr_GET(datal))
		oea_iobat_add(ev64260_pci1_mem_bs_tag.pbs_base & SEGMENT_MASK,
		    BAT_BL_256M);
#endif

	/*
	 * Make sure that I/O space start at 0.
	 */
	out32rb(gt_base + GT_PCI1_IO_Remap, 0);

	datal = in32rb(gt_base + GT_PCI1_IO_Low_Decode);
	datah = in32rb(gt_base + GT_PCI1_IO_High_Decode);
#if defined(GT_PCI1_IOBASE)
	datal &= ~0xfff;
	datal |= (GT_PCI1_IOBASE >> 20);
	out32rb(gt_base + GT_PCI1_IO_Low_Decode, datal);
#endif
#if defined(GT_PCI1_IOSIZE)
	datah &= ~0xfff;
	datah |= (GT_PCI1_IOSIZE + GT_LowAddr_GET(datal) - 1) >> 20;
	out32rb(gt_base + GT_PCI1_IO_High_Decode, datal);
#endif
	ev64260_pci1_io_bs_tag.pbs_offset = GT_LowAddr_GET(datal);
	ev64260_pci1_io_bs_tag.pbs_limit = GT_HighAddr_GET(datah) + 1 -
	    ev64260_pci1_io_bs_tag.pbs_offset;

	bus_space_init(&ev64260_pci1_io_bs_tag, "pci1-ioport",
	    ex_storage[bs], sizeof(ex_storage[bs]));
	bs++;
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

static void
gt_find_memory(paddr_t endkernel)
{
	paddr_t start = ~0, end = 0;
	const struct gt_decode_info *di;
	int i, j = 0, first = 1;

	/*
	 * Round kernel end to a page boundary.
	 */
	for (i = 0; i < 4; i++) {
		paddr_t nstart, nend;

		di = &decode_regs[i];
		nstart = GT_LowAddr_GET(in32rb(gt_base + di->low_decode));
		nend = GT_HighAddr_GET(in32rb(gt_base + di->high_decode)) + 1;
		if (nstart >= nend)
			continue;
		if (first) {
			/*
			 * First entry?  Just remember it.
			 */
			start = nstart;
			end = nend;
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
