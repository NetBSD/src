/* $NetBSD: machdep.c,v 1.4.2.4 2024/10/26 15:28:55 martin Exp $ */

/*
 * Copyright (c) 2002, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.4.2.4 2024/10/26 15:28:55 martin Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_inet.h"
#include "opt_ns.h"
#include "opt_oea.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/kgdb.h>
#include <sys/ksyms.h>
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

#include <machine/powerpc.h>
#include <machine/wii.h>

#include <powerpc/bus_funcs.h>
#include <powerpc/db_machdep.h>
#include <powerpc/pio.h>
#include <powerpc/pmap.h>
#include <powerpc/spr.h>
#include <powerpc/trap.h>

#include <powerpc/oea/bat.h>
#include <powerpc/oea/spr.h>
#include <powerpc/pic/picvar.h>

#include <ddb/db_extern.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/usb/ukbdvar.h>

#include "ksyms.h"
#include "ukbd.h"

#ifndef WII_DEFAULT_CMDLINE
#define WII_DEFAULT_CMDLINE "root=ld0a"
#endif

#define IBM750CL_SPR_HID4	1011
#define	 L2_CCFI		0x00100000	/* L2 complete castout prior
						 * to L2 flash invalidate.
						 */

#define MINI_MEM2_START		0x13f00000	/* Start of reserved MEM2 for MINI */

extern u_int l2cr_config;

struct powerpc_bus_space wii_mem_tag = {
	.pbs_flags = _BUS_SPACE_BIG_ENDIAN |
		     _BUS_SPACE_MEM_TYPE,
	.pbs_offset = 0,
	.pbs_base = 0x0c000000,
	.pbs_limit = 0x0dffffff,
	.pbs_extent = NULL,
};

static char ex_storage[1][EXTENT_FIXED_STORAGE_SIZE(EXTMAP_RANGES)]
    __attribute__((aligned(8)));

static bus_addr_t
wii_dma_phys_to_bus_mem(bus_dma_tag_t t, bus_addr_t addr)
{
	return addr;
}

static bus_addr_t
wii_dma_bus_mem_to_phys(bus_dma_tag_t t, bus_addr_t addr)
{
	return addr;
}

static int
wii_mem2_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	struct mem_region *mem, *avail;

	/* Restrict memory used for DMA to ranges in MEM2 */
	mem_regions(&mem, &avail);
	if (mem[1].size == 0) {
		return ENOMEM;
	}

	return _bus_dmamem_alloc_range(t, size, alignment, boundary, segs,      
	    nsegs, rsegs, flags, mem[1].start,
	    mem[1].start + mem[1].size - PAGE_SIZE - 1);
}

struct powerpc_bus_dma_tag wii_bus_dma_tag = {
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
	wii_dma_phys_to_bus_mem,
	wii_dma_bus_mem_to_phys,
};

struct powerpc_bus_dma_tag wii_mem2_bus_dma_tag = {
	0,				/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	wii_mem2_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	wii_dma_phys_to_bus_mem,
	wii_dma_bus_mem_to_phys,
};


/*
 * Global variables used here and there
 */
struct mem_region physmemr[3], availmemr[3];
char wii_cmdline[1024];

void initppc(u_int, u_int, u_int, void *); /* Called from locore */
void wii_dolphin_elf_loader_id(void);

static void wii_setup(void);
static void init_decrementer(void);

