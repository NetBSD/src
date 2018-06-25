/*	$NetBSD: amlogic_machdep.c,v 1.21.18.1 2018/06/25 07:25:40 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amlogic_machdep.c,v 1.21.18.1 2018/06/25 07:25:40 pgoyette Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_md.h"
#include "opt_amlogic.h"
#include "opt_arm_debug.h"
#include "opt_multiprocessor.h"

#include "amlogic_com.h"
#include "arml2cc.h"
#include "ukbd.h"
#include "genfb.h"
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

#include <dev/usb/ukbdvar.h>
#include <net/if_ether.h>

#ifndef AMLOGIC_MAX_BOOT_STRING
#define AMLOGIC_MAX_BOOT_STRING 1024
#endif

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[AMLOGIC_MAX_BOOT_STRING];
char *boot_args = NULL;
char *boot_file = NULL;
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

#ifdef VERBOSE_INIT_ARM
static void
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
static void
amlogic_putstr(const char *s)
{
	for (const char *p = s; *p; p++) {
		amlogic_putchar(*p);
	}
}
#define DPRINTF(...)		printf(__VA_ARGS__)
#define DPRINT(x)		amlogic_putstr(x)
#else
#define DPRINTF(...)
#define DPRINT(x)
#endif

static psize_t
amlogic_get_ram_size(void)
{
	const bus_space_handle_t ao_bsh =
	    AMLOGIC_CORE_VBASE + AMLOGIC_SRAM_OFFSET;
	return bus_space_read_4(&armv7_generic_bs_tag, ao_bsh, 0) << 20;
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
	DPRINT("initarm:");

	DPRINT(" devmap");
	pmap_devmap_register(devmap);

	DPRINT(" bootstrap");
	amlogic_bootstrap();

#ifdef MULTIPROCESSOR
	DPRINT(" ncpu");
	const bus_addr_t cbar = armreg_cbar_read();
	if (cbar) {
		const bus_space_handle_t scu_bsh =
		    cbar - AMLOGIC_CORE_BASE + AMLOGIC_CORE_VBASE;
		uint32_t scu_cfg = bus_space_read_4(&armv7_generic_bs_tag,
		    scu_bsh, SCU_CFG);
		arm_cpu_max = (scu_cfg & SCU_CFG_CPUMAX) + 1;
		membar_producer();
	}
#endif

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	DPRINT(" cpufunc");
	if (set_cpufuncs())
		panic("cpu not recognized!");

	DPRINT(" consinit");
	consinit();

#if NARML2CC > 0
        /*
         * Probe the PL310 L2CC
         */
	DPRINTF(" l2cc");
        const bus_space_handle_t pl310_bh =
            AMLOGIC_CORE_VBASE + AMLOGIC_PL310_OFFSET;
        arml2cc_init(&armv7_generic_bs_tag, pl310_bh, 0);
#endif

	DPRINTF(" cbar=%#x", armreg_cbar_read());

	DPRINTF(" ok\n");

	DPRINTF("uboot: args %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = amlogic_reset;

	/* Talk to the user */
	DPRINTF("\nNetBSD/evbarm (amlogic) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	DPRINTF("KERNEL_BASE=0x%x, KERNEL_VM_BASE=0x%x, KERNEL_VM_BASE - KERNEL_BASE=0x%x, KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE, KERNEL_VM_BASE, KERNEL_VM_BASE - KERNEL_BASE, KERNEL_BASE_VOFFSET);

	ram_size = amlogic_get_ram_size();

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
	DPRINTF("ram_size = 0x%x\n", (int)ram_size);
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

	if (mapallmem_p) {
		if (uboot_args[3] < ram_size) {
			const char * const args = (const char *)
			    (uboot_args[3] + KERNEL_BASE_VOFFSET);
			strlcpy(bootargs, args, sizeof(bootargs));
		}
	}

	DPRINTF("bootargs: %s\n", bootargs);

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	/* we've a specific device_register routine */
	evbarm_device_register = amlogic_device_register;

	db_trap_callback = amlogic_db_trap;

	amlogic_cpufreq_bootstrap();

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

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

#if NAMLOGIC_COM > 0
        const bus_space_handle_t bsh =
            AMLOGIC_CORE_VBASE + (consaddr - AMLOGIC_CORE_BASE);
	amlogic_com_cnattach(&armv7_generic_bs_tag, bsh, conspeed, conmode);
#else
#error only UART console is supported
#endif
}

