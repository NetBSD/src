/* $NetBSD: opms_jazzio.c,v 1.6.82.1 2008/07/18 16:37:26 simonb Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: opms_jazzio.c,v 1.6.82.1 2008/07/18 16:37:26 simonb Exp $");

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

static int	opms_jazzio_match(device_t, cfdata_t, void *);
static void	opms_jazzio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(opms_jazzio, sizeof(struct opms_softc),
    opms_jazzio_match, opms_jazzio_attach, NULL, NULL);

static int
opms_jazzio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct jazzio_attach_args *ja = aux;

	if (strcmp(ja->ja_name, "pms") != 0)
		return 0;

	if (!opms_common_match(ja->ja_bust, &pccons_jazzio_conf))
		return 0;

	return 1;
}

static void
opms_jazzio_attach(device_t parent, device_t self, void *aux)
{
	struct opms_softc *sc = device_private(self);
	struct jazzio_attach_args *ja = aux;

	sc->sc_dev = self;

	print_normal("\n");

	jazzio_intr_establish(ja->ja_intr, opmsintr, self);
	opms_common_attach(sc, ja->ja_bust, &pccons_jazzio_conf);
}
