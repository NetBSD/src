/*	$NetBSD: autoconf.c,v 1.22.2.1 2010/11/06 08:08:15 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.22.2.1 2010/11/06 08:08:15 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disk.h>
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
#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ata/wdvar.h>
#include <dev/scsipi/sdvar.h>

#include <machine/bootinfo.h>
#include <machine/pte.h>
#include <machine/intr.h>

void genppc_cpu_configure(void);
static void findroot(void);

static int bus, target, lun, drive;
static const char *name = NULL;

/*
 * Determine i/o configuration for a machine.
 */

void
cpu_configure(void)
{

	findroot();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	genppc_cpu_configure();
}

void
cpu_rootconf(void)
{

	aprint_normal("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
static void
findroot(void)
{
	struct btinfo_rootdevice *rdev;
	int part;
	char *p;

	rdev = (struct btinfo_rootdevice *)lookup_bootinfo(BTINFO_ROOTDEVICE);
	if (rdev == NULL)
		return;
	p = rdev->rootdevice;
	if (strncmp(p, "/dev/disk/", 10) != 0)
		/* unknwon device... */
		return;
	p += 10;
	if (strncmp(p, "scsi/", 5) == 0) {
		name = "sd";
		p += 5;

		bus = 0;
		while (isdigit(*p))
			bus = bus * 10 + (*p++) - '0';
		if (*p++ != '/')
			return;
		target = 0;
		while (isdigit(*p))
			target = target * 10 + (*p++) - '0';
		if (*p++ != '/')
			return;
		lun = 0;
		while (isdigit(*p))
			lun = lun * 10 + (*p++) - '0';
	} else if (strncmp(p, "ide/", 4) == 0) {
		name = "wd";
		p += 4;

		bus = 0;
		while (isdigit(*p))
			bus = bus * 10 + (*p++) - '0';
		if (*p++ != '/')
			return;
		if (strncmp(p, "master/0", 8) == 0) {
			drive = 0;
			p += 8;
		} else if (strncmp(p, "slave/0", 7) == 0) {
			drive = 1;
			p += 7;
		} else
			return;
	} else if (strcmp(p, "floppy") == 0)
		return;
	else
		/* unknwon disk... */
		return;

	if (*p != '_' || !isdigit(*(p + 1)))
		return;
	p++;
	part = 0;
	while (isdigit(*p))
		part = part * 10 + (*p++) - '0';
	if (p != '\0')
		return;

	booted_partition = part;
}

void
device_register(device_t dev, void *aux)
{
	device_t bdev, cdev;

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
		int pbus, device;

		pci_decompose_tag(pa->pa_pc, pa->pa_tag, &pbus, &device, NULL);
		if (pbus == 0 && device == 12)
			/* Internal SCSI uses PCI clock as SCSI clock */
			prop_dictionary_set_bool(dict, "use_pciclock", 1);
	}
#endif

	if (booted_device != NULL)
		return;
	/*
	 * Check boot device.
	 * It is sd/wd connected by the onboard controller to be supported.
	 */
	if (device_is_a(dev, "sd") && strcmp(name, "sd") == 0) {
		struct scsipibus_attach_args *sa = aux;

		bdev = device_parent(dev);
		if (!device_is_a(bdev, "scsibus"))
			return;
		cdev = device_parent(bdev);
		if (!device_is_a(cdev, "siop"))
			return;

		if (sa->sa_periph->periph_target == target &&
		    sa->sa_periph->periph_lun == lun)
			booted_device = dev;
	} else if (device_is_a(dev, "wd") && strcmp(name, "wd") == 0) {
		struct ata_device *adev = aux;

		bdev = device_parent(dev);
		if (!device_is_a(bdev, "atabus"))
			return;
		cdev = device_parent(bdev);
		if (!device_is_a(cdev, "wdc"))
			return;
		if (!device_is_a(device_parent(cdev), "isa"))
			return;

		if (adev->adev_drv_data->drive == drive)
			booted_device = dev;
	}
}

