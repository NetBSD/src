/*	$Id: mpcsa_machdep.c,v 1.9.28.1 2018/07/28 04:37:32 pgoyette Exp $	*/
/*	$NetBSD: mpcsa_machdep.c,v 1.9.28.1 2018/07/28 04:37:32 pgoyette Exp $	*/

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
 * Machine dependent functions for kernel setup for Iyonix.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpcsa_machdep.c,v 1.9.28.1 2018/07/28 04:37:32 pgoyette Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_pmap_debug.h"

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
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <arm/locore.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>

#define	DRAM_BLOCKS	1
#include <machine/bootconfig.h>
#include <machine/autoconf.h>

#include "seeprom.h"
#if NSEEPROM > 0
#include <net/if.h>
#include <net/if_ether.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/at24cxxvar.h>
#endif
#include <arm/at91/at91twivar.h>

#if TODO
#include "epcom.h"
#if NEPCOM > 0
#include <arm/ep93xx/epcomvar.h>
#endif

#include "isa.h"
#if NISA > 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#include <machine/isa_machdep.h>

#include <evbarm/mpcsa/mpcsareg.h>
#endif	// TODO

#include <arm/at91/at91rm9200reg.h>
#include <arm/at91/at91rm9200busvar.h>

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
static struct arm32_dma_range mpcsa_dma_ranges[4];
static struct at91bus_machdep mpcsabus;
static at91bus_tag_t old_at91bus_tag;

/* Prototypes */
static void mpcsa_device_register(device_t dev, void *aux);
static void mpcsabus_init(struct at91bus_clocks *);
static void mpcsabus_peripheral_clock(int pid, int enable);
static uint32_t mpcsabus_gpio_mask(int pid);


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

	if (AT91_CHIP_ID() == AT91RM9200_CHIP_ID) {
		memcpy(&mpcsabus, at91bus_tag, sizeof(mpcsabus));
		mpcsabus.init = mpcsabus_init;
		mpcsabus.peripheral_clock = mpcsabus_peripheral_clock;
		mpcsabus.gpio_mask = mpcsabus_gpio_mask;
		old_at91bus_tag = at91bus_tag;
		at91bus_tag = &mpcsabus;
	}

 	/* Fake bootconfig structure for the benefit of pmap.c */
 	/* XXX must make the memory description h/w independent */
 	bootconfig.dramblocks = 1;
 	bootconfig.dram[0].address = 0x20000000UL;
 	bootconfig.dram[0].pages =   0x04000000UL / PAGE_SIZE;
	ret = at91bus_setup(&bootconfig);

	if (AT91_CHIP_ID() != AT91RM9200_CHIP_ID)
		panic("%s: processor is not AT91RM9200", __FUNCTION__);

	/* we've a specific device_register routine */
	evbarm_device_register = mpcsa_device_register;

	/* We return the new stack pointer address */
	return ret;
}


bus_dma_tag_t
at91_bus_dma_init(struct arm32_bus_dma_tag *dma_tag_template)
{
	int i;
	struct arm32_bus_dma_tag *dmat;

	for (i = 0; i < bootconfig.dramblocks; i++) {
		mpcsa_dma_ranges[i].dr_sysbase = bootconfig.dram[i].address;
		mpcsa_dma_ranges[i].dr_busbase = bootconfig.dram[i].address;
		mpcsa_dma_ranges[i].dr_len = bootconfig.dram[i].pages * 
			PAGE_SIZE;
	}

	dmat = dma_tag_template;

	dmat->_ranges = mpcsa_dma_ranges;
	dmat->_nranges = bootconfig.dramblocks;

	return dmat;
}

void mpcsabus_init(struct at91bus_clocks *clocks)
{
	(*old_at91bus_tag->init)(clocks);
}

uint32_t mpcsabus_gpio_mask(int pid)
{
	switch (pid) {
	case PID_PIOA:	return ~0x00000300U;
	case PID_PIOB:	return ~0x0000783FU;
	case PID_PIOC:	return ~0x00000000U;
	case PID_PIOD:	return ~0x0003F000U;
	default:	return ~0x00000000U;
	}
}

void mpcsabus_peripheral_clock(int pid, int enable)
{
	switch (pid) {
	case PID_TWI:
		if (enable) {
			PIOA_WRITE(PIO_ASR, 0x06000000);	// assign to peripheral A
			PIOA_WRITE(PIO_PDR, 0x06000000);	// assign to peripherals
			PIOA_WRITE(PIO_MDER, 0x06000000);	// I2C pins in open-drain mode
		}
		break;

	case PID_SPI:
		if (enable) {
			PIOA_WRITE(PIO_ASR, 0x00000007);	// assign to peripheral A
			PIOA_WRITE(PIO_PDR, 0x00000007);	// assign to peripherals
		}
		break;
	}
	(*old_at91bus_tag->peripheral_clock)(pid, enable);
}


static void mpcsa_device_register(device_t dev, void *aux)
{
	static uint8_t eth_addr[ETHER_ADDR_LEN];

	if (device_is_a(dev, "at91emac")) {
		cfdriver_t cd = config_cfdriver_lookup("at91twi");
		device_t twi_dev = 0;
		i2c_tag_t i2c = 0;
		if (cd && (twi_dev = device_lookup(cd, 0)) != NULL) {
			struct at91twi_softc *sc = device_private(twi_dev);
			i2c = &sc->sc_i2c;
		}
		if (i2c && seeprom_bootstrap_read(i2c, 0x50, 0x00, 4096,
					   eth_addr, ETHER_ADDR_LEN) == 0) {
			prop_data_t pd = prop_data_create_data_nocopy(
				eth_addr, ETHER_ADDR_LEN);
			KASSERT(pd != NULL);
			if (prop_dictionary_set(device_properties(dev),
						"mac-address", pd) == FALSE) {
				printf("WARNING: unable to set mac-addr property "
				       "for %s\n", device_xname(dev));
			}
		} else {
			printf("%s: WARNING: unable to read MAC address from SEEPROM\n",
			       device_xname(dev));
		}
	}
}
