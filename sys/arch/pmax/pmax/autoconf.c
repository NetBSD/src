/*	$NetBSD: autoconf.c,v 1.40 1999/09/20 18:29:21 thorpej Exp $	*/

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
 * from: Utah Hdr: autoconf.c 1.31 91/01/21
 *
 *	@(#)autoconf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.40 1999/09/20 18:29:21 thorpej Exp $");

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>

#include <pmax/dev/device.h>
#include <pmax/pmax/turbochannel.h>


/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cpuspeed = 30;	/* approx # instr per usec. */

/*
 * XXX This should really be in a tcasic driver, or something.
 * XXX But right now even the 3100 code uses it.
 */
tc_option_t tc_slot_info[TC_MAX_LOGICAL_SLOTS];


void configure_scsi __P((void));

void findroot __P((struct device **, int *));

/*
 * Determine mass storage and memory configuration for a machine.
 * Print cpu type, and then iterate over an array of devices
 * found on the baseboard or in turbochannel option slots.
 * Once devices are configured, enable interrupts, and probe
 * for attached scsi devices.
 */
void
cpu_configure()
{
	/* Kick off autoconfiguration. */
	(void)splhigh();
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* Reset any bus errors due to probing nonexistent devices. */
	(*platform.bus_reset)();

	/* Configuration is finished, turn on interrupts. */
	_splnone();	/* enable all source forcing SOFT_INTs cleared */

	/*
	 * Probe SCSI bus using old-style pmax configuration table.
	 * We do not yet have machine-independent SCSI support or polled
	 * SCSI.
	 */
	printf("Beginning old-style SCSI device autoconfiguration\n");
	configure_scsi();
}

void
cpu_rootconf()
{
	struct device *booted_device;
	int booted_partition;

	findroot(&booted_device, &booted_partition);

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 */
void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
	int i, majdev, unit, part, controller;
	struct pmax_scsi_device *dp;
	const char *bootdv_name;

	/*
	 * Default to "not found".
	 */
	*devpp = NULL;
	*partp = 0;
	bootdv_name = NULL;

	if ((bootdev & B_MAGICMASK) != B_DEVMAGIC)
		return;

	majdev = B_TYPE(bootdev);
	for (i = 0; dev_name2blk[i].d_name != NULL; i++) {
		if (majdev == dev_name2blk[i].d_maj) {
			bootdv_name = dev_name2blk[i].d_name;
			break;
		}
	}

	if (bootdv_name == NULL) {
#if defined(DEBUG)
		printf("findroot(): no name2blk for boot device %d\n", majdev);
#endif
		return;
	}

	controller = B_CONTROLLER(bootdev);
	part = B_PARTITION(bootdev);
	unit = B_UNIT(bootdev);

	for (dp = scsi_dinit; dp->sd_driver != NULL; dp++) {
		if (dp->sd_alive && dp->sd_drive == unit &&
		    dp->sd_ctlr == controller &&
		    dp->sd_driver->d_name[0] == bootdv_name[0] &&
		    dp->sd_driver->d_name[1] == bootdv_name[1]) {
			*devpp = dp->sd_devp;
			*partp = part;
			return;
		}
	}
#if defined(DEBUG)
	printf("findroot(): no driver for boot device %s\n", bootdv_name);
#endif
}

/*
 * Look at the string 'cp' and decode the boot device.
 * Boot names can be something like 'rz(0,0,0)vmunix' or '5/rz0/vmunix'.
 */
void
makebootdev(cp)
	char *cp;
{
	int majdev, unit, part, ctrl;

	if (*cp >= '0' && *cp <= '9') {
		/* XXX should be able to specify controller */
		if (cp[1] != '/' || cp[4] < '0' || cp[4] > '9')
			goto defdev;
		unit = cp[4] - '0';
		if (cp[5] >= 'a' && cp[5] <= 'h')
			part = cp[5] - 'a';
		else
			part = 0;
		cp += 2;
		for (majdev = 0; dev_name2blk[majdev].d_name != NULL;
		    majdev++) {
			if (cp[0] == dev_name2blk[majdev].d_name[0] &&
			    cp[1] == dev_name2blk[majdev].d_name[1]) {
				bootdev = MAKEBOOTDEV(
				    dev_name2blk[majdev].d_maj, 0, 0,
				    unit, part);
				return;
			}
		}
		goto defdev;
	}
	for (majdev = 0; dev_name2blk[majdev].d_name != NULL; majdev++)
		if (cp[0] == dev_name2blk[majdev].d_name[0] &&
		    cp[1] == dev_name2blk[majdev].d_name[1] &&
		    cp[2] == '(')
			goto fndmaj;
defdev:
	bootdev = B_DEVMAGIC;
	return;

fndmaj:
	majdev = dev_name2blk[majdev].d_maj;
	for (ctrl = 0, cp += 3; *cp >= '0' && *cp <= '9'; )
		ctrl = ctrl * 10 + *cp++ - '0';
	if (*cp == ',')
		cp++;
	for (unit = 0; *cp >= '0' && *cp <= '9'; )
		unit = unit * 10 + *cp++ - '0';
	if (*cp == ',')
		cp++;
	for (part = 0; *cp >= '0' && *cp <= '9'; )
		part = part * 10 + *cp++ - '0';
	if (*cp != ')')
		goto defdev;
	bootdev = MAKEBOOTDEV(majdev, 0, ctrl, unit, part);
}
