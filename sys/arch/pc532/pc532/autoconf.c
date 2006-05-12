/*	$NetBSD: autoconf.c,v 1.44 2006/05/12 06:05:23 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.44 2006/05/12 06:05:23 simonb Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */

u_long bootdev = 0;		/* should be dev_t, but not until 32 bits */

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{
	extern int safepri;
	int i;
	static const char *ipl_names[] = IPL_NAMES;

	splhigh();
	safepri = splhigh();

	/* Find out what the hardware configuration looks like! */
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("No mainbus found!");

	for (i = 0; i < NIPL; i++)
		if (*ipl_names[i])
			printf("%s%s=%x", i?", ":"", ipl_names[i], imask[i]);
	printf("\n");

	safepri = imask[IPL_ZERO];
	spl0();
}

void
cpu_rootconf(void)
{
	booted_partition = B_PARTITION(bootdev);

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
device_register(struct device *dev, void *aux)
{
	static int found;
	static struct device *booted_controller;
	struct device *parent = device_parent(dev);

	if (found)
		return;

	/*
	 * Check for NCR SCSI controller.
	 */
	if (device_is_a(dev, "ncr")) {
		booted_controller = dev;
		return;
	}

	/* XXX Adaptec SCSI controller one day... */

	/*
	 * If we found the boot controller, if check disk/cdrom device
	 * on that controller matches.
	 */
	if (booted_controller &&
	    (device_is_a(dev, "sd") || device_is_a(dev, "cd"))) {
		struct scsipibus_attach_args *sa = aux;

		if (device_parent(parent) != booted_controller)
			return;
		if (B_UNIT(bootdev) != sa->sa_periph->periph_target)
			return;
		booted_device = dev;
		found = 1;
		return;
	}
}
