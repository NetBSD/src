/*	$NetBSD: rpi_machdep.c,v 1.3.2.5 2013/02/14 21:38:00 riz Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rpi_machdep.c,v 1.3.2.5 2013/02/14 21:38:00 riz Exp $");

#include "opt_evbarm_boardtype.h"

#include "sdhc.h"
#include "dotg.h"
#include "bcmspi.h"
#include "bsciic.h"
#include "plcom.h"
#include "genfb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/bus.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <prop/proplib.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/vmparam.h>
#include <machine/autoconf.h>
#include <machine/bootconfig.h>
#include <machine/pmap.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_pmvar.h>
#include <arm/broadcom/bcm2835_mbox.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcpm.h>
#include <evbarm/rpi/vcprop.h>

#include <evbarm/rpi/rpi.h>

#if NPLCOM > 0
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#if NGENFB > 0
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>
#endif

#include "ksyms.h"

static void setup_real_page_tables(void);
static void rpi_bootparams(void);
static void rpi_device_register(device_t, void *);

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */
u_int cpu_reset_address;

/* Define various stack sizes in pages */
#define FIQ_STACK_SIZE	1
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#define UND_STACK_SIZE	1

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_end;
u_int free_pages;
int physmem = 0;

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t fiqstack;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

extern char etext[];
extern char __data_start[], _edata[];
extern char __bss_start[], __bss_end__[];
extern char _end[];

extern int KERNEL_BASE_phys[];
extern int KERNEL_BASE_virt[];

#define KERNEL_PT_SYS		0  /* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1  /* Page table for mapping kernel */
#define KERNEL_PT_KERNEL_NUM	4

#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
/* Page tables for mapping kernel VM */
#define KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */

#define	KERN_VTOPDIFF	((vaddr_t)KERNEL_BASE_phys - (vaddr_t)KERNEL_BASE_virt)
#define KERN_VTOPHYS(va) ((paddr_t)((vaddr_t)va + KERN_VTOPDIFF))
#define KERN_PHYSTOV(pa) ((vaddr_t)((paddr_t)pa - KERN_VTOPDIFF))

#ifndef RPI_FB_WIDTH
#define RPI_FB_WIDTH	1280
#endif
#ifndef RPI_FB_HEIGHT
#define RPI_FB_HEIGHT	720
#endif

#define	PLCONADDR 0x20201000

#ifndef CONSDEVNAME
#define CONSDEVNAME "plcom"
#endif

#ifndef PLCONSPEED
#define PLCONSPEED B115200
#endif
#ifndef PLCONMODE
#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef PLCOMCNUNIT
#define PLCOMCNUNIT -1
#endif

#if (NPLCOM > 0)
static const bus_addr_t consaddr = (bus_addr_t)PLCONADDR;

int plcomcnspeed = PLCONSPEED;
int plcomcnmode = PLCONMODE;
#endif

#include "opt_kgdb.h"
#ifdef KGDB
#include <sys/kgdb.h>
#endif

void
cpu_reboot(int howto, char *bootstr)
{

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
		printf("rebooting...\r\n");
		bcm2835_system_reset();
	}

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
	}

	printf("rebooting...\r\n");
	bcm2835_system_reset();

	/*NOTREACHED*/
}

/* Smallest amount of RAM start.elf could give us. */
#define RPI_MINIMUM_SPLIT (128U * 1024 * 1024)

