/*	$NetBSD: dz_uba.c,v 1.2 1999/01/19 21:04:48 ragge Exp $ */
/*
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden. All rights reserved.
 * Copyright (c) 1996  Ken C. Wellsch.  All rights reserved.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/pte.h>
#include <machine/trap.h>
#include <machine/scb.h>

#include <vax/uba/ubareg.h>
#include <vax/uba/ubavar.h>

#include <vax/uba/dzreg.h>
#include <vax/uba/dzvar.h>

#include "ioconf.h"

static	int	dz_uba_match __P((struct device *, struct cfdata *, void *));
static	void	dz_uba_attach __P((struct device *, struct device *, void *));

struct	cfattach dz_uba_ca = {
	sizeof(struct dz_softc), dz_uba_match, dz_uba_attach
};

/* Autoconfig handles: setup the controller to interrupt, */
/* then complete the housecleaning for full operation */

static int
dz_uba_match(parent, cf, aux)
        struct device *parent;
	struct cfdata *cf;
        void *aux;
{
	struct uba_attach_args *ua = aux;
	register dzregs *dzaddr;
	register int n;

	dzaddr = (dzregs *) ua->ua_addr;

	/* Reset controller to initialize, enable TX interrupts */
	/* to catch floating vector info elsewhere when completed */

	dzaddr->dz_csr = (DZ_CSR_MSE | DZ_CSR_TXIE);
	dzaddr->dz_tcr = 1;	/* Force a TX interrupt */

	DELAY(100000);	/* delay 1/10 second */

	dzaddr->dz_csr = DZ_CSR_RESET;

	/* Now wait up to 3 seconds for reset/clear to complete. */

	for (n = 0; n < 300; n++) {
		DELAY(10000);
		if ((dzaddr->dz_csr & DZ_CSR_RESET) == 0)
			break;
	}

	/* If the RESET did not clear after 3 seconds, */
	/* the controller must be broken. */

	if (n >= 300)
		return (0);

	/* Register the TX interrupt handler */

	ua->ua_ivec = dzxint;

       	return (1);
}

static void
dz_uba_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	struct uba_softc *uh = (void *)parent;
	struct	dz_softc *sc = (void *)self;
	register struct uba_attach_args *ua = aux;
	register dzregs *da;

	da = (dzregs *) ua->ua_addr;
	sc->sc_dr.dr_csr = &da->dz_csr;
	sc->sc_dr.dr_rbuf = &da->dz_rbuf;
	sc->sc_dr.dr_dtr = &da->dz_dtr;
	sc->sc_dr.dr_break = &da->dz_break;
	sc->sc_dr.dr_tbuf = &da->dz_tbuf;
	sc->sc_dr.dr_tcr = &da->dz_tcr;
	sc->sc_dr.dr_dcd = &da->dz_dcd;
	sc->sc_dr.dr_ring = &da->dz_ring;

#ifdef QBA
	if (uh->uh_type == QBA)
		sc->sc_type = DZ_DZV;
	else
#endif
		sc->sc_type = DZ_DZ;

	/* Now register the RX interrupt handler */
	scb_vecalloc(ua->ua_cvec - 4, dzrint, self->dv_unit, SCB_ISTACK);

	dzattach(sc);
}
