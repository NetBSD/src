/*	$NetBSD: autoconf.c,v 1.21.10.1 2009/05/13 17:16:33 jym Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba 
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.21.10.1 2009/05/13 17:16:33 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include "pci.h"
#if NPCI > 0
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#endif

#include <machine/pte.h>
#include <machine/intr.h>

void genppc_cpu_configure(void);
static void findroot(void);

/*
 * Determine i/o configuration for a machine.
 */

void
cpu_configure(void)
{

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");
	
	genppc_cpu_configure();
}

void
cpu_rootconf(void)
{
	findroot();

	aprint_normal("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(void)
{
	int unit, part;
	device_t dv;
	const char *name;

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;
	
	name = devsw_blk2name((bootdev >> B_TYPESHIFT) & B_TYPEMASK);
	if (name == NULL)
		return;
	
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	if ((dv = device_find_by_driver_unit(name, unit)) != NULL) {
		booted_device = dv;
		booted_partition = part;
	}
}

void
device_register(struct device *dev, void *aux)
{

#if NPCI > 0
	if (device_is_a(dev, "genfb") &&
	    device_is_a(device_parent(dev), "pci")) {
		prop_dictionary_t dict = device_properties(dev);
		struct pci_attach_args *pa = aux;
		pcireg_t bar0;
		uint32_t fbaddr;

		bar0 = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_START);
		fbaddr = PCI_MAPREG_MEM_ADDR(bar0);

		prop_dictionary_set_bool(dict, "is_console", 1);
		prop_dictionary_set_uint32(dict, "width", 640);
		prop_dictionary_set_uint32(dict, "height", 480);
		prop_dictionary_set_uint32(dict, "depth", 8);
		prop_dictionary_set_uint32(dict, "address", fbaddr);
	}

	if (device_is_a(dev, "siop") &&
	    device_is_a(device_parent(dev), "pci")) {
		prop_dictionary_t dict = device_properties(dev);
		struct pci_attach_args *pa = aux;
		int bus, device;

		pci_decompose_tag(pa->pa_pc, pa->pa_tag, &bus, &device, NULL);
		if (bus == 0 && device == 12)
			/* Internal SCSI uses PCI clock as SCSI clock */
			prop_dictionary_set_bool(dict, "use_pciclock", 1);
	}
#endif
}