static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_fwrev		vbt_fwrev;
	struct vcprop_tag_boardmodel	vbt_boardmodel;
	struct vcprop_tag_boardrev	vbt_boardrev;
	struct vcprop_tag_macaddr	vbt_macaddr;
	struct vcprop_tag_memory	vbt_memory;
	struct vcprop_tag_boardserial	vbt_serial;
	struct vcprop_tag_cmdline	vbt_cmdline;
	struct vcprop_tag_clockrate	vbt_emmcclockrate;
	struct vcprop_tag_clockrate	vbt_armclockrate;
	struct vcprop_tag end;
} vb __packed __aligned(16) =
{
	.vb_hdr = {
		.vpb_len = sizeof(vb),
		.vpb_rcode = VCPROP_PROCESS_REQUEST,
	},
	.vbt_fwrev = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_FIRMWAREREV,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_fwrev),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_boardmodel = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_BOARDMODEL,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_boardmodel),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_boardrev = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_BOARDREVISION,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_boardrev),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_macaddr = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_MACADDRESS,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_macaddr),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_memory = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_ARMMEMORY,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_memory),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_serial = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_BOARDSERIAL,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_serial),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_cmdline = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CMDLINE,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_cmdline),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
	},
	.vbt_emmcclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_emmcclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_EMMC
	},
	.vbt_armclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_armclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_ARM
	},
	.end = {
		.vpt_tag = VCPROPTAG_NULL
	}
};

#if NGENFB > 0
static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_edidblock	vbt_edid;
	struct vcprop_tag end;
} vb_edid __packed __aligned(16) =
{
	.vb_hdr = {
		.vpb_len = sizeof(vb_edid),
		.vpb_rcode = VCPROP_PROCESS_REQUEST,
	},
	.vbt_edid = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_EDID_BLOCK,
			.vpt_len = VCPROPTAG_LEN(vb_edid.vbt_edid),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.blockno = 0,
	},
	.end = {
		.vpt_tag = VCPROPTAG_NULL
	}
};

static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_fbres		vbt_res;
	struct vcprop_tag_fbres		vbt_vres;
	struct vcprop_tag_fbdepth	vbt_depth;
	struct vcprop_tag_fbpixelorder	vbt_pixelorder;
	struct vcprop_tag_fbalpha	vbt_alpha;
	struct vcprop_tag_allocbuf	vbt_allocbuf;
	struct vcprop_tag_blankscreen	vbt_blank;
	struct vcprop_tag_fbpitch	vbt_pitch;
	struct vcprop_tag end;
} vb_setfb __packed __aligned(16) =
{
	.vb_hdr = {
		.vpb_len = sizeof(vb_setfb),
		.vpb_rcode = VCPROP_PROCESS_REQUEST,
	},
	.vbt_res = {
		.tag = {
			.vpt_tag = VCPROPTAG_SET_FB_RES,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_res),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.width = 0,
		.height = 0,
	},
	.vbt_vres = {
		.tag = {
			.vpt_tag = VCPROPTAG_SET_FB_VRES,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_vres),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.width = 0,
		.height = 0,
	},
	.vbt_depth = {
		.tag = {
			.vpt_tag = VCPROPTAG_SET_FB_DEPTH,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_depth),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.bpp = 32,
	},
	.vbt_pixelorder = {
		.tag = {
			.vpt_tag = VCPROPTAG_SET_FB_PIXEL_ORDER,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_pixelorder),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.state = VCPROP_PIXEL_BGR,
	},
	.vbt_alpha = {
		.tag = {
			.vpt_tag = VCPROPTAG_SET_FB_ALPHA_MODE,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_alpha),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.state = VCPROP_ALPHA_IGNORED,
	},
	.vbt_allocbuf = {
		.tag = {
			.vpt_tag = VCPROPTAG_ALLOCATE_BUFFER,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_allocbuf),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.address = PAGE_SIZE,	/* alignment */
	},
	.vbt_blank = {
		.tag = {
			.vpt_tag = VCPROPTAG_BLANK_SCREEN,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_blank),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
		.state = VCPROP_BLANK_OFF,
	},
	.vbt_pitch = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_FB_PITCH,
			.vpt_len = VCPROPTAG_LEN(vb_setfb.vbt_pitch),
			.vpt_rcode = VCPROPTAG_REQUEST,
		},
	},
	.end = {
		.vpt_tag = VCPROPTAG_NULL,
	},
};
#endif

