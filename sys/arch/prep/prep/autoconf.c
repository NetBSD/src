/*	$NetBSD: autoconf.c,v 1.29 2022/05/22 11:27:34 andvar Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 * Setup the system to run on the current machine.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.29 2022/05/22 11:27:34 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <machine/pte.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>
#include <machine/isa_machdep.h>
#include <dev/isa/isareg.h>
#include <prep/pnpbus/pnpbusvar.h>

#include "opt_nvram.h"

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */
extern char bootpath[256];

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
	    booted_device ? device_xname(booted_device) : "<unknown>");

	rootconf();
}

/*
 * On the PReP, we do not have the fw-boot-device readable until the NVRAM
 * device is attached.  This occurs rather late in the autoconf process.
 * We build a OWF-like device node name for each device and store it in
 * fw-path.  Later when trying to figure out our boot device, we will match
 * it against these strings.  This serves a dual purpose, because if we ever
 * wish to change the bootlist on the machine via the NVRAM, we will have to
 * plug in these addresses for each device we want to use.  The kernel can
 * then query each device for fw-path, and generate a bootlist string.
 */

void
device_register(device_t dev, void *aux)
{
	device_t parent;
	char devpath[256];
	prop_string_t str1;
	int n;

	/* Certain devices will *never* be bootable.  short circuit them. */

	if (device_is_a(dev, "com") || device_is_a(dev, "attimer") ||
	    device_is_a(dev, "pcppi") || device_is_a(dev, "mcclock") ||
	    device_is_a(dev, "mkclock") || device_is_a(dev, "lpt") ||
	    device_is_a(dev, "pckbc") || device_is_a(dev, "pckbd") ||
	    device_is_a(dev, "vga") || device_is_a(dev, "wsdisplay") ||
	    device_is_a(dev, "wskbd") || device_is_a(dev, "wsmouse") ||
	    device_is_a(dev, "pms") || device_is_a(dev, "cpu"))
		return;

	if (device_is_a(dev, "pchb")) {
		struct pci_attach_args *pa = aux;
		prop_bool_t rav;

		if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_MOT &&
		    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_MOT_RAVEN) {
			rav = prop_bool_create(true);
			KASSERT(rav != NULL);
			(void) prop_dictionary_set(
			    device_properties(device_parent(dev)),
			    "prep-raven-pchb", rav);
			prop_object_release(rav);
		}
	}

	if (device_is_a(dev, "mainbus")) {
		str1 = prop_string_create_cstring("/");
		KASSERT(str1 != NULL);
		(void) prop_dictionary_set(device_properties(dev),
					   "prep-fw-path-component", str1);
		prop_object_release(str1);
		return;
	}
	parent = device_parent(dev);

	n = 0;
	if (device_is_a(dev, "pci")) {
		if (device_is_a(parent, "ppb"))
			n = snprintf(devpath, sizeof(devpath), "");
		else
			n = snprintf(devpath, sizeof(devpath), "pci@%x",
			    prep_io_space_tag.pbs_offset);
	}
	if (device_is_a(parent, "pci")) {
		struct pci_attach_args *pa = aux;

		n = snprintf(devpath, sizeof(devpath), "pci%x,%x@%x,%x",
		    PCI_VENDOR(pa->pa_id), PCI_PRODUCT(pa->pa_id),
		    pa->pa_device, pa->pa_function);
	}
	if (device_is_a(parent, "pnpbus")) {
		struct pnpbus_dev_attach_args *pna = aux;
		struct pnpbus_io *io;

		n = snprintf(devpath, sizeof(devpath), "%s@",
		    pna->pna_devid);
		io = SIMPLEQ_FIRST(&pna->pna_res.io);
		if (n > sizeof(devpath))
			n = sizeof(devpath);
		if (io != NULL)
			n += snprintf(devpath + n, sizeof(devpath) - n, "%x",
			    io->minbase);
	}

	if (n > sizeof(devpath))
		n = sizeof(devpath);
	/* we can't trust the device tag on the ethernet, because
	 * the spec lies about how it is formed.  Therefore we will leave it
	 * blank, and trim the end off any ethernet stuff. */
	if (device_class(dev) == DV_IFNET)
		n += snprintf(devpath + n, sizeof(devpath) - n, ":");
	else if (device_is_a(dev, "cd"))
		n = snprintf(devpath, sizeof(devpath), "cdrom@");
	else if (device_class(dev) == DV_DISK)
		n = snprintf(devpath, sizeof(devpath), "harddisk@");
	else if (device_class(dev) == DV_TAPE)
		n = snprintf(devpath, sizeof(devpath), "tape@");
	else if (device_is_a(dev, "fd"))
		n = snprintf(devpath, sizeof(devpath), "floppy@");

	if (device_is_a(parent, "scsibus") || device_is_a(parent, "atapibus")) {
		struct scsipibus_attach_args *sa = aux;

		/* periph_target is target for scsi, drive # for atapi */
		if (n > sizeof(devpath))
			n = sizeof(devpath);
		n += snprintf(devpath + n, sizeof(devpath) - n, "%d",
		    sa->sa_periph->periph_target);
		if (n > sizeof(devpath))
			n = sizeof(devpath);
		if (device_is_a(parent, "scsibus"))
			n += snprintf(devpath + n, sizeof(devpath) - n, ",%d",
			    sa->sa_periph->periph_lun);
	} else if (device_is_a(parent, "atabus") ||
	    device_is_a(parent, "pciide")) {
		struct ata_device *adev = aux;

		if (n > sizeof(devpath))
			n = sizeof(devpath);
		n += snprintf(devpath + n, sizeof(devpath) - n, "%d",
		    adev->adev_drv_data->drive);
	} else if (device_is_a(dev, "fd")) {
		if (n > sizeof(devpath))
			n = sizeof(devpath);
		/* XXX device_unit() abuse */
		n += snprintf(devpath + n, sizeof(devpath) - n, "%d",
		    device_unit(dev));
	}

	str1 = prop_string_create_cstring(devpath);
	KASSERT(str1 != NULL);
	(void)prop_dictionary_set(device_properties(dev),
	    "prep-fw-path-component", str1);
	prop_object_release(str1);
}

