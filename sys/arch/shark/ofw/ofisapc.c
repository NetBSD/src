/*	$NetBSD: ofisapc.c,v 1.7.60.1 2009/05/13 17:18:23 jym Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 *  OFW Glue for PCCONS Driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofisapc.c,v 1.7.60.1 2009/05/13 17:18:23 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/intr.h>
#include <machine/irqhandler.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <shark/shark/i8042reg.h>
#include <shark/shark/ns87307reg.h>


int ofisapcprobe(struct device *, struct cfdata *, void *);
void ofisapcattach(struct device *, struct device *, void *);


CFATTACH_DECL(ofisapc, sizeof(struct device),
    ofisapcprobe, ofisapcattach, NULL, NULL);

extern struct cfdriver ofisapc_cd;


int
ofisapcprobe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	char type[64];
	char name[64];

	/* At a minimum, must match type and name properties. */
	if ( OF_getprop(oba->oba_phandle, "device_type", type,
	    sizeof(type)) < 0 || strcmp(type, "keyboard") != 0 ||
	    OF_getprop(oba->oba_phandle, "name", name, sizeof(name)) < 0 ||
	    strcmp(name, "keyboard") != 0)
		return 0;

	/* Better than a generic match. */
	return 2;
}


void
ofisapcattach(struct device *parent, struct device *dev, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	static struct isa_attach_args ia;
	struct isa_io ia_io[1];
	struct isa_irq ia_irq[1];

	printf("\n");
    
#define BASE_KEYBOARD 0x60

	/*
	 * Start with the Keyboard and mouse device configuration in the 
	 * SuperIO H/W
	 */
	(void)i87307KbdConfig(&isa_io_bs_tag, BASE_KEYBOARD, IRQ_KEYBOARD);
	(void)i87307MouseConfig (&isa_io_bs_tag, IRQ_MOUSE);

	/* XXX - Hard-wire the ISA attach args for now. -JJK */
	ia.ia_iot = &isa_io_bs_tag;
	ia.ia_memt = &isa_mem_bs_tag;
	ia.ia_ic = NULL;			/* not used */

	ia.ia_nio = 1;
	ia.ia_io = ia_io;
	ia.ia_io[0].ir_addr = BASE_KEYBOARD;
	ia.ia_io[0].ir_size = I8042_NPORTS;

	ia.ia_nirq = 1;
	ia.ia_irq = ia_irq;
	ia.ia_irq[0].ir_irq = IRQ_KEYBOARD;

	ia.ia_niomem = 0;
	ia.ia_ndrq = 0;

	ia.ia_aux = (void *)oba->oba_phandle;

	config_found(dev, &ia, NULL);
}
