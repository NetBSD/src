/*	$NetBSD: autoconf.c,v 1.66.20.2 2008/01/09 01:47:05 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)autoconf.c	8.4 (Berkeley) 10/1/93
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.66.20.2 2008/01/09 01:47:05 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/disk.h>

#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/viareg.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "scsibus.h"

static void findbootdev(void);
#if NSCSIBUS > 0
static int target_to_unit(u_long, u_long, u_long);
#endif /* NSCSIBUS > 0 */

/*
 * cpu_configure:
 * called at boot time, configure all devices on the system
 */
void
cpu_configure(void)
{

	mrg_init();		/* Init Mac ROM Glue */
	startrtclock();		/* start before ADB attached */

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("No mainbus found!");

	(void)spl0();
}

void
cpu_rootconf(void)
{

	findbootdev();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

/*
 * Yanked from i386/i386/autoconf.c (and tweaked a bit)
 */

u_long	bootdev;

static void
findbootdev(void)
{
	struct device *dv;
	int major, unit, controller;
	char buf[32];
	const char *name;

	booted_device = NULL;
	booted_partition = 0;	/* Assume root is on partition a */

	major = B_TYPE(bootdev);
	name = devsw_blk2name(major);
	if (name == NULL)
		return;

	unit = B_UNIT(bootdev);

	switch (major) {
	case 4: /* SCSI drive */
#if NSCSIBUS > 0
		bootdev &= ~(B_UNITMASK << B_UNITSHIFT); /* XXX */
		unit = target_to_unit(-1, unit, 0);
		bootdev |= (unit << B_UNITSHIFT); /* XXX */
#else /* NSCSIBUS > 0 */
		panic("Boot device is on a SCSI drive but SCSI support "
		    "is not present");
#endif /* NSCSIBUS > 0 */
		break;
	case 22: /* IDE drive */
		/*
		 * controller(=channel=buses) uses only IDE drive.
		 * Here, controller always is 0.
		 */
		controller = B_CONTROLLER(bootdev);
		unit = unit + (controller<<1);
		break;
	}

	sprintf(buf, "%s%d", name, unit);
	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	    dv = TAILQ_NEXT(dv, dv_list)) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			booted_device = dv;
			return;
		}
	}
}

/*
 * Map a SCSI bus, target, lun to a device number.
 * This could be tape, disk, CD.  The calling routine, though,
 * assumes DISK.  It would be nice to allow CD, too...
 */
#if NSCSIBUS > 0
static int
target_to_unit(u_long bus, u_long target, u_long lun)
{
	struct scsibus_softc	*scsi;
	struct scsipi_periph	*periph;
	struct device		*sc_dev;
extern	struct cfdriver		scsibus_cd;

	if (target < 0 || target > 7 || lun < 0 || lun > 7) {
		printf("scsi target to unit, target (%ld) or lun (%ld)"
			" out of range.\n", target, lun);
		return -1;
	}

	if (bus == -1) {
		for (bus = 0 ; bus < scsibus_cd.cd_ndevs ; bus++) {
			if (scsibus_cd.cd_devs[bus]) {
				scsi = (struct scsibus_softc *)
						scsibus_cd.cd_devs[bus];
				periph = scsipi_lookup_periph(scsi->sc_channel,
				    target, lun);
				if (periph != NULL) {
					sc_dev = (struct device *)
							periph->periph_dev;
					return device_unit(sc_dev);
				}
			}
		}
		return -1;
	}
	if (bus < 0 || bus >= scsibus_cd.cd_ndevs) {
		printf("scsi target to unit, bus (%ld) out of range.\n", bus);
		return -1;
	}
	if (scsibus_cd.cd_devs[bus]) {
		scsi = (struct scsibus_softc *) scsibus_cd.cd_devs[bus];
		periph = scsipi_lookup_periph(scsi->sc_channel,
		    target, lun);
		if (periph != NULL) {
			sc_dev = (struct device *) periph->periph_dev;
			return device_unit(sc_dev);
		}
	}
	return -1;
}
#endif /* NSCSIBUS > 0 */
