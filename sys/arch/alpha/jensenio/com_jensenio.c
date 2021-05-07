/* $NetBSD: com_jensenio.c,v 1.19 2021/05/07 16:58:34 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: com_jensenio.c,v 1.19 2021/05/07 16:58:34 thorpej Exp $");

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
#include <sys/cpu.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

struct com_jensenio_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* Jensen-specific goo. */
	struct jensenio_scb_intrhand sc_jih;
};

static int	com_jensenio_match(device_t, cfdata_t , void *);
static void	com_jensenio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_jensenio, sizeof(struct com_jensenio_softc),
    com_jensenio_match, com_jensenio_attach, NULL, NULL);

static int
com_jensenio_match(device_t parent, cfdata_t match, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, match->cf_name) == 0)
		return (1);

	return (0);
}

static void
com_jensenio_attach(device_t parent, device_t self, void *aux)
{
	struct com_jensenio_softc *jsc = device_private(self);
	struct com_softc *sc = &jsc->sc_com;
	struct jensenio_attach_args *ja = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	if (com_is_console(ja->ja_iot, ja->ja_ioaddr, &ioh) == 0 &&
	    bus_space_map(ja->ja_iot, ja->ja_ioaddr, COM_NPORTS, 0,
		&ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	com_init_regs(&sc->sc_regs, ja->ja_iot, ioh, ja->ja_ioaddr);

	sc->sc_frequency = COM_FREQ;

	/*
	 * According to comments in Linux drivers/tty/serial/8250/8250.h,
	 * the driver has to set OUT1 and OUT2 lines for "some ALPHA"
	 * otherwise "the machine locks up with endless interrupts."
	 * Note OUT2 (MCR_IENABLE) is set in MI com_attach_subr()
	 * so we have to set OUT1 (MCR_DSR) in the MD attachment.
	 * See also PR/36628.
	 */
	SET(sc->sc_mcr, MCR_DRS);

	com_attach_subr(sc);

	jensenio_intr_establish(&jsc->sc_jih, ja->ja_irq[0],
	    0, comintr, sc);

	aprint_normal_dev(self, "interrupting at vector 0x%x\n",
	    ja->ja_irq[0]);

	if (!pmf_device_register1(self, com_suspend, com_resume, com_cleanup)) {
		aprint_error_dev(self, "could not establish shutdown hook");
	}
}