static void
rpi_bootparams(void)
{
	bus_space_tag_t iot = &bcm2835_bs_tag;
	bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(BCM2835_ARMMBOX_BASE);
	uint32_t res;

	bcm2835_mbox_write(iot, ioh, BCMMBOX_CHANPM, (
#if (NSDHC > 0)
	    (1 << VCPM_POWER_SDCARD) | 
#endif
#if (NPLCOM > 0)
	    (1 << VCPM_POWER_UART0) |
#endif
#if (NDOTG > 0)
	    (1 << VCPM_POWER_USB) | 
#endif
#if (NBSCIIC > 0)
	    (1 << VCPM_POWER_I2C0) | (1 << VCPM_POWER_I2C1) | 
	/*  (1 << VCPM_POWER_I2C2) | */
#endif
#if (NBCMSPI > 0)
	    (1 << VCPM_POWER_SPI) | 
#endif
	    0) << 4);

	bcm2835_mbox_write(iot, ioh, BCMMBOX_CHANARM2VC, KERN_VTOPHYS(&vb));

	bcm2835_mbox_read(iot, ioh, BCMMBOX_CHANARM2VC, &res);

	/*
	 * No need to invalid the cache as the memory has never been referenced
	 * by the ARM.
	 *
	 * cpu_dcache_inv_range((vaddr_t)&vb, sizeof(vb));
	 *
	 */

	if (!vcprop_buffer_success_p(&vb.vb_hdr)) {
		bootconfig.dramblocks = 1;
		bootconfig.dram[0].address = 0x0;
		bootconfig.dram[0].pages = atop(RPI_MINIMUM_SPLIT);
		return;
	}

	struct vcprop_tag_memory *vptp_mem = &vb.vbt_memory;

	if (vcprop_tag_success_p(&vptp_mem->tag)) {
		size_t n = vcprop_tag_resplen(&vptp_mem->tag) /
		    sizeof(struct vcprop_memory);

    		bootconfig.dramblocks = 0;

		for (int i = 0; i < n && i < DRAM_BLOCKS; i++) {
			bootconfig.dram[i].address = vptp_mem->mem[i].base;
			bootconfig.dram[i].pages = atop(vptp_mem->mem[i].size);
			bootconfig.dramblocks++;
		}
	}

	if (vcprop_tag_success_p(&vb.vbt_armclockrate.tag))
		curcpu()->ci_data.cpu_cc_freq = vb.vbt_armclockrate.rate;

#ifdef VERBOSE_INIT_ARM
	if (vcprop_tag_success_p(&vb.vbt_fwrev.tag))
		printf("%s: firmware rev %x\n", __func__,
		    vb.vbt_fwrev.rev);
	if (vcprop_tag_success_p(&vb.vbt_macaddr.tag))
		printf("%s: mac-address  %llx\n", __func__,
		    vb.vbt_macaddr.addr);
	if (vcprop_tag_success_p(&vb.vbt_boardmodel.tag))
		printf("%s: board model  %x\n", __func__,
		    vb.vbt_boardmodel.model);
	if (vcprop_tag_success_p(&vb.vbt_boardrev.tag))
		printf("%s: board rev    %x\n", __func__,
		    vb.vbt_boardrev.rev);
	if (vcprop_tag_success_p(&vb.vbt_serial.tag))
		printf("%s: board serial %llx\n", __func__,
		    vb.vbt_serial.sn);

	if (vcprop_tag_success_p(&vb.vbt_cmdline.tag))
		printf("%s: cmdline      %s\n", __func__,
		    vb.vbt_cmdline.cmdline);
#endif
}

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel, and stay at the same address
 * throughout whole kernel's life time.
 *
 * We use this table twice; once with bootstrap page table, and once
 * with kernel's page table which we build up in initarm().
 *
 * Since we map these registers into the bootstrap page table using
 * pmap_devmap_bootstrap() which calls pmap_map_chunk(), we map
 * registers segment-aligned and segment-rounded in order to avoid
 * using the 2nd page tables.
 */
#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static inline
pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc   p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1<<14)-1));
}

