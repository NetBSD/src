/*	$Id: mmnet_machdep.c,v 1.2.38.1 2018/07/28 04:37:32 pgoyette Exp $	*/
/*	$NetBSD: mmnet_machdep.c,v 1.2.38.1 2018/07/28 04:37:32 pgoyette Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
 * Based on tsarm_machdep.c
 *
 * Copyright (c) 2001, 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Based on code written by Jason R. Thorpe and Steve C. Woodford for
 * Wasabi Systems, Inc.
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
 */

/*
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
 * Machine dependant functions for kernel setup for Iyonix.
 */
/* Adaptation for Propox MMnet by Aymeric Vincent is in the public domain */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mmnet_machdep.c,v 1.2.38.1 2018/07/28 04:37:32 pgoyette Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#define	DRAM_BLOCKS	1
#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <machine/autoconf.h>

#include "isa.h"

#include <arm/at91/at91sam9260reg.h>
#include <arm/at91/at91sam9260busvar.h>

#include "ksyms.h"

#include <arm/at91/at91busvar.h>
#include <arm/at91/at91pdcreg.h>
#include <arm/at91/at91dbgureg.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91streg.h>

/* boot configuration: */
BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

/* hmmm */
static struct arm32_dma_range mmnet_dma_ranges[4];
static struct at91bus_machdep mmnetbus;
static at91bus_tag_t old_at91bus_tag;

/* Prototypes */
static void mmnet_device_register(device_t dev, void *aux);
static void mmnetbus_init(struct at91bus_clocks *);
static void mmnetbus_peripheral_clock(int pid, int enable);
static uint32_t mmnetbus_gpio_mask(int pid);


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

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		printf("\r\n");
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
		printf("\r\nrebooting...\r\n");
		goto reset;
	}

	/* Disable console buffering */

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

	pmf_system_shutdown(boothowto);

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("\r\n");
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
	}

	printf("\r\nrebooting...\r\n");
 reset:
	/*
	 * Make really really sure that all interrupts are disabled,
	 * and poke the Internal Bus and Peripheral Bus reset lines.
	 */
	(void) disable_interrupts(I32_bit|F32_bit);
	STREG(ST_WDMR)	= ST_WDMR_EXTEN | ST_WDMR_RSTEN | 1;
	STREG(ST_CR)	= ST_CR_WDRST;
	for (;;);
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
 *   Initialising interrupt controllers to a sane default state
 */
u_int
initarm(void *arg)
{
	u_int ret;
	/*
	 * basic AT91 initialization:
	 */
	if (at91bus_init())
		panic("%s: at91bus_init() failed", __FUNCTION__);

	if (AT91_CHIP_ID() == AT91SAM9260_CHIP_ID) {
		memcpy(&mmnetbus, at91bus_tag, sizeof(mmnetbus));
		mmnetbus.init = mmnetbus_init;
		mmnetbus.peripheral_clock = mmnetbus_peripheral_clock;
		mmnetbus.gpio_mask = mmnetbus_gpio_mask;
		old_at91bus_tag = at91bus_tag;
		at91bus_tag = &mmnetbus;
	}

 	/* Fake bootconfig structure for the benefit of pmap.c */
 	/* XXX must make the memory description h/w independent */
 	bootconfig.dramblocks = 1;
 	bootconfig.dram[0].address = 0x20000000UL;
 	bootconfig.dram[0].pages =   0x04000000UL / PAGE_SIZE;
	ret = at91bus_setup(&bootconfig);

	if (AT91_CHIP_ID() != AT91SAM9260_CHIP_ID)
		panic("%s: processor is not AT91SAM9260", __FUNCTION__);

	/* we've a specific device_register routine */
	evbarm_device_register = mmnet_device_register;

	/* We return the new stack pointer address */
	return ret;
}


bus_dma_tag_t
at91_bus_dma_init(struct arm32_bus_dma_tag *dma_tag_template)
{
	int i;
	struct arm32_bus_dma_tag *dmat;

	for (i = 0; i < bootconfig.dramblocks; i++) {
		mmnet_dma_ranges[i].dr_sysbase = bootconfig.dram[i].address;
		mmnet_dma_ranges[i].dr_busbase = bootconfig.dram[i].address;
		mmnet_dma_ranges[i].dr_len = bootconfig.dram[i].pages * 
			PAGE_SIZE;
	}

	dmat = dma_tag_template;

	dmat->_ranges = mmnet_dma_ranges;
	dmat->_nranges = bootconfig.dramblocks;

	return dmat;
}

void mmnetbus_init(struct at91bus_clocks *clocks)
{
	(*old_at91bus_tag->init)(clocks);
}

uint32_t mmnetbus_gpio_mask(int pid)
{
	switch (pid) {
	case PID_PIOA:	return ~0x00000300U;
	case PID_PIOB:	return ~0x0000783FU;
	case PID_PIOC:	return ~0x00000000U;
	default:	return ~0x00000000U;
	}
}

void mmnetbus_peripheral_clock(int pid, int enable)
{
	switch (pid) {
	case PID_TWI:
		if (enable) {
			PIOA_WRITE(PIO_ASR, 0x06000000);
			PIOA_WRITE(PIO_PDR, 0x06000000);
			PIOA_WRITE(PIO_MDER, 0x06000000);
		}
		break;

	case PID_SPI0:
		if (enable) {
			PIOA_WRITE(PIO_ASR, 0x0000000f);
			PIOA_WRITE(PIO_PDR, 0x0000000f);
		}
		break;

	case PID_SPI1:
		if (enable) {
			PIOB_WRITE(PIO_ASR, 0x0000000f);
			PIOB_WRITE(PIO_PDR, 0x0000000f);
		}
		break;
	}
	(*old_at91bus_tag->peripheral_clock)(pid, enable);
}


static void mmnet_device_register(device_t dev, void *aux)
{
#if 0
	if (device_is_a(dev, "at91semac")) {
		// propagate mac address from u-boot
	}
#endif
}
