/* $Id: imx23_olinuxino_machdep.c,v 1.2.4.4 2017/12/03 11:36:05 jdolecek Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include "opt_imx.h"

#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/types.h>

#include <uvm/uvm_prot.h>

#include <machine/bootconfig.h>
#include <machine/db_machdep.h>
#include <machine/pmap.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/arm32/machdep.h>
#include <arm/arm32/pte.h>

#include <arm/imx/imx23_clkctrlreg.h>
#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_rtcreg.h>
#include <arm/imx/imx23_uartdbgreg.h>
#include <arm/imx/imx23var.h>

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#include "opt_evbarm_boardtype.h"
#include "opt_machdep.h"

#define	KERNEL_VM_BASE	(KERNEL_BASE + 0x8000000)
#define	KERNEL_VM_SIZE	0x20000000

#define L1_PAGE_TABLE (DRAM_BASE + MEMSIZE * 1024 * 1024 - L1_TABLE_SIZE)
#define BOOTIMX23_ARGS (L1_PAGE_TABLE - MAX_BOOT_STRING - 1)

#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#define PLCONSPEED 115200

#define REG_RD(reg) *(volatile uint32_t *)(reg)
#define REG_WR(reg, val)						\
do {									\
	*(volatile uint32_t *)((reg)) = val;				\
} while (0)

#define KERNEL_BASE_PHYS ((paddr_t)&KERNEL_BASE_phys)
#define KERNEL_BASE_VIRT ((vaddr_t)&KERNEL_BASE_virt)
#define KERN_VTOPHYS(va) \
        ((paddr_t)((vaddr_t)(va) - KERNEL_BASE + KERNEL_BASE_PHYS))
#define KERN_PHYSTOV(pa) \
        ((vaddr_t)((paddr_t)(pa) + KERNEL_BASE_VIRT + KERNEL_BASE))

/*
 * Static device map for i.MX23 peripheral address space.
 */
#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))
static const struct pmap_devmap devmap[] = {
	{
		_A(APBH_BASE),			/* Virtual address. */
		_A(APBH_BASE),			/* Physical address. */
		_S(APBH_SIZE + APBX_SIZE),	/* APBX located after APBH. */
		VM_PROT_READ|VM_PROT_WRITE,	/* Protection bits. */
		PTE_NOCACHE			/* Cache attributes. */
	},
	{ 0, 0, 0, 0, 0 }
};
#undef _A
#undef _S

static struct plcom_instance imx23_pi = {
	.pi_type = PLCOM_TYPE_PL011,
	.pi_iot = &imx23_bus_space,
	.pi_size = PL011COM_UART_SIZE,
	.pi_iobase = HW_UARTDBG_BASE
};

extern char KERNEL_BASE_phys;
extern char KERNEL_BASE_virt;
BootConfig bootconfig;
char *boot_args;
static char kernel_boot_args[MAX_BOOT_STRING];

#define SSP_DIV 2
#define IO_FRAC 27

static void power_vddio_from_dcdc(int, int);
static void set_ssp_div(unsigned int);
static void set_io_frac(unsigned int);
static void bypass_ssp(void);

/*
 * Initialize ARM and return new SVC stack pointer.
 */
u_int
initarm(void *arg)
{
        psize_t ram_size;

	if (set_cpufuncs())
		panic("set_cpufuncs failed");

	pmap_devmap_register(devmap);
	consinit();

#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");
#undef BDSTR
#undef _BDSTR

	/*
	 * SSP_CLK setup was postponed here from bootimx23 because SB wasn't
	 * able to load kernel if clocks were changed.
	 */
	power_vddio_from_dcdc(3300, 2925);
	set_ssp_div(SSP_DIV);
	set_io_frac(IO_FRAC);
	bypass_ssp();

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	/* Copy boot arguments passed from bootimx23. */
	boot_args = (char *)KERN_PHYSTOV(BOOTIMX23_ARGS);
	memcpy(kernel_boot_args, boot_args, MAX_BOOT_STRING);
#ifdef BOOT_ARGS
	strcpy(kernel_boot_args, BOOT_ARGS);
#endif
	boot_args = kernel_boot_args;
#ifdef VERBOSE_INIT_ARM
	printf("boot_args @ %lx: '%s'\n", KERN_PHYSTOV(BOOTIMX23_ARGS),
	    boot_args);
#endif
	parse_mi_bootargs(boot_args);

	ram_size = MEMSIZE * 1024 * 1024;

	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = DRAM_BASE;
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;
	bootconfig.dram[0].flags = BOOT_DRAM_CAN_DMA | BOOT_DRAM_PREFER;

        arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
            ((vsize_t)&KERNEL_BASE_phys));

        arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap,
	    false);

        return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

