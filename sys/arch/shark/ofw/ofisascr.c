/*	$NetBSD: ofisascr.c,v 1.1 2002/02/10 01:57:58 thorpej Exp $	*/

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
 *  OFW Glue for Smart Card Driver
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/intr.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <shark/shark/sequoia.h>

int ofisascrprobe __P((struct device *, struct cfdata *, void *));
void ofisascrattach __P((struct device *, struct device *, void *));


struct cfattach ofisascr_ca = {
	sizeof(struct device), ofisascrprobe, ofisascrattach
};

extern struct cfdriver ofisascr_cd;


int
ofisascrprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofbus_attach_args *oba = aux;
	char type[64];
	char name[64];

	/* At a minimum, must match type and name properties. */
	if ( OF_getprop(oba->oba_phandle, "device_type", type,
	    sizeof(type)) < 0 || strcmp(type, "ISO7816") != 0 ||
	    OF_getprop(oba->oba_phandle, "name", name, sizeof(name)) < 0 ||
	    strcmp(name, "scr") != 0)
		return 0;
	
	/* Match, we dont have models yet  */
	return 2;
}


void
ofisascrattach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
	struct ofbus_attach_args *oba = aux;
	struct isa_attach_args ia;
	struct isa_io ia_io[1];
    
	printf("\n");

	/* XXX - Hard-wire the ISA attach args for now. -JJK */
	ia.ia_iot = &isa_io_bs_tag;
	ia.ia_memt = &isa_mem_bs_tag;
	ia.ia_ic = NULL;			/* not used */

	ia.ia_nio = 1;
	ia.ia_io = ia_io;
	ia.ia_io[0].ir_addr = SEQUOIA_BASE;
	ia.ia_io[0].ir_size = SEQUOIA_NPORTS;

	ia.ia_niomem = 0;
	ia.ia_nirq = 0;
	ia.ia_ndrq = 0;

	ia.ia_aux = (void *)oba->oba_phandle;
    
	config_found(dev, &ia, NULL);
}
