/*	$NetBSD: autoconf.c,v 1.17 2003/05/25 14:02:49 tsutsui Exp $	*/

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
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/adrsmap.h>
#include <machine/romcall.h>

#include <newsmips/newsmips/machid.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cpuspeed = 10;	/* approx # instr per usec. */

struct device *booted_device;
int booted_partition;

static void findroot __P((void));

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
	extern struct idrom idrom;

	printf("SONY NET WORK STATION, Model %s, ", idrom.id_model);
	printf("Machine ID #%d\n", idrom.id_serial);

	/*
	 * Kick off autoconfiguration
	 */
	softintr_init();
	_splnone();	/* enable all interrupts */
	splhigh();	/* ...then disable device interrupts */

	if (systype == NEWS3400) {
		*(char *)INTEN0 = INTEN0_BERR;	/* only buserr occurs */
		*(char *)INTEN1 = 0;
	}

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* Enable hardware interrupt registers. */
	enable_intr();

	/* Configuration is finished, turn on interrupts. */
	_splnone();	/* enable all source forcing SOFT_INTs cleared */
}

void
cpu_rootconf()
{
	findroot();

	printf("boot device: %s\n",
	       booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 */
void
findroot()
{
	int ctlr, unit, part, type;
	struct device *dv;

	if (BOOTDEV_MAG(bootdev) != 5)	/* NEWS-OS's B_DEVMAGIC */
		return;

	ctlr = BOOTDEV_CTLR(bootdev);	/* SCSI ID */
	unit = BOOTDEV_UNIT(bootdev);
	part = BOOTDEV_PART(bootdev);	/* LUN */
	type = BOOTDEV_TYPE(bootdev);

	if (type != BOOTDEV_SD)
		return;

	/*
	 * XXX assumes only one controller exists.
	 */
	for (dv = alldevs.tqh_first; dv; dv=dv->dv_list.tqe_next) {
		if (strcmp(dv->dv_xname, "scsibus0") == 0) {
			struct scsibus_softc *sdv = (void *)dv;
			struct scsipi_periph *periph;

			periph = scsipi_lookup_periph(sdv->sc_channel,
			    ctlr, 0);
			if (periph == NULL)
				continue;

			booted_device = periph->periph_dev;
			booted_partition = part;
			return;
		}
	}
}
