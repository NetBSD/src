/* $NetBSD: dec_2000_300.c,v 1.3 2001/04/25 17:53:04 bouyer Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_2000_300.c,v 1.3 2001/04/25 17:53:04 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/conf.h>

#include <dev/eisa/eisavar.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8042reg.h>

#include <dev/ic/comvar.h>
#include <dev/ic/comreg.h>
#include <dev/ic/pckbcvar.h>

#include <alpha/jensenio/jenseniovar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "pckbd.h"

void dec_2000_300_init(void);
static void dec_2000_300_cons_init(void);
static void dec_2000_300_device_register(struct device *, void *);

#ifdef KGDB
#include <machine/db_machdep.h>

static const char *kgdb_devlist[] = {
	"com",
	NULL,
};
#endif /* KGDB */

void
dec_2000_300_init(void)
{

	platform.family = "DECpc AXP 150 (\"Jensen\")";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		/* XXX Don't know the system variations, yet. */
		platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "jensenio";
	platform.cons_init = dec_2000_300_cons_init;
	platform.device_register = dec_2000_300_device_register;
}

static void
dec_2000_300_cons_init(void)
{
	struct ctb_tt *ctb;
	struct jensenio_config *jcp;
	extern struct jensenio_config jensenio_configuration;

	jcp = &jensenio_configuration;
	jensenio_init(jcp, 0);

	ctb = (struct ctb_tt *)(((caddr_t)hwrpb) + hwrpb->rpb_ctb_off);

	/*
	 * The Jensen uses an older (pre-Type 4) CTB format.  The
	 * console type is specified directly by ctb_type, and only
	 * minimal info is given, only for the serial console.
	 *
	 * Thankfully, the only graphics device we can have is
	 * ISA/EISA, so it really doesn't matter too much.
	 */

	switch (ctb->ctb_type) {
	case CTB_PRINTERPORT:
		/* serial console... */
		/* XXX */
		{
#if 0
			printf("CTB CSR = 0x%08lx\n", ctb->ctb_csr);
			printf("CTB BAUD = %lu\n", ctb->ctb_baud);
#endif
			/*
			 * Delay to allow PROM putchars to complete.
			 * FIFO depth * character time,
			 * character time = (1000000 / (defaultrate / 10))
			 */
			DELAY(160000000 / ctb->ctb_baud);

			if (comcnattach(&jcp->jc_internal_iot, 0x3f8,
			    ctb->ctb_baud, COM_FREQ,
			    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
				panic("can't init serial console");

			break;
		}

	case CTB_GRAPHICS:
#if NPCKBD > 0
		/* display console... */
		/* XXX */
		(void) pckbc_cnattach(&jcp->jc_internal_iot, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);

		isa_display_console(&jcp->jc_eisa_iot, &jcp->jc_eisa_memt);
#else
		panic("not configured to use display && keyboard console");
#endif
		break;


	default:
		goto badconsole;
	}
#ifdef KGDB
	/* Attach the KGDB device. */
	alpha_kgdb_init(kgdb_devlist, &jcp->jc_internal_iot);
#endif /* KGDB */

	return;
 badconsole:
	printf("ctb->ctb_type = 0x%lx\n", ctb->ctb_type);
	printf("ctb->ctb_csr = 0x%lx\n", ctb->ctb_csr);
	printf("ctb->ctb_baud = %lu\n", ctb->ctb_baud);

	panic("consinit: unknown console type %lu\n",
	    ctb->ctb_type);
}

static void
dec_2000_300_device_register(struct device *dev, void *aux)
{
	static int found, initted, scsiboot, netboot;
	static struct device *eisadev, *isadev, *scsidev;
	struct bootdev_data *b = bootdev_data;
	struct device *parent = dev->dv_parent;
	struct cfdata *cf = dev->dv_cfdata;
	struct cfdriver *cd = cf->cf_driver;

	if (found)
		return;

	if (!initted) {
		scsiboot = (strcmp(b->protocol, "SCSI") == 0);
		netboot = (strcmp(b->protocol, "BOOTP") == 0);
#if 0
		printf("scsiboot = %d, netboot = %d\n", scsiboot, netboot);
#endif
		initted = 1;
	}

	if (eisadev == NULL && strcmp(cd->cd_name, "eisa") == 0)
		eisadev = dev;

	if (isadev == NULL && strcmp(cd->cd_name, "isa") == 0)
		isadev = dev;

	if (scsiboot && (scsidev == NULL)) {
		if (parent != eisadev)
			return;
		else {
			struct eisa_attach_args *ea = aux;

			if (b->slot != ea->ea_slot)
				return;

			scsidev = dev;
#if 0
			printf("\nscsidev = %s\n", scsidev->dv_xname);
#endif
			return;
		}
	}

	if (scsiboot &&
	    (!strcmp(cd->cd_name, "sd") ||
	     !strcmp(cd->cd_name, "st") ||
	     !strcmp(cd->cd_name, "cd"))) {
		struct scsipibus_attach_args *sa = aux;

		if (parent->dv_parent != scsidev)
			return;

		if (b->unit / 100 != sa->sa_periph->periph_target)
			return;

		/* XXX LUN! */

		switch (b->boot_dev_type) {
		case 0:
			if (strcmp(cd->cd_name, "sd") &&
			    strcmp(cd->cd_name, "cd"))
				return;
			break;
		case 1:
			if (strcmp(cd->cd_name, "st"))
				return;
			break;
		default:
			return;
		}

		/* we've found it! */
		booted_device = dev;
#if 0
		printf("\nbooted_device = %s\n", booted_device->dv_xname);
#endif
		found = 1;
		return;
	}

	if (netboot) {
		/*
		 * XXX WHAT ABOUT ISA NETWORK CARDS?
		 */
		if (parent != eisadev)
			return;
		else {
			struct eisa_attach_args *ea = aux;

			if (b->slot != ea->ea_slot)
				return;

			booted_device = dev;
#if 0
			printf("\nbooted_device = %s\n", booted_device->dv_xname);
#endif
			found = 1;
			return;
		}
	}
}
