/*	$NetBSD: autoconf.c,v 1.4.2.4 2001/04/21 17:54:05 bouyer Exp $	*/

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

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/mainboard.h>
#include <machine/autoconf.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cpuspeed = 25;	/* approx # instr per usec. */

extern int initcpu __P((void));		/*XXX*/

void	findroot __P((struct device **, int *));

struct mipsco_intrhand intrtab[MAX_INTR_COOKIES];
struct device  *booted_device;
static int	booted_partition;

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
  	int s;

	softintr_init();
	/*
	 * Kick off autoconfiguration
	 */
	s = splhigh();
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");
	initcpu();
}

void
cpu_rootconf()
{
	findroot(&booted_device, &booted_partition);

	printf("boot device: %s\n",
	       booted_device ? booted_device->dv_xname : "<unknown>");
	setroot(booted_device, booted_partition);
}

dev_t	bootdev = NULL;
char	boot_class;
int	boot_id, boot_lun, boot_part;

/*
 * Attempt to find the device from which we were booted.
 */
void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
	struct device *dv;

	for (dv = TAILQ_FIRST(&alldevs); dv; dv = TAILQ_NEXT(dv, dv_list)) {
		if (dv->dv_class == boot_class && dv->dv_unit == boot_id) {
			*devpp = dv;
			*partp = boot_part;
			return;
		}
	}

	/*
	 * Default to "not found".
	 */
	*devpp = NULL;
	*partp = 0;
	return;
}

void
makebootdev(cp)
	char *cp;
{
	boot_class = -1;
	boot_id = boot_lun = boot_part = 0;

	if (strlen(cp) < 6)
		return;
	if (strncmp(cp, "dk", 2) == 0 && cp[4] == '(') { /* Disk */
		cp += 5;
		if (*cp >= '0' && *cp <= '9')
			boot_lun = *cp++ - '0';
		if (*cp == ',')
			cp += 1;
		if (*cp >= '0' && *cp <= '9')
			boot_id = *cp++ - '0';
		if (*cp == ',')
			cp += 1;
		if (*cp >= '0' && *cp <= '7')
			boot_part = *cp - '0';
		boot_class = DV_DISK;
		return;
	}
}
