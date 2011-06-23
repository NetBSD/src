/*	$NetBSD: autoconf.c,v 1.3.4.1 2011/06/23 14:19:05 cherry Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: autoconf.c 1.31 91/01/21
 *
 *	@(#)autoconf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.3.4.1 2011/06/23 14:19:05 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

static int		 booted_bus, booted_unit;
static const char	*booted_controller;

/*
 * Configure all devices on system
 */     
void
cpu_configure(void)
{

	/* Kick off autoconfiguration. */
	(void)splhigh();

	/* Interrupt initialization. */
	intr_init();

	evcnt_attach_static(&emips_clock_evcnt);
	evcnt_attach_static(&emips_fpu_evcnt);
	evcnt_attach_static(&emips_memerr_evcnt);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Reset any bus errors due to probing nonexistent devices. */
	(*platform.bus_reset)();

	/* Configuration is finished, turn on interrupts. */
	spl0();		/* enable all source forcing SOFT_INTs cleared */
}

/*
 * Look at the string 'cp' and decode the boot device.
 * Boot names are something like '0/ace(0,0)/netbsd' or 'tftp()/nfsnetbsd'
 * meaning:
 *  [BusNumber/]<ControllerName>([<DiskNumber>,<PartitionNumber])/<kernelname>
 */
void
makebootdev(char *cp)
{
	int i;
	static char booted_controller_name[8];

	booted_device = NULL;
	booted_bus = booted_unit = booted_partition = 0;
	booted_controller = NULL;

	if (*cp >= '0' && *cp <= '9') {
	        booted_bus = *cp++ - '0';
		if (*cp == '/')
			cp++;
	}

	if (strncmp(cp, "tftp(", 5) == 0) {
		booted_controller = "BOOTP";
		goto out;
	}

	/*
	 * Stash away the controller name and use it later
	 */
	for (i = 0; i < 7 && *cp && *cp != '('; i++)
		booted_controller_name[i] = *cp++;
	booted_controller_name[7] = 0; /* sanity */

	if (*cp == '(')
		cp++;
	if (*cp >= '0' && *cp <= '9') 
		booted_unit = *cp++ - '0';

	if (*cp == ',')
		cp++;
	if (*cp >= '0' && *cp <= '9')
		booted_partition = *cp - '0';
	booted_controller = booted_controller_name;

 out:
#if DEBUG
	printf("bootdev: %d/%s(%d,%d)\n",
	    booted_bus, booted_controller, booted_unit, booted_partition);
#endif
	return;
}

void
cpu_rootconf(void)
{

	printf("boot device: %s part%d\n",
	    booted_device ? device_xname(booted_device) : "<unknown>",
	    booted_partition);

	setroot(booted_device, booted_partition);
}

/*
 * Try to determine the boot device.
 */
void
device_register(device_t dev, void *aux)
{
	static int found, initted, netboot;
	static device_t ebusdev;
	device_t parent = device_parent(dev);

	if (found)
		return;

#if 0
	printf("\n[device_register(%s,%d) class %d]\n",
	    device_xname(dev), device_unit(dev), device_class(dev));
#endif

	if (!initted) {
		netboot = (strcmp(booted_controller, "BOOTP") == 0);
		initted = 1;
	}

	/*
	 * Remember the EBUS
	 */
	if (device_is_a(dev, "ebus")) {
		ebusdev = dev;
		return;
	}

	/*
	 * Check if netbooting.
	 */
	if (netboot) {

		/* Only one Ethernet interface (on ebus). */
		if ((parent == ebusdev) &&
		    device_is_a(dev, "enic")) {
			booted_device = dev;
			found = 1;
			return;
		}

		/* allow any network adapter */
		if (device_class(dev) == DV_IFNET &&
		    device_is_a(parent, "ebus")) {
			booted_device = dev;
			found = 1;
			return;
		}

		/* The NIC might be found after the disk, so bail out here */
		return;
	}

	/* BUGBUG How would I get to the bus */
	if (device_is_a(dev, booted_controller) &&
	    (device_unit(dev) == booted_unit)) {
		booted_device = dev;
		found = 1;
		return;
	}
}
