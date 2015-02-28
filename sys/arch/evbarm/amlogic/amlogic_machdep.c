/*	$NetBSD: amlogic_machdep.c,v 1.8 2015/02/28 18:50:15 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amlogic_machdep.c,v 1.8 2015/02/28 18:50:15 jmcneill Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_amlogic.h"
#include "opt_arm_debug.h"

#include "amlogic_com.h"
#if 0
#include "prcm.h"
#include "sdhc.h"
#include "ukbd.h"
#endif
#include "arml2cc.h"
#include "act8846pm.h"
#include "ether.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/atomic.h>
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

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_crureg.h>
#include <arm/amlogic/amlogic_var.h>
#include <arm/amlogic/amlogic_comreg.h>
#include <arm/amlogic/amlogic_comvar.h>

#include <arm/cortex/pl310_reg.h>
#include <arm/cortex/scu_reg.h>

#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/pl310_var.h>

#include <arm/cortex/gtmr_var.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/amlogic/platform.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>
#include <net/if_ether.h>

#ifndef AMLOGIC_MAX_BOOT_STRING
#define AMLOGIC_MAX_BOOT_STRING 1024
#endif

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[AMLOGIC_MAX_BOOT_STRING];
char *boot_args = NULL;
char *boot_file = NULL;
#if 0
static uint8_t amlogic_edid[128];	/* EDID storage */
#endif
u_int uboot_args[4] = { 0 };	/* filled in by amlogic_start.S (not in bss) */

/* Same things, but for the free (unused by the kernel) memory. */

extern char KERNEL_BASE_phys[];
extern char _end[];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)
#define	AMLOGIC_CORE_VOFFSET	(AMLOGIC_CORE_VBASE - AMLOGIC_CORE_BASE)
/* Prototypes */

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

static void init_clocks(void);
static void amlogic_device_register(device_t, void *);
static void amlogic_reset(void);

