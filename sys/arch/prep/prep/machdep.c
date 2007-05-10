/*	$NetBSD: machdep.c,v 1.66.14.8 2007/05/10 15:46:09 garbled Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.66.14.8 2007/05/10 15:46:09 garbled Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_openpic.h"

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
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <net/netisr.h>

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/platform.h>
#include <machine/powerpc.h>
#include <machine/residual.h>
#include <machine/trap.h>

#include <powerpc/oea/bat.h>
#include <powerpc/openpic.h>
#include <arch/powerpc/pic/picvar.h>

#include <dev/cons.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "opt_interrupt.h"

void initppc(u_long, u_long, u_int, void *);
void dumpsys(void);
static void prep_init(void);
static void init_intr(void);

char bootinfo[BOOTINFO_MAXSIZE];
char bootpath[256];

vaddr_t prep_intr_reg;			/* PReP interrupt vector register */
uint32_t prep_intr_reg_off;		/* IVR offset within the mapped page */

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

paddr_t avail_end;			/* XXX temporary */
struct pic_ops *isa_pic;
int isa_pcmciamask = 0x8b28;

extern int primary_pic;
extern struct platform_quirkdata platform_quirks[];

RESIDUAL *res;
RESIDUAL resdata;

void
initppc(u_long startkernel, u_long endkernel, u_int args, void *btinfo)
{

	/*
	 * copy bootinfo
	 */
	memcpy(bootinfo, btinfo, sizeof(bootinfo));

	/*
	 * copy residual data
	 */
	{
		struct btinfo_residual *resinfo;

		resinfo =
		    (struct btinfo_residual *)lookup_bootinfo(BTINFO_RESIDUAL);
		if (!resinfo)
			panic("not found residual information in bootinfo");

		if (((RESIDUAL *)resinfo->addr != 0) &&
		    ((RESIDUAL *)resinfo->addr)->ResidualLength != 0) {
			memcpy(&resdata, resinfo->addr, sizeof(resdata));
			res = &resdata;
		} else
			panic("No residual data.");
	}
	aprint_normal("got residual data\n");

	/*
	 * Set memory region
	 */
	{
		u_long memsize = res->TotalMemory;

		physmemr[0].start = 0;
		physmemr[0].size = memsize & ~PGOFSET;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = memsize - availmemr[0].start;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Set CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		extern u_long ticks_per_sec, ns_per_tick;

		clockinfo =
		    (struct btinfo_clock *)lookup_bootinfo(BTINFO_CLOCK);
		if (!clockinfo)
			panic("not found clock information in bootinfo");

		ticks_per_sec = clockinfo->ticks_per_sec;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	prep_initppc(startkernel, endkernel, args);
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	/*
	 * Do common startup.
	 */
	oea_startup(res->VitalProductData.PrintableModel);

	/*
	 * General prep setup using pnp residual. Also provides for
	 * external interrupt handler install
	 */
	prep_init();

	/*
	 * Initialize soft interrupt framework.
	 */
	softintr__init();

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

	/*
	 * Gather the pci interrupt routings.
         */
	setup_pciroutinginfo();
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
		(size_t)help < (size_t)bootinfo + sizeof (bootinfo));

	return (NULL);
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

	if (howto & RB_HALT) {
                printf("\n");
                printf("The operating system has halted.\n");
                printf("Please press any key to reboot.\n\n");
                cnpollc(1);	/* for proper keyboard command handling */
                cngetc();
                cnpollc(0);
	}

	printf("rebooting...\n\n");

	reset_prep();

	for (;;)
		continue;
	/* NOTREACHED */
}

struct powerpc_bus_space prep_eisa_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_offset = 0x80000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0x0000f000,
};

struct powerpc_bus_space prep_eisa_mem_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_offset = 0xC0000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0x3f000000,
};

#if defined(PIC_OPENPIC)

