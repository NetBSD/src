/*	$NetBSD: rpckbd_iomd.c,v 1.1.2.2 2001/03/27 15:30:29 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * rpckbd attach routined for iomd
 *
 * Created:	13/03/2001
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/select.h>
#include <sys/poll.h>

#include <machine/bus.h>
#include <machine/irqhandler.h>
#include <machine/kbd.h>
#include <arm32/dev/rpckbdvar.h>
#include <arm32/iomd/iomdreg.h>
#include <arm32/iomd/iomdvar.h>

/* Local function prototypes */

static int  rpckbd_iomd_probe  __P((struct device *, struct cfdata *, void *));
static void rpckbd_iomd_attach __P((struct device *, struct device *, void *));

extern struct rpckbd_softc console_kbd;

/* Device structures */

struct cfattach rpckbd_iomd_ca = {
	sizeof(struct rpckbd_softc), rpckbd_iomd_probe, rpckbd_iomd_attach
};

static int
rpckbd_iomd_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct kbd_attach_args *ka = aux;

	if (strcmp(ka->ka_name, "rpckbd") == 0)
		return(1);

	return(0);
}


static void
rpckbd_iomd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct rpckbd_softc *sc = (void *)self;
	struct kbd_attach_args *ka = aux;
	int error, isconsole;

	isconsole = (sc->sc_device.dv_unit == 0);

	if (isconsole) {
		console_kbd.sc_device = sc->sc_device;
		sc = &console_kbd;
	};

	sc->sc_iot = ka->ka_iot;
	sc->sc_ioh = ka->ka_ioh;


	error = rpckbd_init((void *)sc, isconsole, IOMD_KBDDAT, IOMD_KBDCR);
	if (error == 1)
		printf(": Cannot enable keyboard");
	else if (error == 2)
		printf(": No keyboard present");

	if (error == 0) {
		sc->sc_ih = intr_claim(ka->ka_rxirq, IPL_TTY, "kbd rx", rpckbd_intr, sc);
		if (!sc->sc_ih)
			panic("%s: Cannot claim RX interrupt\n", sc->sc_device.dv_xname);
	};
}

/* End of rpckbd_iomd.c */

