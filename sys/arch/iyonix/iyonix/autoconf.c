/*	$NetBSD: autoconf.c,v 1.2.6.1 2006/04/22 11:37:40 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2.6.1 2006/04/22 11:37:40 simonb Exp $");

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

struct device *booted_device;
int booted_partition;

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

	/* Initialize software interrupts. */
	softintr_init();

	maa.ma_name = "mainbus";

	config_rootfound("mainbus", &maa);

	/* Time to start taking interrupts so lets open the flood gates .... */
	spl0();
}

#define BUILTIN_ETHERNET_P(pa)	\
	((pa)->pa_bus == 0 && (pa)->pa_device == 4 && (pa)->pa_function == 0)

#define SETPROP(x, y, z) do {						\
			if (devprop_set(dev, x,				\
			    y, z, 0, 0) != 0)				\
				printf("WARNING: unable to set " x " "	\
				   "property for %s\n", dev->dv_xname);	\
			} while (/*CONSTCOND*/0)

void
device_register(struct device *dev, void *aux)
{
	struct device *pdev;

	if ((pdev = device_parent(dev)) != NULL &&
	    device_is_a(pdev, "pci")) {
		struct pci_attach_args *pa = aux;

		if (BUILTIN_ETHERNET_P(pa)) {
			uint16_t cfg1, cfg2, swdpin;

			/*
			 * We set these configuration registers to 0,
			 * because it's the closest we have to "leave them
			 * alone". That and, it works.
			 */
			cfg1 = 0;
			cfg2 = 0;
			swdpin = 0;

			SETPROP("mac-addr", iyonix_macaddr, ETHER_ADDR_LEN);
			SETPROP("i82543-cfg1", &cfg1, sizeof(cfg1));
			SETPROP("i82543-cfg2", &cfg2, sizeof(cfg2));
			SETPROP("i82543-swdpin", &swdpin, sizeof(swdpin));
		}
	}
}