#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap rpi_devmap[] = {
	{
		_A(RPI_KERNEL_IO_VBASE),	/* 0xf2000000 */
		_A(RPI_KERNEL_IO_PBASE),	/* 0x20000000 */
		_S(RPI_KERNEL_IO_VSIZE),	/* 16Mb */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{ 0, 0, 0, 0, 0 }
};

#undef  _A
#undef  _S

/*
 * u_int initarm(...)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 */
u_int
initarm(void *arg)
{

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), rpi_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	rpi_bootparams();

	if (vcprop_tag_success_p(&vb.vbt_armclockrate.tag)) {
		curcpu()->ci_data.cpu_cc_freq = vb.vbt_armclockrate.rate;
#ifdef VERBOSE_INIT_ARM
		printf("%s: arm clock   %d\n", __func__,
		    vb.vbt_armclockrate.rate);
#endif
	}

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	bootconfig.dramblocks = 1;
	physical_end = bootconfig.dram[0].pages * PAGE_SIZE;
	physmem = bootconfig.dram[0].pages;
	physical_start = 0;

	/*
	 * Our kernel is at the beginning of memory, so set our free space to
	 * all the memory after the kernel.
	 */
	physical_freestart = KERN_VTOPHYS(round_page((vaddr_t)_end));
	physical_freeend = physical_end;
	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * This is going to do all the hard work of setting up the first and
	 * and second level page tables.  Pages of memory will be allocated
	 * and mapped for other structures that are required for system
	 * operation.  When it returns, physical_freestart and free_pages will
	 * have been updated to reflect the allocations that were made.  In
	 * addition, kernel_l1pt, kernel_pt_table[], systempage, irqstack,
	 * abtstack, undstack, kernelstack, msgbufphys will be set to point to
	 * the memory that was allocated for them.
	 */
	setup_real_page_tables();

	/*
	 * Okay, the kernel starts 8kB in from the bottom of physical
	 * memory.  We are going to allocate our bootstrap pages upwards
	 * from physical_freestart.
	 *
	 * We need to allocate some fixed page tables to get the kernel
	 * going.  We allocate one page directory and a number of page
	 * tables and store the physical addresses in the kernel_pt_table
	 * array.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 4K bounaries.  What we do is allocate the
	 * page directory on the first 16K boundary that we encounter, and
	 * the page tables on 4K boundaries otherwise.  Since we allocate
	 * at least 3 L2 page tables, we are guaranteed to encounter at
	 * least one 16K aligned region.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	    physical_freestart, free_pages, free_pages);
#endif

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init.
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

	arm32_vector_init(ARM_VECTORS_HIGH, ARM_VEC_ALL);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("init subsystems: stacks ");
#endif
	set_stackptr(PSR_FIQ32_MODE,
	    fiqstack.pv_va + FIQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);
   
	/* flag used in locore.s */
	extern u_int cpu_reset_needs_v4_MMU_disable;
	cpu_reset_needs_v4_MMU_disable = 0;
	cpu_reset_address = (u_int) bcm2835_system_reset;

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

	if (vcprop_tag_success_p(&vb.vbt_cmdline.tag))
		strlcpy(bootargs, vb.vbt_cmdline.cmdline, sizeof(bootargs));
	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

#ifdef KGDB
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* we've a specific device_register routine */
	evbarm_device_register = rpi_device_register;

	/* We return the new stack pointer address */
	return kernelstack.pv_va + USPACE_SVC_STACK_TOP;
}

void
consinit(void)
{
	static int consinit_called = 0;
#if (NPLCOM > 0 && defined(PLCONSOLE))
	static struct plcom_instance rpi_pi = {
		.pi_type = PLCOM_TYPE_PL011,
		.pi_flags = PLC_FLAG_32BIT_ACCESS,
		.pi_iot = &bcm2835_bs_tag,
		.pi_size = BCM2835_UART0_SIZE
	};
#endif
	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if (NPLCOM > 0 && defined(PLCONSOLE))
	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 */
	rpi_pi.pi_iobase = consaddr;
	
	plcomcnattach(&rpi_pi, plcomcnspeed, BCM2835_UART0_CLK,
	    plcomcnmode, PLCOMCNUNIT);

#endif
}

