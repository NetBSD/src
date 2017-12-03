/*	$NetBSD: machdep.c,v 1.11.12.2 2017/12/03 11:36:11 jdolecek Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.11.12.2 2017/12/03 11:36:11 jdolecek Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_ns.h"
#include "opt_ipkdb.h"

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
#include <sys/malloc.h>
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
#include <machine/pmppc.h>

#include <powerpc/db_machdep.h>
#include <powerpc/pio.h>
#include <powerpc/pmap.h>
#include <powerpc/trap.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pic/picvar.h>

#include <ddb/db_extern.h>

#include <dev/cons.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700uic.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "ksyms.h"

struct powerpc_bus_space pmppc_mem_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0, 0, 0xffffffff,
	NULL,
};
struct powerpc_bus_space pmppc_pci_io_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0, CPC_PCI_IO_BASE, 0xffffffff,
	NULL,
};

static char ex_storage[1][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));


#ifdef KGDB
char kgdb_devname[] = KGDB_DEVNAME;
int comkgdbaddr = KGDB_DEVADDR;
int comkgdbrate = KGDB_DEVRATE;

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comkgdbmode = KGDB_DEVMODE;

void kgdb_port_init(void);
#endif /* KGDB */

/*
 * Global variables used here and there
 */
struct mem_region physmemr[2], availmemr[2];

struct a_config a_config;

void initppc(u_int, u_int, u_int, void *); /* Called from locore */
void pmppc_setup(void);
void setleds(int leds);

void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{
	extern void consinit(void);
	extern u_long ticks_per_sec;
	extern unsigned char edata[], end[];

	memset(&edata, 0, end - edata); /* clear BSS */

	pmppc_setup();

	physmemr[0].start = 0;
	physmemr[0].size = a_config.a_mem_size;
	physmemr[1].size = 0;
	availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
	availmemr[0].size = a_config.a_mem_size - availmemr[0].start;
	availmemr[1].size = 0;

#ifdef BOOTHOWTO
	/*
	 * boothowto
	 */
	boothowto = BOOTHOWTO;
#endif

	if (bus_space_init(&pmppc_mem_tag, "iomem",
			   ex_storage[0], sizeof(ex_storage[0])))
		panic("bus_space_init failed");

	/*
	 * Initialize the BAT registers
	 */
	oea_batinit(
	    PMPPC_FLASH_BASE, BAT_BL_256M, /* flash (etc) memory 256M area */
	    CPC_PCI_MEM_BASE, BAT_BL_256M, /* PCI memory 256M area */
	    CPC_PCI_IO_BASE,  BAT_BL_128M, /* PCI I/O 128M area */
	    0);

	/*
	 * Set up trap vectors
	 */
	oea_init(NULL);

	/*
	 * Get CPU clock
	 */
	ticks_per_sec = a_config.a_bus_freq;
	ticks_per_sec /= 4;	/* 4 cycles per DEC tick */
	cpu_timebase = ticks_per_sec;

	/*
	 * Set up console.
	 */
	consinit();		/* XXX should not be here */

	printf("console set up\n");

	uvm_md_init();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#ifdef IPKDB
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
#ifdef KGDB
	kgdb_port_init();
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif
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

	oea_startup(NULL);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/* Set up the PCI bus tag. */
	if (bus_space_init(&pmppc_pci_io_tag, "pcimem", NULL, 0))
		panic("bus_space_init pci failed");

	/* Set up interrupt controller */
	cpc700_init_intr(&pmppc_mem_tag, CPC_UIC_BASE,
	    CPC_INTR_MASK(PMPPC_I_ETH_INT), 0);

	pic_init();
	(void)setup_cpc700();
	oea_install_extint(pic_ext_intr);

#if 0
/* XXX doesn't seem to be needed anymore */
	/*
	 * Now allow hardware interrupts.
	 */
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
	    : "=r"(msr) : "K"(PSL_EE));
#endif
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{
	static int initted;
#if (NCOM > 0)
	bus_space_tag_t tag;
#endif

	if (initted)
		return;
	initted = 1;

#if (NCOM > 0)
	tag = &pmppc_mem_tag;

	if(comcnattach(tag, CPC_COM0, 9600, CPC_COM_SPEED(a_config.a_bus_freq),
	    COM_TYPE_NORMAL,
            ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
		panic("can't init serial console");
	else
		return;
#endif

	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
}

#ifdef KGDB
void
kgdb_port_init(void)
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = &pmppc_mem_tag;
		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate,
				CPC_COM_SPEED(a_config.a_bus_freq),
				COM_TYPE_NORMAL, comkgdbmode);
	}
#endif
}
#endif

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;
	extern void disable_intr(void);

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		printf("halted\n\n");
		while(1);
	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
	doshutdownhooks();

	pmf_system_shutdown(boothowto);
	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;

	disable_intr();

        /* Write the two byte reset sequence to the reset register. */
	out8(PMPPC_RESET, PMPPC_RESET_SEQ_STEP1);
	out8(PMPPC_RESET, PMPPC_RESET_SEQ_STEP2);

	while (1);
}

void
setleds(int leds)
{
	out8(PMPPC_LEDS, leds);
}

void
pmppc_setup(void)
{
	uint config0, config1;

	config0 = in8(PMPPC_CONFIG0);
	config1 = in8(PMPPC_CONFIG1);

	/* from page 2-8 in the Artesyn User's manual */
	a_config.a_boot_device = config1 & 0x80 ? A_BOOT_FLASH : A_BOOT_ROM;
	a_config.a_has_ecc = (config1 & 0x40) != 0;
	switch (config1 & 0x30) {
	case 0x00: a_config.a_mem_size = 32 * 1024 * 1024; break;
	case 0x10: a_config.a_mem_size = 64 * 1024 * 1024; break;
	case 0x20: a_config.a_mem_size = 128 * 1024 * 1024; break;
	case 0x30: a_config.a_mem_size = 256 * 1024 * 1024; break;
	}
	a_config.a_l2_cache = (config1 >> 2) & 3;
	switch (config1 & 0x03) {
	case 0x00: a_config.a_bus_freq = 66666666; break;
	case 0x01: a_config.a_bus_freq = 83333333; break;
	case 0x02: a_config.a_bus_freq = 100000000; break;
	case 0x03: a_config.a_bus_freq = 0; break; /* XXX */
	}
	a_config.a_is_monarch = (config0 & 0x80) == 0;
	a_config.a_has_eth = (config0 & 0x20) != 0;
	a_config.a_has_rtc = (config0 & 0x10) == 0;
	switch (config0 & 0x0c) {
	case 0x00: a_config.a_flash_size = 256 * 1024 * 1024; break;
	case 0x04: a_config.a_flash_size = 128 * 1024 * 1024; break;
	case 0x08: a_config.a_flash_size = 64 * 1024 * 1024; break;
	case 0x0c: a_config.a_flash_size = 32 * 1024 * 1024; break;
	}
	switch (config0 & 0x03) {
	case 0x00: a_config.a_flash_width = 64; break;
	case 0x01: a_config.a_flash_width = 32; break;
	case 0x02: a_config.a_flash_width = 16; break;
	case 0x03: a_config.a_flash_width = 0; break;
	}
}
