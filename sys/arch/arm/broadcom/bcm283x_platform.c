/*	$NetBSD: bcm283x_platform.c,v 1.3 2018/03/17 18:34:09 ryo Exp $	*/

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm283x_platform.c,v 1.3 2018/03/17 18:34:09 ryo Exp $");

#include "opt_arm_debug.h"
#include "opt_bcm283x.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_evbarm_boardtype.h"
#include "opt_kgdb.h"
#include "opt_fdt.h"
#include "opt_rpi.h"
#include "opt_vcprop.h"

#include "sdhc.h"
#include "bcmsdhost.h"
#include "bcmdwctwo.h"
#include "bcmspi.h"
#include "bsciic.h"
#include "plcom.h"
#include "com.h"
#include "genfb.h"
#include "ukbd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <net/if_ether.h>

#include <prop/proplib.h>

#include <dev/fdt/fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <libfdt.h>

#include <arm/arm32/machdep.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_intr.h>
#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_pmwdogvar.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcpm.h>
#include <evbarm/rpi/vcprop.h>

#include <arm/fdt/arm_fdtvar.h>

#include <arm/cortex/gtmr_var.h>

#if NGENFB > 0
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>
#include <dev/wscons/wsconsio.h>
#endif

#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

void bcm283x_platform_early_putchar(vaddr_t, paddr_t, char c);
void bcm2835_platform_early_putchar(char c);
void bcm2836_platform_early_putchar(char c);
void bcm2837_platform_early_putchar(char c);

extern void bcmgenfb_set_console_dev(device_t dev);
void bcmgenfb_set_ioctl(int(*)(void *, void *, u_long, void *, int, struct lwp *));
extern void bcmgenfb_ddb_trap_callback(int where);
static int rpi_ioctl(void *, void *, u_long, void *, int, lwp_t *);

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

/* Prototypes for all the bus_space structure functions */
bs_protos(bcm2835);
bs_protos(bcm2835_a4x);
bs_protos(armv7_generic);
bs_protos(armv7_generic_a4x);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(a4x);
bs_protos(bs_notimpl);

struct arm32_dma_range bcm2835_dma_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = BCM2835_BUSADDR_CACHE_COHERENT,
	}
};

struct arm32_dma_range bcm2836_dma_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = BCM2835_BUSADDR_CACHE_DIRECT,
	}
};


#if defined(SOC_BCM2835)
static const struct pmap_devmap *
bcm2835_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(BCM2835_PERIPHERALS_VBASE, BCM2835_PERIPHERALS_BASE,
		    BCM2835_PERIPHERALS_SIZE),	/* 16Mb */

		DEVMAP_ENTRY_END
	};

	return devmap;
}
#endif

#if defined(SOC_BCM2836)
static const struct pmap_devmap *
bcm2836_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(BCM2836_PERIPHERALS_VBASE, BCM2836_PERIPHERALS_BASE,
		    BCM2835_PERIPHERALS_SIZE),	/* 16Mb */

		DEVMAP_ENTRY(BCM2836_ARM_LOCAL_VBASE, BCM2836_ARM_LOCAL_BASE,
		    BCM2836_ARM_LOCAL_SIZE),

		DEVMAP_ENTRY_END
	};

	return devmap;
}
#endif

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */

#define KERN_VTOPDIFF	KERNEL_BASE_VOFFSET
#define KERN_VTOPHYS(va) ((paddr_t)((vaddr_t)va - KERN_VTOPDIFF))
#define KERN_PHYSTOV(pa) ((vaddr_t)((paddr_t)pa + KERN_VTOPDIFF))


#ifndef RPI_FB_WIDTH
#define RPI_FB_WIDTH	1280
#endif
#ifndef RPI_FB_HEIGHT
#define RPI_FB_HEIGHT	720
#endif

int uart_clk = BCM2835_UART0_CLK;
int core_clk;