void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{
	extern uint32_t ticks_per_sec;
	extern uint32_t ticks_per_msec;
	extern unsigned char edata[], end[];
	extern struct wii_argv wii_argv;
	uint32_t mem2_start, mem2_end;
	register_t scratch;

	memset(&edata, 0, end - edata); /* clear BSS */

	wii_cmdline[0] = '\0';
	if (wii_argv.magic == WII_ARGV_MAGIC) {
		void *ptr = (void *)(uintptr_t)(wii_argv.cmdline & ~0x80000000);
		if (ptr != NULL) {
			memcpy(wii_cmdline, ptr, wii_argv.length);
		}
	} else {
		snprintf(wii_cmdline, sizeof(wii_cmdline), WII_DEFAULT_CMDLINE);
	}

	mem2_start = in32(GLOBAL_MEM2_AVAIL_START) & ~0x80000000;
	mem2_end = in32(GLOBAL_MEM2_AVAIL_END) & ~0x80000000;
	if (mem2_start < WII_MEM2_BASE) {
		/* Must have been booted from MINI. */
		mem2_start = WII_MEM2_BASE + DSP_MEM_SIZE;
		mem2_end = MINI_MEM2_START;
	}
	/*
	 * Clear GLOBAL_MEM2_AVAIL_{START,END} so we can detect the correct
	 * memory size when soft resetting from IOS to MINI.
	 */
	out32(GLOBAL_MEM2_AVAIL_START, 0);
	out32(GLOBAL_MEM2_AVAIL_END, 0);

	/* MEM1 24MB 1T-SRAM */
	physmemr[0].start = WII_MEM1_BASE;
	physmemr[0].size = WII_MEM1_SIZE;

	/* MEM2 64MB GDDR3 */
	physmemr[1].start = WII_MEM2_BASE;
	physmemr[1].size = WII_MEM2_SIZE;

	physmemr[2].size = 0;

	/* MEM1 available memory */
	availmemr[0].start = ((endkernel & ~0x80000000) + PGOFSET) & ~PGOFSET;
	availmemr[0].size = physmemr[0].size - availmemr[0].start;
	/* External framebuffer is at the end of MEM1 */
	availmemr[0].size -= XFB_SIZE;

	/* MEM2 available memory */
	availmemr[1].start = mem2_start;
	availmemr[1].size = mem2_end - mem2_start;

	availmemr[2].size = 0;

#ifdef BOOTHOWTO
	/*
	 * boothowto
	 */
	boothowto = BOOTHOWTO;
#endif

	/* HID4[L2_CCFI] must be set to 1 for correct operation of L2 cache */
	mtspr(IBM750CL_SPR_HID4, mfspr(IBM750CL_SPR_HID4) | L2_CCFI);

	/* Configure L2 cache */
	l2cr_config = L2CR_L2E | L2CR_L2PE;

	if (bus_space_init(&wii_mem_tag, "iomem",
			   ex_storage[0], sizeof(ex_storage[0]))) {
		panic("bus_space_init failed");
	}

	/*
	 * Initialize the BAT registers
	 */
	oea_batinit(
	    WII_IOMEM_BASE, BAT_BL_32M,
	    0);

	/*
	 * Set up trap vectors
	 */
	oea_init(NULL);

	/*
	 * Get CPU clock
	 */
	ticks_per_sec = TIMEBASE_FREQ_HZ;
	ticks_per_msec = ticks_per_sec / 1000;
	cpu_timebase = ticks_per_sec;

	wii_setup();

	uvm_md_init();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

	/* Now enable translation (and machine checks/recoverable interrupts) */
	__asm __volatile ("sync; mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
			  : "=r"(scratch)
			  : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	/*
	 * Setup decrementer
	 */
	init_decrementer();
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
	extern void pi_init_intr(void);

	oea_startup(NULL);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/* Set up interrupt controller */
	pic_init();
	pi_init_intr();
	oea_install_extint(pic_ext_intr);
}

/*
 * No early console support.
 */
void
consinit(void)
{
#if NUKBD > 0
	ukbd_cnattach();
#endif
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	extern void disable_intr(void);

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (!cold && (howto & RB_DUMP)) {
		oea_dumpsys();
	}
	pmf_system_shutdown(boothowto);
	doshutdownhooks();

	disable_intr();

	/* Force halt on panic to capture the cause on screen. */
	if (panicstr != NULL) {
		howto |= RB_HALT;
	}
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		printf("power off\n\n");
		out32(HW_GPIOB_OUT, in32(HW_GPIOB_OUT) | __BIT(GPIO_SHUTDOWN));
		delay(100000);
		printf("power off failed!\n\n");
	}
	if (howto & RB_HALT) {
		printf("halted\n\n");
		while (1);
	}

	printf("rebooting\n\n");
	out32(HW_RESETS, in32(HW_RESETS) & ~RSTBINB);
	while (1);
}

static void
wii_setup(void)
{
	/* Turn on the drive slot LED. */
	wii_slot_led(true);

	/* Enable PPC access to SHUTDOWN GPIO. */
	out32(HW_GPIO_OWNER, in32(HW_GPIO_OWNER) | __BIT(GPIO_SHUTDOWN));

	/* Enable PPC access to EXI bus. */
	out32(HW_AIPPROT, in32(HW_AIPPROT) | ENAHBIOPI);
}

static void
init_decrementer(void)
{
	extern uint32_t ns_per_tick;
	extern uint32_t ticks_per_intr;
	extern uint32_t ticks_per_sec;
	int scratch, msr;

	KASSERT(ticks_per_sec != 0);

	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
			: "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	ns_per_tick = 1000000000 / ticks_per_sec;
	ticks_per_intr = ticks_per_sec / hz;
	cpu_timebase = ticks_per_sec;

	curcpu()->ci_lasttb = mftbl();

	mtspr(SPR_DEC, ticks_per_intr);
	mtmsr(msr);
}
