/*	$NetBSD: dc_ds.c,v 1.15 2000/01/10 03:24:31 simonb Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * this driver contributed by Jonathan Stone
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/dc7085cons.h>		/* XXX */

#include <pmax/ibus/ibusvar.h>
#include <pmax/dev/dcvar.h>

#include <pmax/pmax/kn01.h>
#include <pmax/pmax/kn02.h>
#include <pmax/pmax/kn230.h>
#include <pmax/pmax/pmaxtype.h>

/*
 * Autoconfig definition of driver front-end
 */
int	dc_ds_match __P((struct device * parent, struct cfdata *match, void *aux));
void	dc_ds_attach __P((struct device *parent, struct device *self, void *aux));

struct cfattach dc_ds_ca = {
	sizeof(struct dc_softc), dc_ds_match, dc_ds_attach
};


/*
 * Initialize a line for (polled) console I/O
 */
int
dc_ds_consinit(dev)
	dev_t dev;
{
	u_int32_t dcaddr;

	switch (systype) {
	  case DS_PMAX:
		dcaddr = KN01_SYS_DZ;
		break;
	  case DS_3MAX:
		dcaddr = KN02_SYS_DZ;
		break;
	  case DS_MIPSMATE:
		dcaddr = KN230_SYS_DZ0;
		break;
	  default:
		/* XXX error?? */
		break;
	}

	/* let any pending PROM output from boot drain */
	DELAY(100000);
	dc_consinit(dev, (void *)MIPS_PHYS_TO_KSEG1(dcaddr));
	return (1);
}


/*
 * Match driver on decstation (2100,3100,5100) based on name
 */
int
dc_ds_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ibus_attach_args *iba = aux;


	if (strcmp(iba->ia_name, "dc") != 0 &&
	    strcmp(iba->ia_name, "mdc") != 0 &&
	    strcmp(iba->ia_name, "dc7085") != 0)
		return (0);

	if (badaddr((caddr_t)iba->ia_addr, 2))
		return (0);

	return (1);
}


void
dc_ds_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ibus_attach_args *iba = aux;
	caddr_t dcaddr;
	struct dc_softc *sc = (void*) self;


	dcaddr = (caddr_t)iba->ia_addr;
	(void) dcattach(sc, (void*)MIPS_PHYS_TO_KSEG1(dcaddr),
			/* dtr/dsr mask: comm port only */
			1 << DCCOMM_PORT,
			/* rts/cts mask: none */
			0x0,
			0, DCCOMM_PORT);

	ibus_intr_establish(parent, (void*)iba->ia_cookie, IPL_TTY, dcintr, sc);
	printf("\n");
}