static int
prep_setup_openpic(PPC_DEVICE *dev)
{
	uint32_t l;
	uint8_t *p;
	void *v;
	int tag, size, item;
	unsigned char *baseaddr = NULL;

	l = be32toh(dev->AllocatedOffset);
	p = res->DevicePnPHeap + l;

	/* look for the large vendor item that describes the MPIC's memory
	 * range */
	for (; p[0] != END_TAG; p += size) {
		struct _L4_Pack *pack = (void *)p;
		struct _L4_PPCPack *pa = &pack->L4_Data.L4_PPCPack;

		tag = *p;
		v = p;
		if (tag_type(p[0]) == PNP_SMALL) {
			size = tag_small_count(tag) + 1;
			continue;
		}
		size = (p[1] | (p[2] << 8)) + 3 /* tag + length */;
		item = tag_large_item_name(tag);
		if (item != LargeVendorItem || pa->Type != LV_GenericAddress)
			continue;
		/* otherwise, we have a memory packet */
		if (pa->PPCData[0] == 1)
			baseaddr = (unsigned char *)mapiodev(
			    le64dec(&pa->PPCData[4]) | PREP_BUS_SPACE_IO,
			    le64dec(&pa->PPCData[12]));
		else if (pa->PPCData[0] == 2)
			baseaddr = (unsigned char *)mapiodev(
			    le64dec(&pa->PPCData[4]) | PREP_BUS_SPACE_MEM,
			    le64dec(&pa->PPCData[12]));
		if (baseaddr == NULL)
			return 0;
		pic_init();
		isa_pic = setup_prepivr();
		(void)setup_openpic(baseaddr, 0);
		primary_pic = 1;
		/* set up the IVR as a cascade on openpic 0 */
		intr_establish(16, IST_LEVEL, IPL_NONE, pic_handle_intr,
		    isa_pic);
		oea_install_extint(pic_ext_intr);
		return 1;
	}
	return 0;
}

#endif /* PIC_OPENPIC */

/*
 * Locate and setup the isa_ivr.
 */

static void
setup_ivr(PPC_DEVICE *dev)
{
	uint32_t l, addr;
	uint8_t *p;
	void *v;
	int tag, size, item;

	l = be32toh(dev->AllocatedOffset);
	p = res->DevicePnPHeap + l;

	/* Find the IVR vector's Generic Address in a LVI */
	for (; p[0] != END_TAG; p += size) {
		struct _L4_Pack *pack = (void *)p;
		struct _L4_PPCPack *pa = &pack->L4_Data.L4_PPCPack;

		tag = *p;
		v = p;
		if (tag_type(p[0]) == PNP_SMALL) {
			size = tag_small_count(tag) + 1;
			continue;
		}
		size = (p[1] | (p[2] << 8)) + 3 /* tag + length */;
		item = tag_large_item_name(tag);
		if (item != LargeVendorItem || pa->Type != LV_GenericAddress)
			continue;
		/* otherwise we have a memory packet */
		addr = le64dec(&pa->PPCData[4]) & ~(PAGE_SIZE-1);
		prep_intr_reg_off = le64dec(&pa->PPCData[4]) & (PAGE_SIZE-1); 
		prep_intr_reg = (vaddr_t)mapiodev(addr, PAGE_SIZE);
		if (!prep_intr_reg)
			panic("startup: no room for interrupt register");
		return;
	}
}

/*
 * There are a few things that need setting up early on in the prep 
 * architecture.  Foremost of these is the MPIC (if present) and the
 * l2 cache controller.  This is a cut-down version of pnpbus_search()
 * that looks for specific devices, and sets them up accordingly.
 * This should also look for and wire up the interrupt vector.
 */

static void
prep_init()
{
	PPC_DEVICE *ppc_dev;
	int i, foundmpic;
	uint32_t ndev;

	ndev = be32toh(res->ActualNumDevices);
	ppc_dev = res->Devices;
	foundmpic = 0;
	prep_intr_reg = 0;

	for (i = 0; i < ((ndev > MAX_DEVICES) ? MAX_DEVICES : ndev); i++) {
		if (ppc_dev[i].DeviceId.DevId == 0x41d00000) /* ISA_PIC */
			setup_ivr(&ppc_dev[i]);
#if defined(PIC_OPENPIC)
		if (ppc_dev[i].DeviceId.DevId == 0x244d000d) { /* MPIC */
			foundmpic = prep_setup_openpic(&ppc_dev[i]);
		}
#else
		;
#endif

	}
	if (!prep_intr_reg) {
		/*
		 * For some reason we never found one, this is known to
		 * occur on certain motorola VME boards.  Instead we need
		 * to just hardcode it.
		 */
		prep_intr_reg = (vaddr_t) mapiodev(PREP_INTR_REG, PAGE_SIZE);
		if (!prep_intr_reg)
			panic("startup: no room for interrupt register");
		prep_intr_reg_off = INTR_VECTOR_REG;
	}
	if (!foundmpic)
		init_intr();
}

static void
init_intr(void)
{
        int i;
        openpic_base = 0;

	pic_init();
        i = find_platform_quirk(res->VitalProductData.PrintableModel);
        if (i != -1)
                if (platform_quirks[i].quirk & PLAT_QUIRK_ISA_HANDLER &&
                    platform_quirks[i].isa_intr_handler == EXT_INTR_I8259) {
			isa_pic = setup_i8259();
                        return;
                }
	isa_pic = setup_prepivr();
        oea_install_extint(pic_ext_intr);
}
