/* $NetBSD: opms_jazzio.c,v 1.4 2003/07/15 00:04:50 lukem Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: opms_jazzio.c,v 1.4 2003/07/15 00:04:50 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arc/dev/pcconsvar.h>
#include <arc/dev/opmsvar.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/pccons_jazziovar.h>

int	opms_jazzio_match __P((struct device *, struct cfdata *, void *));
void	opms_jazzio_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(opms_jazzio, sizeof(struct opms_softc),
    opms_jazzio_match, opms_jazzio_attach, NULL, NULL);

int
opms_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;

	if (strcmp(ja->ja_name, "pms") != 0)
		return (0);

	if (!opms_common_match(ja->ja_bust, &pccons_jazzio_conf))
		return (0);

	return (1);
}

void
opms_jazzio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct opms_softc *sc = (struct opms_softc *)self;
	struct jazzio_attach_args *ja = aux;

	printf("\n");

	jazzio_intr_establish(ja->ja_intr, opmsintr, self);
	opms_common_attach(sc, ja->ja_bust, &pccons_jazzio_conf);
}