static void
setup_real_page_tables(void)
{
	/*
	 * We need to allocate some fixed page tables to get the kernel going.
	 *
	 * We are going to allocate our bootstrap pages from the beginning of
	 * the free space that we just calculated.  We allocate one page
	 * directory and a number of page tables and store the physical
	 * addresses in the kernel_pt_table array.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 4K boundaries.  What we do is allocate the
	 * page directory on the first 16K boundary that we encounter, and
	 * the page tables on 4K boundaries otherwise.  Since we allocate
	 * at least 3 L2 page tables, we are guaranteed to encounter at
	 * least one 16K aligned region.
	 */
	
#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

	/*
	 * Define a macro to simplify memory allocation.  As we allocate the
	 * memory, make sure that we don't walk over our temporary first level
	 * translation table.
	 */
#define valloc_pages(var, np)						\
	(var).pv_pa = physical_freestart;				\
	physical_freestart += ((np) * PAGE_SIZE);			\
	if (physical_freestart > (physical_freeend - L1_TABLE_SIZE))	\
		panic("%s: out of memory", __func__);			\
	free_pages -= (np);						\
	(var).pv_va = KERN_PHYSTOV((var).pv_pa);			\
	memset((char *)(var).pv_va, 0, ((np) * PAGE_SIZE));

	int loop, pt_index;

	pt_index = 0;
	kernel_l1pt.pv_pa = 0;
	kernel_l1pt.pv_va = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if ((physical_freestart & (L1_TABLE_SIZE - 1)) == 0 &&
		    kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
		} else {
			valloc_pages(kernel_pt_table[pt_index],
			    L2_TABLE_SIZE / PAGE_SIZE);
			++pt_index;
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa ||
	    (kernel_l1pt.pv_pa & (L1_TABLE_SIZE - 1)) != 0)
		panic("%s: Failed to align the kernel page directory", __func__);

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(systempage, 1);
	systempage.pv_va = ARM_VECTORS_HIGH;

	/* Allocate stacks for all modes */
	valloc_pages(fiqstack, FIQ_STACK_SIZE);
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

	/* Allocate the message buffer. */
	pv_addr_t msgbuf;
	int msgbuf_pgs = round_page(MSGBUFSIZE) / PAGE_SIZE;
	valloc_pages(msgbuf, msgbuf_pgs);
	msgbufphys = msgbuf.pv_pa;

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at 0x%08lx\n", kernel_l1pt.pv_pa);
#endif

	/*
	 * Now we start construction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	vaddr_t l1_va = kernel_l1pt.pv_va;
	paddr_t l1_pa = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1_va, ARM_VECTORS_HIGH & ~(0x00400000 - 1),
	    &kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; loop++)
		pmap_link_l2pt(l1_va, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; loop++)
		pmap_link_l2pt(l1_va, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	extern char etext[], _end[];
	size_t textsize = (uintptr_t)etext - KERNEL_BASE;
	size_t totalsize = (uintptr_t)_end - KERNEL_BASE;
	u_int logical;

	textsize = (textsize + PGOFSET) & ~PGOFSET;
	totalsize = (totalsize + PGOFSET) & ~PGOFSET;

	logical = 0x00000000;	/* offset of kernel in RAM */

	logical += pmap_map_chunk(l1_va, KERNEL_BASE + logical,
	    physical_start + logical, textsize,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	logical += pmap_map_chunk(l1_va, KERNEL_BASE + logical,
	    physical_start + logical, totalsize - textsize,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	pmap_map_chunk(l1_va, fiqstack.pv_va, fiqstack.pv_pa,
	    FIQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop)
		pmap_map_chunk(l1_va, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	/* Map the vector page. */
	pmap_map_entry(l1_va, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/*
	 * Map integrated peripherals at same address in first level page
	 * table so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1_va, rpi_devmap);

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about where all the bits and pieces live. */
	printf("%22s       Physical              Virtual        Num\n", " ");
	printf("%22s Starting    Ending    Starting    Ending   Pages\n", " ");

	static const char mem_fmt[] =
	    "%20s: 0x%08lx 0x%08lx 0x%08lx 0x%08lx %d\n";
	static const char mem_fmt_nov[] =
	    "%20s: 0x%08lx 0x%08lx                       %d\n";

	printf(mem_fmt, "SDRAM", physical_start, physical_end-1,
	    KERN_PHYSTOV(physical_start), KERN_PHYSTOV(physical_end-1),
	    physmem);
	printf(mem_fmt, "text section",
	       KERN_VTOPHYS(KERNEL_BASE), KERN_VTOPHYS(etext-1),
	       (vaddr_t)KERNEL_BASE, (vaddr_t)etext-1,
	       (int)(textsize / PAGE_SIZE));
	printf(mem_fmt, "data section",
	       KERN_VTOPHYS(__data_start), KERN_VTOPHYS(_edata),
	       (vaddr_t)__data_start, (vaddr_t)_edata,
	       (int)((round_page((vaddr_t)_edata)
		      - trunc_page((vaddr_t)__data_start)) / PAGE_SIZE));
	printf(mem_fmt, "bss section",
	       KERN_VTOPHYS(__bss_start), KERN_VTOPHYS(__bss_end__),
	       (vaddr_t)__bss_start, (vaddr_t)__bss_end__,
	       (int)((round_page((vaddr_t)__bss_end__)
		      - trunc_page((vaddr_t)__bss_start)) / PAGE_SIZE));
	printf(mem_fmt, "L1 page directory",
	    kernel_l1pt.pv_pa, kernel_l1pt.pv_pa + L1_TABLE_SIZE - 1,
	    kernel_l1pt.pv_va, kernel_l1pt.pv_va + L1_TABLE_SIZE - 1,
	    L1_TABLE_SIZE / PAGE_SIZE);
	printf(mem_fmt, "Exception Vectors",
	    systempage.pv_pa, systempage.pv_pa + PAGE_SIZE - 1,
	    (vaddr_t)ARM_VECTORS_HIGH, (vaddr_t)ARM_VECTORS_HIGH + PAGE_SIZE - 1,
	    1);
	printf(mem_fmt, "FIQ stack",
	    fiqstack.pv_pa, fiqstack.pv_pa + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    fiqstack.pv_va, fiqstack.pv_va + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    FIQ_STACK_SIZE);
	printf(mem_fmt, "IRQ stack",
	    irqstack.pv_pa, irqstack.pv_pa + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    irqstack.pv_va, irqstack.pv_va + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    IRQ_STACK_SIZE);
	printf(mem_fmt, "ABT stack",
	    abtstack.pv_pa, abtstack.pv_pa + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    abtstack.pv_va, abtstack.pv_va + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    ABT_STACK_SIZE);
	printf(mem_fmt, "UND stack",
	    undstack.pv_pa, undstack.pv_pa + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    undstack.pv_va, undstack.pv_va + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    UND_STACK_SIZE);
	printf(mem_fmt, "SVC stack",
	    kernelstack.pv_pa, kernelstack.pv_pa + (UPAGES * PAGE_SIZE) - 1,
	    kernelstack.pv_va, kernelstack.pv_va + (UPAGES * PAGE_SIZE) - 1,
	    UPAGES);
	printf(mem_fmt_nov, "Message Buffer",
	    msgbufphys, msgbufphys + msgbuf_pgs * PAGE_SIZE - 1, msgbuf_pgs);
	printf(mem_fmt, "Free Memory", physical_freestart, physical_freeend-1,
	    KERN_PHYSTOV(physical_freestart), KERN_PHYSTOV(physical_freeend-1),
	    free_pages);
#endif
	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table  @%#lx...", l1_pa);
#endif

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(l1_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

#ifdef VERBOSE_INIT_ARM
	printf("OK.\n");
#endif
}

#if NGENFB > 0
static bool
rpi_fb_parse_mode(const char *s, uint32_t *pwidth, uint32_t *pheight)
{
	if (strncmp(s, "fb", 2) != 0)
		return false;

	if (strncmp(s, "fb:", 3) == 0) {
		char *x = strchr(s + 3, 'x');
		if (x) {
			*pwidth = strtoul(s + 3, NULL, 10);
			*pheight = strtoul(x + 1, NULL, 10);
		}	 
	}

	return true;
}

static bool
rpi_fb_get_edid_mode(uint32_t *pwidth, uint32_t *pheight)
{
	struct edid_info ei;
	uint8_t edid_data[1024];
	uint32_t res;
	int error;
	
	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_edid,
	    sizeof(vb_edid), &res);
	if (error) {
		printf("%s: mbox request failed (%d)\n", __func__, error);
		return false;
	}

	if (!vcprop_buffer_success_p(&vb_edid.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_edid.vbt_edid.tag) ||
	    vb_edid.vbt_edid.status != 0)
		return false;

	memset(edid_data, 0, sizeof(edid_data));
	memcpy(edid_data, vb_edid.vbt_edid.data,
	    sizeof(vb_edid.vbt_edid.data));
	edid_parse(edid_data, &ei);
#ifdef VERBOSE_INIT_ARM
	edid_print(&ei);
#endif

	if (ei.edid_preferred_mode) {
		*pwidth = ei.edid_preferred_mode->hdisplay;
		*pheight = ei.edid_preferred_mode->vdisplay;
	}

	return true;
}

/*
 * Initialize framebuffer console.
 *
 * Some notes about boot parameters:
 *  - If "fb=disable" is present, ignore framebuffer completely.
 *  - If "fb=<width>x<height> is present, use the specified mode.
 *  - If "console=fb" is present, attach framebuffer to console.
 */
static bool
rpi_fb_init(prop_dictionary_t dict)
{
	uint32_t width = 0, height = 0;
	uint32_t res;
	char *ptr;
	int integer; 
	int error;

	if (get_bootconf_option(boot_args, "fb",
			      BOOTOPT_TYPE_STRING, &ptr)) {
		if (rpi_fb_parse_mode(ptr, &width, &height) == false)
			return false;
	}
	if (width == 0 || height == 0) {
		rpi_fb_get_edid_mode(&width, &height);
	}
	if (width == 0 || height == 0) {
		width = RPI_FB_WIDTH;
		height = RPI_FB_HEIGHT;
	}

	vb_setfb.vbt_res.width = width;
	vb_setfb.vbt_res.height = height;
	vb_setfb.vbt_vres.width = width;
	vb_setfb.vbt_vres.height = height;
	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_setfb,
	    sizeof(vb_setfb), &res);
	if (error) {
		printf("%s: mbox request failed (%d)\n", __func__, error);
		return false;
	}

	if (!vcprop_buffer_success_p(&vb_setfb.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_res.tag) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_vres.tag) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_depth.tag) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_pixelorder.tag) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_allocbuf.tag) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_blank.tag) ||
	    !vcprop_tag_success_p(&vb_setfb.vbt_pitch.tag)) {
		printf("%s: prop tag failed\n", __func__);
		return false;
	}

