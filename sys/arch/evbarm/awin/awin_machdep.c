/*	$NetBSD: awin_machdep.c,v 1.2 2014/09/04 02:34:30 jmcneill Exp $ */

/*
 * Machine dependent functions for kernel setup for TI OSK5912 board.
 * Based on lubbock_machdep.c which in turn was based on iq80310_machhdep.c
 *
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
__KERNEL_RCSID(0, "$NetBSD: awin_machdep.c,v 1.2 2014/09/04 02:34:30 jmcneill Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_com.h"
#include "opt_allwinner.h"
#include "opt_arm_debug.h"

#include "com.h"
#include "ukbd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/gpio.h>

#include <uvm/uvm_extern.h>

#include <sys/conf.h>
#include <dev/cons.h>
#include <dev/md.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>
#include <arm/armreg.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>
#include <arm/mainbus/mainbus.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/awin/platform.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>
#include <net/if_ether.h>

#define	AWIN_board	__CONCAT(AWIN_, BOARDTYPE)

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;
char *boot_file = NULL;
static uint8_t uboot_enaddr[ETHER_ADDR_LEN];

#if AWIN_board == AWIN_cubieboard
bool cubietruck_p;
#elif AWIN_board == AWIN_cubietruck
#define cubietruck_p	true
#else
#define cubietruck_p	false
#endif

/*
 * uboot_args are filled in by awin_start.S and must be in .data
 * and not .bss since .bss is cleared after uboot_args are filled in.
 */
uintptr_t uboot_args[4] = { 0 };

/* Same things, but for the free (unused by the kernel) memory. */

extern char KERNEL_BASE_phys[];	/* physical start of kernel */
extern char _end[];		/* physical end of kernel */

#if NAWIN_FB > 0
#if NCOM > 0
int use_fb_console = false;
#else
int use_fb_console = true;
#endif
#endif

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS	((paddr_t)KERNEL_BASE_phys)
#define AWIN_CORE_VOFFSET	(AWIN_CORE_VBASE - AWIN_CORE_PBASE)

/* Prototypes */

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

static void awin_device_register(device_t, void *);

#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

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

