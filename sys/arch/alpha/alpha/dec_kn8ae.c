/* $NetBSD: dec_kn8ae.c,v 1.7.2.4 1997/07/22 06:09:46 cgd Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_kn8ae.c,v 1.7.2.4 1997/07/22 06:09:46 cgd Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/conf.h>

#include <dev/isa/isavar.h>
#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

const char *
dec_kn8ae_model_name()
{
	static const char *srv = "AlphaServer 8400";

	if ((hwrpb->rpb_variation & SV_ST_MASK) != 0) {
		static char s[80];
		sprintf(s, "%s, System Variation %lx\n", srv,
		    hwrpb->rpb_variation & SV_ST_MASK);
		return ((const char *)s);
	} else {
		return (srv);
	}
}

void
dec_kn8ae_cons_init()
{
	struct ctb *ctb;

	ctb = (struct ctb *)(((caddr_t)hwrpb) + hwrpb->rpb_ctb_off);

	/*
	 * The AXP 8X00 seems to encode the
	 * type of console in the ctb_type field,
	 * not the ctb_term_type field.
	 */
	if (ctb->ctb_type != 2) {
		panic("consinit: unsupported console type %d\n",
		    ctb->ctb_term_type);
		/* NOTREACHED */
	} else {
		/*
		 * XXX: We don't know what kind of Console this is
		 * XXX: yet, so we won't change anything and let
		 * XXX: the prom cnputc routine remap the prom in
		 * XXX: as needed.
		 */
	}
}

const char *
dec_kn8ae_iobus_name()
{

	return ("tlsb");
}

/* #define	BDEBUG	1 */
void
dec_kn8ae_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	static int found, initted, scsiboot, netboot;
	static struct device *pcidev, *scsidev;
	struct bootdev_data *b = bootdev_data;
	struct device *parent = dev->dv_parent;
	struct cfdata *cf = dev->dv_cfdata;
	struct cfdriver *cd = cf->cf_driver;

	if (found)
		return;

	if (!initted) {
		scsiboot = (strcmp(b->protocol, "scsi") == 0);
		netboot = (strcmp(b->protocol, "bootp") == 0);
		netboot = (strcmp(b->protocol, "mop") == 0);
#if	BDEBUG
		printf("proto:%s bus:%d slot:%d chan:%d", b->protocol,
		    b->bus, b->slot, b->channel);
		if (b->remote_address)
			printf(" remote_addr:%s", b->remote_address);
		printf(" un:%d bdt:%d", b->unit, b->boot_dev_type);
		if (b->ctrl_dev_type)
			printf(" cdt:%s\n", b->ctrl_dev_type);
		else
			printf("\n");
		printf("scsiboot = %d, netboot = %d\n", scsiboot, netboot);
#endif
		initted = 1;
	}

	if (pcidev == NULL) {
		if (strcmp(cd->cd_name, "pci"))
			return;
		else {
			struct pcibus_attach_args *pba = aux;

			if ((b->slot / 1000) != pba->pba_bus)
				return;
	
			pcidev = dev;
#if	BDEBUG
			printf("\npcidev = %s\n", pcidev->dv_xname);
#endif
			return;
		}
	}

	if (scsiboot && (scsidev == NULL)) {
		if (parent != pcidev)
			return;
		else {
			struct pci_attach_args *pa = aux;

			if ((b->slot % 1000) != pa->pa_device)
				return;

			/* XXX function? */
	
			scsidev = dev;
#if	BDEBUG
			printf("\nscsidev = %s\n", scsidev->dv_xname);
#endif

			return;
		}
	}

	if (scsiboot &&
	    (!strcmp(cd->cd_name, "sd") ||
	     !strcmp(cd->cd_name, "st") ||
	     !strcmp(cd->cd_name, "cd"))) {
		struct scsibus_attach_args *sa = aux;

		if (parent->dv_parent != scsidev)
			return;

		if (b->unit / 100 != sa->sa_sc_link->target)
			return;

		/* XXX LUN! */

		/*
		 * the value in boot_dev_type is some wierd number
		 * XXX: Only support SD booting for now.
		 */
		if (strcmp(cd->cd_name, "sd") &&
		    strcmp(cd->cd_name, "cd") &&
		    strcmp(cd->cd_name, "st"))
			return;

		/* we've found it! */
		booted_device = dev;
#if	BDEBUG
		printf("\nbooted_device = %s\n", booted_device->dv_xname);
#endif
		found = 1;
	}

	if (netboot) {
		if (parent != pcidev)
			return;
		else {
			struct pci_attach_args *pa = aux;

			if ((b->slot % 1000) != pa->pa_device)
				return;

			/* XXX function? */
	
			booted_device = dev;
#if	BDEBUG
			printf("\nbooted_device = %s\n", booted_device->dv_xname);
#endif
			found = 1;
			return;
		}
	}
}