bs_protos(bs_notimpl);

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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap devmap[] = {
	{
		.pd_va = _A(AMLOGIC_CORE_VBASE),
		.pd_pa = _A(AMLOGIC_CORE_BASE),
		.pd_size = _S(AMLOGIC_CORE_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

#undef	_A
#undef	_S

#ifdef DDB
static void
amlogic_db_trap(int where)
{
	/* NOT YET */
}
#endif

void amlogic_putchar(char c);
void
amlogic_putchar(char c)
{
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;
	int timo = 150000;

	while ((uartaddr[UART_STATUS_REG/4] & UART_STATUS_TX_EMPTY) == 0) {
		if (--timo == 0)
			break;
	}

	uartaddr[UART_WFIFO_REG/4] = c;

	while ((uartaddr[UART_STATUS_REG/4] & UART_STATUS_TX_EMPTY) == 0) {
		if (--timo == 0)
			break;
	}
}

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
	psize_t ram_size = 0;
	*(volatile int *)CONSADDR_VA  = 0x40;	/* output '@' */

	amlogic_putchar('d');
	pmap_devmap_register(devmap);

	amlogic_putchar('b');
	amlogic_bootstrap();

	amlogic_putchar('!');

#ifdef MULTIPROCESSOR
	uint32_t scu_cfg = bus_space_read_4(&amlogic_bs_tag,
	    amlogic_core0_bsh, ROCKCHIP_SCU_OFFSET + SCU_CFG);
	arm_cpu_max = (scu_cfg & SCU_CFG_CPUMAX) + 1;
	membar_producer();
#endif

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	init_clocks();

	consinit();
#ifdef MULTIPROCESSOR
	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
#endif

#if NARML2CC > 0
        /*
         * Probe the PL310 L2CC
         */
	printf("probe the PL310 L2CC\n");
        const bus_space_handle_t pl310_bh =
            AMLOGIC_CORE_VBASE + AMLOGIC_PL310_OFFSET;
        arml2cc_init(&amlogic_bs_tag, pl310_bh, 0);
        amlogic_putchar('l');
#endif

	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

#ifdef KGDB
	kgdb_port_init();
#endif

	cpu_reset_address = amlogic_reset;

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (amlogic) booting ...\n");
#endif

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");

	printf("initarm: cbar=%#x\n", armreg_cbar_read());
	printf("KERNEL_BASE=0x%x, KERNEL_VM_BASE=0x%x, KERNEL_VM_BASE - KERNEL_BASE=0x%x, KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE, KERNEL_VM_BASE, KERNEL_VM_BASE - KERNEL_BASE, KERNEL_BASE_VOFFSET);
#endif

#if notyet
	ram_size = amlogic_get_ram_size();
#endif

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	if (ram_size > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (ram_size >> 20),     
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif

	/*
	 * If MEMSIZE specified less than what we really have, limit ourselves
	 * to that.
	 */
#ifdef MEMSIZE
	if (ram_size == 0 || ram_size > (unsigned)MEMSIZE * 1024 * 1024)
		ram_size = (unsigned)MEMSIZE * 1024 * 1024;
	printf("ram_size = 0x%x\n", (int)ram_size);
#else
	KASSERTMSG(ram_size > 0, "RAM size unknown and MEMSIZE undefined");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = 0x00000000; /* DDR PHY addr */
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
	KASSERT(ram_size <= KERNEL_VM_BASE - KERNEL_BASE);
#else
	const bool mapallmem_p = false;
#endif
	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap,
	    mapallmem_p);

	printf("bootargs: %s\n", bootargs);

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	/* we've a specific device_register routine */
	evbarm_device_register = amlogic_device_register;

	db_trap_callback = amlogic_db_trap;

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

}

static void
init_clocks(void)
{
	/* NOT YET */
}

#if NAMLOGIC_COM > 0
#ifndef CONSADDR
#error Specify the address of the console UART with the CONSADDR option.
#endif
#ifndef CONSPEED
#define CONSPEED 115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

static const bus_addr_t consaddr = CONSADDR;
static const int conspeed = CONSPEED;
static const int conmode = CONMODE;
#endif

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	amlogic_putchar('e');

#if NAMLOGIC_COM > 0
        const bus_space_handle_t bsh =
            AMLOGIC_CORE_VBASE + (consaddr - AMLOGIC_CORE_BASE);
	amlogic_com_cnattach(&amlogic_bs_tag, bsh, conspeed, conmode);
#endif

#if NUKBD > 0
	ukbd_cnattach();	/* allow USB keyboard to become console */
#endif

	amlogic_putchar('f');
	amlogic_putchar('g');
}

void
amlogic_reset(void)
{
	bus_space_tag_t bst = &amlogic_bs_tag;
	bus_space_handle_t bsh = amlogic_core_bsh;
	bus_size_t off = AMLOGIC_CBUS_OFFSET;

	bus_space_write_4(bst, bsh, off + WATCHDOG_TC_REG,
	    WATCHDOG_TC_CPUS | WATCHDOG_TC_ENABLE | 1);
	bus_space_write_4(bst, bsh, off + WATCHDOG_RESET_REG, 0);

	for (;;) {
		__asm("wfi");
	}
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
static const vaddr_t comkgdbaddr = KGDB_DEVADDR;
static const int comkgdbspeed = KGDB_DEVRATE;
static const int comkgdbmode = KGDB_DEVMODE;

void
static kgdb_port_init(void)
{
	static int kgdbsinit_called = 0;

	if (kgdbsinit_called != 0)
		return;

	kgdbsinit_called = 1;

	bus_space_handle_t bh;
	if (bus_space_map(&amlogic_a4x_bs_tag, comkgdbaddr, ROCKCHIP_COM_SIZE, 0, &bh))
		panic("kgdb port can not be mapped.");

	if (com_kgdb_attach(&amlogic_a4x_bs_tag, comkgdbaddr, comkgdbspeed,
			ROCKCHIP_COM_FREQ, COM_TYPE_NORMAL, comkgdbmode))
		panic("KGDB uart can not be initialized.");

	bus_space_unmap(&amlogic_a4x_bs_tag, bh, ROCKCHIP_COM_SIZE);
}
#endif

void
amlogic_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &amlogic_bs_tag;
		return;
	}

	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "a9tmr") || device_is_a(self, "a9wdt")) {
                prop_dictionary_set_uint32(dict, "frequency",
		    amlogic_get_rate_a9periph());

		return;
	}

	if (device_is_a(self, "arml2cc")) {
		/*
		 * L2 cache regs are at C4200000 and A9 periph base is
		 * at C4300000; pass as a negative offset for the benefit
		 * of armperiph bus.
		 */
		prop_dictionary_set_uint32(dict, "offset", 0xfff00000);
	}
}
