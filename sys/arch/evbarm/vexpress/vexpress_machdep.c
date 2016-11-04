/*	$NetBSD: vexpress_machdep.c,v 1.2.4.1 2016/11/04 14:49:00 pgoyette Exp $	*/

/*
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Sergio L. Pascual.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vexpress_machdep.c,v 1.2.4.1 2016/11/04 14:49:00 pgoyette Exp $");

#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_arm_debug.h"

#include "ukbd.h"
#include "arml2cc.h"	// RPZ why is it not called opt_l2cc.h?

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
#include <arm/cortex/pl310_var.h>

#include <arm/arm32/machdep.h>
#include <arm/mainbus/mainbus.h>

#include <evbarm/vexpress/vexpress_var.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/vexpress/platform.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>
#include <net/if_ether.h>

#include "plcom.h"

#if NPLCOM > 0
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#define PLCONADDR 0x1c090000

#ifndef CONSDEVNAME
#define CONSDEVNAME "plcom"
#endif

#ifndef PLCONSPEED
#define PLCONSPEED B115200
#endif
#ifndef PLCONMODE
#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)	/* 8N1 */
#endif
#ifndef PLCOMCNUNIT
#define PLCOMCNUNIT -1
#endif

#if (NPLCOM > 0)
static const bus_addr_t consaddr = (bus_addr_t) PLCONADDR;

int plcomcnspeed = PLCONSPEED;
int plcomcnmode = PLCONMODE;
#endif

#if (NPLCOM > 0 && (defined(PLCONSOLE) || defined(KGDB)))
static struct plcom_instance vexpress_pi = {
	.pi_type = PLCOM_TYPE_PL011,
	.pi_flags = PLC_FLAG_32BIT_ACCESS,
	.pi_iot = &vexpress_bs_tag,
	.pi_size = PL011COM_UART_SIZE
};
#endif

/*
 * kernel start and end from the linker
 */
extern char KERNEL_BASE_phys[];	/* physical start of kernel */
extern char KERNEL_BASE_virt[];	/* virtual start of kernel */
extern char _end[];		/* physical end of kernel */
#define KERNEL_BASE_PHYS	((paddr_t)KERNEL_BASE_phys)

#define KERN_VTOPDIFF	((vaddr_t)KERNEL_BASE_phys - (vaddr_t)KERNEL_BASE_virt)
#define KERN_VTOPHYS(va)	((paddr_t)((vaddr_t)va + (vaddr_t)KERN_VTOPDIFF))
#define KERN_PHYSTOV(pa)	((vaddr_t)((paddr_t)pa - (vaddr_t)KERN_VTOPDIFF))

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;

/* prototypes */
void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif
static void vexpress_device_register(device_t, void *);

/*
 * Our static device mappings at fixed virtual addresses so we can use them
 * while booting the kernel.
 *
 * Map the extents segment-aligned and segment-rounded in size to avoid L2
 * page tables
 */

#define _A(a)   ((a) & ~L1_S_OFFSET)
#define _S(s)   (((s) + L1_S_SIZE - 1) & (~(L1_S_SIZE-1)))

static const struct pmap_devmap vexpress_devmap[] = {
	{
		/* map in core IO space */
		.pd_va = _A(VEXPRESS_CORE_VBASE),
		.pd_pa = _A(VEXPRESS_CORE_PBASE),
		.pd_size = _S(VEXPRESS_CORE_SIZE),
		.pd_prot = VM_PROT_READ | VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};
#undef _A
#undef _S

/*
 * u_int initarm(...)
 *
 * Our entry point from the assembly before main() is called.
 * - take a copy of the config we got from uboot
 * - init the physical console
 * - setting up page tables for the kernel
 */

u_int
initarm(void *arg)
{
#ifdef MEMSIZE
	psize_t memsize = (unsigned) MEMSIZE * 1024 * 1024;
#else
	/* If MEMSIZE is not defined, use QEMU's default value (128 MB) */
	psize_t memsize = (unsigned) 128 * 1024 * 1024;
#endif

	pmap_devmap_register(vexpress_devmap);

	set_cpufuncs();

	consinit();

	/* Talk to the user */
#define BDSTR(s)        _BDSTR(s)
#define _BDSTR(s)       #s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

#ifdef VERBOSE_INIT_ARM
	printf("initarm: cbar=%#x\n", armreg_cbar_read());
#endif

	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = KERN_VTOPHYS(KERNEL_BASE);
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;

	arm32_bootmem_init(bootconfig.dram[0].address, memsize,
	    (uintptr_t) KERNEL_BASE_phys);

	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, vexpress_devmap,
	    true);

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	cortex_pmc_ccnt_init();

	/* We've a specific device_register routine */
	evbarm_device_register = vexpress_device_register;

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if (NPLCOM > 0 && defined(PLCONSOLE))
	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 */
	vexpress_pi.pi_iobase = consaddr;

	plcomcnattach(&vexpress_pi, plcomcnspeed, 3000000,
	    plcomcnmode, PLCOMCNUNIT);

#endif
}

void
vexpress_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore registers (which armperiph uses).
		 */
		struct mainbus_attach_args *const mb = aux;
		mb->mb_iot = &vexpress_bs_tag;
		return;
	}
#if defined(CPU_CORTEXA7) || defined(CPU_CORTEXA15)
	if (device_is_a(self, "armgtmr")) {
		/*
		 * The frequency of the generic timer is the reference
		 * frequency.
		 */
		prop_dictionary_set_uint32(dict, "frequency", VEXPRESS_REF_FREQ);
		return;
	}
#endif
}
