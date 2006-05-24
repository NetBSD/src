/*	$NetBSD: autoconf.c,v 1.11.8.2 2006/05/24 10:57:10 yamt Exp $	*/

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

/*
 * Setup the system to run on the current machine.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.11.8.2 2006/05/24 10:57:10 yamt Exp $");

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

	printf("biomask %x netmask %x ttymask %x\n",
	    imask[IPL_BIO] & 0x1fffffff,
	    imask[IPL_NET] & 0x1fffffff,
	    imask[IPL_TTY] & 0x1fffffff);

	spl0();
}

void
cpu_rootconf(void)
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
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
device_register(struct device *dev, void *aux)
{
	struct device *parent, *d;
	char devpath[256], dtmp[256];
	prop_string_t str1;
	int found = 0;

	/* Certain devices will *never* be bootable.  short circuit them. */

	if (device_is_a(dev, "com") || device_is_a(dev, "attimer") ||
	    device_is_a(dev, "pcppi") || device_is_a(dev, "mcclock") ||
	    device_is_a(dev, "mkclock") || device_is_a(dev, "lpt"))
		return;

	if (device_is_a(dev, "mainbus")) {
		str1 = prop_string_create_cstring("/");
		KASSERT(str1 != NULL);
		(void) prop_dictionary_set(device_properties(dev),
					   "fw-path", str1);
		prop_object_release(str1);
		return;
	}

	parent = device_parent(dev);
	str1 = prop_dictionary_get(device_properties(parent), "fw-path");
	if (str1 == NULL)
		return;
	KASSERT(prop_string_size(str1) < sizeof(dtmp));
	strcpy(dtmp, prop_string_cstring_nocopy(str1));

	if (device_is_a(dev, "pci")) {
		if (device_is_a(parent, "ppb"))
			sprintf(devpath, "%s/", dtmp);
		else
			sprintf(devpath, "%spci@%x/", dtmp,
			    prep_io_space_tag.pbs_offset);
		found++;
	}
	if (device_is_a(parent, "pci")) {
		struct pci_attach_args *pa = aux;

		sprintf(devpath, "%spci%x,%x@%x,%x", dtmp,
		    PCI_VENDOR(pa->pa_id), PCI_PRODUCT(pa->pa_id),
		    pa->pa_device, pa->pa_function);
		found++;
	}
	if (device_is_a(dev, "pnpbus")) {
		/* because the pnpbus attaches at mainbus, we need to manually
		 * grab it's parent device components.  We grab them from
		 * ISA because thats the most likely attachment. 
		 */
		str1 = NULL;
		TAILQ_FOREACH(d, &alldevs, dv_list) {
			if (device_is_a(d, "isa")) {
				str1 = prop_dictionary_get(
				    device_properties(d), "fw-path");
				break;
			}
		}
		if (str1 != NULL) {
			KASSERT(prop_string_size(str1) < sizeof(devpath));
			strcpy(devpath, prop_string_cstring_nocopy(str1));
			found++;
		}
	}
	if (device_is_a(parent, "pnpbus")) {
		struct pnpbus_dev_attach_args *pna = aux;
		struct pnpbus_io *io;

		sprintf(devpath, "%s/%s@", dtmp, pna->pna_devid);
		io = SIMPLEQ_FIRST(&pna->pna_res.io);
		if (io != NULL)
			sprintf(devpath, "%s%x", devpath, io->minbase);
		found++;
	}

	if (!found)
		strcpy(devpath, dtmp);

	/* we can't trust the device tag on the ethernet, because
	 * the spec lies about how it is formed.  Therefore we will leave it
	 * blank, and trim the end off any ethernet stuff. */
	if (device_class(dev) == DV_IFNET)
		sprintf(devpath, "%s:", devpath);
	else if (device_is_a(dev, "cd"))
		sprintf(devpath, "%s/cdrom@", devpath);
	else if (device_class(dev) == DV_DISK)
		sprintf(devpath, "%s/harddisk@", devpath);
	else if (device_class(dev) == DV_TAPE)
		sprintf(devpath, "%s/tape@", devpath);
	else if (device_is_a(dev, "fd"))
		sprintf(devpath, "%s/floppy@", devpath);

	if (device_is_a(parent, "scsibus") || device_is_a(parent, "atapibus")) {
		struct scsipibus_attach_args *sa = aux;

		/* periph_target is target for scsi, drive # for atapi */
		sprintf(devpath, "%s%d", devpath, sa->sa_periph->periph_target);
		if (device_is_a(parent, "scsibus"))
			sprintf(devpath, "%s,%d", devpath,
			    sa->sa_periph->periph_lun);
	} else if (device_is_a(parent, "atabus") ||
	    device_is_a(parent, "pciide")) {
		struct ata_device *adev = aux;

		sprintf(devpath, "%s%d", devpath, adev->adev_drv_data->drive);
	} else if (device_is_a(dev, "fd")) {
		/* XXX device_unit() abuse */
		sprintf(devpath, "%s%d", devpath, device_unit(dev));
	}

	str1 = prop_string_create_cstring(devpath);
	KASSERT(str1 != NULL);
	(void) prop_dictionary_set(device_properties(dev), "fw-path", str1);
	prop_object_release(str1);
#if defined(NVRAM_DUMP)
	printf("prop %s\n", devpath);
#endif
}

/*
 * This routine looks at each device, and tries to match them to the bootpath
 */

void
findroot(void)
{
	struct device *d;
	char *cp;
	prop_string_t str;
	size_t len;

	/* first trim the ethernet crap off the bootpath */
	cp = strchr(bootpath, ':');
	if (cp != NULL) {
		cp++;
		*cp = '\0';
	}
	len = strlen(bootpath);
#if defined(NVRAM_DUMP)
	printf("Modified bootpath: %s\n", bootpath);
#endif
	TAILQ_FOREACH(d, &alldevs, dv_list) {
		str = prop_dictionary_get(device_properties(d), "fw-path");
		if (str == NULL)
			continue;
#if defined(NVRAM_DUMP)
		printf("dev %s: fw-path: %s\n", d->dv_xname,
		    prop_string_cstring_nocopy(str));
#endif
		if (strncmp(prop_string_cstring_nocopy(str), bootpath,
		    len) == 0) {
			booted_device = d;
			booted_partition = 0; /* XXX ??? */
			return;
		}
	}
}
