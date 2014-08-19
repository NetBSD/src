/* $NetBSD: lpt_sableio.c,v 1.8.12.1 2014/08/20 00:02:42 tls Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: lpt_sableio.c,v 1.8.12.1 2014/08/20 00:02:42 tls Exp $");

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

#include <dev/pci/pcivar.h>
#include <dev/isa/isavar.h>

#include <alpha/sableio/sableiovar.h>

struct lpt_sableio_softc {
	struct lpt_softc sc_lpt;	/* real "lpt" softc */

	/* STDIO-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	lpt_sableio_match(device_t, cfdata_t , void *);
void	lpt_sableio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lpt_sableio, sizeof(struct lpt_sableio_softc),
    lpt_sableio_match, lpt_sableio_attach, NULL, NULL);

int
lpt_sableio_match(device_t parent, cfdata_t match, void *aux)
{
	struct sableio_attach_args *sa = aux;

	/* Always present. */
	if (strcmp(sa->sa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
lpt_sableio_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_sableio_softc *ssc = device_private(self);
	struct lpt_softc *sc = &ssc->sc_lpt;
	struct sableio_attach_args *sa = aux;
	const char *intrstr;
	char buf[PCI_INTRSTR_LEN];

	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;

	if (bus_space_map(sc->sc_iot, sa->sa_ioaddr, LPT_NPORTS, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_normal("\n");
	aprint_naive("\n");

	lpt_attach_subr(sc);

	intrstr = pci_intr_string(sa->sa_pc, sa->sa_sableirq[0],
	    buf, sizeof(buf));
	ssc->sc_ih = pci_intr_establish(sa->sa_pc, sa->sa_sableirq[0],
	    IPL_TTY, lptintr, sc);
	if (ssc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);
}
