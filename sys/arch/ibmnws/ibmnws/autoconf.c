/*	$NetBSD: autoconf.c,v 1.11.8.1 2012/08/08 15:51:06 martin Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

#include <machine/pte.h>
#include <machine/intr.h>

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

#if 0
	aprint_normal("howto %x bootdev %x ", boothowto, bootdev);
#endif

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

#define	BUILTIN_ETHERNET_P(pa)						\
	((pa)->pa_bus == 0 && (pa)->pa_device == 2 && (pa)->pa_function == 0)

#define	BUILTIN_VIDEO_P(pa)						\
	((pa)->pa_bus == 0 && (pa)->pa_device == 4 && (pa)->pa_function == 0)

void
device_register(device_t dev, void *aux)
{
	device_t pdev;
	prop_dictionary_t dict;

	if ((pdev = device_parent(dev)) != NULL && device_is_a(pdev, "pci")) {
		struct pci_attach_args *pa = aux;

		dict = device_properties(dev);
		if (BUILTIN_ETHERNET_P(pa)) {
			if (! prop_dictionary_set_bool(dict,
						       "am79c970-no-eeprom",
						       true)) {
				aprint_normal("WARNING: unable to set "
				       "am79c970-no-eeprom property for %s\n",
				       device_xname(dev));
			}
		}

		if (BUILTIN_VIDEO_P(pa)) {
			if (! prop_dictionary_set_uint32(dict,  "width", 1024)) {
				aprint_normal("WARNING: unable to set "
					      "width property for %s\n",
					      device_xname(dev));
			}
			if (! prop_dictionary_set_uint32(dict,  "height", 768)) {
				aprint_normal("WARNING: unable to set "
					      "height property for %s\n",
					      device_xname(dev));
			}
			if (! prop_dictionary_set_uint32(dict,  "depth", 8)) {
				aprint_normal("WARNING: unable to set "
					      "depth property for %s\n",
					      device_xname(dev));
			}
			if (! prop_dictionary_set_uint32(dict,  "address", 0)) {
				aprint_normal("WARNING: unable to set "
					      "address property for %s\n",
					      device_xname(dev));
			}
			if (! prop_dictionary_set_bool(dict,  "is_console", true)) {
				aprint_normal("WARNING: unable to set "
					      "address property for %s\n",
					      device_xname(dev));
			}

		}
	}
}