void
amlogic_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh = amlogic_core_bsh;
	bus_size_t off = AMLOGIC_CBUS_OFFSET;

	bus_space_write_4(bst, bsh, off + WATCHDOG_TC_REG,
	    WATCHDOG_TC_CPUS | WATCHDOG_TC_ENABLE | 0xfff);
	bus_space_write_4(bst, bsh, off + WATCHDOG_RESET_REG, 0);

	for (;;) {
		__asm("wfi");
	}
}

static uint32_t
amlogic_get_boot_device(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh = amlogic_core_bsh;
	bus_size_t off = AMLOGIC_BOOTINFO_OFFSET;

	return bus_space_read_4(bst, bsh, off + 4);
}

void
amlogic_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &armv7_generic_bs_tag;
		return;
	}

	if (device_is_a(self, "cpu") && device_unit(self) == 0) {
		amlogic_cpufreq_init();
	}

	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "arma9tmr") || device_is_a(self, "a9wdt")) {
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

	if (device_is_a(self, "awge") && device_unit(self) == 0) {
		uint8_t enaddr[ETHER_ADDR_LEN];
		if (get_bootconf_option(boot_args, "awge0.mac-address",
		    BOOTOPT_TYPE_MACADDR, enaddr)) {
			prop_data_t pd = prop_data_create_data(enaddr,
			    sizeof(enaddr));
			prop_dictionary_set(dict, "mac-address", pd);
			prop_object_release(pd);
		}
	}

	if (device_is_a(self, "amlogicsdhc") ||
	    device_is_a(self, "amlogicsdio")) {
		prop_dictionary_set_uint32(dict, "boot_id",
		    amlogic_get_boot_device());
	}

#if NGENFB > 0
	if (device_is_a(self, "genfb")) {
		char *ptr;
		int scale, depth;
		amlogic_genfb_set_console_dev(self);
#ifdef DDB
		db_trap_callback = amlogic_genfb_ddb_trap_callback;
#endif
		if (get_bootconf_option(boot_args, "console",
		    BOOTOPT_TYPE_STRING, &ptr) && strncmp(ptr, "fb", 2) == 0) {
			prop_dictionary_set_bool(dict, "is_console", true);
#if NUKBD > 0
			ukbd_cnattach();
#endif
		} else {
			prop_dictionary_set_bool(dict, "is_console", false);
		}
		if (get_bootconf_option(boot_args, "fb.scale",
		    BOOTOPT_TYPE_INT, &scale) && scale > 0) {
			prop_dictionary_set_uint32(dict, "scale", scale);
		}
		if (get_bootconf_option(boot_args, "fb.depth",
		    BOOTOPT_TYPE_INT, &depth)) {
			prop_dictionary_set_uint32(dict, "depth", depth);
		}
	}
#endif
}

#if defined(MULTIPROCESSOR)
void amlogic_mpinit(uint32_t);

static void
amlogic_mpinit_delay(u_int n)
{
	for (volatile int i = 0; i < n; i++)
		;
}

