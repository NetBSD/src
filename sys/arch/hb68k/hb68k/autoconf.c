/*	$NetBSD: autoconf.c,v 1.1 2026/07/19 01:48:21 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 * 	The Regents of the University of California. All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 * 
 *	@(#)autoconf.c  8.2 (Berkeley) 1/12/94
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.1 2026/07/19 01:48:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_boot.h>
#include <dev/fdt/fdt_platform.h>

#include <dev/ofw/openfirm.h>

#include <machine/intr.h>

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure(void)
{

	booted_device = NULL;	/* set by device drivers (if found) */

	if (config_rootfound("fdtroot", NULL) == NULL)
		panic("autoconfig failed, no root");

	spl0();
}

/*
 * This is our "private" copy of booted_device; we'll use it if other
 * mechanisms don't work out.
 */
device_t machine_booted_device;

/* Maximum size is NAME=wedgename\0 */
#define	MAX_BOOTSPEC	(128 + 6)
static char bootspec_buf[MAX_BOOTSPEC + 1];

const char *
machine_set_bootspec(const char *cp)
{
	int i;

	for (i = 0; i < MAX_BOOTSPEC; i++) {
		if (*cp == '\0' || *cp == ' ' || *cp == '\t') {
			break;
		}
		bootspec_buf[i] = *cp++;
	}
	bootspec_buf[i] = '\0';
	bootspec = bootspec_buf;
	while (! (*cp == '\0' || *cp == ' ' || *cp == '\t')) {
		cp++;
	}
	return cp;
}

void
cpu_rootconf(void)
{
#if 0
	bootinfo_setup_initrd();
#endif

	/*
	 * See if we can determine the boot device from generic
	 * properties in the device tree.  If we can't figure it
	 * out that way, then we'll use what device_register()
	 * was able to find.
	 */
	fdt_cpu_rootconf();
	if (booted_device == NULL) {
		booted_device = machine_booted_device;
	}

	if (booted_device != NULL && machine_booted_device != NULL &&
	    booted_device != machine_booted_device) {
		aprint_normal("boot device: %s (%s)\n",
		    device_xname(booted_device),
		    device_xname(machine_booted_device));
	} else {
		aprint_normal("boot device: %s\n",
		    booted_device != NULL ? device_xname(booted_device)
					  : "<unknown>");
	}
	rootconf();
}

void
device_register(device_t dev, void *aux)
{
	const struct fdt_platform *plat = machine_platform();

	if (plat->fp_device_register != NULL) {
		(*plat->fp_device_register)(dev, aux);
	}
}
