/* $NetBSD: lpt_jensenio.c,v 1.13 2014/03/29 19:28:25 christos Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: lpt_jensenio.c,v 1.13 2014/03/29 19:28:25 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

struct lpt_jensenio_softc {
	struct lpt_softc sc_lpt;	/* real "lpt" softc */

	/* Jensen-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	lpt_jensenio_match(device_t, cfdata_t , void *);
void	lpt_jensenio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lpt_jensenio, sizeof(struct lpt_jensenio_softc),
    lpt_jensenio_match, lpt_jensenio_attach, NULL, NULL);

int
lpt_jensenio_match(device_t parent, cfdata_t match, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
lpt_jensenio_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_jensenio_softc *jsc = device_private(self);
	struct lpt_softc *sc = &jsc->sc_lpt;
	struct jensenio_attach_args *ja = aux;
	const char *intrstr;
	char buf[64];

	sc->sc_dev = self;
	sc->sc_iot = ja->ja_iot;

	if (bus_space_map(sc->sc_iot, ja->ja_ioaddr, LPT_NPORTS, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_normal("\n");
	aprint_naive("\n");

	lpt_attach_subr(sc);

	intrstr = eisa_intr_string(ja->ja_ec, ja->ja_irq[0],
	    buf, sizeof(buf));
	jsc->sc_ih = eisa_intr_establish(ja->ja_ec, ja->ja_irq[0],
	    IST_EDGE, IPL_TTY, lptintr, sc);
	if (jsc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);
}
