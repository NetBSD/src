/*	$NetBSD: com_mainbus.c,v 1.12 2009/11/27 03:23:03 rmind Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include "opt_algor_p4032.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: com_mainbus.c,v 1.12 2009/11/27 03:23:03 rmind Exp $");

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

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_mainbus_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* mainbus-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	com_mainbus_match(device_t, cfdata_t , void *);
void	com_mainbus_attach(device_t, device_t, void *);
void	com_mainbus_cleanup(void *);

CFATTACH_DECL_NEW(com_mainbus, sizeof(struct com_mainbus_softc),
    com_mainbus_match, com_mainbus_attach, NULL, NULL);

int
com_mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* Always present. */
	if (strcmp(ma->ma_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
com_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct com_mainbus_softc *msc = device_private(self);
	struct com_softc *sc = &msc->sc_com;
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	if (com_is_console(ma->ma_st, ma->ma_addr, &ioh) == 0 &&
	    bus_space_map(ma->ma_st, ma->ma_addr, COM_NPORTS, 0, &ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, ma->ma_st, ioh, ma->ma_addr);
	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	msc->sc_ih = (*algor_intr_establish)(ma->ma_irq, comintr, sc);
	if (msc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt\n");
		return;
	}

	if (!pmf_device_register1(self, com_suspend, com_resume, com_cleanup)) {
		aprint_error_dev(self, "could not establish shutdown hook");
	}

}