static void
gen_fwpath(device_t dev)
{
	device_t parent;
	prop_string_t str1, str2, str3;

	parent = device_parent(dev);
	str1 = prop_dictionary_get(device_properties(dev),
	    "prep-fw-path-component");
	if (str1 == NULL)
		return;
	if (parent == NULL) {
		prop_dictionary_set(device_properties(dev), "prep-fw-path",
		    str1);
		return;
	}
	str2 = prop_dictionary_get(device_properties(parent), "prep-fw-path");
	if (str2 == NULL) {
		prop_dictionary_set(device_properties(dev), "prep-fw-path",
		    str1);
		return;
	}
	str3 = prop_string_copy(str2);
	KASSERT(str3 != NULL);
	if (!(prop_string_equals_cstring(str3, "/") ||
	      prop_string_equals_cstring(str1, "")))
		prop_string_append_cstring(str3, "/");
	if (!prop_string_equals_cstring(str1, "/"))
		prop_string_append(str3, str1);
#if defined(NVRAM_DUMP)
	printf("%s devpath: %s+%s == %s\n", device_xname(dev),
	    prop_string_cstring_nocopy(str2),
	    prop_string_cstring_nocopy(str1),
	    prop_string_cstring_nocopy(str3));
#endif
	(void)prop_dictionary_set(device_properties(dev),
	    "prep-fw-path", str3);
	prop_object_release(str3);
}

/*
 * Generate properties for each device by totaling up its parent device
 */
static void
build_fwpath(void)
{
	device_t dev, d;
	deviter_t di, inner_di;
	prop_string_t str1;

	/* First, find all the PCI busses */
	for (dev = deviter_first(&di, DEVITER_F_ROOT_FIRST); dev != NULL;
	     dev = deviter_next(&di)) {
		if (device_is_a(dev, "pci") || device_is_a(dev, "mainbus") ||
		    device_is_a(dev, "pcib") || device_is_a(dev, "pceb") ||
		    device_is_a(dev, "ppb"))
			gen_fwpath(dev);
	}
	deviter_release(&di);

	/* Now go find the ISA bus and fix it up */
	for (dev = deviter_first(&di, DEVITER_F_ROOT_FIRST); dev != NULL;
	     dev = deviter_next(&di)) {
		if (device_is_a(dev, "isa"))
			gen_fwpath(dev);
	}
	deviter_release(&di);

	for (dev = deviter_first(&di, DEVITER_F_ROOT_FIRST); dev != NULL;
	     dev = deviter_next(&di)) {
		/* skip the ones we already computed above */
		if (device_is_a(dev, "pci") || device_is_a(dev, "pcib") ||
		    device_is_a(dev, "pceb") || device_is_a(dev, "isa") ||
		    device_is_a(dev, "ppb"))
			continue;
		/* patch in the properties for the pnpbus */
		if (device_is_a(dev, "pnpbus")) {
			for (d = deviter_first(&inner_di, DEVITER_F_ROOT_FIRST);
			     d != NULL;
			     d = deviter_next(&inner_di)) {
				if (!device_is_a(d, "isa"))
					continue;
				str1 = prop_dictionary_get(device_properties(d),
					"prep-fw-path");
				if (str1 == NULL)
					continue;
				prop_dictionary_set(device_properties(dev),
					"prep-fw-path", str1);
			}
			deviter_release(&inner_di);
		} else
			gen_fwpath(dev);
	}
	deviter_release(&di);
}


/*
 * This routine looks at each device, and tries to match them to the bootpath
 */

void
findroot(void)
{
	device_t d;
	deviter_t di;
	char *cp;
	prop_string_t str;
	size_t len;

	/* first rebuild all the device paths */
	build_fwpath();

	/* now trim the ethernet crap off the bootpath */
	cp = strchr(bootpath, ':');
	if (cp != NULL) {
		cp++;
		*cp = '\0';
	}
	len = strlen(bootpath);
#if defined(NVRAM_DUMP)
	printf("Modified bootpath: %s\n", bootpath);
#endif
	for (d = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     d != NULL;
	     d = deviter_next(&di)) {
		str = prop_dictionary_get(device_properties(d), "prep-fw-path");
		if (str == NULL)
			continue;
#if defined(NVRAM_DUMP)
		printf("dev %s: fw-path: %s\n", device_xname(d),
		    prop_string_cstring_nocopy(str));
#endif
		if (strncmp(prop_string_cstring_nocopy(str), bootpath,
		    len) == 0)
			break;
	}
	deviter_release(&di);
	if (d != NULL) {
		booted_device = d;
		booted_partition = 0; /* XXX ??? */
	}
}