static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_clockrate	vbt_uartclockrate;
	struct vcprop_tag_clockrate	vbt_vpuclockrate;
	struct vcprop_tag end;
} vb_uart __cacheline_aligned = {
	.vb_hdr = {
		.vpb_len = sizeof(vb_uart),
		.vpb_rcode = VCPROP_PROCESS_REQUEST,
	},
	.vbt_uartclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb_uart.vbt_uartclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_UART
	},
	.vbt_vpuclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb_uart.vbt_vpuclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_CORE
	},
	.end = {
		.vpt_tag = VCPROPTAG_NULL
	}
};

static struct {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_fwrev		vbt_fwrev;
	struct vcprop_tag_boardmodel	vbt_boardmodel;
	struct vcprop_tag_boardrev	vbt_boardrev;
	struct vcprop_tag_macaddr	vbt_macaddr;
	struct vcprop_tag_memory	vbt_memory;
	struct vcprop_tag_boardserial	vbt_serial;
	struct vcprop_tag_dmachan	vbt_dmachan;
	struct vcprop_tag_cmdline	vbt_cmdline;
	struct vcprop_tag_clockrate	vbt_emmcclockrate;
	struct vcprop_tag_clockrate	vbt_armclockrate;
	struct vcprop_tag_clockrate	vbt_vpuclockrate;
	struct vcprop_tag end;
} vb __cacheline_aligned = {
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
	.vbt_dmachan = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_DMACHAN,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_dmachan),
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
	.vbt_vpuclockrate = {
		.tag = {
			.vpt_tag = VCPROPTAG_GET_CLOCKRATE,
			.vpt_len = VCPROPTAG_LEN(vb.vbt_vpuclockrate),
			.vpt_rcode = VCPROPTAG_REQUEST
		},
		.id = VCPROP_CLK_CORE
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
} vb_edid __cacheline_aligned = {
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
	struct vcprop_tag_fbalpha	vbt_alpha;
	struct vcprop_tag_allocbuf	vbt_allocbuf;
	struct vcprop_tag_blankscreen	vbt_blank;
	struct vcprop_tag_fbpitch	vbt_pitch;
	struct vcprop_tag end;
} vb_setfb __cacheline_aligned = {
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

static int rpi_video_on = WSDISPLAYIO_VIDEO_ON;

#if defined(RPI_HWCURSOR)
#define CURSOR_BITMAP_SIZE	(64 * 8)
#define CURSOR_ARGB_SIZE	(64 * 64 * 4)
static uint32_t hcursor = 0;
static bus_addr_t pcursor = 0;
static uint32_t *cmem = NULL;
static int cursor_x = 0, cursor_y = 0, hot_x = 0, hot_y = 0, cursor_on = 0;
static uint32_t cursor_cmap[4];
static uint8_t cursor_mask[8 * 64], cursor_bitmap[8 * 64];
#endif

u_int
bcm283x_clk_get_rate_uart(void)
{

	if (vcprop_tag_success_p(&vb_uart.vbt_uartclockrate.tag))
		return vb_uart.vbt_uartclockrate.rate;
	return 0;
}

u_int
bcm283x_clk_get_rate_vpu(void)
{

	if (vcprop_tag_success_p(&vb.vbt_vpuclockrate.tag) &&
	    vb.vbt_vpuclockrate.rate > 0) {
		return vb.vbt_vpuclockrate.rate;
	}
	return 0;
}

u_int
bcm283x_clk_get_rate_emmc(void)
{

	if (vcprop_tag_success_p(&vb.vbt_emmcclockrate.tag) &&
	    vb.vbt_emmcclockrate.rate > 0) {
		return vb.vbt_emmcclockrate.rate;
	}
	return 0;
}



static void
bcm283x_uartinit(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	uint32_t res;

	bcm2835_mbox_write(iot, ioh, BCMMBOX_CHANARM2VC,
	    KERN_VTOPHYS(&vb_uart));

	bcm2835_mbox_read(iot, ioh, BCMMBOX_CHANARM2VC, &res);

	cpu_dcache_inv_range((vaddr_t)&vb_uart, sizeof(vb_uart));

	if (vcprop_tag_success_p(&vb_uart.vbt_uartclockrate.tag))
		uart_clk = vb_uart.vbt_uartclockrate.rate;
	if (vcprop_tag_success_p(&vb_uart.vbt_vpuclockrate.tag))
		core_clk = vb_uart.vbt_vpuclockrate.rate;
}

#if defined(SOC_BCM2835)
static void
bcm2835_uartinit(void)
{
	const paddr_t pa = BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_ARMMBOX_BASE);
	const bus_space_tag_t iot = &bcm2835_bs_tag;
	const bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(pa);

	bcm283x_uartinit(iot, ioh);
}
#endif

