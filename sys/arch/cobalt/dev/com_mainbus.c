/*	$NetBSD: com_mainbus.c,v 1.6 2003/07/15 01:29:23 lukem Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_mainbus.c,v 1.6 2003/07/15 01:29:23 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/intr_machdep.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>


struct com_mainbus_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

static int	com_mainbus_probe(struct device *, struct cfdata *, void *);
static void	com_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_mainbus, sizeof(struct com_mainbus_softc),
    com_mainbus_probe, com_mainbus_attach, NULL, NULL);

int
com_mainbus_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	/* XXX probe */

	return 1;
}

struct com_softc *com0; /* XXX */

void
com_mainbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct com_mainbus_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct mainbus_attach_args *maa = aux;

	sc->sc_ioh = maa->ma_ioh;
	sc->sc_iot = maa->ma_iot;
	sc->sc_iobase = maa->ma_addr;

	sc->sc_frequency = COM_FREQ * 10;

	/* XXX console check */
	/* XXX map */

	com_attach_subr(sc);

	cpu_intr_establish(maa->ma_level, IPL_SERIAL, comintr, sc);

	return;
}