static void
amlogic_mpinit_cpu(int cpu)
{
	const bus_addr_t cbar = armreg_cbar_read();
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	const bus_space_handle_t scu_bsh =
	    cbar - AMLOGIC_CORE_BASE + AMLOGIC_CORE_VBASE;
	const bus_space_handle_t ao_bsh =
	    AMLOGIC_CORE_VBASE + AMLOGIC_AOBUS_OFFSET;
	const bus_space_handle_t cbus_bsh =
	    AMLOGIC_CORE_VBASE + AMLOGIC_CBUS_OFFSET;
	uint32_t pwr_sts, pwr_cntl0, pwr_cntl1, cpuclk, mempd0;

	pwr_sts = bus_space_read_4(bst, scu_bsh, SCU_CPU_PWR_STS);
	pwr_sts &= ~(3 << (8 * cpu));
	bus_space_write_4(bst, scu_bsh, SCU_CPU_PWR_STS, pwr_sts);

	pwr_cntl0 = bus_space_read_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_CTRL0_REG);
	pwr_cntl0 &= ~((3 << 18) << ((cpu - 1) * 2));
	bus_space_write_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_CTRL0_REG, pwr_cntl0);

	amlogic_mpinit_delay(5000);

	cpuclk = bus_space_read_4(bst, cbus_bsh, AMLOGIC_CBUS_CPU_CLK_CNTL_REG);
	cpuclk |= (1 << (24 + cpu));
	bus_space_write_4(bst, cbus_bsh, AMLOGIC_CBUS_CPU_CLK_CNTL_REG, cpuclk);

	mempd0 = bus_space_read_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_MEM_PD0_REG);
	mempd0 &= ~((uint32_t)(0xf << 28) >> ((cpu - 1) * 4));
	bus_space_write_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_MEM_PD0_REG, mempd0);

	pwr_cntl1 = bus_space_read_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_CTRL1_REG);
	pwr_cntl1 &= ~((3 << 4) << ((cpu - 1) * 2));
	bus_space_write_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_CTRL1_REG, pwr_cntl1);

	amlogic_mpinit_delay(10000);

	for (;;) {
		pwr_cntl1 = bus_space_read_4(bst, ao_bsh,
		    AMLOGIC_AOBUS_PWR_CTRL1_REG) & ((1 << 17) << (cpu - 1));
		if (pwr_cntl1)
			break;
		amlogic_mpinit_delay(10000);
	}

	pwr_cntl0 = bus_space_read_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_CTRL0_REG);
	pwr_cntl0 &= ~(1 << cpu);
	bus_space_write_4(bst, ao_bsh, AMLOGIC_AOBUS_PWR_CTRL0_REG, pwr_cntl0);

	cpuclk = bus_space_read_4(bst, cbus_bsh, AMLOGIC_CBUS_CPU_CLK_CNTL_REG);
	cpuclk &= ~(1 << (24 + cpu));
	bus_space_write_4(bst, cbus_bsh, AMLOGIC_CBUS_CPU_CLK_CNTL_REG, cpuclk);

	bus_space_write_4(bst, scu_bsh, SCU_CPU_PWR_STS, pwr_sts);
}

void
amlogic_mpinit(uint32_t mpinit_vec)
{
	const bus_addr_t cbar = armreg_cbar_read();
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	volatile int i;
	uint32_t ctrl, hatched = 0;
	int cpu;

	if (cbar == 0)
		return;

	const bus_space_handle_t scu_bsh =
	    cbar - AMLOGIC_CORE_BASE + AMLOGIC_CORE_VBASE;
	const bus_space_handle_t cpuconf_bsh =
	    AMLOGIC_CORE_VBASE + AMLOGIC_CPUCONF_OFFSET;

	const uint32_t scu_cfg = bus_space_read_4(bst, scu_bsh, SCU_CFG);
	const u_int ncpus = (scu_cfg & SCU_CFG_CPUMAX) + 1;
	if (ncpus < 2)
		return;

	for (cpu = 1; cpu < ncpus; cpu++) {
		bus_space_write_4(bst, cpuconf_bsh,
		    AMLOGIC_CPUCONF_CPU_ADDR_REG(cpu), mpinit_vec);
		amlogic_mpinit_cpu(cpu);
		hatched |= __BIT(cpu);
	}
	ctrl = bus_space_read_4(bst, cpuconf_bsh, AMLOGIC_CPUCONF_CTRL_REG);
	for (cpu = 0; cpu < ncpus; cpu++) {
		ctrl |= __BIT(cpu);
	}
	bus_space_write_4(bst, cpuconf_bsh, AMLOGIC_CPUCONF_CTRL_REG, ctrl);

	__asm __volatile("sev");

	for (i = 0x10000000; i > 0; i--) {
		__asm __volatile("dmb" ::: "memory");
		if (arm_cpu_hatched == hatched)
			break;
	}
}
#endif
