/*	$NetBSD: hil_intio.c,v 1.3.6.2 2011/06/06 09:05:36 jruoho Exp $	*/
/*	$OpenBSD: hil_intio.c,v 1.8 2007/01/06 20:10:57 miod Exp $	*/

/*
 * Copyright (c) 2005, Miodrag Vallat.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/intr.h>

#include <dev/cons.h>

#include <hp300/dev/intiovar.h>

#include <machine/hil_machdep.h>
#include <dev/hil/hilvar.h>

static int	hil_intio_match(device_t, cfdata_t, void *);
static void	hil_intio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(hil_intio, sizeof(struct hil_softc),
    hil_intio_match, hil_intio_attach, NULL, NULL);

int
hil_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	static int hil_matched = 0;

	if (strcmp("hil", ia->ia_modname) != 0)
		return 0;

	/* Allow only one instance. */
	if (hil_matched != 0)
		return 0;

	if (badaddr((void *)ia->ia_addr))	/* should not happen! */
		return 0;

	return 1;
}

int hil_is_console = -1;	/* undecided */

void
hil_intio_attach(device_t parent, device_t self, void *aux)
{
	struct hil_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_bst = ia->ia_bst;
	if (bus_space_map(sc->sc_bst, ia->ia_iobase,
	    HILMAPSIZE, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map hil controller\n");
		return;
	}

	/*
	 * Check that the configured console device is a wsdisplay.
	 */
	if (major(cn_tab->cn_dev) != devsw_name2chr("wsdisplay", NULL, 0))
		hil_is_console = 0;

	hil_attach(sc, &hil_is_console);
	intr_establish(hil_intr, sc, ia->ia_ipl, IPL_TTY);

	config_interrupts(self, hil_attach_deferred);
}
