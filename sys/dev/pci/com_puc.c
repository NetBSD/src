/*	$NetBSD: com_puc.c,v 1.1.24.1 2001/03/22 02:10:05 he Exp $	*/

/*
 * Copyright (c) 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

/*
 * Machine-independent ns16x50 serial port ('com') driver attachment to
 * "PCI Universal Communications" controller driver.
 *
 * Author: Christopher G. Demetriou, May 17, 1998.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pucvar.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_puc_softc {
	struct com_softc sc_com;	/* real "com" softc */

	/* puc-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	com_puc_probe __P((struct device *, struct cfdata *, void *));
void	com_puc_attach __P((struct device *, struct device *, void *));

struct cfattach com_puc_ca = {
	sizeof(struct com_puc_softc), com_puc_probe, com_puc_attach
};

int
com_puc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct puc_attach_args *aa = aux;

	/*
	 * Locators already matched, just check the type.
	 */
	if (aa->type != PUC_PORT_TYPE_COM)
		return (0);

	return (1);
}

void
com_puc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_puc_softc *psc = (void *)self;
	struct com_softc *sc = &psc->sc_com;
	struct puc_attach_args *aa = aux;
	const char *intrstr;

	/*
	 * XXX This driver assumes that 'com' ports attached to 'puc'
	 * XXX can not be console.  That isn't unreasonable, because PCI
	 * XXX devices are supposed to be dynamically mapped, and com
	 * XXX console ports want fixed addresses.  When/if baseboard
	 * XXX 'com' ports are identified as PCI/communications/serial
	 * XXX devices and are known to be mapped at the standard
	 * XXX addresses, if they can be the system console then we have
	 * XXX to cope with doing the mapping right.  Then this will get
	 * XXX really ugly.  Of course, by then we might know the real
	 * XXX definition of PCI/communications/serial, and attach 'com'
	 * XXX directly on PCI.
	 */

	sc->sc_iobase = aa->a;
	sc->sc_iot = aa->t;
	sc->sc_ioh = aa->h;
	sc->sc_frequency = aa->flags & PUC_COM_CLOCKMASK;

	intrstr = pci_intr_string(aa->pc, aa->intrhandle);
	psc->sc_ih = pci_intr_establish(aa->pc, aa->intrhandle, IPL_SERIAL,
	    comintr, sc);
	if (psc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf(": interrupting at %s\n", intrstr);
	printf("%s", sc->sc_dev.dv_xname);

	com_attach_subr(sc);
}