/*
 * Initialize console.
 */
void
consinit(void)
{
	/* consinit() is called from also from the main(). */
	static int consinit_called = 0;

	if (consinit_called)
		return;

	plcomcnattach(&imx23_pi, PLCONSPEED, IMX23_UART_CLK, PLCONMODE, 0);

	consinit_called = 1;

	return;
}

/*
 * Reboot or halt the system.
 */
void
cpu_reboot(int howto, char *bootstr)
{
	static int cpu_reboot_called = 0;

	boothowto |= howto;
	
	/*
	 * If this is the first invocation of cpu_reboot() and the RB_NOSYNC
	 * flag is not set in howto; sync and unmount the system disks by
	 * calling vfs_shutdown(9) and set the time of day clock by calling
	 * resettodr(9).
	 */
	if (!cpu_reboot_called && !(boothowto & RB_NOSYNC)) {
		vfs_shutdown();
		resettodr();
	}

	cpu_reboot_called = 1;

	IRQdisable;	/* FIQ's stays on because they are special. */

	/*
	 * If rebooting after a crash (i.e., if RB_DUMP is set in howto, but
	 * RB_HALT is not), save a system crash dump.
	 */
	if ((boothowto & RB_DUMP) && !(boothowto & RB_HALT)) {
		panic("please implement crash dump!"); // XXX
		for(;;);
		/* NOTREACHED */
	}

	/* Run any shutdown hooks by calling pmf_system_shutdown(9). */
	pmf_system_shutdown(boothowto);

	printf("system %s.\n", boothowto & RB_HALT ? "halted" : "rebooted");

	if (boothowto & RB_HALT) {
		/* Enable i.MX233 wait-for-interrupt mode. */
		REG_WR(HW_CLKCTRL_BASE + HW_CLKCTRL_CPU,
		    (REG_RD(HW_CLKCTRL_BASE + HW_CLKCTRL_CPU) |
		    HW_CLKCTRL_CPU_INTERRUPT_WAIT));

		/* Disable FIQ's and wait for interrupt (which never arrives) */
		__asm volatile(				\
		    "mrs r0, cpsr\n\t"			\
		    "orr r0, #0x40\n\t"			\
		    "msr cpsr_c, r0\n\t"		\
		    "mov r0, #0\n\t"			\
		    "mcr p15, 0, r0, c7, c0, 4\n\t"
		);

		for(;;);

		/* NOT REACHED */
	}

	/* Reboot the system. */
	REG_WR(HW_RTC_BASE + HW_RTC_WATCHDOG, 10000);
	REG_WR(HW_RTC_BASE + HW_RTC_CTRL_SET, HW_RTC_CTRL_WATCHDOGEN);
	REG_WR(HW_RTC_BASE + HW_RTC_WATCHDOG, 0);

	for(;;);

	/* NOT REACHED */
}

/*
 * Delay us microseconds.
 */
void
delay(unsigned int us)
{
	uint32_t start;
	uint32_t now;
	uint32_t elapsed;
	uint32_t total;
	uint32_t last;

	total = 0;
	last = 0;
	start = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

	do {
		now = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

		if (start <= now)
			elapsed = now - start;
		else	/* Take care of overflow. */
			elapsed = (UINT32_MAX - start) + 1 + now;

		total += elapsed - last;
		last = elapsed;

	} while (total < us);

	return;
}
#include <arm/imx/imx23_powerreg.h>
#define PWR_VDDIOCTRL   (HW_POWER_BASE + HW_POWER_VDDIOCTRL)
#define PWR_CTRL        (HW_POWER_BASE + HW_POWER_CTRL)
#define PWR_CTRL_S      (HW_POWER_BASE + HW_POWER_CTRL_SET)
#define PWR_CTRL_C      (HW_POWER_BASE + HW_POWER_CTRL_CLR)

