/*	$NetBSD: autoconf.c,v 1.29 2001/04/25 17:53:18 bouyer Exp $	*/

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
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 * 
 *	@(#)autoconf.c  8.2 (Berkeley) 1/12/94
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/vmparam.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/pte.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#ifdef MVME147
#include <mvme68k/dev/pccreg.h>
#endif
#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
#include <mvme68k/dev/pcctworeg.h>
#endif


struct device *booted_device;	/* boot device */

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure()
{

	booted_device = NULL;	/* set by device drivers (if found) */

	softintr_init();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");
}

void
cpu_rootconf()
{

	printf("boot device: %s",
		(booted_device) ? booted_device->dv_xname : "<unknown>");

	if (bootpart)
		printf(" (partition %d)\n", bootpart);
	else
		printf("\n");

	setroot(booted_device, bootpart);
}

void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	static struct device *controller;
	static int foundboot;
	struct device *parent;
	struct cfdriver *cd;

	if (foundboot)
		return;

	parent = dev->dv_parent;
	cd = dev->dv_cfdata->cf_driver;

	if (controller == NULL && parent) {
		struct cfdriver *pcd = parent->dv_cfdata->cf_driver;

		switch (machineid) {
#ifdef MVME147
		case MVME_147:
			/*
			 * We currently only support booting from the 147's
			 * onboard scsi and ethernet. So ensure this
			 * device's parent is the PCC driver.
			 */
			if (strcmp(pcd->cd_name, "pcc"))
				return;

			if (bootaddr == PCC_PADDR(PCC_WDSC_OFF) &&
			    strcmp(cd->cd_name, "wdsc") == 0) {
				controller = dev;
				return;
			}

			if (bootaddr == PCC_PADDR(PCC_LE_OFF) &&
			    strcmp(cd->cd_name, "le") == 0) {
				booted_device = dev;
				foundboot = 1;
				return;
			}

			break;
#endif /* MVME_147 */

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
		case MVME_162:
		case MVME_167:
		case MVME_172:
		case MVME_177:
			/*
			 * We currently only support booting from the 16x and 17x
			 * onboard scsi and ethernet. So ensure this
			 * device's parent is the PCCTWO driver.
			 */
			if (strcmp(pcd->cd_name, "pcctwo"))
				return;

			if (bootaddr == PCCTWO_PADDR(PCCTWO_NCRSC_OFF) &&
			    strcmp(cd->cd_name, "ncrsc") == 0) {
				controller = dev;
				return;
			}

			if (bootaddr == PCCTWO_PADDR(PCCTWO_IE_OFF) &&
			    strcmp(cd->cd_name, "ie") == 0) {
				booted_device = dev;
				foundboot = 1;
				return;
			}

			break;
#endif /* MVME_162 || MVME_167 || MVME_172 || MVME_177 */

		default:
			break;
		}

		return;
	}

	/*
	 * Find out which device on the scsibus we booted from
	 */
	if (strcmp(cd->cd_name, "sd") == 0 ||
	    strcmp(cd->cd_name, "cd") == 0 ||
	    strcmp(cd->cd_name, "st") == 0) {
		struct scsipibus_attach_args *sa = aux;

		if (parent->dv_parent != controller ||
		    bootdevlun != sa->sa_periph->periph_target)
			return;

		booted_device = dev;
		foundboot = 1;
	}
}
