/*	$NetBSD: dec_kn20aa.c,v 1.6 1996/06/12 22:06:50 cgd Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <dev/cons.h>

#include <machine/rpb.h>

#include <dev/isa/isavar.h>
#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#include <alpha/alpha/dec_kn20aa.h>

char *
dec_kn20aa_modelname()
{

	switch (hwrpb->rpb_variation & SV_ST_MASK) {
	case 0:
		return "AlphaStation 600 5/266 (KN20AA)";
		
	default:
		printf("unknown system variation %lx\n",
		    hwrpb->rpb_variation & SV_ST_MASK);
		return NULL;
	}
}

void
dec_kn20aa_consinit()
{
	struct ctb *ctb;
	struct cia_config *ccp;
	extern struct cia_config cia_configuration;

	ccp = &cia_configuration;
	cia_init(ccp);

	ctb = (struct ctb *)(((caddr_t)hwrpb) + hwrpb->rpb_ctb_off);

	printf("ctb->ctb_term_type = 0x%lx\n", ctb->ctb_term_type);
	printf("ctb->ctb_turboslot = 0x%lx\n", ctb->ctb_turboslot);

	switch (ctb->ctb_term_type) {
	case 2: 
		/* serial console ... */
		/* XXX */
		{
			extern bus_chipset_tag_t comconsbc;	/* set */
			extern bus_io_handle_t comcomsioh;	/* set */
			extern int comconsaddr, comconsinit;	/* set */
			extern int comdefaultrate;
			extern int comcngetc __P((dev_t));
			extern void comcnputc __P((dev_t, int));
			extern void comcnpollc __P((dev_t, int));
			static struct consdev comcons = { NULL, NULL,
			    comcngetc, comcnputc, comcnpollc, NODEV, 1 };

			comconsaddr = 0x3f8;
			comconsinit = 0;
			comconsbc = &ccp->cc_bc;
			if (bus_io_map(comconsbc, comconsaddr, COM_NPORTS,
			    &comconsioh))
				panic("can't map serial console I/O ports");
			comconscflag = (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8;
			cominit(comconsbc, comconsioh, comdefaultrate);

			cn_tab = &comcons;
			comcons.cn_dev = makedev(26, 0);	/* XXX */
			break;
		}

	case 3:
		/* display console ... */
		/* XXX */
		pci_display_console(&ccp->cc_bc, &ccp->cc_pc,
		    (ctb->ctb_turboslot >> 8) & 0xff,
		    ctb->ctb_turboslot & 0xff, 0);
		break;

	default:
		panic("consinit: unknown console type %d\n",
		    ctb->ctb_term_type);
	}
}
