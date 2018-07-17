
/*
 * Copyright (c) 2013 Linu Cherian
 * Copyright (c) 2013 Sughosh Ganu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: hawk_machdep.c,v 1.2 2018/07/17 18:41:01 christos Exp $");

#include "opt_timer.h"
#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_md.h"
#include "opt_com.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/bus.h>

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
#include <machine/autoconf.h>
#include <arm/armreg.h>

#include <arm/locore.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/omap/omapl1x_reg.h>
#include <arm/omap/omapl1x_misc.h>
#include <arm/omap/omap_tipb.h>
#include <arm/omap/omap_var.h>

#include <evbarm/hawk/hawk.h>

#include <prop/proplib.h>

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

/* filled in before cleaning bss. keep in .data */
u_int uboot_args[4] __attribute__((__section__(".data")));

static struct arm32_dma_range omapl1x_dma_ranges[4];
extern char KERNEL_BASE_phys[];

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#define ETHER_ADDR_LEN	6

static void hawk_device_register(device_t self, void *aux);
static int get_mac_addr(char *opts, const char *opt, char *enaddr);
static void convert_mac_addr(char *addrstr, char *enaddr);

/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */
void
cpu_reboot(int howto, char *bootstr)
{
#ifdef DIAGNOSTIC
	/* info */
	printf("boot: howto=%08x curproc=%p\n", howto, curproc);
#endif

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		goto reset;
	}

	/* Disable console buffering */
/*	cnpollc(1);*/

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
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");

reset:
	/*
	 * On Davinci processor, the reset is done
	 * through watchdog timeout.
	 */
	omapl1x_reset();
	/*NOTREACHED*/
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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap devmap[] = {
	{
		.pd_va = _A(OMAPL1X_KERNEL_IO_VBASE),
		.pd_pa = _A(OMAPL138_IO_BASE),
		.pd_size = L1_S_SIZE,
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

#undef	_A
#undef	_S

static void
convert_mac_addr (char *addrstr, char *enaddr)
{
	char *end;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; ++i) {
		enaddr[i] = addrstr ? strtoul(addrstr, &end, 16) : 0;
		if (addrstr)
			addrstr = (*end) ? end + 1 : end;
	}
}

static int
get_mac_addr (char *opts, const char *opt, char *enaddr)
{
	char *ptr;
	char *optstart;

	ptr = opts;

	while (*ptr) {
		/* Find start of option */
		while (*ptr == ' ' || *ptr == '\t' || *ptr == ';')
			++ptr;

		if (*ptr == '\0')
			break;

		/* Find the end of option */
		optstart = ptr;
		while (*ptr != 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '=')
			++ptr;

		if (*ptr == '=') {
			/* compare the option */
			if (strncmp(optstart, opt, (ptr - optstart)) == 0) {
				if (*ptr =='=')
					++ptr;
				convert_mac_addr(ptr, enaddr);
				return 1;
			}
		}
		/* skip to next option */
		while (*ptr != ' ' && *ptr != '\t' && *ptr != ';' &&
		       *ptr != '\0') {
			++ptr;
		}
	}
	return 0;
}

static void
hawk_device_register (device_t self, void *aux)
{
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0 };
	prop_data_t pd;

	if (device_is_a(self, "emac")) {
		/* Get the mac address from the u-boot args */
		if (get_mac_addr(bootargs, "mac-address", enaddr)) {
			pd = prop_data_create_data(enaddr, ETHER_ADDR_LEN);
			KASSERT(pd != NULL);
			if (prop_dictionary_set(device_properties(self),
						"mac-address", pd) == false) {
				printf("Unable to set mac-address property for %s\n",
				       device_xname(self));
			}
			prop_object_release(pd);
		}
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
	vaddr_t addr;
	paddr_t emac_cppi_start, emac_cppi_end;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers */
	pmap_devmap_register(devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();
	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	strlcpy(bootargs, (char *)uboot_args[3], sizeof(bootargs));

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = OMAPL138_MEM_BASE;
	bootconfig.dram[0].pages = MEMSIZE_BYTES / PAGE_SIZE;

	arm32_bootmem_init(bootconfig.dram[0].address,
	    bootconfig.dram[0].pages * PAGE_SIZE, (uintptr_t)KERNEL_BASE_phys);

	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap,
	    false);

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

	/* we've a specific device_register routine */
	evbarm_device_register = hawk_device_register;

	addr = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

	/*
	 * Now add the emac's cppi ram to the vm pool
	 * XXX Not sure if there is a better place to
	 * do this.
	*/
	emac_cppi_start = EMAC_CPPI_RAM_BASE;
	emac_cppi_end = EMAC_CPPI_RAM_BASE + EMAC_CPPI_RAM_SIZE;
	uvm_page_physload(atop(emac_cppi_start), atop(emac_cppi_end),
			  atop(emac_cppi_start), atop(emac_cppi_end),
			  VM_FREELIST_ISADMA);

	return addr;
}

#ifndef CONSADDR
#error Specify the address of the console UART with the CONSADDR option.
#endif
#ifndef CONSPEED
#define CONSPEED 115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

static const vaddr_t consaddr = CONSADDR;
static const int conspeed = CONSPEED;
static const int conmode = CONMODE;

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	if (comcnattach(&omap_a4x_bs_tag, consaddr, conspeed,
			OMAPL1X_COM_FREQ, COM_TYPE_16550_NOERS, conmode))
		panic("Serial console can not be initialized.");
}