static void
power_vddio_from_dcdc(int target, int brownout)
{
        uint32_t tmp_r;

        /* BO_OFFSET must be withing 2700mV - 3475mV */
        if (brownout > 3475)
                brownout = 3475;
        else if (brownout < 2700)
                brownout = 2700;


        /* Set LINREG_OFFSET one step below TRG. */
        tmp_r = REG_RD(PWR_VDDIOCTRL);
        tmp_r &= ~HW_POWER_VDDIOCTRL_LINREG_OFFSET;
        tmp_r |= __SHIFTIN(2, HW_POWER_VDDIOCTRL_LINREG_OFFSET);
        REG_WR(PWR_VDDIOCTRL, tmp_r);
        delay(10000);

        /* Enable VDDIO switching converter output. */
        tmp_r = REG_RD(PWR_VDDIOCTRL);
        tmp_r &= ~HW_POWER_VDDIOCTRL_DISABLE_FET;
        REG_WR(PWR_VDDIOCTRL, tmp_r);
        delay(10000);

        /* Set target voltage and brownout level. */
        tmp_r = REG_RD(PWR_VDDIOCTRL);
        tmp_r &= ~(HW_POWER_VDDIOCTRL_BO_OFFSET | HW_POWER_VDDIOCTRL_TRG);
        tmp_r |= __SHIFTIN(((target - brownout) / 25),
                HW_POWER_VDDIOCTRL_BO_OFFSET);
        tmp_r |= __SHIFTIN(((target - 2800) / 25), HW_POWER_VDDIOCTRL_TRG);
        REG_WR(PWR_VDDIOCTRL, tmp_r);
        delay(10000);

        /* Enable PWDN_BRNOUT. */
        REG_WR(PWR_CTRL_C, HW_POWER_CTRL_VDDIO_BO_IRQ);
        
        tmp_r = REG_RD(PWR_VDDIOCTRL);
        tmp_r |= HW_POWER_VDDIOCTRL_PWDN_BRNOUT;
        REG_WR(PWR_VDDIOCTRL, tmp_r);

        return;
}
#include <arm/imx/imx23_clkctrlreg.h>
#define CLKCTRL_SSP     (HW_CLKCTRL_BASE + HW_CLKCTRL_SSP)
#define CLKCTRL_FRAC    (HW_CLKCTRL_BASE + HW_CLKCTRL_FRAC)
#define CLKCTRL_SEQ_C   (HW_CLKCTRL_BASE + HW_CLKCTRL_CLKSEQ_CLR)

static
void set_ssp_div(unsigned int div)
{
        uint32_t tmp_r;

        tmp_r = REG_RD(CLKCTRL_SSP);
        tmp_r &= ~HW_CLKCTRL_SSP_CLKGATE;
        REG_WR(CLKCTRL_SSP, tmp_r);

        while (REG_RD(CLKCTRL_SSP) & HW_CLKCTRL_SSP_BUSY)
                ;

        tmp_r = REG_RD(CLKCTRL_SSP);
        tmp_r &= ~HW_CLKCTRL_SSP_DIV;
        tmp_r |= __SHIFTIN(div, HW_CLKCTRL_SSP_DIV);
        REG_WR(CLKCTRL_SSP, tmp_r);

        while (REG_RD(CLKCTRL_SSP) & HW_CLKCTRL_SSP_BUSY)
                ;

        return;

}
static
void set_io_frac(unsigned int frac)
{
        uint8_t *io_frac;
        uint32_t tmp_r;
        
        io_frac = (uint8_t *)(CLKCTRL_FRAC);
        io_frac++; /* emi */
        io_frac++; /* pix */
        io_frac++; /* io */
        tmp_r = (*io_frac)<<24;
        tmp_r &= ~(HW_CLKCTRL_FRAC_CLKGATEIO | HW_CLKCTRL_FRAC_IOFRAC);
        tmp_r |= __SHIFTIN(frac, HW_CLKCTRL_FRAC_IOFRAC);

        *io_frac = (uint8_t)(tmp_r>>24);

        return;
}
static
void bypass_ssp(void)
{
        REG_WR(CLKCTRL_SEQ_C, HW_CLKCTRL_CLKSEQ_BYPASS_SSP);

        return;
}


