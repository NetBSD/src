/*	$NetBSD: autoconf.c,v 1.38.2.2 2007/12/27 00:43:19 mjf Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.38.2.2 2007/12/27 00:43:19 mjf Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/sysconf.h>
#include <machine/machtype.h>
#include <machine/autoconf.h>
#include <machine/vmparam.h>	/* for PAGE_SIZE */

#include <dev/pci/pcivar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

static struct device *booted_controller = NULL;
static int	booted_slot, booted_unit;
static const char	*booted_protocol = NULL;

extern struct platform platform;

void
cpu_configure()
{
	int s;

	s = splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/*
	 * Clear latched bus error registers which may have been
	 * caused by probes for non-existent devices.
	 */
	(*platform.bus_reset)();

	/*
	 * Hardware interrupts will be enabled in cpu_initclocks(9)
	 * to avoid hardclock(9) by CPU INT5 before softclockintr is
	 * initialized in initclocks().
	 */
}

/*
 * Look at the string 'cp' and decode the boot device.  Boot names
 * can be something like 'bootp(0)netbsd' or
 * 'scsi(0)disk(1)rdisk(0)partition(0)netbsd' or
 * 'dksc(0,1,0)netbsd'
 */
void
makebootdev(const char *cp)
{
	if (booted_protocol != NULL)
		return;

	booted_slot = booted_unit = booted_partition = 0;

	if (strncmp(cp, "pci(", 4) == 0) {
		cp += 4;

		while (*cp && *cp != ')')
		    cp++;

		if (*cp != ')')
		    return;

		cp++;
	}

	if (strncmp(cp, "scsi(", 5) == 0) {
		cp += 5;
		if (*cp >= '0' && *cp <= '9')
			booted_slot = *cp++ - '0';
		if (strncmp(cp, ")disk(", 6) == 0) {
			cp += 6;
			if (*cp >= '0' && *cp <= '9')
				booted_unit = *cp++ - '0';
		}
		/* XXX can rdisk() ever be other than 0? */
		if (strncmp(cp, ")rdisk(0)partition(", 19) == 0) {
			cp += 19;
			while (*cp >= '0' && *cp <= '9')
				booted_partition =
					booted_partition * 10 + *cp++ - '0';
		}
		if (*cp != ')')
			return;	/* XXX ? */
		booted_protocol = "SCSI";
		return;
	}
	if (strncmp(cp, "dksc(", 5) == 0) {
		cp += 5;
		if (*cp >= '0' && *cp <= '9')
			booted_slot = *cp++ - '0';
		if (*cp == ',') {
			++cp;
			if (*cp >= '0' || *cp <= '9')
				booted_unit = *cp++ - '0';
			if (*cp == ',') {
				++cp;
				if (*cp >= '0' && *cp <= '9')
					booted_partition = *cp++ - '0';
			}
		}
		if (*cp != ')')
			return;		/* XXX ??? */
		booted_protocol = "SCSI";
		return;
	}
	if (strncmp(cp, "bootp(", 6) == 0) {
		/* XXX controller number?  Needed to
		   handle > 1 network controller */
		booted_protocol = "BOOTP";
		return;
	}
}

void
cpu_rootconf()
{
	printf("boot device: %s\n",
		booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

/*
 * Try to determine the boot device and set up some device properties
 * to handle machine dependent quirks.
 */

#define BUILTIN_AHC_P(pa)	\
    (((pa)->pa_bus == 0 && (pa)->pa_device == 1 && (pa)->pa_function == 0) || \
     ((pa)->pa_bus == 0 && (pa)->pa_device == 2 && (pa)->pa_function == 0))

void
device_register(struct device *dev, void *aux)
{
	static int found, initted, scsiboot, netboot;
	struct device *parent = device_parent(dev);

	if (mach_type == MACH_SGI_IP32 &&
	    parent != NULL && device_is_a(parent, "pci")) {
		struct pci_attach_args *pa = aux;

		if (BUILTIN_AHC_P(pa)) {
			if (prop_dictionary_set_bool(device_properties(dev),
			    "aic7xxx-use-target-defaults", true) == false) {
				printf("WARNING: unable to set "
				    "aic7xxx-use-target-defaults property "
				    "for %s\n", dev->dv_xname);
			}

			if (prop_dictionary_set_bool(device_properties(dev),
			    "aic7xxx-override-ultra", true) == false) {
				printf("WARNING: unable to set "
				    "aic7xxx-override-ultra property for %s\n",
				    dev->dv_xname);
			}
		}
	}

	/*
	 * The Set Engineering GIO Fast Ethernet controller has restrictions
	 * on DMA boundaries.
	 */
	if (device_is_a(dev, "tl")) {
		struct device *grandparent;
		prop_number_t gfe_boundary;

		grandparent = device_parent(parent);
		if (grandparent != NULL && device_is_a(grandparent, "giopci")) {
			gfe_boundary = prop_number_create_integer(PAGE_SIZE);
			KASSERT(gfe_boundary != NULL);

			if (prop_dictionary_set(device_properties(dev),
			    "tl-dma-page-boundary", gfe_boundary) == false) {
				printf("WARNING: unable to set "
				    "tl-dma-page-boundary property "
				    "for %s\n", dev->dv_xname);
			}
			prop_object_release(gfe_boundary);
			return;
		}
	}

	if (found)
		return;

	if (!initted && booted_protocol) {
		scsiboot = strcmp(booted_protocol, "SCSI") == 0;
		netboot = (strcmp(booted_protocol, "BOOTP") == 0);
		initted = 1;
	}

	/*
	 * Handle SCSI boot device definitions
	 * wdsc -- IP12/22/24
	 * ahc -- IP32
	 */
	if ( (scsiboot && device_is_a(dev, "wdsc")) ||
	     (scsiboot && device_is_a(dev, "ahc")) ) {
		/* XXX device_unit() abuse */
		if (device_unit(dev) == booted_slot)
			booted_controller = dev;
		return;
	}

	/*
	 * If we found the boot controller, if check disk/tape/cdrom device
	 * on that controller matches.
	 */
	if (booted_controller &&
	    (device_is_a(dev, "sd") ||
	     device_is_a(dev, "st") ||
	     device_is_a(dev, "cd"))) {
		struct scsipibus_attach_args *sa = aux;

		if (device_parent(parent) != booted_controller)
			return;
		if (booted_unit != sa->sa_periph->periph_target)
			return;
		booted_device = dev;
		found = 1;
		return;
	}

	/*
	 * Check if netboot device.
	 */
	if (netboot &&
	    (device_is_a(dev, "sq") ||
	     device_is_a(dev, "mec"))) {
		/* XXX Check unit number? (Which we don't parse yet) */
		booted_device = dev;
		found = 1;
		return;
	}
}
