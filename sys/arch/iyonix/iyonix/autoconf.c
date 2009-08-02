/*	$NetBSD: autoconf.c,v 1.11 2009/08/02 11:32:05 gavan Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.11 2009/08/02 11:32:05 gavan Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/autoconf.h>
#include <machine/intr.h>

#include <iyonix/iyonix/iyonixvar.h>

#include <acorn32/include/bootconfig.h>

struct device *booted_device;
int booted_partition;

extern struct bootconfig bootconfig;

/*
 * Set up the root device from the boot args
 */
void
cpu_rootconf(void)
{
	aprint_normal("boot device: %s\n",
	    booted_device != NULL ? booted_device->dv_xname : "<unknown>");
	setroot(booted_device, booted_partition);
}


/*
 * void cpu_configure()
 *
 * Configure all the root devices
 * The root devices are expected to configure their own children
 */
void
cpu_configure(void)
{
	struct mainbus_attach_args maa;

	(void) splhigh();
	(void) splserial();	/* XXX need an splextreme() */

	maa.ma_name = "mainbus";

	config_rootfound("mainbus", &maa);

	/* Time to start taking interrupts so lets open the flood gates .... */
	spl0();
}

#define BUILTIN_ETHERNET_P(pa)	\
	((pa)->pa_bus == 0 && (pa)->pa_device == 4 && (pa)->pa_function == 0)

#define SETPROP(x, y)							\
	do {								\
		if (prop_dictionary_set(device_properties(dev),		\
						x, y) == false) {	\
			printf("WARNING: unable to set " x " "		\
			   "property for %s\n", dev->dv_xname);		\
		}							\
		prop_object_release(y);					\
	} while (/*CONSTCOND*/0)

void
device_register(struct device *dev, void *aux)
{
	struct device *pdev;

	if ((pdev = device_parent(dev)) != NULL &&
	    device_is_a(pdev, "pci")) {
		struct pci_attach_args *pa = aux;

		if (BUILTIN_ETHERNET_P(pa)) {
			prop_number_t cfg1, cfg2, swdpin;
			prop_data_t mac;

			/*
			 * We set these configuration registers to 0,
			 * because it's the closest we have to "leave them
			 * alone". That and, it works.
			 */
			cfg1 = prop_number_create_integer(0);
			KASSERT(cfg1 != NULL);
			cfg2 = prop_number_create_integer(0);
			KASSERT(cfg2 != NULL);
			swdpin = prop_number_create_integer(0);
			KASSERT(swdpin != NULL);

			mac = prop_data_create_data_nocopy(iyonix_macaddr,
							   ETHER_ADDR_LEN);
			KASSERT(mac != NULL);

			SETPROP("mac-addr", mac);
			SETPROP("i82543-cfg1", cfg1);
			SETPROP("i82543-cfg2", cfg2);
			SETPROP("i82543-swdpin", swdpin);
		}
	}

	if (device_is_a(dev, "genfb") &&
	    device_is_a(device_parent(dev), "pci") ) {
		prop_dictionary_t dict = device_properties(dev);
		struct pci_attach_args *pa = aux;
		pcireg_t bar0, bar1;
		uint32_t fbaddr;
		bus_space_handle_t vgah;

		bar0 = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_START);
		bar1 = pci_conf_read(pa->pa_pc, pa->pa_tag,
			PCI_MAPREG_START + 0x04);

		/*
		 * We need to prod the VGA card to disable interrupts, since
		 * RISC OS has been using them and we don't know how to
		 * handle them. This assumes that we have a NVidia
		 * GeForce 2 MX card as supplied with the Iyonix and
		 * as (probably) required by RISC OS in order to boot.
		 * If you write your own RISC OS driver for a different card,
		 * you're on your own.
		 */

/* We're guessing at the numbers here, guys */
#define VGASIZE 0x1000
#define IRQENABLE_ADDR 0x140

		bus_space_map(pa->pa_memt, PCI_MAPREG_MEM_ADDR(bar0), 
			VGASIZE, 0, &vgah);
		bus_space_write_4(pa->pa_memt, vgah, 0x140, 0);
		bus_space_unmap(pa->pa_memt, vgah, 0x1000);

		fbaddr = PCI_MAPREG_MEM_ADDR(bar1);

		prop_dictionary_set_bool(dict, "is_console", 1);
		prop_dictionary_set_uint32(dict, "width",
			bootconfig.width + 1);
		prop_dictionary_set_uint32(dict, "height",
			bootconfig.height + 1);
		prop_dictionary_set_uint32(dict, "depth",
			1 << bootconfig.log2_bpp);
		prop_dictionary_set_uint32(dict, "address", fbaddr);
	}
}
