/* $NetBSD: com_sableio.c,v 1.10 2009/12/04 11:13:04 njoly Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: com_sableio.c,v 1.10 2009/12/04 11:13:04 njoly Exp $");

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
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/pci/pcivar.h>
#include <dev/isa/isavar.h>

#include <alpha/sableio/sableiovar.h>

struct com_sableio_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* STDIO-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	com_sableio_match(device_t, cfdata_t , void *);
void	com_sableio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_sableio, sizeof(struct com_sableio_softc),
    com_sableio_match, com_sableio_attach, NULL, NULL);

int
com_sableio_match(device_t parent, cfdata_t match, void *aux)
{
	struct sableio_attach_args *sa = aux;

	/* Always present. */
	if (strcmp(sa->sa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
com_sableio_attach(device_t parent, device_t self, void *aux)
{
	struct com_sableio_softc *ssc = device_private(self);
	struct com_softc *sc = &ssc->sc_com;
	struct sableio_attach_args *sa = aux;
	const char *intrstr;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	if (com_is_console(sa->sa_iot, sa->sa_ioaddr, &ioh) == 0 &&
	    bus_space_map(sa->sa_iot, sa->sa_ioaddr, COM_NPORTS, 0,
		&ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, sa->sa_iot, ioh, sa->sa_ioaddr);

	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	intrstr = pci_intr_string(sa->sa_pc, sa->sa_sableirq[0]);
	ssc->sc_ih = pci_intr_establish(sa->sa_pc, sa->sa_sableirq[0],
	    IPL_SERIAL, comintr, sc);
	if (ssc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	if (!pmf_device_register1(self, com_suspend, com_resume, com_cleanup)) {
		aprint_error_dev(self, "could not establish shutdown hook");
	}
}