#ifdef VERBOSE_INIT_ARM
	printf("%s: addr = 0x%x size = %d\n", __func__,
	    vb_setfb.vbt_allocbuf.address,
	    vb_setfb.vbt_allocbuf.size);
	printf("%s: depth = %d\n", __func__, vb_setfb.vbt_depth.bpp);
	printf("%s: pitch = %d\n", __func__,
	    vb_setfb.vbt_pitch.linebytes);
	printf("%s: width = %d height = %d\n", __func__,
	    vb_setfb.vbt_res.width, vb_setfb.vbt_res.height);
	printf("%s: vwidth = %d vheight = %d\n", __func__,
	    vb_setfb.vbt_vres.width, vb_setfb.vbt_vres.height);
	printf("%s: pixelorder = %d\n", __func__,
	    vb_setfb.vbt_pixelorder.state);
#endif

	if (vb_setfb.vbt_allocbuf.address == 0 ||
	    vb_setfb.vbt_allocbuf.size == 0 ||
	    vb_setfb.vbt_res.width == 0 ||
	    vb_setfb.vbt_res.height == 0 ||
	    vb_setfb.vbt_vres.width == 0 ||
	    vb_setfb.vbt_vres.height == 0 ||
	    vb_setfb.vbt_pitch.linebytes == 0) {
		printf("%s: failed to set mode %ux%u\n", __func__,
		    width, height);
		return false;
	}

	prop_dictionary_set_uint32(dict, "width", vb_setfb.vbt_res.width);
	prop_dictionary_set_uint32(dict, "height", vb_setfb.vbt_res.height);
	prop_dictionary_set_uint8(dict, "depth", vb_setfb.vbt_depth.bpp);
	prop_dictionary_set_uint16(dict, "linebytes",
	    vb_setfb.vbt_pitch.linebytes);
	prop_dictionary_set_uint32(dict, "address",
	    vb_setfb.vbt_allocbuf.address);
	if (vb_setfb.vbt_pixelorder.state == VCPROP_PIXEL_BGR)
		prop_dictionary_set_bool(dict, "is_bgr", true);

	/* if "genfb.type=<n>" is passed in cmdline, override wsdisplay type */
	if (get_bootconf_option(boot_args, "genfb.type",
				BOOTOPT_TYPE_INT, &integer)) {
		prop_dictionary_set_uint32(dict, "wsdisplay_type", integer);
	}

	return true;
}
#endif