static const struct pmap_devmap devmap[] = {
	{
		/*
		 * Map all of core area, this gets us everything and
		 * it's only 3MB.
		 */
		.pd_va = _A(AWIN_CORE_VBASE),
		.pd_pa = _A(AWIN_CORE_PBASE),
		.pd_size = _S(AWIN_CORE_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{
		/*
		 * Map all 1MB of SRAM area.
		 */
		.pd_va = _A(AWIN_SRAM_VBASE),
		.pd_pa = _A(AWIN_SRAM_PBASE),
		.pd_size = _S(AWIN_SRAM_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_CACHE
	},
	{0}
};

#undef	_A
#undef	_S

#ifdef PMAP_NEED_ALLOC_POOLPAGE
static struct boot_physmem bp_highgig = {
	.bp_start = AWIN_SDRAM_PBASE / NBPG,
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0,
};
#endif

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
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg)
{
	pmap_devmap_register(devmap);
	awin_bootstrap(AWIN_CORE_VBASE, CONADDR_VA);

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* The console is going to try to map things.  Give pmap a devmap. */
	consinit();

#ifdef VERBOSE_INIT_ARM
	printf("\nuboot arg = %#"PRIxPTR", %#"PRIxPTR", %#"PRIxPTR", %#"PRIxPTR"\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);
#endif

#ifdef KGDB
	kgdb_port_init();
#endif

	cpu_reset_address = awin_wdog_reset;

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (" __STRING(BOARDTYPE) ") booting ...\n");
#endif

	const uint8_t *uboot_bootinfo = (void*)uboot_args[0];

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");

#if defined(CPU_CORTEXA7) || defined(CPU_CORTEXA9) || defined(CPU_CORTEXA15)
	if (!CPU_ID_CORTEX_A8_P(curcpu()->ci_arm_cpuid)) {
		printf("initarm: cbar=%#x\n", armreg_cbar_read());
	}
#endif
#endif

	/*
	 * Set up the variables that define the availability of physical
	 * memory.
	 */
	psize_t ram_size = awin_memprobe();

#if AWIN_board == AWIN_cubieboard
	/* the cubietruck has 2GB whereas the cubieboards only has 1GB */
	cubietruck_p = (ram_size == 0x80000000);
#endif

	/*
	 * If MEMSIZE specified less than what we really have, limit ourselves
	 * to that.
	 */
#ifdef MEMSIZE
	if (ram_size == 0 || ram_size > (unsigned)MEMSIZE * 1024 * 1024)
		ram_size = (unsigned)MEMSIZE * 1024 * 1024;
#else
	KASSERTMSG(ram_size > 0, "RAM size unknown and MEMSIZE undefined");
#endif

	/*
	 * Configure DMA tags
	 */
	awin_dma_bootstrap(ram_size);

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = AWIN_SDRAM_PBASE;
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (ram_size > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		   __func__, (unsigned long) (ram_size >> 20),
		   (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else
	const bool mapallmem_p = false;
#endif
	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, devmap,
	    mapallmem_p);

	if (mapallmem_p) {
		/*
		 * "bootargs" env variable is passed as 4th argument
		 * to kernel but it's using the physical address and
		 * we to convert that to a virtual address.
		 */
		if (uboot_args[3] - AWIN_SDRAM_PBASE < ram_size) {
			const char * const args = (const char *)
			     (uboot_args[3] + KERNEL_BASE_VOFFSET);
			strlcpy(bootargs, args, sizeof(bootargs));
		}
		if (uboot_args[0]
		   && uboot_args[0] - AWIN_SDRAM_PBASE < ram_size) {
			uboot_bootinfo =
			    (void*)(uboot_args[0] + KERNEL_BASE_VOFFSET);
		}
	}

	/* copy u-boot bootinfo ethernet address */
	memcpy(uboot_enaddr, uboot_bootinfo + 0x250, sizeof(uboot_enaddr));

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	/* we've a specific device_register routine */
	evbarm_device_register = awin_device_register;

#if NAWIN_FB > 0
	char *ptr;
	if (get_bootconf_option(boot_args, "console",
		    BOOTOPT_TYPE_STRING, &ptr) && strncmp(ptr, "fb", 2) == 0) {
		use_fb_console = true;
	}
#endif
	/*
	 * If we couldn't map all of memory via TTBR1, limit the memory the
	 * kernel can allocate from to be from the highest available 1GB.
	 */
#ifdef PMAP_NEED_ALLOC_POOLPAGE
	if (atop(ram_size) > bp_highgig.bp_pages) {
		bp_highgig.bp_start += atop(ram_size) - bp_highgig.bp_pages;
		arm_poolpage_vmfreelist = bp_highgig.bp_freelist;
		return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
		    &bp_highgig, 1);
	}
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

}

#if NCOM > 0
#ifndef CONADDR
#define CONADDR		(AWIN_CORE_PBASE + AWIN_UART0_OFFSET)
#endif
#ifndef CONSPEED
#define CONSPEED 115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB | HUPCL)) | CS8) /* 8N1 */
#endif

__CTASSERT(AWIN_CORE_PBASE + AWIN_UART0_OFFSET <= CONADDR);
__CTASSERT(CONADDR <= AWIN_CORE_PBASE + AWIN_UART7_OFFSET);
__CTASSERT(CONADDR % AWIN_UART_SIZE == 0);
static const bus_addr_t conaddr = CONADDR;
static const int conspeed = CONSPEED;
static const int conmode = CONMODE;
#endif

void
consinit(void)
{
	bus_space_tag_t bst = &awin_a4x_bs_tag;
#if NCOM > 0
	bus_space_handle_t bh;
#endif
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0
	if (bus_space_map(bst, conaddr, AWIN_UART_SIZE, 0, &bh))
		panic("Serial console can not be mapped.");

	if (comcnattach(bst, conaddr, conspeed, AWIN_UART_FREQ,
		    COM_TYPE_NORMAL, conmode))
		panic("Serial console can not be initialized.");

	bus_space_unmap(bst, bh, AWIN_UART_SIZE);
#else
#error only COM console is supported

#if NUKBD > 0
	ukbd_cnattach();	/* allow USB keyboard to become console */
#endif
#endif
}

#ifdef KGDB
#ifndef KGDB_DEVADDR
#error Specify the address of the kgdb UART with the KGDB_DEVADDR option.
#endif
#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE 115200
#endif

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

__CTASSERT(KGDB_DEVADDR != CONADDR);
__CTASSERT(AWIN_CORE_PBASE + AWIN_UART0_OFFSET <= KGDB_DEVADDR);
__CTASSERT(KGDB_DEVADDR <= AWIN_CORE_PBASE + AWIN_UART7_OFFSET);
__CTASSERT(KGDB_DEVADDR % AWIN_UART_SIZE == 0);

static const vaddr_t comkgdbaddr = KGDB_DEVADDR;
static const int comkgdbspeed = KGDB_DEVRATE;
static const int comkgdbmode = KGDB_DEVMODE;

void
static kgdb_port_init(void)
{
	bus_space_tag_t bst = &awin_a4x_bs_tag;
	static bool kgdbsinit_called;

	if (kgdbsinit_called)
		return;

	kgdbsinit_called = true;

	bus_space_handle_t bh;
	if (bus_space_map(bst, comkgdbaddr, AWIN_UART_SIZE, 0, &bh))
		panic("kgdb port can not be mapped.");

	if (com_kgdb_attach(bst, comkgdbaddr, comkgdbspeed, AWIN_REF_FREQ,
		    COM_TYPE_NORMAL, comkgdbmode))
		panic("KGDB uart can not be initialized.");

	bus_space_unmap(bst, bh, AWIN_UART_SIZE);
}
#endif

void
awin_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore regisers (which armperiph uses). 
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &awin_bs_tag;
		return;
	}
 
#if defined(CPU_CORTEXA7) || defined(CPU_CORTEXA15)
	if (device_is_a(self, "armgtmr")) {
		/*
		 * The frequency of the generic timer is the reference
		 * frequency.
		 */
                prop_dictionary_set_uint32(dict, "frequency", AWIN_REF_FREQ);
		return;
	}
#endif

	if (device_is_a(self, "awinio")) {
#if AWIN_board == AWIN_cubieboard || AWIN_board == AWIN_cubietruck
		if (cubietruck_p) {
			prop_dictionary_set_bool(dict, "no-awe", true);
		} else {
			prop_dictionary_set_bool(dict, "no-awge", true);
		}
#elif AWIN_board == AWIN_bpi
		prop_dictionary_set_bool(dict, "no-awe", true);
#endif
		return;
	}

	if (device_is_a(self, "awingpio")) {
		/*
		 * These are GPIOs being used for various functions.
		 */
		prop_dictionary_set_cstring(dict, "satapwren",
		    (cubietruck_p ? ">PH12" : ">PB8"));
		prop_dictionary_set_cstring(dict, "usb0drv",
		    (cubietruck_p ? ">PH17" : ">PB2"));
		prop_dictionary_set_cstring(dict, "usb2drv", ">PH3");
		prop_dictionary_set_cstring(dict, "usb0iddet",
		    (cubietruck_p ? "<PH19" : "<PH4"));
		prop_dictionary_set_cstring(dict, "usb0vbusdet",
		    (cubietruck_p ? "<PH22" : "<PH5"));
		prop_dictionary_set_cstring(dict, "usb1drv", ">PH6");
		prop_dictionary_set_cstring(dict, "status-led1", ">PH21");
		prop_dictionary_set_cstring(dict, "status-led2", ">PH20");
		if (cubietruck_p) {
			prop_dictionary_set_cstring(dict, "status-led3", ">PH11");
			prop_dictionary_set_cstring(dict, "status-led4", ">PH7");
		} else {
			prop_dictionary_set_cstring(dict, "hdd5ven", ">PH17");
			prop_dictionary_set_cstring(dict, "emacpwren", ">PH19");
		}
		prop_dictionary_set_cstring(dict, "mmc0detect", "<PH1");
		prop_dictionary_set_cstring(dict, "audiopactrl", ">PH15");

		/*
		 * These pins have no connections.
		 */
		prop_dictionary_set_uint32(dict, "nc-b", 0x0003d0e8);
		prop_dictionary_set_uint32(dict, "nc-c", 0x00ff0000);
		prop_dictionary_set_uint32(dict, "nc-h", 0x03c53f04);
		prop_dictionary_set_uint32(dict, "nc-i", 0x003fc03f);
		return;
	}
	if (device_is_a(self, "awge") || device_is_a(self, "awe")) {
		prop_data_t blob =
		    prop_data_create_data(uboot_enaddr, ETHER_ADDR_LEN);
		prop_dictionary_set(dict, "mac-address", blob);
		prop_object_release(blob);
	}

	if (device_is_a(self, "ehci")) {
		return;
	}

	if (device_is_a(self, "ahcisata")) {
		/* PIO PB<8> / PIO PH<12> output */
		prop_dictionary_set_cstring(dict, "power-gpio", "satapwren");
		return;
	}

	if (device_is_a(self, "awinmmc")) {
		struct awinio_attach_args * const aio = aux;
		if (aio->aio_loc.loc_port == 0) {
			prop_dictionary_set_cstring(dict,
			    "detect-gpio", "mmc0detect");
			prop_dictionary_set_cstring(dict,
			    "led-gpio", "status-led2");
		}
		return;
	}

	if (device_is_a(self, "awinac")) {
		prop_dictionary_set_cstring(dict, "pactrl-gpio", "audiopactrl");
		return;
	}

	if (device_is_a(self, "com")) {
#if NAWIN_FB > 0
		if (use_fb_console)
			prop_dictionary_set_bool(dict, "is_console", false);
#endif
		return;
	}
}