bus_dma_tag_t
omap_bus_dma_init(struct arm32_bus_dma_tag *dma_tag_template)
{
	int i;
	struct arm32_bus_dma_tag *dmat;

	for (i = 0; i < bootconfig.dramblocks; i++) {
		omapl1x_dma_ranges[i].dr_sysbase = bootconfig.dram[i].address;
		omapl1x_dma_ranges[i].dr_busbase = bootconfig.dram[i].address;
		omapl1x_dma_ranges[i].dr_len = bootconfig.dram[i].pages * 
			PAGE_SIZE;
	}

	dmat = dma_tag_template;

	dmat->_ranges = omapl1x_dma_ranges;
	dmat->_nranges = bootconfig.dramblocks;

	return dmat;
}

uint8_t
omapl1x_usbclk_enable(struct omapl1x_syscfg *syscfg)
{
	uint32_t timeout;
	volatile uint32_t phyclkgd, cfgchip2;

	timeout = 0x3ffffff;

	/* 
	 * Write the key sequences to the KICKnR registers to unclock the
	 * access to the SYSCFG module registers
	*/
	bus_space_write_4(syscfg->syscfg_iot, syscfg->syscfg_ioh, KICK0R,
			 SYSCFG_KICK0R_KEY);
	bus_space_write_4(syscfg->syscfg_iot, syscfg->syscfg_ioh, KICK1R,
			 SYSCFG_KICK1R_KEY);

	cfgchip2 = bus_space_read_4(syscfg->syscfg_iot, syscfg->syscfg_ioh,
				   CFGCHIP2);

	cfgchip2 &= ~USB0REF_FREQ;
	cfgchip2 |= (USB0REF_FREQ_24M | USB0PHY_PLLON | USB1SUSPENDM |
		     USB0PHYCLKMUX | USB0VBDTCTEN | USB0SESNDEN);
	cfgchip2 &= ~(USB0OTGMODE | USB0OTGPWDN | USB0PHYPWDN |
		      USB1PHYCLKMUX | RESET);

	bus_space_write_4(syscfg->syscfg_iot, syscfg->syscfg_ioh, CFGCHIP2,
			 cfgchip2);

	cfgchip2 = bus_space_read_4(syscfg->syscfg_iot, syscfg->syscfg_ioh,
				   CFGCHIP2);

	do {
		phyclkgd = bus_space_read_4(syscfg->syscfg_iot,
					    syscfg->syscfg_ioh, CFGCHIP2);
		if (phyclkgd & USB0PHYCLKGD) {
			return 1;
		}
	} while (--timeout);

	return 0;
}

u_int64_t
omapl1x_get_tc_freq (void)
{
	return (u_int64_t)OMAPL1X_TIMER3_FREQ;
}