static void
rpi_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

#if NSDHC > 0
	if (device_is_a(dev, "sdhc") &&
	    vcprop_tag_success_p(&vb.vbt_emmcclockrate.tag) &&
	    vb.vbt_emmcclockrate.rate > 0) {
		prop_dictionary_set_uint32(dict,
		    "frequency", vb.vbt_emmcclockrate.rate);
	}
	if (booted_device == NULL &&
	    device_is_a(dev, "ld") &&
	    device_is_a(device_parent(dev), "sdmmc")) {
		booted_partition = 0;
		booted_device = dev;
	}
#endif
	if (device_is_a(dev, "usmsc") &&
	    vcprop_tag_success_p(&vb.vbt_macaddr.tag)) {
		const uint8_t enaddr[ETHER_ADDR_LEN] = {
		     (vb.vbt_macaddr.addr >> 0) & 0xff,
		     (vb.vbt_macaddr.addr >> 8) & 0xff,
		     (vb.vbt_macaddr.addr >> 16) & 0xff,
		     (vb.vbt_macaddr.addr >> 24) & 0xff,
		     (vb.vbt_macaddr.addr >> 32) & 0xff,
		     (vb.vbt_macaddr.addr >> 40) & 0xff
		};

		prop_data_t pd = prop_data_create_data(enaddr, ETHER_ADDR_LEN);
		KASSERT(pd != NULL);
		if (prop_dictionary_set(device_properties(dev), "mac-address",
		    pd) == false) {
			aprint_error_dev(dev,
			    "WARNING: Unable to set mac-address property\n");
		}
		prop_object_release(pd);
	}

#if NGENFB > 0
	if (device_is_a(dev, "genfb")) {
		char *ptr;
		if (rpi_fb_init(dict) == false)
			return;
		if (get_bootconf_option(boot_args, "console",
		    BOOTOPT_TYPE_STRING, &ptr) && strncmp(ptr, "fb", 2) == 0) {
			prop_dictionary_set_bool(dict, "is_console", true);
#if NUKBD > 0
			/* allow ukbd to be the console keyboard */
			ukbd_cnattach();
#endif
		} else {
			prop_dictionary_set_bool(dict, "is_console", false);
		}
	}
#endif
}

#if 0
SYSCTL_SETUP(sysctl_machdep_rpi, "sysctl machdep subtree setup (rpi)")
{
	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_HEX|CTLFLAG_PRIVATE,
	    CTLTYPE_QUAD, "serial", NULL, NULL, 0,
	    &vb.vbt_serial.sn, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}
#endif
