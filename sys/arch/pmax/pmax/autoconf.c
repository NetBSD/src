/*	$NetBSD: autoconf.c,v 1.61 2001/08/27 02:00:17 nisimura Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.61 2001/08/27 02:00:17 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <pmax/pmax/pmaxtype.h>

#include <dev/tc/tcvar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "opt_dec_3100.h"
#include "opt_dec_5100.h"

struct intrhand		 intrtab[MAX_DEV_NCOOKIES];
struct device		*booted_device;
static struct device	*booted_controller;
static int		 booted_slot, booted_unit, booted_partition;
static char		*booted_protocol;

/*
 * Configure all devices on system
 */     
void
cpu_configure()
{
	/* Kick off autoconfiguration. */
	(void)splhigh();

	softintr_init();
	evcnt_attach_static(&pmax_clock_evcnt);
	evcnt_attach_static(&pmax_fpu_evcnt);
	evcnt_attach_static(&pmax_memerr_evcnt);

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* Reset any bus errors due to probing nonexistent devices. */
	(*platform.bus_reset)();

	/* Configuration is finished, turn on interrupts. */
	_splnone();	/* enable all source forcing SOFT_INTs cleared */
}

/*
 * Look at the string 'cp' and decode the boot device.  Boot names
 * can be something like 'rz(0,0,0)vmunix' or '5/rz0/vmunix'.
 *
 * 2100/3100/5100 allows abbrivation;
 *	dev(controller[,uni-number[,partition-number]]])[filename]
 */
void
makebootdev(cp)
	char *cp;
{
	booted_device = NULL;
	booted_slot = booted_unit = booted_partition = 0;
	booted_protocol = NULL;

#if defined(DEC_3100) || defined(DEC_5100)
	if (cp[0] == 'r' && cp[1] == 'z' && cp[2] == '(') {
		cp += 3;
		if (*cp >= '0' && *cp <= '9')
			booted_slot = *cp++ - '0';
		if (*cp == ',')
			cp += 1;
		if (*cp >= '0' && *cp <= '9')
			booted_unit = *cp++ - '0';
		if (*cp == ',')
			cp += 1;
		if (*cp >= '0' && *cp <= '9')
			booted_partition = *cp - '0';
		booted_protocol = "SCSI";
		return;
	}
	if (strncmp(cp, "tftp(", 5) == 0) {
		booted_protocol = "BOOTP";
		return;
	}
	if (strncmp(cp, "mop(", 4) == 0) {
		booted_protocol = "MOP";
		return;
	}
#endif

	if (cp[0] >= '0' && cp[0] <= '9' && cp[1] == '/') {
		booted_slot = cp[0] - '0';
		if (cp[2] == 'r' && cp[3] == 'z'
		    && cp[4] >= '0' && cp[4] <= '9') {
			booted_protocol = "SCSI";
			booted_unit = cp[4] - '0';
		}
		else if (strncmp(cp+2, "tftp", 4) == 0)
			booted_protocol = "BOOTP";
		else if (strncmp(cp+2, "mop", 3) == 0)
			booted_protocol = "MOP";
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
 * Try to determine the boot device.
 */
void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	static int found, initted, scsiboot, netboot;
	static struct device *ioasicdev;
	struct device *parent = dev->dv_parent;
	struct cfdata *cf = dev->dv_cfdata;
	struct cfdriver *cd = cf->cf_driver;

	if (found)
		return;

	if (!initted) {
		scsiboot = strcmp(booted_protocol, "SCSI") == 0;
		netboot = (strcmp(booted_protocol, "BOOTP") == 0) ||
		    (strcmp(booted_protocol, "MOP") == 0);
		initted = 1;
	}

	/*
	 * Check if IOASIC was the boot slot.
	 */
	if (strcmp(cd->cd_name, "ioasic") == 0) {
		struct tc_attach_args *ta = aux;

		if (ta->ta_slot == booted_slot)
			ioasicdev = dev;
		return;
	}

	/*
	 * Check for ASC controller on either IOASIC or TC option card.
	 */
	if (scsiboot && strcmp(cd->cd_name, "asc") == 0) {
		struct tc_attach_args *ta = aux;

		/*
		 * If boot was from IOASIC controller, ioasicdev will
		 * be the ASC parent.
		 * If boot was from a TC option card, the TC slot number
		 * of the ASC will match the boot slot.
		 */
		if (parent == ioasicdev ||
		    ta->ta_slot == booted_slot) {
			booted_controller = dev;
			return;
		}
	}

	/*
	 * If an SII device is configured, it's currently the only
	 * possible SCSI boot device.
	 */
	if (scsiboot && strcmp(cd->cd_name, "sii") == 0) {
		booted_controller = dev;
		return;
	}

	/*
	 * If we found the boot controller, if check disk/tape/cdrom device
	 * on that controller matches.
	 */
	if (booted_controller && (strcmp(cd->cd_name, "sd") == 0 ||
	    strcmp(cd->cd_name, "st") == 0 ||
	    strcmp(cd->cd_name, "cd") == 0)) {
		struct scsipibus_attach_args *sa = aux;

		if (parent->dv_parent != booted_controller)
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
	if (netboot && strcmp(cd->cd_name, "le") == 0) {
		struct tc_attach_args *ta = aux;

#if defined(DEC_3100) || defined(DEC_5100)
		/* Only one Ethernet interface on 2100/3100/5100. */
		if (systype == DS_PMAX || systype == DS_MIPSMATE) {
			booted_device = dev;
			found = 1;
			return;
		}
#endif
		if (parent == ioasicdev ||
		    ta->ta_slot == booted_slot) {
			booted_device = dev;
			found = 1;
			return;
		}
	}
}
