/* $NetBSD: dec_kn300.c,v 1.3 1998/04/17 02:45:20 mjacob Exp $ */

/*
 * Copyright (c) 1998 by Matthew Jacob
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_kn300.c,v 1.3 1998/04/17 02:45:20 mjacob Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/conf.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isavar.h>
#include <dev/isa/pckbcvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "pckbd.h"

#ifndef	CONSPEED
#define	CONSPEED	TTYDEF_SPEED
#endif
static int comcnrate = CONSPEED;

void dec_kn300_init __P((void));
void dec_kn300_cons_init __P((void));
static void dec_kn300_device_register __P((struct device *, void *));
#define	ALPHASERVER_4100	"AlphaServer 4100"

const struct alpha_variation_table dec_kn300_variations[] = {
	{ 0, ALPHASERVER_4100 },
	{ 0, NULL },
};

void
dec_kn300_init()
{
	u_int64_t variation;

	platform.family = ALPHASERVER_4100;

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_kn300_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "mcbus";
	platform.cons_init = NULL;
	platform.device_register = dec_kn300_device_register;
}


/*
 * Yes, I know this should be done as part of cons_init, but this
 * is broken in that I can't do this until I do a more or less
 * complete configure.
 */
void
dec_kn300_cons_init()
{
	struct ctb *ctb;
	ctb = (struct ctb *)(((caddr_t)hwrpb) + hwrpb->rpb_ctb_off);

	switch (ctb->ctb_term_type) {
	case 2: 
		/* serial console ... */
		/*
		 * Delay to allow PROM putchars to complete.
		 * FIFO depth * character time,
		 * character time = (1000000 / (defaultrate / 10))
		 */
		DELAY(160000000 / comcnrate);

		if (comcnattach(&mcpcia_eisaccp->cc_iot, 0x3f8, comcnrate,
		    COM_FREQ, (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)) {
			panic("can't init serial console");

		}
		break;

	case 3:
	{
		extern struct mcpcia_softc *mcpcias;
		struct mcpcia_config *ccp;
		struct mcpcia_softc *mcp = mcpcias;
		/* display console ... */
		/*
		 * Pathetic, but works for now.
		 * The Alpha4100 SRM won't let you put a vga in any pci
		 * bus but what it considers PCI0 - for us that's the
		 * *second* mcpcia.
		 */
		if (mcp && mcp->mcpcia_next) {
#if	NPCKBD > 0
			ccp = &mcp->mcpcia_next->mcpcia_cc;
		
			(void) pckbc_cnattach(&ccp->cc_iot, PCKBC_KBD_SLOT);

			if ((ctb->ctb_turboslot & 0xffff) == 0)
				isa_display_console(&ccp->cc_iot,
				    &ccp->cc_memt);
			else
				pci_display_console(&ccp->cc_iot, &ccp->cc_memt,
				    &ccp->cc_pc,
				    (ctb->ctb_turboslot >> 8) & 0xff,
				    ctb->ctb_turboslot & 0xff, 0);
#else
			panic("not configured to use display && keyboard "
				"console");
#endif
		} else {
			printf("CANNOT DETERMINE CONSOLE'S PCI BUS\n");
		}
		break;
	}
	default:
		printf("ctb->ctb_term_type = 0x%lx\n", ctb->ctb_term_type);
		printf("ctb->ctb_turboslot = 0x%lx\n", ctb->ctb_turboslot);
		panic("dec_kn300_cons_init: unknown console type %d\n",
		    ctb->ctb_term_type);
	}
}

/* #define	BDEBUG	1 */
static void
dec_kn300_device_register(dev, aux)
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
		scsiboot = (strcmp(b->protocol, "scsi") == 0) ||
		    (strcmp(b->protocol, "SCSI") == 0);
		netboot = (strcmp(b->protocol, "bootp") == 0) ||
		    (strcmp(b->protocol, "mop") == 0);
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
		struct scsipibus_attach_args *sa = aux;

		if (parent->dv_parent != scsidev)
			return;

		if (b->unit / 100 != sa->sa_sc_link->scsipi_scsi.target)
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