#if defined(SOC_BCM2836)
static void
bcm2836_uartinit(void)
{
	const paddr_t pa = BCM2836_PERIPHERALS_BUS_TO_PHYS(BCM2835_ARMMBOX_BASE);
	const bus_space_tag_t iot = &bcm2836_bs_tag;
	const bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(pa);

	bcm283x_uartinit(iot, ioh);
}
#endif

#define	BCM283x_MINIMUM_SPLIT	(128U * 1024 * 1024)

static size_t bcm283x_memorysize;

static void
bcm283x_bootparams(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	uint32_t res;

	bcm2835_mbox_write(iot, ioh, BCMMBOX_CHANPM, (
#if (NSDHC > 0)
	    (1 << VCPM_POWER_SDCARD) |
#endif
#if (NPLCOM > 0)
	    (1 << VCPM_POWER_UART0) |
#endif
#if (NBCMDWCTWO > 0)
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

	cpu_dcache_inv_range((vaddr_t)&vb, sizeof(vb));

	if (!vcprop_buffer_success_p(&vb.vb_hdr)) {
		bootconfig.dramblocks = 1;
		bootconfig.dram[0].address = 0x0;
		bootconfig.dram[0].pages = atop(BCM283x_MINIMUM_SPLIT);
		return;
	}

	struct vcprop_tag_memory *vptp_mem = &vb.vbt_memory;
	if (vcprop_tag_success_p(&vptp_mem->tag)) {
		size_t n = vcprop_tag_resplen(&vptp_mem->tag) /
		    sizeof(struct vcprop_memory);

		bcm283x_memorysize = 0;
		bootconfig.dramblocks = 0;

		for (int i = 0; i < n && i < DRAM_BLOCKS; i++) {
			bootconfig.dram[i].address = vptp_mem->mem[i].base;
			bootconfig.dram[i].pages = atop(vptp_mem->mem[i].size);
			bootconfig.dramblocks++;

			bcm283x_memorysize += vptp_mem->mem[i].size;
		}
	}

	if (vcprop_tag_success_p(&vb.vbt_armclockrate.tag))
		curcpu()->ci_data.cpu_cc_freq = vb.vbt_armclockrate.rate;

#ifdef VERBOSE_INIT_ARM
	if (vcprop_tag_success_p(&vb.vbt_memory.tag)) {
		printf("%s: memory size  %d\n", __func__,
		    vb.vbt_armclockrate.rate);
	}
	if (vcprop_tag_success_p(&vb.vbt_armclockrate.tag))
		printf("%s: arm clock    %d\n", __func__,
		    vb.vbt_armclockrate.rate);
	if (vcprop_tag_success_p(&vb.vbt_fwrev.tag))
		printf("%s: firmware rev %x\n", __func__,
		    vb.vbt_fwrev.rev);
	if (vcprop_tag_success_p(&vb.vbt_boardmodel.tag))
		printf("%s: board model  %x\n", __func__,
		    vb.vbt_boardmodel.model);
	if (vcprop_tag_success_p(&vb.vbt_macaddr.tag))
		printf("%s: mac-address  %llx\n", __func__,
		    vb.vbt_macaddr.addr);
	if (vcprop_tag_success_p(&vb.vbt_boardrev.tag))
		printf("%s: board rev    %x\n", __func__,
		    vb.vbt_boardrev.rev);
	if (vcprop_tag_success_p(&vb.vbt_serial.tag))
		printf("%s: board serial %llx\n", __func__,
		    vb.vbt_serial.sn);
	if (vcprop_tag_success_p(&vb.vbt_dmachan.tag))
		printf("%s: DMA channel mask 0x%08x\n", __func__,
		    vb.vbt_dmachan.mask);

	if (vcprop_tag_success_p(&vb.vbt_cmdline.tag))
		printf("%s: cmdline      %s\n", __func__,
		    vb.vbt_cmdline.cmdline);
#endif
}

#if defined(SOC_BCM2835)
static void
bcm2835_bootparams(void)
{
	const paddr_t pa = BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_ARMMBOX_BASE);
	const bus_space_tag_t iot = &bcm2835_bs_tag;
	const bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(pa);

	bcm283x_bootparams(iot, ioh);
}
#endif

#if defined(SOC_BCM2836)
static void
bcm2836_bootparams(void)
{
	const paddr_t pa = BCM2836_PERIPHERALS_BUS_TO_PHYS(BCM2835_ARMMBOX_BASE);
	const bus_space_tag_t iot = &bcm2836_bs_tag;
	const bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(pa);

	bcm283x_bootparams(iot, ioh);
}

static void
bcm2836_bootstrap(void)
{
	arm_cpu_max = 4;
	extern int cortex_mmuinfo;

	cortex_mmuinfo = armreg_ttbr_read();
#ifdef VERBOSE_INIT_ARM
	printf("%s: cortex_mmuinfo %x\n", __func__, cortex_mmuinfo);
#endif

	extern void cortex_mpstart(void);

	for (size_t i = 1; i < arm_cpu_max; i++) {
		bus_space_tag_t iot = &bcm2836_bs_tag;
		bus_space_handle_t ioh = BCM2836_ARM_LOCAL_VBASE;

		bus_space_write_4(iot, ioh,
		    BCM2836_LOCAL_MAILBOX3_SETN(i),
		    (uint32_t)cortex_mpstart);
	}

	/* Wake up AP in case firmware has placed it in WFE state */
	__asm __volatile("sev" ::: "memory");

	for (int loop = 0; loop < 16; loop++) {
		if (arm_cpu_hatched == __BITS(arm_cpu_max - 1, 1))
			break;
		gtmr_delay(10000);
	}

	for (size_t i = 1; i < arm_cpu_max; i++) {
		if ((arm_cpu_hatched & (1 << i)) == 0) {
			printf("%s: warning: cpu%zu failed to hatch\n",
			    __func__, i);
		}
	}
}

#endif	/* SOC_BCM2836 */

#if NGENFB > 0
static bool
rpi_fb_parse_mode(const char *s, uint32_t *pwidth, uint32_t *pheight)
{
	char *x;

	if (strncmp(s, "disable", 7) == 0)
		return false;

	x = strchr(s, 'x');
	if (x) {
		*pwidth = strtoul(s, NULL, 10);
		*pheight = strtoul(x + 1, NULL, 10);
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
rpi_fb_init(prop_dictionary_t dict, void *aux)
{
	uint32_t width = 0, height = 0;
	uint32_t res;
	char *ptr;
	int integer;
	int error;
	bool is_bgr = true;

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

	/*
	 * Old firmware uses BGR. New firmware uses RGB. The get and set
	 * pixel order mailbox properties don't seem to work. The firmware
	 * adds a kernel cmdline option bcm2708_fb.fbswap=<0|1>, so use it
	 * to determine pixel order. 0 means BGR, 1 means RGB.
	 *
	 * See https://github.com/raspberrypi/linux/issues/514
	 */
	if (get_bootconf_option(boot_args, "bcm2708_fb.fbswap",
				BOOTOPT_TYPE_INT, &integer)) {
		is_bgr = integer == 0;
	}
	prop_dictionary_set_bool(dict, "is_bgr", is_bgr);

	/* if "genfb.type=<n>" is passed in cmdline, override wsdisplay type */
	if (get_bootconf_option(boot_args, "genfb.type",
				BOOTOPT_TYPE_INT, &integer)) {
		prop_dictionary_set_uint32(dict, "wsdisplay_type", integer);
	}

#if defined(RPI_HWCURSOR)
	struct fdt_attach_args *faa = aux;
	bus_space_handle_t hc;

	hcursor = rpi_alloc_mem(CURSOR_ARGB_SIZE, PAGE_SIZE,
	    MEM_FLAG_L1_NONALLOCATING | MEM_FLAG_HINT_PERMALOCK);
	pcursor = rpi_lock_mem(hcursor);
#ifdef RPI_IOCTL_DEBUG
	printf("hcursor: %08x\n", hcursor);
	printf("pcursor: %08x\n", (uint32_t)pcursor);
	printf("fb: %08x\n", (uint32_t)vb_setfb.vbt_allocbuf.address);
#endif
	if (bus_space_map(faa->faa_bst, pcursor, CURSOR_ARGB_SIZE,
	    BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE, &hc) != 0) {
		printf("couldn't map cursor memory\n");
	} else {
		int i, j, k;

		cmem = bus_space_vaddr(faa->faa_bst, hc);
		k = 0;
		for (j = 0; j < 64; j++) {
			for (i = 0; i < 64; i++) {
				cmem[i + k] =
				 ((i & 8) ^ (j & 8)) ? 0xa0ff0000 : 0xa000ff00;
			}
			k += 64;
		}
		cpu_dcache_wb_range((vaddr_t)cmem, CURSOR_ARGB_SIZE);
		rpi_fb_initcursor(pcursor, 0, 0);
#ifdef RPI_IOCTL_DEBUG
		rpi_fb_movecursor(600, 400, 1);
#else
		rpi_fb_movecursor(cursor_x, cursor_y, cursor_on);
#endif
	}
#endif

	return true;
}


#if defined(RPI_HWCURSOR)
static int
rpi_fb_do_cursor(struct wsdisplay_cursor *cur)
{
	int pos = 0;
	int shape = 0;

	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {
		if (cursor_on != cur->enable) {
			cursor_on = cur->enable;
			pos = 1;
		}
	}
	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {

		hot_x = cur->hot.x;
		hot_y = cur->hot.y;
		pos = 1;
		shape = 1;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {

		cursor_x = cur->pos.x;
		cursor_y = cur->pos.y;
		pos = 1;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		int i;
		uint32_t val;

		for (i = 0; i < min(cur->cmap.count, 3); i++) {
			val = (cur->cmap.red[i] << 16 ) |
			      (cur->cmap.green[i] << 8) |
			      (cur->cmap.blue[i] ) |
			      0xff000000;
			cursor_cmap[i + cur->cmap.index + 2] = val;
		}
		shape = 1;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		int err;

		err = copyin(cur->mask, cursor_mask, CURSOR_BITMAP_SIZE);
		err += copyin(cur->image, cursor_bitmap, CURSOR_BITMAP_SIZE);
		if (err != 0)
			return EFAULT;
		shape = 1;
	}
	if (shape) {
		int i, j, idx;
		uint8_t mask;

		for (i = 0; i < CURSOR_BITMAP_SIZE; i++) {
			mask = 0x01;
			for (j = 0; j < 8; j++) {
				idx = ((cursor_mask[i] & mask) ? 2 : 0) |
				    ((cursor_bitmap[i] & mask) ? 1 : 0);
				cmem[i * 8 + j] = cursor_cmap[idx];
				mask = mask << 1;
			}
		}
		/* just in case */
		cpu_dcache_wb_range((vaddr_t)cmem, CURSOR_ARGB_SIZE);
		rpi_fb_initcursor(pcursor, hot_x, hot_y);
	}
	if (pos) {
		rpi_fb_movecursor(cursor_x, cursor_y, cursor_on);
	}
	return 0;
}
#endif

static int
rpi_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{

	switch (cmd) {
	case WSDISPLAYIO_SVIDEO:
		{
			int d = *(int *)data;
			if (d == rpi_video_on)
				return 0;
			rpi_video_on = d;
			rpi_fb_set_video(d);
#if defined(RPI_HWCURSOR)
			rpi_fb_movecursor(cursor_x, cursor_y,
			                  d ? cursor_on : 0);
#endif
		}
		return 0;
	case WSDISPLAYIO_GVIDEO:
		*(int *)data = rpi_video_on;
		return 0;
#if defined(RPI_HWCURSOR)
	case WSDISPLAYIO_GCURPOS:
		{
			struct wsdisplay_curpos *cp = (void *)data;

			cp->x = cursor_x;
			cp->y = cursor_y;
		}
		return 0;
	case WSDISPLAYIO_SCURPOS:
		{
			struct wsdisplay_curpos *cp = (void *)data;

			cursor_x = cp->x;
			cursor_y = cp->y;
			rpi_fb_movecursor(cursor_x, cursor_y, cursor_on);
		}
		return 0;
	case WSDISPLAYIO_GCURMAX:
		{
			struct wsdisplay_curpos *cp = (void *)data;

			cp->x = 64;
			cp->y = 64;
		}
		return 0;
	case WSDISPLAYIO_SCURSOR:
		{
			struct wsdisplay_cursor *cursor = (void *)data;

			return rpi_fb_do_cursor(cursor);
		}
#endif
	default:
		return EPASSTHROUGH;
	}
}

#endif

SYSCTL_SETUP(sysctl_machdep_rpi, "sysctl machdep subtree setup (rpi)")
{
	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY,
	    CTLTYPE_INT, "firmware_revision", NULL, NULL, 0,
	    &vb.vbt_fwrev.rev, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY,
	    CTLTYPE_INT, "board_model", NULL, NULL, 0,
	    &vb.vbt_boardmodel.model, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY,
	    CTLTYPE_INT, "board_revision", NULL, NULL, 0,
	    &vb.vbt_boardrev.rev, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_HEX|CTLFLAG_PRIVATE,
	    CTLTYPE_QUAD, "serial", NULL, NULL, 0,
	    &vb.vbt_serial.sn, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}

#if defined(SOC_BCM2835)
static void
bcm2835_platform_bootstrap(void)
{

	fdtbus_set_decoderegprop(false);

	bcm2835_uartinit();

	bcm2835_bootparams();
}
#endif

#if defined(SOC_BCM2836)
static void
bcm2836_platform_bootstrap(void)
{

	fdtbus_set_decoderegprop(false);

	bcm2836_uartinit();

	bcm2836_bootparams();

	bcm2836_bootstrap();
}
#endif

#if defined(SOC_BCM2835)
static void
bcm2835_platform_init_attach_args(struct fdt_attach_args *faa)
{

	faa->faa_bst = &bcm2835_bs_tag;
	faa->faa_a4x_bst = &bcm2835_a4x_bs_tag;
	faa->faa_dmat = &bcm2835_bus_dma_tag;

	bcm2835_bus_dma_tag._ranges = bcm2835_dma_ranges;
	bcm2835_bus_dma_tag._nranges = __arraycount(bcm2835_dma_ranges);
	bcm2835_dma_ranges[0].dr_len = bcm283x_memorysize;
}
#endif

#if defined(SOC_BCM2836)
static void
bcm2836_platform_init_attach_args(struct fdt_attach_args *faa)
{

	faa->faa_bst = &bcm2836_bs_tag;
	faa->faa_a4x_bst = &bcm2836_a4x_bs_tag;
	faa->faa_dmat = &bcm2835_bus_dma_tag;

	bcm2835_bus_dma_tag._ranges = bcm2836_dma_ranges;
	bcm2835_bus_dma_tag._nranges = __arraycount(bcm2836_dma_ranges);
	bcm2836_dma_ranges[0].dr_len = bcm283x_memorysize;
}
#endif


void
bcm283x_platform_early_putchar(vaddr_t va, paddr_t pa, char c)
{
	volatile uint32_t *uartaddr =
	    (armreg_sctlr_read() & CPU_CONTROL_MMU_ENABLE) ?
		(volatile uint32_t *)va :
		(volatile uint32_t *)pa;

	while ((uartaddr[PL01XCOM_FR / 4] & PL01X_FR_TXFF) != 0)
		continue;

	uartaddr[PL01XCOM_DR / 4] = c;

	while ((uartaddr[PL01XCOM_FR / 4] & PL01X_FR_TXFE) == 0)
		continue;
}

void
bcm2835_platform_early_putchar(char c)
{
	paddr_t pa = BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_UART0_BASE);
	vaddr_t va = BCM2835_IOPHYSTOVIRT(pa);

	bcm283x_platform_early_putchar(va, pa, c);
}

void
bcm2836_platform_early_putchar(char c)
{
	paddr_t pa = BCM2836_PERIPHERALS_BUS_TO_PHYS(BCM2835_UART0_BASE);
	vaddr_t va = BCM2835_IOPHYSTOVIRT(pa);

	bcm283x_platform_early_putchar(va, pa, c);
}

#define	BCM283x_REF_FREQ	19200000

void
bcm2837_platform_early_putchar(char c)
{
#define AUCONSADDR_PA	BCM2836_PERIPHERALS_BUS_TO_PHYS(BCM2835_AUX_UART_BASE)
#define AUCONSADDR_VA	BCM2835_IOPHYSTOVIRT(AUCONSADDR_PA)
	volatile uint32_t *uartaddr =
	    (armreg_sctlr_read() & CPU_CONTROL_MMU_ENABLE) ?
		(volatile uint32_t *)AUCONSADDR_VA :
		(volatile uint32_t *)AUCONSADDR_PA;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = c;
}

static void
bcm283x_platform_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

	if (device_is_a(dev, "bcmdmac") &&
	    vcprop_tag_success_p(&vb.vbt_dmachan.tag)) {
		prop_dictionary_set_uint32(dict,
		    "chanmask", vb.vbt_dmachan.mask);
	}
#if NSDHC > 0
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

		bcmgenfb_set_console_dev(dev);
		bcmgenfb_set_ioctl(&rpi_ioctl);
#ifdef DDB
		db_trap_callback = bcmgenfb_ddb_trap_callback;
#endif
		if (rpi_fb_init(dict, aux) == false)
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

static u_int
bcm283x_platform_uart_freq(void)
{

	return uart_clk;
}

#if defined(SOC_BCM2835)
static const struct arm_platform bcm2835_platform = {
	.devmap = bcm2835_platform_devmap,
	.bootstrap = bcm2835_platform_bootstrap,
	.init_attach_args = bcm2835_platform_init_attach_args,
	.early_putchar = bcm2835_platform_early_putchar,
	.device_register = bcm283x_platform_device_register,
	.reset = bcm2835_system_reset,
	.delay = bcm2835_tmr_delay,
	.uart_freq = bcm283x_platform_uart_freq,
};

ARM_PLATFORM(bcm2835, "brcm,bcm2835", &bcm2835_platform);
#endif

#if defined(SOC_BCM2836)
static u_int
bcm2837_platform_uart_freq(void)
{

	return core_clk * 2;
}

static const struct arm_platform bcm2836_platform = {
	.devmap = bcm2836_platform_devmap,
	.bootstrap = bcm2836_platform_bootstrap,
	.init_attach_args = bcm2836_platform_init_attach_args,
	.early_putchar = bcm2836_platform_early_putchar,
	.device_register = bcm283x_platform_device_register,
	.reset = bcm2835_system_reset,
	.delay = gtmr_delay,
	.uart_freq = bcm283x_platform_uart_freq,
};

static const struct arm_platform bcm2837_platform = {
	.devmap = bcm2836_platform_devmap,
	.bootstrap = bcm2836_platform_bootstrap,
	.init_attach_args = bcm2836_platform_init_attach_args,
	.early_putchar = bcm2837_platform_early_putchar,
	.device_register = bcm283x_platform_device_register,
	.reset = bcm2835_system_reset,
	.delay = gtmr_delay,
	.uart_freq = bcm2837_platform_uart_freq,
};

ARM_PLATFORM(bcm2836, "brcm,bcm2836", &bcm2836_platform);
ARM_PLATFORM(bcm2837, "brcm,bcm2837", &bcm2837_platform);
#endif
