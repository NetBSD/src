/*	$NetBSD: rpi_machdep.c,v 1.21 2013/01/08 16:49:43 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rpi_machdep.c,v 1.21 2013/01/08 16:49:43 skrll Exp $");

#include "opt_evbarm_boardtype.h"

#include "sdhc.h"
#include "bcmspi.h"
#include "bsciic.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/bus.h>

#include <prop/proplib.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <machine/autoconf.h>
#include <machine/vmparam.h>
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

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#include "ksyms.h"

extern int KERNEL_BASE_phys[];
extern int KERNEL_BASE_virt[];

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

static void rpi_bootparams(void);
static void rpi_device_register(device_t, void *);

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */

#define	KERN_VTOPDIFF	((vaddr_t)KERNEL_BASE_phys - (vaddr_t)KERNEL_BASE_virt)
#define KERN_VTOPHYS(va) ((paddr_t)((vaddr_t)va + KERN_VTOPDIFF))
#define KERN_PHYSTOV(pa) ((vaddr_t)((paddr_t)pa - KERN_VTOPDIFF))

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
	pmap_devmap_bootstrap((vaddr_t)armreg_ttbr_read() & -L1_TABLE_SIZE,
	    rpi_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	rpi_bootparams();

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif
	arm32_bootmem_init(bootconfig.dram[0].address,
	    bootconfig.dram[0].pages * PAGE_SIZE, (uintptr_t)KERNEL_BASE_phys);

	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, rpi_devmap,
	    false);

	cpu_reset_address = bcm2835_system_reset;

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

	/* we've a specific device_register routine */
	evbarm_device_register = rpi_device_register;

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
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
rpi_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

	if (device_is_a(dev, "sdhc") &&
	    vcprop_tag_success_p(&vb.vbt_emmcclockrate.tag)) {
		prop_dictionary_set_uint32(dict,
		    "frequency", vb.vbt_emmcclockrate.rate);
	}
}
